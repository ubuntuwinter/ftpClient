#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include <QMainWindow>
#include <QTcpSocket>
#include <QTcpServer>
#include <QException>
#include <QMessageBox>
#include <QHostAddress>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QMainWindow {
    Q_OBJECT

public:

    Client(QWidget *parent = nullptr);
    ~Client();

private slots:

    void connected();                                     // 成功连接
    void disconnected();                                  // 连接断开
    void error(QAbstractSocket::SocketError socketError); // 失败

private slots:

    void on_connectButton_clicked();
    void on_quitButton_clicked();
    void on_loginButton_clicked();
    void on_refreshButton_clicked();

private:

    Ui::Client *ui;
    QTcpSocket cmdSocket;     // 命令连接
    QTcpSocket *dataSocket;   // 数据连接
    QTcpServer listenSocket;  // 监听连接
    QString ip;               // ip地址
    QString port;             // 端口
    bool isConnect { false }; // 是否连接
    bool isLogin { false };   // 是否登录

private:

    void    makeConnection();                  // 建立信号槽
    void    disableBeforeConnect();            // 禁止其他组件
    void    ableAfterConnect();                // 允许其他组件
    void    disableBeforeLogin();              // 禁止其他组件
    void    ableAfterLogin();                  // 允许其他组件
    void    squeeze();                         // 榨干Socket
    quint16 getPort(QString str);              // 获取16进制表示的端口
    void    createDataSocket();                // 创建数据连接
    void    dealWithDataSocket(DEALMODE mode); // 读取DataSocket的内容
    void    closeDataSocket();                 // 关闭数据连接
    void    putLine(QString cmd);              // 发送命令
    QString getResp();                         // 获取回应
    QString putCMD(QString cmd);               // 发送命令
    void    writeCMDLine(QString   str,
                         WRITETYPE type);      // 写入命令行

    // ftp命令
    void ftpLogin(QString user,
                  QString passwd); // 登录
    void ftpQUIT();                // 退出
    void ftpUSER(QString user);    // 用户名
    void ftpPASS(QString passwd);  // 密码
    void ftpPWD();                 // 显示当前目录
    void ftpTYPE();                // 编码
    void ftpPORT();                // 主动模式
    void ftpPASV();                // 被动模式
    void ftpLIST();                // 显示当前目录下文件
};
#endif // CLIENT_H
