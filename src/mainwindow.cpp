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
#elif defined Q_OS_WIN
    DWORD processes[1024], needed;
    if (!EnumProcesses( processes, sizeof(processes), &needed))
    {
        return;
    }
    auto processes_count = needed / sizeof(DWORD);
    for (auto i = 0; i < processes_count; i++)
    {
        auto pid = processes[i];
        if(pid != 0)
        {
            auto process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (process != NULL)
            {
                HMODULE main_module;
                if (EnumProcessModules(process, &main_module, sizeof(main_module), &needed))
                {
                    TCHAR name[MAX_PATH] = TEXT("");
                    GetModuleBaseName(process, main_module, name, sizeof(name)/sizeof(TCHAR));
                    if (QString::fromWCharArray(name) == "5dchesswithmultiversetimetravel.exe")
                    {
                        ui->comboBoxProcess->addItem(QString::number(pid));
                    }
                }
            }
            CloseHandle(process);
        }
    }
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
    auto process = openSelectedProcess();
#ifdef Q_OS_WIN
    if (process == 0)
    {
        return;
    }
#endif
    qint64 base, size;
    HANDLE_ERR(processMemoryBaseSize(process, base, size));

//    QByteArray version_bytes;
//    HANDLE_ERR(readProcessMemory(process, base + OFFSET_VERSION_0, 9, version_bytes));
//    if (version_bytes == VERSION_0)
//    {
//        HANDLE_ERR(readProcessMemory(process, base + OFFSET_VERSION_1, 8, version_bytes));
//        qint64 version_1;
//        QDataStream version_datastream(version_bytes);
//        version_datastream.setByteOrder(QDataStream::LittleEndian);
//        version_datastream >> version_1;
//        if (version_1 == VERSION_1)
//        {
//            HANDLE_ERR(readProcessMemory(process, base + OFFSET_VERSION_2, 8, version_bytes));
//            qint64 version_2;
//            QDataStream version_datastream(version_bytes);
//            version_datastream.setByteOrder(QDataStream::LittleEndian);
//            version_datastream >> version_2;
//            if (version_2 == VERSION_2)
//            {
//                QByteArray address;
//                QByteArray port;
//                HANDLE_ERR(addressAndPort(address, port));
//                HANDLE_ERR(writeProcessMemory(process, base + OFFSET_ADDRESS, address));
//                HANDLE_ERR(writeProcessMemory(process, base + OFFSET_PORT, port));
//                infoSuccess();
//                return;
//            }
//        }
//    }
//    errorUnmatchedGameVersion();

    QByteArray address, port;
    HANDLE_ERR(addressAndPort(address, port));
    if (address != OFFICIAL_ADDRESS)
    {
        auto target = searchProcessMemory(process, base, size, OFFICIAL_ADDRESS);
        HANDLE_ERR(target);
        HANDLE_ERR(writeProcessMemory(process, target, address));
    }
    if (port != OFFICIAL_PORT)
    {
        auto target = searchProcessMemory(process, base, size, OFFICIAL_PORT);
        HANDLE_ERR(target);
        HANDLE_ERR(writeProcessMemory(process, target, port));
    }
    closeProcess(process);
    infoSuccess();
}

result_type MainWindow::processMemoryBaseSize(process_type process, qint64 &base, qint64 &size)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/maps");
    if (!f.exists())
    {
        errorProcessInvalid();
        return ERR;
    }
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return ERR;
    }

    auto parts = f.readLine().split('-');
    if (parts.size() < 2)
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    base = parts[0].toULongLong(nullptr, 16);
    if (base <= 0)
    {
        errorCannotAccessGameMemory();
        return ERR;
    }

    qint64 addr = 0;
    for (QByteArray line = f.readLine(); line.contains(QByteArrayLiteral("5dchesswithmultiversetimetravel")); line = f.readLine())
    {
        parts = f.readLine().split('-');
        if (parts.size() < 2)
        {
            errorCannotAccessGameMemory();
            return ERR;
        }
        parts = parts[1].split(' ');
        if (parts.size() < 2)
        {
            errorCannotAccessGameMemory();
            return ERR;
        }
        addr = parts[0].toULongLong(nullptr, 16);
        if (addr <= 0)
        {
            errorCannotAccessGameMemory();
            return ERR;
        }
    }
    if (addr <= base)
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    size = addr - base;
#elif defined Q_OS_WIN
    HMODULE main_module;
    DWORD needed;
    if (!EnumProcessModules(process, &main_module, sizeof(main_module), &needed))
    {
        return ERR;
    }
    MODULEINFO main_module_info;
    if (!GetModuleInformation(process, main_module, &main_module_info, sizeof(MODULEINFO)))
    {
        return ERR;
    }
    base = (qint64)main_module_info.lpBaseOfDll;
    size = main_module_info.SizeOfImage;
#endif
    return OK;
}

