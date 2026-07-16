#include <QTextCursor>
#include <QTextCharFormat>
#include <QFont>
#include <QButtonGroup>
#include <QIcon>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <algorithm>

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

    auto *alignGroup = new QButtonGroup(this);
    alignGroup->setExclusive(true);
    alignGroup->addButton(ui->alignLeftButton);
    alignGroup->addButton(ui->alignCenterButton);
    alignGroup->addButton(ui->alignRightButton);
    alignGroup->addButton(ui->alignJustifyButton);

    // синхронизация состояния панели форматирования с фактическим форматом под курсором
    connect(ui->textEdit, &QTextEdit::currentCharFormatChanged, this, &MainWindow::onCurrentCharFormatChanged);
    connect(ui->textEdit, &QTextEdit::cursorPositionChanged, this, &MainWindow::onTextEditCursorPositionChanged);

    connect(ui->textEdit, &QTextEdit::undoAvailable, ui->undoButton, &QWidget::setEnabled);
    connect(ui->textEdit, &QTextEdit::redoAvailable, ui->redoButton, &QWidget::setEnabled);

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
        std::sort(images.begin(), images.end(), [](const ImageElement &a, const ImageElement &b) {
            return a.index_inside_vector < b.index_inside_vector;
        });

        for (const ImageElement &img : images) {
            QImage qImg = QImage::fromData(img.binary_data);
            if (!qImg.isNull()) {
                QString urlStr = QString("internal://img_%1_%2.png").arg(p_idx).arg(img.index_inside_vector);
                document->addResource(QTextDocument::ImageResource, QUrl(urlStr), qImg);
            }
        }

        const QList<TextStyleElement> styles = doc.get_styles_paragraph(p_idx);

        QTextBlockFormat blockFormat;

        Qt::Alignment paragraphAlignment = Qt::AlignLeft;
        bool hasExplicitAlignment = false;

        for (const TextStyleElement &style : styles) {
            if (style.alignment != DocAlign::Unknown) {
                hasExplicitAlignment = true;
                if (style.alignment == DocAlign::Center) {
                    paragraphAlignment = Qt::AlignCenter;
                } else if (style.alignment == DocAlign::Right) {
                    paragraphAlignment = Qt::AlignRight;
                } else if (style.alignment == DocAlign::Justify) {
                    paragraphAlignment = Qt::AlignJustify;
                } else if (style.alignment == DocAlign::Left) {
                    paragraphAlignment = Qt::AlignLeft;
                }
                break;
            }
        }

        if (hasExplicitAlignment) {
            blockFormat.setAlignment(paragraphAlignment);
            cursor.setBlockFormat(blockFormat);
        }

        auto formatAt = [&styles](int pos) {
            QTextCharFormat fmt;
            fmt.setFontPointSize(12);

            for (const TextStyleElement &style : styles) {
                if (pos >= style.index_inside_vector && pos < style.index_inside_vector + style.length) {
                    if (style.is_bold) {
                        fmt.setFontWeight(QFont::Bold);
                    }
                    if (style.is_italic) {
                        fmt.setFontItalic(true);
                    }
                    if (style.is_underline) {
                        fmt.setFontUnderline(true);
                    }
                    if (!style.font_name.isEmpty()) {
                        fmt.setFontFamily(style.font_name);
                    }
                    if (style.font_size > 0) {
                        fmt.setFontPointSize(style.font_size);
                    }
                }
            }
            return fmt;
        };

        int textPos = 0;
        int imgIdx = 0;

        while (textPos < p_text.length() || imgIdx < images.size()) {
            if (imgIdx < images.size() && images[imgIdx].index_inside_vector <= textPos) {
                QString urlStr = QString("internal://img_%1_%2.png").arg(p_idx).arg(images[imgIdx].index_inside_vector);
                cursor.setCharFormat(QTextCharFormat());
                cursor.insertImage(urlStr);
                imgIdx++;
                continue;
            }

            int limit = (imgIdx < images.size()) ? images[imgIdx].index_inside_vector : p_text.length();
            QTextCharFormat runFormat = formatAt(textPos);
            int runEnd = textPos + 1;
            while (runEnd < limit && formatAt(runEnd) == runFormat) {
                runEnd++;
            }
            cursor.setCharFormat(runFormat);
            cursor.insertText(p_text.mid(textPos, runEnd - textPos));
            textPos = runEnd;
        }
        cursor.setCharFormat(QTextCharFormat());

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
    Q_UNUSED(paragraphIdx);
    m_ignoreChanges = true;

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(position_in_paragraph);

    if (!styles.isEmpty()) {
        int textPos = 0;
            auto getInsertFormat = [&styles](int pos) {
            QTextCharFormat fmt;
            fmt.setFontPointSize(12);
            for (const TextStyleElement &style : styles) {
                if (pos >= style.index_inside_vector && pos < style.index_inside_vector + style.length) {
                    if (style.is_bold) fmt.setFontWeight(QFont::Bold);
                    if (style.is_italic) fmt.setFontItalic(true);
                    if (style.is_underline) fmt.setFontUnderline(true);
                    if (!style.font_name.isEmpty()) fmt.setFontFamily(style.font_name);
                    if (style.font_size > 0) fmt.setFontPointSize(style.font_size);
                }
            }
            return fmt;
        };

        while (textPos < text.length()) {
            QTextCharFormat runFormat = getInsertFormat(textPos);
            int runEnd = textPos + 1;
            while (runEnd < text.length() && getInsertFormat(runEnd) == runFormat) {
                runEnd++;
            }
            cursor.setCharFormat(runFormat);
            cursor.insertText(text.mid(textPos, runEnd - textPos));
            textPos = runEnd;
        }
    } else {
        QTextCharFormat defaultFmt;
        defaultFmt.setFontPointSize(12);
        cursor.setCharFormat(defaultFmt);
        cursor.insertText(text);
    }

    if (!images.isEmpty()) {
        QTextDocument *document = ui->textEdit->document();
        for (const ImageElement &img : images) {
            QImage qImg = QImage::fromData(img.binary_data);
            if (!qImg.isNull()) {
                QString urlStr = QString("internal://img_net_%1.png").arg(img.index_inside_vector);
                document->addResource(QTextDocument::ImageResource, QUrl(urlStr), qImg);

                cursor.setPosition(position_in_paragraph + img.index_inside_vector);
                cursor.insertImage(urlStr);
            }
        }
    }

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

