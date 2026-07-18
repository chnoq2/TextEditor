#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCharFormat>
#include <QFont>

#include "netclient.h"
#include "protocol/document.h"

QT_BEGIN_NAMESPACE namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow: public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setInitialDocument(const document_standard &doc);

private slots:
    void on_pushButton_clicked();
    void onSettingsClicked();

    void onTextInserted(int paragraphIdx, int position_in_paragraph, const QString &text,
                        const QList<ImageElement>& images, const QList<TextStyleElement>& styles);
    void onTextDeleted(int paragraphIdx, int position_in_paragraph, int length);

    void onSnapshotReceived(const QString &text);

    void onTextChanged();

    void onUserListUpdated(const QList<Protocol::ClientInfo> &users);
    void onTypingStarted(int userId);
    void onTypingStopped(int userId);
    void sendTypingStopIfIdle();


    void on_actionOpenFile_triggered();
    void on_actionSaveFile_triggered();

    void on_boldButton_toggled(bool checked);
    void on_italicButton_toggled(bool checked);
    void on_underlineButton_toggled(bool checked);
    void on_fontComboBox_currentFontChanged(const QFont &font);
    void on_fontSizeComboBox_currentTextChanged(const QString &text);
    void on_alignLeftButton_clicked();
    void on_alignCenterButton_clicked();
    void on_alignRightButton_clicked();
    void on_alignJustifyButton_clicked();
    void on_undoButton_clicked();
    void on_redoButton_clicked();

    void onCurrentCharFormatChanged(const QTextCharFormat &format);
    void onTextEditCursorPositionChanged();

private:
    void loadConnectionSettings();
    bool eventFilter(QObject *obj, QEvent *event) override;

    Ui::MainWindow *ui;
    QWidget *m_lockOverlay = nullptr; // затемнение textEdit, пока документ заблокирован чужим редактированием
    NetClient *m_client;

    bool m_connected = false;
    QString m_userName;
    QString m_ip;
    quint16 m_port = 0;

    bool m_ignoreChanges = false; // чтобы не отправлять обратно то что пришло с сервера
    QString m_lastText;           // предыдущее состояние текста для вычисления diff
    QTimer *m_localTypingTimer;              // авто-стоп собственного "печатает"


    QMap<int, QString> m_userNames;
    QSet<int> m_typingUsers;
    QMap<int, QTimer*> m_typingExpireTimers; // на случай потери TypingStop


    //window_document *WND_doc = nullptr;
};

#endif
