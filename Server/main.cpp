#include <QCoreApplication>
#include <QDateTime>
#include <QTextStream>
#include "server.h"

namespace {

void timestampedMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);

    const char *level = "Debug";
    switch (type) {
    case QtInfoMsg: level = "Info"; break;
    case QtWarningMsg: level = "Warning"; break;
    case QtCriticalMsg: level = "Critical"; break;
    case QtFatalMsg: level = "Fatal"; break;
    default: break;
    }

    QTextStream(stderr) << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << " [" << level << "] " << msg << Qt::endl;
    if (type == QtFatalMsg) abort();
}

} // namespace

int main(int argc, char *argv[])
{
    qInstallMessageHandler(timestampedMessageHandler);

    QCoreApplication a(argc, argv);

    Server server;
    if (!server.start(5000)) return 1;

    return a.exec();
}
