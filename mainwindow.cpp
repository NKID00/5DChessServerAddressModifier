#include "mainwindow.h"
#include "ui_mainwindow.h"

const QByteArray VERSION_0 = QByteArrayLiteral("1.1.0.0f\0");
const QByteArray OFFICIAL_ADDRESS = QByteArrayLiteral("matches.5dchesswithmultiversetimetravel.com\0");
const QByteArray OFFICIAL_PORT = QByteArrayLiteral("39005\0");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(QApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), this, SLOT(on_focusChanged(QWidget *, QWidget *)));
    on_pushButtonRefresh_clicked();
    QPalette palette;
    palette.setColor(QPalette::Text, Qt::GlobalColor::gray);
    ui->lineEditAddress->setPalette(palette);
#ifdef Q_OS_WIN
    ui->label->setText("");
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonRefresh_clicked()
{
    ui->comboBoxProcess->clear();
#ifdef Q_OS_LINUX
    for (QDirIterator it("/proc", QDir::Dirs); it.hasNext(); )
    {
        it.next();
        if (it.fileName() == "." || it.fileName() == "..")
        {
            continue;
        }
        QFile f(it.filePath() + "/comm");
        if (f.exists())
        {
            f.open(QIODevice::ReadOnly);

            if (QString::fromUtf8(f.readAll()).startsWith("5dchesswithmult"))
            {
                ui->comboBoxProcess->addItem(it.fileName());
            }
        }
    }
#elif Q_OS_WIN
#endif
}

void MainWindow::on_focusChanged(QWidget *, QWidget *now)
{
    auto lineEdit = ui->lineEditAddress;
    if (now == lineEdit)
    {
        if (lineEdit->palette().color(QPalette::Text) == Qt::GlobalColor::gray)
        {
            lineEdit->clear();
            // FIXME: buggy when changing global color theme
            QPalette palette;
            palette.setColor(QPalette::Text, QApplication::palette().color(QPalette::Text));
            lineEdit->setPalette(palette);
        }
    }
}


void MainWindow::on_pushButtonModify_clicked()
{
    if(!hasSelectedProcess())
    {
        infoProcessNotSelected();
        return;
    }
    auto process = selectedProcess();
    qint64 base, size;
    RET_VOID_ON_ERR(processMemoryBaseSize(process, base, size));

//    QByteArray version_bytes;
//    RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_0, 9, version_bytes));
//    if (version_bytes == VERSION_0)
//    {
//        RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_1, 8, version_bytes));
//        qint64 version_1;
//        QDataStream version_datastream(version_bytes);
//        version_datastream.setByteOrder(QDataStream::LittleEndian);
//        version_datastream >> version_1;
//        if (version_1 == VERSION_1)
//        {
//            RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_2, 8, version_bytes));
//            qint64 version_2;
//            QDataStream version_datastream(version_bytes);
//            version_datastream.setByteOrder(QDataStream::LittleEndian);
//            version_datastream >> version_2;
//            if (version_2 == VERSION_2)
//            {
//                QByteArray address;
//                QByteArray port;
//                RET_VOID_ON_ERR(addressAndPort(address, port));
//                RET_VOID_ON_ERR(writeProcessMemory(process, base + OFFSET_ADDRESS, address));
//                RET_VOID_ON_ERR(writeProcessMemory(process, base + OFFSET_PORT, port));
//                infoSuccess();
//                return;
//            }
//        }
//    }
//    errorUnmatchedGameVersion();

    QByteArray address, port;
    RET_VOID_ON_ERR(addressAndPort(address, port));
    if (address != OFFICIAL_ADDRESS)
    {
        auto target = searchProcessMemory(process, base, size, OFFICIAL_ADDRESS);
        RET_VOID_ON_ERR(target);
        RET_VOID_ON_ERR(writeProcessMemory(process, target, address));
    }
    if (port != OFFICIAL_PORT)
    {
        auto target = searchProcessMemory(process, base, size, OFFICIAL_PORT);
        RET_VOID_ON_ERR(target);
        RET_VOID_ON_ERR(writeProcessMemory(process, target, port));
    }
    infoSuccess();
}

bool MainWindow::processMemoryBaseSize(process_handle_t process, qint64 &base, qint64 &size)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/maps");
    if (!f.exists())
    {
        errorProcessInvalid();
        return false;
    }
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return false;
    }

    auto parts = f.readLine().split('-');
    if (parts.size() < 2)
    {
        errorCannotAccessGameMemory();
        return false;
    }
    base = parts[0].toULongLong(nullptr, 16);
    if (base <= 0)
    {
        errorCannotAccessGameMemory();
        return false;
    }

    qint64 addr = 0;
    for (QByteArray line = f.readLine(); line.contains(QByteArrayLiteral("5dchesswithmultiversetimetravel")); line = f.readLine())
    {
        parts = f.readLine().split('-');
        if (parts.size() < 2)
        {
            errorCannotAccessGameMemory();
            return false;
        }
        parts = parts[1].split(' ');
        if (parts.size() < 2)
        {
            errorCannotAccessGameMemory();
            return false;
        }
        addr = parts[0].toULongLong(nullptr, 16);
        if (addr <= 0)
        {
            errorCannotAccessGameMemory();
            return false;
        }
    }
    if (addr <= base)
    {
        errorCannotAccessGameMemory();
        return false;
    }
    size = addr - base;
