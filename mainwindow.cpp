#include <QTextCursor>
#include <QIcon>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QSettings>

#include "mainwindow.h"
#include "protocol/document.h"
#include "ui_mainwindow.h"
#include "netclient.h"
#include "settingsdialog.h"
#include "QMessageBox"


MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);

    m_client = new NetClient(this);

    this->setWindowIcon(QIcon(":/source/icon_app.png"));

    loadConnectionSettings();

    m_localTypingTimer = new QTimer(this);
    m_localTypingTimer->setSingleShot(true);
    m_localTypingTimer->setInterval(2000);
    connect(m_localTypingTimer, &QTimer::timeout, this, &MainWindow::sendTypingStopIfIdle);

    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_client, &NetClient::textInserted, this, &MainWindow::onTextInserted);
    connect(m_client, &NetClient::textDeleted, this, &MainWindow::onTextDeleted);


    // изменения
    connect(m_client, &NetClient::documentSnapshotReceived, this, [this](const QByteArray &snapshotData) {
        if (snapshotData.isEmpty()) return;

        QTimer::singleShot(100, this, [this, snapshotData]() {
            document_standard doc;
            QDataStream in(snapshotData);
            in >> doc;

            qDebug() << "[UI Delayed Render] Rendering document with paragraphs:" << doc.get_paragraphs_count();

            setInitialDocument(doc);
        });
    });
    connect(ui->textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextChanged);

    connect(m_client, &NetClient::userListUpdated, this, &MainWindow::onUserListUpdated);
    connect(m_client, &NetClient::typingStarted, this, &MainWindow::onTypingStarted);
    connect(m_client, &NetClient::typingStopped, this, &MainWindow::onTypingStopped);

    connect(m_client, &NetClient::connected, this, [this]() {
        m_connected = true;
        ui->pushButton->setText("Отключиться");
        ui->statusLabel->setText("Подключено");
        ui->statusLabel->setStyleSheet("color: green;");
        m_client->sendSetName(m_userName);
    });

    connect(m_client, &NetClient::disconnected, this, [this]() {
        m_connected = false;
        ui->pushButton->setText("Подключиться");
        ui->statusLabel->setText("Не подключено");
        ui->statusLabel->setStyleSheet("color: red;");
    });

    connect(m_client, &NetClient::connectionError, this, [this](const QString &message) {
        m_connected = false;
        ui->pushButton->setText("Подключиться");
        ui->statusLabel->setText("Ошибка: " + message);
        ui->statusLabel->setStyleSheet("color: red;");
    });



}


void MainWindow::setInitialDocument(const document_standard &doc)
{
    m_ignoreChanges = true;
    ui->textEdit->clear();

    QTextDocument *document = ui->textEdit->document();
    QTextCursor cursor(document);
    size_t total_paragraphs = doc.get_paragraphs_count();

    for (size_t i = 0; i < total_paragraphs; ++i) {
        int p_idx = static_cast<int>(i);
        const QString &p_text = doc.get_full_text()[i];

        QList<ImageElement> images = doc.get_images_for_paragraph(p_idx);
        for (const ImageElement &img : images) {
            QImage qImg = QImage::fromData(img.binary_data);
            if (!qImg.isNull()) {
                QString urlStr = QString("internal://img_%1_%2.png").arg(p_idx).arg(img.index_inside_vector);
                document->addResource(QTextDocument::ImageResource, QUrl(urlStr), qImg);
            }
        }

         cursor.insertText(p_text);

        for (const ImageElement &img : images) {
            QTextCursor imgCursor = cursor;
            imgCursor.insertBlock();
            QString urlStr = QString("internal://img_%1_%2.png").arg(p_idx).arg(img.index_inside_vector);
            imgCursor.insertImage(urlStr);
        }

        if (i < total_paragraphs - 1) {
            cursor.insertBlock();
        }
    }

    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}


void MainWindow::loadConnectionSettings()
{
    QSettings settings;
    m_userName = settings.value("connection/userName").toString();
    m_ip = settings.value("connection/ip", "127.0.0.1").toString();
    m_port = static_cast<quint16>(settings.value("connection/port", 5000).toUInt());
}

void MainWindow::onSettingsClicked()
{
   SettingsDialog dlg(this); if (dlg.exec() == QDialog::Accepted) loadConnectionSettings();
}


MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if (m_connected) {
        m_client->disconnectFromServer();
    } else {

        qDebug() << "[CONNECTING TO]" << m_ip << ":" << m_port;
        m_client->connectToServer(m_ip, m_port);
    }
}

void MainWindow::onTextChanged()
{
    if (m_ignoreChanges) return;

    QString newText = ui->textEdit->toPlainText();
    QString oldText = m_lastText;

    int start = 0;
    while (start < oldText.length() && start < newText.length() && oldText[start] == newText[start]) { start++; }

    int oldEnd = oldText.length();
    int newEnd = newText.length();
    while (oldEnd > start && newEnd > start && oldText[oldEnd - 1] == newText[newEnd - 1]) { oldEnd--; newEnd--; }

    if (oldEnd > start) { m_client->sendDelete(0, start, oldEnd - start); }
    if (newEnd > start) { m_client->sendInsert(0, start, newText.mid(start, newEnd - start)); }

    m_client->sendTypingStart();
    m_localTypingTimer->start();
    m_lastText = newText;
}

void MainWindow::sendTypingStopIfIdle()
{
    m_client->sendTypingStop();
}

void MainWindow::onTextInserted(int paragraphIdx, int position_in_paragraph, const QString &text,
                                const QList<ImageElement>& images, const QList<TextStyleElement>& styles)
{

    Q_UNUSED(paragraphIdx); Q_UNUSED(images); Q_UNUSED(styles);
    m_ignoreChanges = true;
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(position_in_paragraph);
    cursor.insertText(text);
    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}

void MainWindow::onTextDeleted(int paragraphIdx, int position_in_paragraph, int length){
    Q_UNUSED(paragraphIdx);

    m_ignoreChanges = true;

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(position_in_paragraph);
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

void MainWindow::on_choose_document_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Открыть docx"), "", tr("Документы (*.docx *.doc)"));
    if (filePath.isEmpty()) return;

    document_standard doc;
    doc.read_doc(filePath);

    if (doc.get_length() > 0 || doc.get_text() == "") {
        setInitialDocument(doc);

        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << doc;
        m_client->sendPacket(Protocol::DockSnapShot, buffer);
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось распарсить файл!"));
    }
}
