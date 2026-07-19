# TextEditor — сборка проекта

Проект состоит из двух частей:

- **Desktop-клиент** (корень репозитория) — приложение для совместного редактирования документов
- **Server/** — консольный сервер для синхронизации правок между клиентами

Клиент собирался и проверялся на Windows и Linux (Ubuntu 26.04 LTS), сервер разворачивается в Docker-контейнере (проверено на хосте Ubuntu 22.04.5 LTS)

## Требования

### Общие

- Компилятор с поддержкой **C++17**
- **Qt 6**
- Система сборки **qmake**

### Клиент (Windows)

- Windows 11 (x64)
- [Qt Online Installer](https://www.qt.io/download-qt-installer) → установить Qt **6.11.1** с китом **MinGW 64-bit**
- IDE: **Qt Creator**

### Клиент (Linux)

- Ubuntu 26.04 LTS (или совместимый дистрибутив)
- Пакеты: `qt6-base-dev`, `qt6-base-private-dev`, `qt6-base-dev-tools`, `build-essential`

### Сервер (Linux)

- Любой Linux-хост с установленным Docker (проверено на Ubuntu 22.04.5 LTS)

## Сборка клиента (Windows, Qt Creator)

1. Установить Qt 6.11.1 с китом "Desktop Qt 6.11.1 MinGW 64-bit" через Qt Online Installer
2. Открыть `TextEditor.pro` в Qt Creator
3. При запросе выбрать кит **Desktop Qt 6.11.1 MinGW 64-bit** (или другой доступный Qt 6 MinGW-кит)
4. Собрать проект: **Build → Build Project** (или Ctrl+B)
5. Готовый `TextEditor.exe` появится в `build/debug/` (или `build/release/` для релизной конфигурации)

## Сборка клиента (Linux, Ubuntu 26.04 LTS)

1. Установить зависимости:
   ```
   sudo apt install qt6-base-dev qt6-base-private-dev qt6-base-dev-tools build-essential
   ```
2. Склонировать репозиторий:
   ```
   git clone https://github.com/chnoq2/TextEditor.git
   cd /home/chnoq/TextEditor
   ```
3. Собрать проект:
   ```
   qmake6 TextEditor.pro
   make
   ```
4. Запустить:
   ```
   ./build/release/TextEditor
   ```

## Сборка сервера

1. Склонировать репозиторий:
   ```
   git clone https://github.com/chnoq2/TextEditor.git
   cd TextEditor
   ```
2. Собрать Docker-образ:
   ```
   docker build -t texteditor-server .
   ```
3. Запустить контейнер:
   ```
   docker run -d --name texteditor-server -p 5000:5000 texteditor-server
   ```
   Сервер слушает порт **5000** (пробрасывается наружу флагом `-p`)

## Подключение клиента к серверу

При первом запуске клиента открыть настройки подключения и указать IP-адрес и порт машины, на которой запущен сервер
