#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "netclient.h"
#include "protocol/document.h"

#include "window_document.h"

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


    void on_choose_document_clicked();

private:
    void loadConnectionSettings();

    Ui::MainWindow *ui;
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
