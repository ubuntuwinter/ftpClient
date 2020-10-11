#include "client.h"
#include "ui_client.h"
#include <QDebug>

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);
    makeConnection();
    srand(time(nullptr));
    initializeModel();

    // disableBeforeConnect();
}

Client::~Client()
{
    delete ui;
}

void Client::on_connectButton_clicked()
{
    QString host = ui->hostEditor->text();
    QString port = ui->portEditor->text();

    if (host.isEmpty() || port.isEmpty()) {
        writeCMDLine("Please input host and port.", WARNING);
        return;
    }
    cmdSocket.abort();
    writeCMDLine("Connect to " + host + ":" + port + ".", INFO);
    cmdSocket.connectToHost(host, getPort(port));
    cmdSocket.waitForConnected(1000);
}

void Client::on_quitButton_clicked()
{
    ftpQUIT();
}

void Client::on_loginButton_clicked()
{
    QString user =  ui->userEditor->text();
    QString password = ui->passwordEditor->text();

    if (user.isEmpty()) {
        writeCMDLine("Please specify the username.", WARNING);
        return;
    }
    ftpLogin(user, password);
}

void Client::on_refreshButton_clicked()
{
    ftpLIST();
}

void Client::connected()
{
    isConnect = true;
    ableAfterConnect();
    writeCMDLine("Connection has been established.", INFO);
    writeCMDLine(getResp(),                          RECEIVE);
}

void Client::disconnected()
{
    isConnect = false;
    disableBeforeConnect();
    writeCMDLine("Connection has been aborted.",       INFO);
    writeCMDLine("----------------------------------", DEFAULT);
}

void Client::error(QAbstractSocket::SocketError)
{
    if (!isConnect) {
        writeCMDLine("Cann't connect server.", ERROR);
    }
}

void Client::makeConnection()
{
    connect(&cmdSocket, SIGNAL(connected()), this,
            SLOT(connected()));
    connect(&cmdSocket, SIGNAL(disconnected()), this,
            SLOT(disconnected()));
    connect(&cmdSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(error(QAbstractSocket::SocketError)));
}

void Client::initializeModel()
{
    ui->fileView->setModel(&model);

    // 设置表格标头
    model.setHorizontalHeaderItem(0, new QStandardItem("名称"));
    model.setHorizontalHeaderItem(1, new QStandardItem("修改日期"));
    model.setHorizontalHeaderItem(2, new QStandardItem("类型"));
    model.setHorizontalHeaderItem(3, new QStandardItem("大小"));

    // 设置列宽度
    ui->fileView->setColumnWidth(0, 150);
    ui->fileView->setColumnWidth(1, 100);
    ui->fileView->setColumnWidth(2, 75);
    ui->fileView->setColumnWidth(3, 50);
}

void Client::disableBeforeConnect()
{
    disableBeforeLogin();
    ui->connectButton->setEnabled(true);
    ui->quitButton->setEnabled(false);
    ui->loginBox->setEnabled(false);
    ui->hostEditor->setReadOnly(false);
    ui->portEditor->setReadOnly(false);
}

void Client::ableAfterConnect()
{
    ui->connectButton->setEnabled(false);
    ui->quitButton->setEnabled(true);
    ui->loginBox->setEnabled(true);
    ui->hostEditor->setReadOnly(true);
    ui->portEditor->setReadOnly(true);
}

void Client::disableBeforeLogin()
{
    ui->userEditor->setText("");
    ui->passwordEditor->setText("");
    ui->userEditor->setReadOnly(false);
    ui->passwordEditor->setReadOnly(false);
    ui->loginButton->setEnabled(true);
    ui->modeBox->setChecked(false);
    ui->pwdEdit->setText("");
    ui->fileBox->setEnabled(false);
}

void Client::ableAfterLogin()
{
    ui->userEditor->setReadOnly(true);
    ui->passwordEditor->setReadOnly(true);
    ui->loginButton->setEnabled(false);
    ui->fileBox->setEnabled(true);
    ftpPWD();
    ftpLIST();
}

void Client::squeeze()
{
    char buffer[1024] = { 0 };

    while (cmdSocket.bytesAvailable()) {
        cmdSocket.readLine(buffer, 1024);
    }
}

quint16 Client::getPort(QString str)
{
    return QString::number(str.toInt(), 16).toInt(nullptr,
                                                  16);
}

void Client::createDataSocket()
{
    if (!ui->modeBox->isChecked()) {
        dataSocket = new QTcpSocket;
        dataSocket->connectToHost(ip, getPort(port));
        dataSocket->waitForConnected(1000);
    }
    else {
        if (listenSocket.waitForNewConnection(1000)) {
            dataSocket = listenSocket.nextPendingConnection();
            listenSocket.close();
        }
        else {
            listenSocket.close();
            throw QString("Time out.");
        }
    }
}

void Client::dealWithDataSocket(DEALMODE mode)
{
    if (dataSocket->waitForReadyRead()) {
        QString resp;

        switch (mode) {
        case LIST:

            while (dataSocket->bytesAvailable()) {
                char buffer[1024] = { 0 };
                dataSocket->readLine(buffer, 1024);
                resp += buffer;
            }
            break;

        case RETR:
        case STOR:
            break;
        }
    }
}

