#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include <QMainWindow>
#include <QTcpSocket>
#include <QException>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT

public:
    Client(QWidget *parent = nullptr);
    ~Client();

private slots:
    void connected();  // 成功连接
    void disconnected(); // 连接断开
    void error(QAbstractSocket::SocketError socketError); // 失败

private slots:
    void on_connectButton_clicked();
    void on_quitButton_clicked();
    void on_loginButton_clicked();

private:
    Ui::Client *ui;
    QTcpSocket cmdSocket;   // 命令连接
    bool isConnect {false};   // 是否连接
    bool isLogin {false};     // 是否登录

private:
    void makeConnection();       // 建立信号槽
    void disableBeforeConnect(); // 禁止其他组件
    void ableAfterConnect();     // 允许其他组件
    void disableBeforeLogin(); // 禁止其他组件
    void ableAfterLsogin();     // 允许其他组件
    QString getResp();           // 获取回应
    QString putCMD(QString cmd); // 发送命令
    void writeCMDLine(QString str, WRITETYPE type); // 写入命令行

    // ftp命令
    void ftpLogin(QString user, QString passwd); // 登录
    void ftpQUIT(); // 退出
    void ftpUSER(QString user); // 用户名
    void ftpPASS(QString passwd); // 密码
};
#endif // CLIENT_H
