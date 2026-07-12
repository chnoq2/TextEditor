#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE namespace Ui {class SettingsDialog;}
QT_END_NAMESPACE

class SettingsDialog: public QDialog{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

    QString userName() const;
    QString ip() const;
    quint16 port() const;

private slots:
    void onAccepted();

private:
    void loadSettings();
    void saveSettings();

    Ui::SettingsDialog *ui;
};

#endif