void Client::closeDataSocket()
{
    dataSocket->close();
    delete dataSocket;
}

void Client::putLine(QString cmd)
{
    if (!isConnect) {
        throw QString("Time out.");
    }
    cmdSocket.write(cmd.toLatin1(), cmd.length());
    cmdSocket.waitForBytesWritten(1000);
}

QString Client::getResp()
{
    if (!isConnect) {
        return "";
    }

    if (cmdSocket.waitForReadyRead(1000) == false) {
        throw QString("Time out.");
    }
    QString resp;

    char buffer[1024] = { 0 };

    while (cmdSocket.bytesAvailable()) {
        cmdSocket.readLine(buffer, 1024);
        bool readline = false;

        while (buffer[strlen(buffer) - 1] == '\r' ||
               buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
            readline = true;
        }

        resp += buffer;

        if (readline) {
            break;
        }
    }

    if ((resp[0] == '4') || (resp[0] == '5')) {
        throw resp;
    }

    return resp;
}

QString Client::putCMD(QString cmd)
{
    putLine(cmd);
    return getResp();
}

void Client::writeCMDLine(QString str, WRITETYPE type)
{
    QString color = "black";

    switch (type) {
    case DEFAULT:
        break;

    case INFO:
        color = "#000099"; break;

    case SEND:
        color = "#660099"; break;

    case RECEIVE:
        color = "#336633"; break;

    case DATA:
        color = "#333333"; break;

    case WARNING:
        color = "#cc9900"; break;

    case ERROR:
        color = "#cc0000"; break;
    }
    QString html = "<span style='color:" + color + ";'>" + str + "</span><br/>";

    ui->cmdLine->append(html);
}

void Client::ftpLogin(QString user, QString passwd)
{
    try {
        writeCMDLine("USER " + user, SEND);
        QString resp = putCMD("USER " + user + "\r\n");
        writeCMDLine(resp,           RECEIVE);

        if (resp[0] == '3') {
            writeCMDLine("PASS " + passwd, SEND);
            resp = putCMD("PASS " + passwd + "\r\n");
            writeCMDLine(resp,             RECEIVE);
        }

        if (resp[0] == '2') {
            ableAfterLogin();
        }
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpQUIT()
{
    try {
        writeCMDLine("QUIT",             SEND);
        writeCMDLine(putCMD("QUIT\r\n"), RECEIVE);
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpPWD()
{
    try {
        writeCMDLine("PWD", SEND);
        QString resp = putCMD("PWD\r\n");
        writeCMDLine(resp,  RECEIVE);

        // QString pattern = "(?<=\").*(?=\")"; 其实用正向、反向肯定预查更好
        QRegExp reg("\".*\"");
        int     pos = resp.indexOf(reg);

        if (pos >= 0) {
            QString match = reg.cap(0);
            match.remove(0, 1); match.remove(match.length() - 1, 1);
            ui->pwdEdit->setText(match);
        }
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpTYPE()
{
    try {
        writeCMDLine("TYPE I", SEND);
        QString resp = putCMD("TYPE I\r\n");
        writeCMDLine(resp,     RECEIVE);
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpPORT()
{
    QString cmd = "PORT ";
    int     port1 = 128 + (rand() % 64);
    int     port2 = rand() % 256;

    port = QString::number(port1 * 256 + port2);
    listenSocket.close();
    listenSocket.listen(QHostAddress::Any, getPort(port));
    QStringList list = cmdSocket.localAddress().toString().split(".");

    while (!listenSocket.isListening()) {
        QThread::msleep(100);
    }

    cmd += list[0] + "," + list[1] + "," + list[2] + "," + list[3] + "," +
           QString::number(port1) + "," + QString::number(port2);
    writeCMDLine(cmd,                  SEND);
    writeCMDLine(putCMD(cmd + "\r\n"), RECEIVE);
}

void Client::ftpPASV()
{
    writeCMDLine("PASV", SEND);
    QString resp = putCMD("PASV\r\n");

    writeCMDLine(resp,   RECEIVE);

    QRegExp reg("[(].*[)]");
    int     pos = resp.indexOf(reg);

    if (pos >= 0) {
        QString match = reg.cap(0);
        match.remove(0, 1); match.remove(match.length() - 1, 1);
        QStringList list = match.split(",");
        ip = list[0] + "." + list[1] + "." + list[2] + "." + list[3];
        port = QString::number(list[4].toInt() * 256 + list[5].toInt());
    }
}

void Client::ftpLIST()
{
    ftpTYPE();

    try {
        if (!ui->modeBox->isChecked()) {
            ftpPASV();
        } else {
            ftpPORT();
        }

        // 键入LIST
        writeCMDLine("LIST", SEND);
        putLine("LIST\r\n");

        // 打开数据传输
        createDataSocket();

        // 回应消息1
        QString resp = getResp();
        writeCMDLine(resp, RECEIVE);

        // 处理数据
        dealWithDataSocket(LIST);

        // 关闭数据传输
        closeDataSocket();

        // 回应消息2
        resp = getResp();
        writeCMDLine(resp, RECEIVE);
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}
