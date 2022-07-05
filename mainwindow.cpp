#include "mainwindow.h"
#include "ui_mainwindow.h"

const QByteArray VERSION_0 = QByteArrayLiteral("1.1.0.0f\x00");

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
    auto base = processMemoryBase(process);
    RET_VOID_ON_ERR(base);
    char version_buffer[9];
    RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_0, 9, version_buffer));
    QByteArray version_0(version_buffer, 9);
    if (version_0 == VERSION_0)
    {
        RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_1, 8, version_buffer));
        qint64 version_1;
        QDataStream version_datastream(QByteArray(version_buffer, 8));
        version_datastream.setByteOrder(QDataStream::LittleEndian);
        version_datastream >> version_1;
        if (version_1 == VERSION_1)
        {
            RET_VOID_ON_ERR(readProcessMemory(process, base + OFFSET_VERSION_2, 8, version_buffer));
            qint64 version_2;
            QDataStream version_datastream(QByteArray(version_buffer, 8));
            version_datastream.setByteOrder(QDataStream::LittleEndian);
            version_datastream >> version_2;
            if (version_2 == VERSION_2)
            {
                QByteArray address;
                QByteArray port;
                RET_VOID_ON_ERR(addressAndPort(address, port));
                RET_VOID_ON_ERR(writeProcessMemory(process, base + OFFSET_ADDRESS, address.length(), address));
                RET_VOID_ON_ERR(writeProcessMemory(process, base + OFFSET_PORT, port.length(), port));
                infoSuccess();
                return;
            }
        }
    }
    errorUnmatchedGameVersion();
}

qint64 MainWindow::processMemoryBase(process_handle_t process)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/maps");
    if (!f.exists())
    {
        return 0;
    }
    f.open(QIODevice::ReadOnly);
    qint64 base = 0;
    char buffer[1];
    while (f.isReadable())
    {
        f.read(buffer, 1);
        char c = buffer[0];
        if ('0' <= c && c <= '9')
        {
            base *= 16;
            base += c - '0';
        }
        else if ('a' <= c && c <= 'f')
        {
            base *= 16;
            base += c - 'a' + 10;
        }
        else
        {
            return base;
        }
    }
    errorCannotAccessGameMemory();
    return -1;
#elif Q_OS_WIN
#endif
}

bool MainWindow::readProcessMemory(process_handle_t process, qint64 addr, qint64 size, char *buffer)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorCannotAccessGameMemory();
        return false;
    }
    f.open(QIODevice::ReadOnly);
    f.seek(addr);
    if (f.read(buffer, size) != size)
    {
        errorCannotAccessGameMemory();
        return false;
    }
#elif Q_OS_WIN
#endif
    return true;
}

bool MainWindow::writeProcessMemory(process_handle_t process, qint64 addr, qint64 size, const char *buffer)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorCannotAccessGameMemory();
        return false;
    }
    f.open(QIODevice::WriteOnly);
    f.seek(addr);
    if (f.write(buffer, size) != size)
    {
        errorCannotAccessGameMemory();
        return false;
    }
#elif Q_OS_WIN
#endif
    return true;
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

void MainWindow::errorCannotAccessGameMemory()
{
    QMessageBox::critical(this, "Critical Error", "Cannot access game memory. Root permission may be required.", QMessageBox::Ok);
}

void MainWindow::errorUnmatchedGameVersion()
{
    QMessageBox::critical(this, "Critical Error", "Only game version 1.1.0.0f-11-16 is supported.", QMessageBox::Ok);
}
