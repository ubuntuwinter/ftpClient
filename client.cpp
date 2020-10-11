#include "client.h"
#include "ui_client.h"
#include <QDebug>

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
{
    ui->setupUi(this);
    makeConnection();
    disableBeforeConnect();
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
    cmdSocket.connectToHost(host,
                            QString::number(port.toInt(), 16).toInt(nullptr, 16));
    cmdSocket.waitForConnected();
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
    writeCMDLine("Connection has been aborted.", INFO);
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

void Client::disableBeforeConnect()
{
    disableBeforeLogin();
    ui->loginBox->setEnabled(false);
    ui->hostEditor->setReadOnly(false);
    ui->portEditor->setReadOnly(false);
}

void Client::ableAfterConnect()
{
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
}

void Client::ableAfterLsogin()
{
    ui->userEditor->setReadOnly(true);
    ui->passwordEditor->setReadOnly(true);
}

QString Client::getResp()
{
    if (!isConnect) {
        return "";
    }

    if (cmdSocket.waitForReadyRead() == false) {
        return "Timeout.";
    }
    QString resp;

    while (cmdSocket.bytesAvailable()) {
        char buffer[1024] = { 0 };
        cmdSocket.read(buffer, 1024);

        while (buffer[strlen(buffer) - 1] == '\r' ||
               buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
        resp += buffer;
    }

    if ((resp[0] == '4') || (resp[0] == '5')) {
        throw resp;
    }

    return resp;
}

QString Client::putCMD(QString cmd)
{
    if (!isConnect) {
        return "";
    }
    cmdSocket.write(cmd.toLatin1(), cmd.length());

    if (cmdSocket.waitForBytesWritten() == false) {
        return "Timeout.";
    }
    return getResp();
}

void Client::writeCMDLine(QString str, WRITETYPE type)
{
    QString color = "black";

    switch (type) {
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
    QString html = "<p style='color:" + color + ";'>" + str + "</p>";

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
            ableAfterLsogin();
        }
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
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
    }
}