result_type MainWindow::readProcessMemory(process_type process, qint64 addr, qint64 size, QByteArray &data)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorProcessInvalid();
        return ERR;
    }
    f.open(QIODevice::ReadOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    f.seek(addr);
    data = f.read(size);
    if (data.size() != size)
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
#elif defined Q_OS_WIN
    data.resize(size);
    SIZE_T read_size;
    if (!ReadProcessMemory(process, (LPCVOID)addr, (LPVOID)data.data(), (SIZE_T)size, &read_size))
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    if (read_size != (SIZE_T)size)
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
#endif
    return OK;
}

result_type MainWindow::writeProcessMemory(process_type process, qint64 addr, const QByteArray &data)
{
#ifdef Q_OS_LINUX
    QFile f("/proc/" + process + "/mem");
    if (!f.exists())
    {
        errorProcessInvalid();
        return ERR;
    }
    f.open(QIODevice::WriteOnly);
    if (!f.isOpen())
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    f.seek(addr);
    if (f.write(data) != data.size())
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
#elif defined Q_OS_WIN
    DWORD old_protect;
    if (!VirtualProtectEx(process, (LPVOID)addr, (SIZE_T)data.size(), PAGE_EXECUTE_READWRITE, &old_protect))
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    SIZE_T write_size;
    if (!WriteProcessMemory(process, (LPVOID)addr, (LPCVOID)data.data(), (SIZE_T)data.size(), &write_size))
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
    if (write_size != (SIZE_T)data.size())
    {
        errorCannotAccessGameMemory();
        return ERR;
    }
#endif
    return OK;
}

qint64 MainWindow::searchProcessMemory(process_type process, qint64 base, qint64 size, const QByteArray &data)
{
    QByteArray memory;
    readProcessMemory(process, base, size, memory);
    auto offset = memory.indexOf(data);
    if (offset < 0)
    {
        errorFailedSearchGameMemory();
        return -1;
    }
    return base + offset;
}

bool MainWindow::hasSelectedProcess()
{
    return ui->comboBoxProcess->currentText().length() != 0;
}

process_type MainWindow::openSelectedProcess()
{
#ifdef Q_OS_LINUX
    return ui->comboBoxProcess->currentText();
#elif defined Q_OS_WIN
    DWORD pid = ui->comboBoxProcess->currentText().toULong();
    auto process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (process == NULL)
    {
        errorProcessInvalid();
    }
    return process;
#endif
}

void MainWindow::closeProcess(process_type process)
{
#ifdef Q_OS_WIN
    CloseHandle(process);
#endif
}

result_type MainWindow::addressAndPort(QByteArray &address, QByteArray &port)
{
    if (ui->lineEditAddress->palette().color(QPalette::Text) == Qt::GlobalColor::gray)
    {
        errorAddressInvalid();
        return ERR;
    }
    auto text = ui->lineEditAddress->text();
    if (text.length() == 0)
    {
        errorAddressInvalid();
        return ERR;
    }
    if (text.contains(":"))
    {
        auto parts = text.split(":");
        if (parts.length() != 2)
        {
            errorAddressInvalid();
            return ERR;
        }
        if (parts[0].length() > 44)
        {
            errorAddressTooLong();
            return ERR;
        }
        address = parts[0].toUtf8();
        address.append('\0');
        if (parts[1].toUShort() == 0)
        {
            errorAddressInvalid();
            return ERR;
        }
        port = parts[1].toUtf8();
        port.append('\0');
    }
    else
    {
        if (text.length() > 44)
        {
            errorAddressTooLong();
            return ERR;
        }
        address = text.toUtf8();
        address.append('\0');
        port = QByteArrayLiteral("39005\0");
    }
    return OK;
}

void MainWindow::infoProcessNotSelected()
{
    QMessageBox::information(this, tr("Information"), tr("No process is selected. Make sure the game is running."), QMessageBox::Ok);
}

void MainWindow::infoSuccess()
{
    QMessageBox::information(this, tr("Information"), tr("Server address modified successfully."), QMessageBox::Ok);
}

void MainWindow::errorAddressInvalid()
{
    QMessageBox::critical(this, tr("Error"), tr("Address is invalid."), QMessageBox::Ok);
}

void MainWindow::errorAddressTooLong()
{
    QMessageBox::critical(this, tr("Error"), tr("Address (excluding port) cannot be longer than 44 characters."), QMessageBox::Ok);
}

void MainWindow::errorProcessInvalid()
{
    QMessageBox::critical(this, tr("Error"), tr("Process is invalid. Make sure the correct process is selected."), QMessageBox::Ok);
}

void MainWindow::errorCannotAccessGameMemory()
{
#ifdef Q_OS_LINUX
    QMessageBox::critical(this, tr("Error"), tr("Cannot access game memory. Root permission may be required."), QMessageBox::Ok);
#elif defined Q_OS_WIN
    QMessageBox::critical(this, tr("Error"), tr("Cannot access game memory."), QMessageBox::Ok);
#endif
}

void MainWindow::errorFailedSearchGameMemory()
{
    QMessageBox::critical(this, tr("Error"), tr("Cannot find specific location in game memory. Game may need to restart."), QMessageBox::Ok);
}

//void MainWindow::errorUnmatchedGameVersion()
//{
//    QMessageBox::critical(this, tr("Error"), tr("Only game version 1.1.0.0f-11-16 is supported."), QMessageBox::Ok);
//}
