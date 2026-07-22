#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextBlock>
#include <QFont>
#include <QButtonGroup>
#include <QIcon>
#include <QCoreApplication>
#include <QDir>
#include <QTimer>
#include <QSettings>
#include <QFileInfo>
#include <QScrollBar>
#include <algorithm>

#include "mainwindow.h"
#include "protocol/document.h"
#include "txtexporter.h"
#include "ui_mainwindow.h"
#include "netclient.h"
#include "settingsdialog.h"
#include "QMessageBox"
#include <QColorDialog>
#include <QRegularExpression>


MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    ui->textEdit->setStyleSheet("background-color: white;");

    m_client = new NetClient(this);

    this->setWindowIcon(QIcon(":/source/icon_app.png"));

    loadConnectionSettings();

    m_localTypingTimer = new QTimer(this);
    m_localTypingTimer->setSingleShot(true);
    m_localTypingTimer->setInterval(2000); // 2 секунды таймер блокировки
    connect(m_localTypingTimer, &QTimer::timeout, this, &MainWindow::sendTypingStopIfIdle);

    m_lockOverlay = new QWidget(ui->textEdit->viewport());
    m_lockOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_lockOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 64);"); // 25% затемнение при блокировке документа
    m_lockOverlay->setGeometry(ui->textEdit->viewport()->rect());
    m_lockOverlay->hide();

    // без этого затемнение съезжает при скролле
    auto keepOverlayOnTop = [this]() {
        m_lockOverlay->setGeometry(ui->textEdit->viewport()->rect());
        m_lockOverlay->raise();
    };
    connect(ui->textEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, keepOverlayOnTop);
    connect(ui->textEdit->horizontalScrollBar(), &QScrollBar::valueChanged, this, keepOverlayOnTop);

    ui->textEdit->viewport()->installEventFilter(this);

    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(m_client, &NetClient::textInserted, this, &MainWindow::onTextInserted);
    connect(m_client, &NetClient::textDeleted, this, &MainWindow::onTextDeleted);
    connect(m_client, &NetClient::textRestyled, this, &MainWindow::onTextRestyled);


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
        ui->pushButton->setText(tr("Отключиться"));
        ui->statusLabel->setText(tr("Подключено"));
        ui->statusLabel->setStyleSheet("color: green;");
        m_client->sendSetName(m_userName);
        updateWindowTitle();
    });

    connect(m_client, &NetClient::disconnected, this, [this]() {
        m_connected = false;
        ui->pushButton->setText(tr("Подключиться"));
        ui->statusLabel->setText(tr("Не подключено"));
        ui->statusLabel->setStyleSheet("color: red;");
        updateWindowTitle();
    });

    connect(m_client, &NetClient::connectionError, this, [this](const QString &message) {
        m_connected = false;
        ui->pushButton->setText(tr("Подключиться"));
        ui->statusLabel->setText(tr("Ошибка: ") + message);
        ui->statusLabel->setStyleSheet("color: red;");
        updateWindowTitle();
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

    connect(ui->textEdit->document(), &QTextDocument::contentsChanged, this, &MainWindow::updateWordCount);
    updateWordCount();

    updateWindowTitle();
}

void MainWindow::updateWordCount()
{
    QString text = ui->textEdit->toPlainText();
    int charCount = text.length();
    int wordCount = text.isEmpty() ? 0 : text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
    ui->wordCountLabel->setText(tr("Слов: %1 | Символов: %2").arg(wordCount).arg(charCount));
}

void MainWindow::updateWindowTitle()
{
    QString name = m_currentFilePath.isEmpty() ? tr("Без имени") : QFileInfo(m_currentFilePath).fileName();
    QString title = (m_hasUnsavedChanges ? "*" : "") + name + " - TextEditor";
    setWindowTitle(title);
}

void MainWindow::markUnsavedChanges()
{
    if (!m_hasUnsavedChanges) {
        m_hasUnsavedChanges = true;
        updateWindowTitle();
    }
}

void MainWindow::resolveParagraphPosition(int absolutePos, int &paragraphIdx, int &positionInParagraph)
{
    QTextBlock block = ui->textEdit->document()->findBlock(absolutePos);
    paragraphIdx = block.isValid() ? block.blockNumber() : 0;
    positionInParagraph = block.isValid() ? (absolutePos - block.position()) : absolutePos;
}

int MainWindow::resolveAbsolutePosition(int paragraphIdx, int positionInParagraph)
{
    QTextBlock block = ui->textEdit->document()->findBlockByNumber(paragraphIdx);
    return block.isValid() ? block.position() + positionInParagraph : positionInParagraph;
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

        if (p_text.contains(pageBreakHtmlMarker)) {
            cursor.insertBlock();

            cursor.insertHtml(pageBreakHtml);

            cursor.insertBlock();

            QTextBlockFormat defaultBlockFmt;
            defaultBlockFmt.setAlignment(Qt::AlignLeft);
            cursor.setBlockFormat(defaultBlockFmt);
            cursor.setCharFormat(QTextCharFormat());

            continue;
        }

        const QList<TextStyleElement> styles = doc.get_styles_paragraph(p_idx);
        QTextBlockFormat blockFormat;
        Qt::Alignment paragraphAlignment = Qt::AlignLeft;
        int leftIndent = 0;
        int firstLineIndent = 0;

        for (const TextStyleElement &style : styles) {
            if (style.alignment != DocAlign::Unknown) {
                if (style.alignment == DocAlign::Center) paragraphAlignment = Qt::AlignCenter;
                else if (style.alignment == DocAlign::Right) paragraphAlignment = Qt::AlignRight;
                else if (style.alignment == DocAlign::Justify) paragraphAlignment = Qt::AlignJustify;
                else if (style.alignment == DocAlign::Left) paragraphAlignment = Qt::AlignLeft;
            }
            if (style.left_indent > 0) leftIndent = style.left_indent;
            if (style.first_line_indent > 0) firstLineIndent = style.first_line_indent;
        }

        blockFormat.setAlignment(paragraphAlignment);
        blockFormat.setLeftMargin(leftIndent);
        blockFormat.setTextIndent(firstLineIndent);
        cursor.setBlockFormat(blockFormat);

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

        auto formatAt = [&styles](int pos) {
            QTextCharFormat fmt;
            fmt.setFontPointSize(12);
            fmt.setForeground(Qt::black);

            for (const TextStyleElement &style : styles) {
                if (pos >= style.index_inside_vector && pos < style.index_inside_vector + style.length) {
                    if (style.is_bold) fmt.setFontWeight(QFont::Bold);
                    if (style.is_italic) fmt.setFontItalic(true);
                    if (style.is_underline) fmt.setFontUnderline(true);
                    if (!style.font_name.isEmpty()) fmt.setFontFamilies({style.font_name});
                    if (style.font_size > 0) fmt.setFontPointSize(style.font_size);
                    if (style.text_color.isValid()) fmt.setForeground(style.text_color);
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


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textEdit->viewport() && event->type() == QEvent::Resize) {
        m_lockOverlay->setGeometry(ui->textEdit->viewport()->rect());
    }
    return QMainWindow::eventFilter(obj, event);
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

    markUnsavedChanges();

    QString newText = ui->textEdit->toPlainText();
    QString oldText = m_lastText;

    int start = 0;
    while (start < oldText.length() && start < newText.length() && oldText[start] == newText[start]) { start++; }

    int oldEnd = oldText.length();
    int newEnd = newText.length();
    while (oldEnd > start && newEnd > start && oldText[oldEnd - 1] == newText[newEnd - 1]) { oldEnd--; newEnd--; }

    int paragraphIdx = 0, positionInParagraph = 0;
    resolveParagraphPosition(start, paragraphIdx, positionInParagraph);

    bool packetSent = false;
    if (oldEnd > start) { m_client->sendDelete(paragraphIdx, positionInParagraph, oldEnd - start); packetSent = true; }
    if (newEnd > start) { m_client->sendInsert(paragraphIdx, positionInParagraph, newText.mid(start, newEnd - start)); packetSent = true; }

    if (packetSent) {
        m_client->sendTypingStart();
        m_localTypingTimer->start();
    }
    m_lastText = newText;
}

void MainWindow::sendTypingStopIfIdle()
{
    m_client->sendTypingStop();
}

void MainWindow::onTextInserted(int paragraphIdx, int position_in_paragraph, const QString &text,
                                const QList<ImageElement>& images, const QList<TextStyleElement>& styles)
{
    m_ignoreChanges = true;
    int absolutePos = resolveAbsolutePosition(paragraphIdx, position_in_paragraph);

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(absolutePos);

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

                cursor.setPosition(absolutePos + img.index_inside_vector);
                cursor.insertImage(urlStr);
            }
        }
    }

    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}

