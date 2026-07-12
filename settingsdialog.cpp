#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>
#include <QIntValidator>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent): QDialog(parent), ui(new Ui::SettingsDialog){
    ui->setupUi(this);

    ui->portEdit->setValidator(new QIntValidator(1, 65535, this));

    loadSettings();

    connect(ui->okButton, &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog(){
    delete ui;
}

void SettingsDialog::loadSettings(){
    QSettings settings;
    ui->userNameEdit->setText(settings.value("connection/userName").toString());
    ui->ipEdit->setText(settings.value("connection/ip", "127.0.0.1").toString());
    ui->portEdit->setText(settings.value("connection/port", "5000").toString());
}

void SettingsDialog::saveSettings(){
    QSettings settings;
    settings.setValue("connection/userName", ui->userNameEdit->text());
    settings.setValue("connection/ip", ui->ipEdit->text());
    settings.setValue("connection/port", ui->portEdit->text());
}

void SettingsDialog::onAccepted(){
    saveSettings();
    accept();
}

QString SettingsDialog::userName() const{
    return ui->userNameEdit->text();
}

QString SettingsDialog::ip() const{
    return ui->ipEdit->text();
}

quint16 SettingsDialog::port() const{
    return ui->portEdit->text().toUShort();
}
