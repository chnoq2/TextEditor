#include <QTextCursor>
#include <QIcon>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "netclient.h"
#include <QTextCursor>

#include <QIcon>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_client = new NetClient(this);

    this->setWindowIcon(QIcon(":/source/icon_app.png"));


    connect(m_client, &NetClient::textInserted, this, &MainWindow::onTextInserted);
    connect(m_client, &NetClient::textDeleted, this, &MainWindow::onTextDeleted);
    connect(m_client, &NetClient::documentSnapshotReceived, this, &MainWindow::onSnapshotReceived);
    connect(ui->textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(m_client, &NetClient::connected, this, [this]() {ui->statusLabel->setText("Подключено"); ui->statusLabel->setStyleSheet("color: green;");});
}

MainWindow::~MainWindow()
{
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString ip = ui->IP_line->text();
    quint16 port = ui->host_line->text().toUShort();
    m_client->connectToServer(ip, port);
}

void MainWindow::onTextChanged()
{
    if (m_ignoreChanges) return;

    QString newText = ui->textEdit->toPlainText();
    QString oldText = m_lastText;

    int start = 0;
    while (start < oldText.length() && start < newText.length()
           && oldText[start] == newText[start]) {
        start++;
    }

    int oldEnd = oldText.length();
    int newEnd = newText.length();
    while (oldEnd > start && newEnd > start
           && oldText[oldEnd - 1] == newText[newEnd - 1]) {
        oldEnd--;
        newEnd--;
    }

    if (oldEnd > start) {
        m_client->sendDelete(start, oldEnd - start);
    }

    if (newEnd > start) {
        m_client->sendInsert(start, newText.mid(start, newEnd - start));
    }

    m_client->sendTypingStart();
    m_localTypingTimer->start(); // перезапускаем таймер простоя

    m_lastText = newText;
}

void MainWindow::sendTypingStopIfIdle()
{
    m_client->sendTypingStop();
}

void MainWindow::onTextInserted(int position, const QString &text)
{
    m_ignoreChanges = true;

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(position);
    cursor.insertText(text);

    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}

void MainWindow::onTextDeleted(int position, int length)
{
    m_ignoreChanges = true;

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(position);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length);
    cursor.removeSelectedText();

    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}

void MainWindow::onSnapshotReceived(const QString &text)
{
    m_ignoreChanges = true;
    ui->textEdit->setPlainText(text);
    m_lastText = text;
    m_ignoreChanges = false;
}

void MainWindow::onUserListUpdated(const QList<Protocol::ClientInfo> &users)
{
    m_userNames.clear();
    ui->usersListWidget->clear();

    for (const auto &u : users) {
        m_userNames[u.get_id()] = u.get_name();
        QString roleStr = (u.get_role() == Protocol::Writer) ? "Writer" : "Reader";
        QString label = QString("%1 (%2)").arg(u.get_name(), roleStr);
        if (u.get_id() == m_client->myId())
            label += "  — вы";
        ui->usersListWidget->addItem(label);
    }
}

void MainWindow::onTypingStarted(int userId)
{
    if (userId == m_client->myId()) return;

    m_typingUsers.insert(userId);

    if (!m_typingExpireTimers.contains(userId)) {
        QTimer *t = new QTimer(this);
        t->setSingleShot(true);
        connect(t, &QTimer::timeout, this, [this, userId]() { onTypingStopped(userId); });
        m_typingExpireTimers[userId] = t;
    }
    m_typingExpireTimers[userId]->start(3000); // страховка, если TypingStop потерялся

    QStringList names;
    for (int id : m_typingUsers)
        names << m_userNames.value(id, QString("User%1").arg(id));

    ui->typingLabel->setText(names.join(", ") + (names.size() > 1 ? " печатают..." : " печатает..."));
    ui->typingLabel->show();
}

void MainWindow::onTypingStopped(int userId)
{
    m_typingUsers.remove(userId);
    if (m_typingUsers.isEmpty()) {
        ui->typingLabel->hide();
    } else {
        onTypingStarted(*m_typingUsers.begin());
    }
}