void MainWindow::onTextDeleted(int paragraphIdx, int position_in_paragraph, int length){
    m_ignoreChanges = true;

    int absolutePos = resolveAbsolutePosition(paragraphIdx, position_in_paragraph);

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.setPosition(absolutePos);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, length);
    cursor.removeSelectedText();

    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;
}

void MainWindow::onTextRestyled(int paragraphIdx, TextStyleElement style)
{
    m_ignoreChanges = true;

    QTextDocument *document = ui->textEdit->document();
    QTextBlock block = document->findBlockByNumber(paragraphIdx);
    if (!block.isValid()) { m_ignoreChanges = false; return; }

    int absoluteStart = block.position() + style.index_inside_vector;
    int absoluteEnd = absoluteStart + style.length;

    QTextCursor cursor(document);
    cursor.setPosition(absoluteStart);
    cursor.setPosition(absoluteEnd, QTextCursor::KeepAnchor);

    // отправка полного актуального состояния диапазона
    QTextCharFormat fmt;
    fmt.setFontWeight(style.is_bold ? QFont::Bold : QFont::Normal);
    fmt.setFontItalic(style.is_italic);
    fmt.setFontUnderline(style.is_underline);
    if (!style.font_name.isEmpty()) fmt.setFontFamily(style.font_name);
    if (style.font_size > 0) fmt.setFontPointSize(style.font_size);
    if (style.text_color.isValid()) fmt.setForeground(style.text_color);
    cursor.mergeCharFormat(fmt);

    // применяем выравнивание ко всему абзацу
    if (style.alignment != DocAlign::Unknown) {
        Qt::Alignment qtAlign = Qt::AlignLeft;
        if (style.alignment == DocAlign::Center) qtAlign = Qt::AlignCenter;
        else if (style.alignment == DocAlign::Right) qtAlign = Qt::AlignRight;
        else if (style.alignment == DocAlign::Justify) qtAlign = Qt::AlignJustify;

        QTextBlockFormat blockFmt;
        blockFmt.setAlignment(qtAlign);
        cursor.mergeBlockFormat(blockFmt);
    }

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
        // Роли (Writer/Reader) отключены, доступ к редактированию теперь регулируется временной блокировкой по TypingStart/TypingStop
        // QString roleStr = (u.get_role() == Protocol::Writer) ? "Writer" : "Reader";
        // QString label = QString("%1 (%2)").arg(u.get_name(), roleStr);
        QString label = u.get_name();
        if (u.get_id() == m_client->myId())
            label += "  — вы";
        ui->usersListWidget->addItem(label);
    }
}