#elif Q_OS_WIN
#endif
    return true;
}

bool MainWindow::readProcessMemory(process_handle_t process, qint64 addr, qint64 size, QByteArray &data)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorProcessInvalid();
        return false;
    }
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return false;
    }
    f.seek(addr);
    data = f.read(size);
    if (data.size() != size)
    {
        errorCannotAccessGameMemory();
        return false;
    }
#elif Q_OS_WIN
#endif
    return true;
}

bool MainWindow::writeProcessMemory(process_handle_t process, qint64 addr, const QByteArray &data)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorProcessInvalid();
        return false;
    }
    f.open(QIODevice::WriteOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return false;
    }
    f.seek(addr);
    if (f.write(data) != data.size())
    {
        errorCannotAccessGameMemory();
        return false;
    }
#elif Q_OS_WIN
#endif
    return true;
}

qint64 MainWindow::searchProcessMemory(process_handle_t process, qint64 base, qint64 size, const QByteArray &data)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorProcessInvalid();
        return false;
    }
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return false;
    }
    f.seek(base);
    auto memory = f.read(size);
    if (memory.size() != size)
    {
        errorCannotAccessGameMemory();
        return false;
    }
    auto offset = memory.indexOf(data);
    if (offset < 0)
    {
        errorFailedSearchGameMemory();
        return false;
    }
    return base + offset;
#elif Q_OS_WIN
#endif
}

bool MainWindow::hasSelectedProcess()
{
    return ui->comboBoxProcess->currentText().length() != 0;
}

process_handle_t MainWindow::selectedProcess()
{
#ifdef Q_OS_LINUX
    return ui->comboBoxProcess->currentText();
#elif Q_OS_WIN
#endif
}

bool MainWindow::addressAndPort(QByteArray &address, QByteArray &port)
{
    if (ui->lineEditAddress->palette().color(QPalette::Text) == Qt::GlobalColor::gray)
    {
        errorAddressInvalid();
        return false;
    }
    auto text = ui->lineEditAddress->text();
    if (text.length() == 0)
    {
        errorAddressInvalid();
        return false;
    }
    if (text.contains(":"))
    {
        auto parts = text.split(":");
        if (parts.length() != 2)
        {
            errorAddressInvalid();
            return false;
        }
        if (parts[0].length() > 44)
        {
            errorAddressTooLong();
            return false;
        }
        address = parts[0].toUtf8();
        address.append('\0');
        if (parts[1].toUShort() == 0)
        {
            errorAddressInvalid();
            return false;
        }
        port = parts[1].toUtf8();
        port.append('\0');
    }
    else
    {
        if (text.length() > 44)
        {
            errorAddressTooLong();
            return false;
        }
        address = text.toUtf8();
        address.append('\0');
        port = QByteArrayLiteral("39005\0");
    }
    return true;
}

void MainWindow::infoProcessNotSelected()
{
    QMessageBox::information(this, "Information", "No process is selected. Make sure the game is running.", QMessageBox::Ok);
}

void MainWindow::infoSuccess()
{
    QMessageBox::information(this, "Information", "Server address Modified successfully.", QMessageBox::Ok);
}

void MainWindow::errorAddressInvalid()
{
    QMessageBox::critical(this, "Critical Error", "Address is invalid.", QMessageBox::Ok);
}

void MainWindow::errorAddressTooLong()
{
    QMessageBox::critical(this, "Critical Error", "Address (excluding port) cannot be longer than 44 characters.", QMessageBox::Ok);
}

void MainWindow::errorProcessInvalid()
{
    QMessageBox::critical(this, "Critical Error", "Process is invalid. Make sure the correct process is selected.", QMessageBox::Ok);
}

void MainWindow::errorCannotAccessGameMemory()
{
    QMessageBox::critical(this, "Critical Error", "Cannot access game memory. Root permission may be required.", QMessageBox::Ok);
}

void MainWindow::errorFailedSearchGameMemory()
{
    QMessageBox::critical(this, "Critical Error", "Cannot find specific location in game memory. Game may need to restart.", QMessageBox::Ok);
}

void MainWindow::errorUnmatchedGameVersion()
{
    QMessageBox::critical(this, "Critical Error", "Only game version 1.1.0.0f-11-16 is supported.", QMessageBox::Ok);
}
