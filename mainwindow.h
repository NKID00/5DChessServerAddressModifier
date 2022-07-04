#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#ifdef Q_OS_LINUX
using process_handle_t = QString;
#elif Q_OS_WIN
using process_handle_t = HANDLE;
#endif

#define RET_VOID_ON_ERR(expr) if ((expr) <= 0) { return; }

#define DEBUG(expr) { qDebug() << __FILE__ << __LINE__ << expr; }

constexpr qint64 OFFSET_VERSION_0 = 0xefc87;
constexpr qint64 OFFSET_VERSION_1 = 0xe6bf0;
constexpr qint64 OFFSET_VERSION_2 = 0xe6bf8;
constexpr qint64 OFFSET_ADDRESS = 0xefc55;
constexpr qint64 OFFSET_PORT = 0xefc81;

extern const QByteArray VERSION_0;
constexpr qint64 VERSION_1 = 11;
constexpr qint64 VERSION_2 = 16;

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
    qint64 processMemoryBase(process_handle_t process);
    bool readProcessMemory(process_handle_t process, qint64 addr, qint64 size, char *buffer);
    bool writeProcessMemory(process_handle_t process, qint64 addr, qint64 size, const char *buffer);
    bool hasSelectedProcess();
    process_handle_t selectedProcess();
    bool addressAndPort(QByteArray &address, QByteArray &port);
    void infoProcessNotSelected();
    void infoSuccess();
    void errorAddressInvalid();
    void errorAddressTooLong();
    void errorCannotAccessGameMemory();
    void errorUnmatchedGameVersion();
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