void MainWindow::onTypingStarted(int userId)
{
    if (userId == m_client->myId()) return;

    m_typingUsers.insert(userId);
    ui->textEdit->setReadOnly(true); // блокировка редактирования, пока печатает кто то другой
    m_lockOverlay->setGeometry(ui->textEdit->viewport()->rect());
    m_lockOverlay->show();
    m_lockOverlay->raise();

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

    ui->typingLabel->setText("Документ заблокирован — " + names.join(", ") + " печатает...");
    ui->typingLabel->show();
}

void MainWindow::onTypingStopped(int userId)
{
    m_typingUsers.remove(userId);
    if (m_typingUsers.isEmpty()) {
        ui->typingLabel->hide();
        ui->textEdit->setReadOnly(false); // никто больше не печатает — снимаем блокировку
        m_lockOverlay->hide();
    } else {
        onTypingStarted(*m_typingUsers.begin());
    }
}

void MainWindow::on_actionOpenFile_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Открыть docx"), "", tr("Документы (*.docx)"));
    if (filePath.isEmpty()) return;

    document_standard doc;
    doc.read_doc(filePath);

    if (doc.get_paragraphs_count() > 0) {
        setInitialDocument(doc);

        m_currentFilePath = filePath;
        m_hasUnsavedChanges = false;
        updateWindowTitle();

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
    QString filePath = m_currentFilePath;
    // если текущий файл не txt переспрашиваем путь, чтобы не потерять
    if (filePath.isEmpty() || !filePath.endsWith(".txt", Qt::CaseInsensitive)) {
        filePath = QFileDialog::getSaveFileName(this, tr("Сохранить как"), "", tr("Текстовые файлы (*.txt)"));
        if (filePath.isEmpty()) return;
    }

    TxtExporter txtExporter;
    DocumentExporter &exporter = txtExporter;

    if (exporter.exportDocument(ui->textEdit->document(), filePath)) {
        m_currentFilePath = filePath;
        m_hasUnsavedChanges = false;
        updateWindowTitle();
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить файл."));
    }
}

