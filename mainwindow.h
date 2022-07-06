#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>

#include <Qt>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QIODevice>
#include <QPalette>
#include <QMainWindow>
#include <QMessageBox>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#ifdef Q_OS_LINUX
using process_type = QString;
#elif defined Q_OS_WIN
using process_type = HANDLE;
#endif

using result_type = int;
constexpr result_type OK = 1;
constexpr result_type ERR = -1;
#define HANDLE_ERR(expr) if ((expr) <= 0) { closeProcess(process); return; }

#define DEBUG(expr) { qDebug() << __FILE__ << __LINE__ << expr; }

constexpr qint64 OFFSET_VERSION_0 = 0xefc87;
constexpr qint64 OFFSET_VERSION_1 = 0xe6bf0;
constexpr qint64 OFFSET_VERSION_2 = 0xe6bf8;
constexpr qint64 OFFSET_ADDRESS = 0xefc55;
constexpr qint64 OFFSET_PORT = 0xefc81;

extern const QByteArray VERSION_0;
constexpr qint64 VERSION_1 = 11;
constexpr qint64 VERSION_2 = 16;
extern const QByteArray OFFICIAL_ADDRESS;
extern const QByteArray OFFICIAL_PORT;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonRefresh_clicked();
    void on_focusChanged(QWidget *, QWidget *now);
    void on_pushButtonModify_clicked();

private:
    result_type processMemoryBaseSize(process_type process, qint64 &base, qint64 &size);
    result_type readProcessMemory(process_type process, qint64 addr, qint64 size, QByteArray &data);
    result_type writeProcessMemory(process_type process, qint64 addr, const QByteArray &data);
    qint64 searchProcessMemory(process_type process, qint64 base, qint64 size, const QByteArray &data);
    bool hasSelectedProcess();
    process_type openSelectedProcess();
    void closeProcess(process_type process);
    result_type addressAndPort(QByteArray &address, QByteArray &port);
    void infoProcessNotSelected();
    void infoSuccess();
    void errorAddressInvalid();
    void errorAddressTooLong();
    void errorProcessInvalid();
    void errorCannotAccessGameMemory();
    void errorFailedSearchGameMemory();
    void errorUnmatchedGameVersion();
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