// пункт "Открыть файл..." в выпадающем меню кнопки "Файл" риббона (раньше была отдельная кнопка)
void MainWindow::on_actionOpenFile_triggered()
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

void MainWindow::on_actionSaveFile_triggered()
{
    QMessageBox::information(this, tr("Сохранение"), tr("Сохранение документа скоро будет доступно."));
}

// Панель форматирования (Главная)
void MainWindow::on_boldButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_italicButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(checked);
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_underlineButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(checked);
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_fontComboBox_currentFontChanged(const QFont &font)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(font.family());
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_fontSizeComboBox_currentTextChanged(const QString &text)
{
    bool ok = false;
    qreal size = text.toDouble(&ok);
    if (ok && size > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(size);
        ui->textEdit->mergeCurrentCharFormat(fmt);
    }
}

void MainWindow::on_alignLeftButton_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignLeft);
}

void MainWindow::on_alignCenterButton_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignHCenter);
}

void MainWindow::on_alignRightButton_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignRight);
}

void MainWindow::on_alignJustifyButton_clicked()
{
    ui->textEdit->setAlignment(Qt::AlignJustify);
}

// для отмены и возврата изменения используется встроенный стек textEdit
void MainWindow::on_undoButton_clicked()
{
    ui->textEdit->undo();
}

void MainWindow::on_redoButton_clicked()
{
    ui->textEdit->redo();
}

// вызывается при перемещении курсора / изменении выделения, чтобы кнопки Ж/К/Ч, шрифт и размер показывали формат текста под курсором, а не последнее нажатое состояние.
void MainWindow::onCurrentCharFormatChanged(const QTextCharFormat &format)
{
    ui->boldButton->blockSignals(true);
    ui->boldButton->setChecked(format.fontWeight() == QFont::Bold);
    ui->boldButton->blockSignals(false);

    ui->italicButton->blockSignals(true);
    ui->italicButton->setChecked(format.fontItalic());
    ui->italicButton->blockSignals(false);

    ui->underlineButton->blockSignals(true);
    ui->underlineButton->setChecked(format.fontUnderline());
    ui->underlineButton->blockSignals(false);

    ui->fontComboBox->blockSignals(true);
    ui->fontComboBox->setCurrentFont(format.font());
    ui->fontComboBox->blockSignals(false);

    qreal pointSize = format.fontPointSize();
    if (pointSize <= 0) {
        pointSize = ui->textEdit->font().pointSize();
    }
    ui->fontSizeComboBox->blockSignals(true);
    ui->fontSizeComboBox->setCurrentText(QString::number(pointSize));
    ui->fontSizeComboBox->blockSignals(false);
}

// QTextEdit не даёт отдельного сигнала на смену выравнивания, поэтому подглядываем текущее выравнивание при каждом движении курсора и обновляем кнопки
void MainWindow::onTextEditCursorPositionChanged()
{
    Qt::Alignment align = ui->textEdit->alignment();

    ui->alignLeftButton->blockSignals(true);
    ui->alignLeftButton->setChecked(align.testFlag(Qt::AlignLeft));
    ui->alignLeftButton->blockSignals(false);

    ui->alignCenterButton->blockSignals(true);
    ui->alignCenterButton->setChecked(align.testFlag(Qt::AlignHCenter));
    ui->alignCenterButton->blockSignals(false);

    ui->alignRightButton->blockSignals(true);
    ui->alignRightButton->setChecked(align.testFlag(Qt::AlignRight));
    ui->alignRightButton->blockSignals(false);

    ui->alignJustifyButton->blockSignals(true);
    ui->alignJustifyButton->setChecked(align.testFlag(Qt::AlignJustify));
    ui->alignJustifyButton->blockSignals(false);
}