// шлёт остальным полный текущий формат/выравнивание диапазона
void MainWindow::sendRestyleForRange(int start, int length)
{
    if (length <= 0 || m_ignoreChanges) return;

    QTextDocument *document = ui->textEdit->document();
    int end = start + length;

    QTextBlock block = document->findBlock(start);
    while (block.isValid() && block.position() < end) {
        int blockTextStart = block.position();
        int blockTextEnd = blockTextStart + block.text().length(); // без учёта разделителя абзаца

        int rangeStart = qMax(start, blockTextStart);
        int rangeEnd = qMin(end, blockTextEnd);

        if (rangeEnd > rangeStart) {
            QTextCursor cursor(document);
            cursor.setPosition(rangeStart);
            cursor.setPosition(rangeEnd, QTextCursor::KeepAnchor);

            QTextCharFormat fmt = cursor.charFormat();
            QTextBlockFormat blockFmt = cursor.blockFormat();

            TextStyleElement style(rangeStart - blockTextStart, rangeEnd - rangeStart);
            style.is_bold = fmt.fontWeight() == QFont::Bold;
            style.is_italic = fmt.fontItalic();
            style.is_underline = fmt.fontUnderline();
            QStringList fontFamilies = fmt.fontFamilies().toStringList();
            style.font_name = fontFamilies.isEmpty() ? QString() : fontFamilies.constFirst();
            style.font_size = static_cast<int>(fmt.fontPointSize());
            style.text_color = fmt.foreground().color();

            Qt::Alignment qtAlign = blockFmt.alignment();
            if (qtAlign & Qt::AlignHCenter) style.alignment = DocAlign::Center;
            else if (qtAlign & Qt::AlignRight) style.alignment = DocAlign::Right;
            else if (qtAlign & Qt::AlignJustify) style.alignment = DocAlign::Justify;
            else style.alignment = DocAlign::Left;

            m_client->sendRestyle(block.blockNumber(), style);
        }

        block = block.next();
    }
}

