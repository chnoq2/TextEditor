#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "netclient.h"

QT_BEGIN_NAMESPACE namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow: public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;


private slots:
    void on_pushButton_clicked();
    void onTextInserted(int position, const QString &text);
    void onTextDeleted(int position, int length);
    void onSnapshotReceived(const QString &text);

    void onTextChanged();

    void onUserListUpdated(const QList<Protocol::ClientInfo> &users);
    void onTypingStarted(int userId);
    void onTypingStopped(int userId);
    void sendTypingStopIfIdle();


private:
    Ui::MainWindow *ui;
    NetClient *m_client;
    bool m_ignoreChanges = false; // чтобы не отправлять обратно то что пришло с сервера
    QString m_lastText;           // предыдущее состояние текста для вычисления diff

    QMap<int, QString> m_userNames;
    QSet<int> m_typingUsers;
    QMap<int, QTimer*> m_typingExpireTimers; // на случай потери TypingStop
    QTimer *m_localTypingTimer;              // авто-стоп собственного "печатает"
};

#endif
