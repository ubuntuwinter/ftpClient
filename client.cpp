#include "client.h"
#include "ui_client.h"
#include <QDebug>

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client), model(new QStandardItemModel)
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
    delete model;
}

void Client::closeEvent(QCloseEvent *)
{
    if (isConnect) {
        cmdSocket.close();
        isConnect = false;
        disableBeforeConnect();
    }
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

    if (cmdSocket.waitForConnected(3000) == false) {
        QMessageBox::critical(this, "Error", "Cann't connect server.");
        writeCMDLine("Cann't connect server.", ERROR);
    }
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

void Client::on_newButton_clicked()
{
    QString newDirName = QInputDialog::getText(this, "新建文件夹", "请输入新建文件夹的名称");

    if (newDirName.isEmpty()) return;

    ftpMKD(newDirName);

    if (refresh) { ftpLIST(); refresh = false; }
}

void Client::on_deleteButton_clicked()
{
    QModelIndex index = ui->fileView->currentIndex();

    if (model->data(model->index(index.row(), 2)).toString() == "文件夹") {
        ftpRMD(model->data(model->index(index.row(), 0)).toString());
    } else {
        ftpDELE(model->data(model->index(index.row(), 0)).toString());
    }

    if (refresh) { ftpLIST();  refresh = false; }
}

void Client::on_renameButton_clicked()
{
    QModelIndex index = ui->fileView->currentIndex();
    QString     newName = QInputDialog::getText(this, "重命名", "请输入新的名称");

    if (newName.isEmpty()) return;

    ftpRename(model->data(model->index(index.row(), 0)).toString(), newName);

    if (refresh) { ftpLIST();  refresh = false; }
}

void Client::on_fileView_doubleClicked(const QModelIndex& index)
{
    if (model->data(model->index(index.row(), 2)).toString() == "文件夹") {
        ftpCWD(model->data(model->index(index.row(), 0)).toString());

        if (refresh) { ftpPWD(); ftpLIST(); refresh = false; }
    }
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

void Client::makeConnection()
{
    connect(&cmdSocket, SIGNAL(connected()), this,
            SLOT(connected()));
    connect(&cmdSocket, SIGNAL(disconnected()), this,
            SLOT(disconnected()));
}

void Client::initializeModel()
{
    // 设置表格标头
    model->setHorizontalHeaderItem(0, new QStandardItem("名称"));
    model->setHorizontalHeaderItem(1, new QStandardItem("修改日期"));
    model->setHorizontalHeaderItem(2, new QStandardItem("类型"));
    model->setHorizontalHeaderItem(3, new QStandardItem("大小"));

    // 按类型排序
    model->sort(2, Qt::DescendingOrder);

    // 应用模型
    ui->fileView->setModel(model);
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
    model->clear();
    initializeModel();
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
        dataSocket->waitForConnected(3000);
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
    switch (mode) {
    case LIST: {
        QString resp;

        if (dataSocket->waitForReadyRead(1000)) {
            while (dataSocket->bytesAvailable()) {
                char buffer[1024] = { 0 };
                dataSocket->readLine(buffer, 1024);
                resp += buffer;
            }
        }

        QStringList list = resp.split("\r\n");

        model->clear();

        for (auto iter = list.begin(); iter != list.end() - 1; ++iter) {
            QStringList listlist = (*iter).simplified().split(" ");
            QList<QStandardItem *> row;
            row.append(new QStandardItem(listlist[NAME]));
            row.append(new QStandardItem(listlist[MONTH] + " " +
                                         listlist[DAY] + " " +
                                         listlist[TIME]));
            row.append(new QStandardItem(listlist[PERMISSION][0] ==
                                         'd' ? "文件夹" : ""));

            row.append(new QStandardItem(listlist[PERMISSION][0]
                                         == 'd' ? "" : getSizeString(listlist[
                                                                         SIZE])));
            model->appendRow(row);
        }

        // 加入..
        QList<QStandardItem *> row;
        row.append(new QStandardItem(".."));
        row.append(new QStandardItem(""));
        row.append(new QStandardItem("文件夹"));
        row.append(new QStandardItem(""));
        model->insertRow(0, row);

        initializeModel();
        break;
    }

    case RETR:
    case STOR:
        break;
    }

    // 关闭数据传输
    closeDataSocket();
}

void Client::closeDataSocket()
{
    dataSocket->close();
    delete dataSocket;
}

void Client::putLine(QString cmd)
{
    if (!isConnect) {
        throw QString("Not connect.");
    }
    cmdSocket.write(cmd.toLatin1(), cmd.length());
    cmdSocket.waitForBytesWritten(3000);
}

QString Client::getResp()
{
    if (!isConnect) {
        return "";
    }

    if (cmdSocket.waitForReadyRead(3000) == false) {
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

QString Client::getSizeString(QString o_size)
{
    long long size = o_size.toLongLong();
    QVector<long long> sizelist;

    long long base = 1;

    for (int i = 0; i < 4; i++) {
        base *= 1024;
    }

    while (size > 0 && base > 0) {
        sizelist.push_back(size / base);
        size %= base;
        base /= 1024;
    }

    QString sizeString;

    for (int i = 0; i < sizelist.size(); i++) {
        if (sizelist[i] != 0) {
            sizeString += QString::number(sizelist[i]);

            switch (i) {
            case 0:
                sizeString += "TB";
                break;

            case 1:
                sizeString += "GB";
                break;

            case 2:
                sizeString += "MB";
                break;

            case 3:
                sizeString += "KB";
                break;

            case 4:
                sizeString += "B";
                break;
            }
            break;
        }
    }
    return sizeString;
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

        // 回应消息2
        resp = getResp();
        writeCMDLine(resp, RECEIVE);
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpCWD(QString dirname)
{
    try {
        QString cmd = "CWD " + dirname;
        writeCMDLine(cmd,  SEND);
        QString resp = putCMD(cmd + "\r\n");
        writeCMDLine(resp, RECEIVE);
        refresh = true;
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpMKD(QString newDirName)
{
    try {
        QString cmd = "MKD " + newDirName;
        writeCMDLine(cmd,  SEND);
        QString resp = putCMD(cmd + "\r\n");
        writeCMDLine(resp, RECEIVE);
        refresh = true;
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpRMD(QString dirname)
{
    try {
        QString cmd = "RMD " + dirname;
        writeCMDLine(cmd,  SEND);
        QString resp = putCMD(cmd + "\r\n");
        writeCMDLine(resp, RECEIVE);
        refresh = true;
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpDELE(QString filename)
{
    try {
        QString cmd = "DELE " + filename;
        writeCMDLine(cmd,  SEND);
        QString resp = putCMD(cmd + "\r\n");
        writeCMDLine(resp, RECEIVE);
        refresh = true;
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpRename(QString oldname, QString newname)
{
    try {
        ftpRNFR(oldname);
        ftpRNTO(newname);
        refresh = true;
    } catch (QString& e) {
        QMessageBox::critical(this, "Error", e);
        writeCMDLine(e, ERROR);
        squeeze();
    }
}

void Client::ftpRNFR(QString name)
{
    QString cmd = "RNFR " + name;

    writeCMDLine(cmd,  SEND);
    QString resp = putCMD(cmd + "\r\n");

    writeCMDLine(resp, RECEIVE);
}

void Client::ftpRNTO(QString name)
{
    QString cmd = "RNTO " + name;

    writeCMDLine(cmd,  SEND);
    QString resp = putCMD(cmd + "\r\n");

    writeCMDLine(resp, RECEIVE);
}