// Панель форматирования (Главная)
void MainWindow::on_boldButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
    ui->textEdit->mergeCurrentCharFormat(fmt);
    markUnsavedChanges();

    // рассылаем если реально что-то выделено
    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection())
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
}

void MainWindow::on_italicButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(checked);
    ui->textEdit->mergeCurrentCharFormat(fmt);
    markUnsavedChanges();

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection())
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
}

void MainWindow::on_underlineButton_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(checked);
    ui->textEdit->mergeCurrentCharFormat(fmt);
    markUnsavedChanges();

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection())
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
}

void MainWindow::on_fontComboBox_currentFontChanged(const QFont &font)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(font.family());
    ui->textEdit->mergeCurrentCharFormat(fmt);
    markUnsavedChanges();

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection())
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
}

void MainWindow::on_fontSizeComboBox_currentTextChanged(const QString &text)
{
    bool ok = false;
    qreal size = text.toDouble(&ok);
    if (ok && size > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(size);
        ui->textEdit->mergeCurrentCharFormat(fmt);
        markUnsavedChanges();

        QTextCursor cursor = ui->textEdit->textCursor();
        if (cursor.hasSelection())
            sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
    }
}

// панель цвета текста
void MainWindow::on_colorButton_clicked()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    QColor initial = cursor.charFormat().foreground().color();

    QColor color = QColorDialog::getColor(initial, this, tr("Цвет текста"));
    if (!color.isValid()) return;

    QTextCharFormat fmt;
    fmt.setForeground(color);
    ui->textEdit->mergeCurrentCharFormat(fmt);
    markUnsavedChanges();

    if (cursor.hasSelection())
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
}


void MainWindow::on_imageButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Вставить картинку"), "", tr("Изображения (*.png *.jpg *.jpeg *.bmp)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QImage qImg = QImage::fromData(data);
    if (qImg.isNull()) {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось загрузить изображение."));
        return;
    }

    QTextCursor cursor = ui->textEdit->textCursor();
    int insertPos = cursor.position();
    QString urlStr = QString("internal://img_local_%1.png").arg(insertPos);

    int paragraphIdx = 0, positionInParagraph = 0;
    resolveParagraphPosition(insertPos, paragraphIdx, positionInParagraph);

    m_ignoreChanges = true;
    ui->textEdit->document()->addResource(QTextDocument::ImageResource, QUrl(urlStr), qImg);
    cursor.insertImage(urlStr);
    m_lastText = ui->textEdit->toPlainText();
    m_ignoreChanges = false;

    markUnsavedChanges();

    ImageElement img;
    img.index_inside_vector = 0;
    img.resolution = "Auto";
    img.binary_data = data;

    m_client->sendInsert(paragraphIdx, positionInParagraph, QString(), {img}, {});
    m_client->sendTypingStart();
    m_localTypingTimer->start();
}

// применяем выравнивание ко всему выделенному фрагменту
void MainWindow::applyAlignmentToSelection(Qt::Alignment qtAlign)
{
    QTextCursor cursor = ui->textEdit->textCursor();

    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(qtAlign);
    cursor.mergeBlockFormat(blockFmt);
    markUnsavedChanges();

    if (cursor.hasSelection()) {
        sendRestyleForRange(cursor.selectionStart(), cursor.selectionEnd() - cursor.selectionStart());
    } else {
        QTextBlock block = cursor.block();
        sendRestyleForRange(block.position(), block.length());
    }
}

void MainWindow::on_alignLeftButton_clicked()
{
    applyAlignmentToSelection(Qt::AlignLeft);
}

void MainWindow::on_alignCenterButton_clicked()
{
    applyAlignmentToSelection(Qt::AlignHCenter);
}

void MainWindow::on_alignRightButton_clicked()
{
    applyAlignmentToSelection(Qt::AlignRight);
}

void MainWindow::on_alignJustifyButton_clicked()
{
    applyAlignmentToSelection(Qt::AlignJustify);
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
