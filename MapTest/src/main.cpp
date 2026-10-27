#include <WaferMapWindow.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMessageBox>
#include <QPixmap>
#include <QTimer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

namespace
{
// 统一使用 UTF-8，保证中文路径和中文提示在 Windows 下正常显示。
void setUtf8Locale()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    if (codec != nullptr)
    {
        QTextCodec::setCodecForLocale(codec);
    }
#endif
}
}

int main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5 需要显式开启高 DPI 缩放；Qt 6 默认已经开启。
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    setUtf8Locale();

    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("QT-Wafer-Map"));
    QCoreApplication::setApplicationName(QStringLiteral("QT Wafer Map"));
    QCoreApplication::setApplicationVersion(QStringLiteral(QTWAFERMAP_VERSION_STR));

    // 命令行参数：
    //   QTWaferMap.exe [地图文件] [--csv] [--screenshot 输出.png]
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "View, edit and validate wafer maps. Optionally pass a map file to open at startup."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("map"),
                                 QStringLiteral("Wafer map file (.txt, .map, .csv or .xlsx)."));

    // 强制按 CSV 读取；默认仍按扩展名/内容自动判断。
    QCommandLineOption csvOption(QStringLiteral("csv"),
                                 QStringLiteral("Force CSV loading."));
    csvOption.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(csvOption);

    // 无界面自动化入口：加载完成后保存窗口截图并退出。
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
                                        QStringLiteral("Save a window snapshot to <file> and exit."),
                                        QStringLiteral("file"));
    parser.addOption(screenshotOption);

    parser.process(application);

    WaferMapWindow window;
    window.resize(1600, 900);
    window.setWindowTitle(QStringLiteral("QT Wafer Map"));

    // 如果命令行传入了地图文件，则启动时直接加载。
    // 这里使用 MapLoadOptions 重载，不会弹出任何加载配置窗口。
    const QStringList files = parser.positionalArguments();
    if (!files.isEmpty())
    {
        QString errorMessage;
        MapLoadOptions options;
        options.mode = parser.isSet(csvOption) ? CsvLoadMode : DefaultLoadMode;
        if (!window.loadMapFile(files.first(), options, &errorMessage))
        {
            // 截图模式下不能弹模态框，否则无人值守进程会一直卡住。
            if (parser.isSet(screenshotOption))
            {
                qWarning().noquote() << "Map load failed:" << errorMessage;
                return 3;
            }
            QMessageBox::warning(&window,
                                 QObject::tr("地图加载失败"),
                                 errorMessage.isEmpty() ? window.lastError() : errorMessage);
        }
    }

    window.show();

    // 延迟 400ms 让界面完成一次布局和绘制，再抓取窗口截图。
    if (parser.isSet(screenshotOption))
    {
        const QString screenshotPath = parser.value(screenshotOption);
        QTimer::singleShot(400, &application, [&window, &application, screenshotPath]() {
            window.grab().save(screenshotPath);
            application.quit();
        });
    }

    return application.exec();
}
