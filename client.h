#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include <QMainWindow>
#include <QTcpSocket>
#include <QTcpServer>
#include <QException>
#include <QMessageBox>
#include <QHostAddress>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QFileDialog>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class Client; }
QT_END_NAMESPACE

class Client : public QMainWindow {
    Q_OBJECT

public:

    Client(QWidget *parent = nullptr);
    ~Client();

protected:

    void closeEvent(QCloseEvent *) override;

private slots:

    void connected();    // 成功连接
    void disconnected(); // 连接断开

private slots:

    void on_connectButton_clicked();                          // 连接
    void on_quitButton_clicked();                             // 退出连接
    void on_loginButton_clicked();                            // 登录
    void on_refreshButton_clicked();                          // 刷新
    void on_newButton_clicked();                              // 新建
    void on_deleteButton_clicked();                           // 删除
    void on_uploadButton_clicked();                           // 上传
    void on_downloadButton_clicked();                         // 下载
    void on_renameButton_clicked();                           // 重命名
    void on_fileView_doubleClicked(const QModelIndex& index); // 双击
    void on_abortButton_clicked();                            // 中止

private:

    Ui::Client *ui;
    QTcpSocket cmdSocket;               // 命令连接
    QTcpSocket *dataSocket { nullptr }; // 数据连接
    QTcpServer listenSocket;            // 监听连接
    QString ip;                         // ip地址
    QString port;                       // 端口
    QStandardItemModel *model;          // 文件数据模型
    bool canWrite { true };             // 是否写入命令行
    bool isConnect { false };           // 是否连接
    bool refresh { false };             // 是否登录
    bool info{ false };                 // 是否提示

private:

    void           makeConnection();                           // 建立信号槽
    void           initializeModel();                          // 初始化文件模型
    void           disableBeforeConnect();                     // 禁止其他组件
    void           ableAfterConnect();                         // 允许其他组件
    void           disableBeforeLogin();                       // 禁止其他组件
    void           ableAfterLogin();                           // 允许其他组件
    void           squeeze();                                  // 榨干Socket
    quint16        getPort(QString str);                       // 获取16进制表示的端口
    void           createDataSocket();                         // 创建数据连接
    void           dealWithDataSocket(DEALMODE mode,
                                      QString  filename = ""); // 读取DataSocket的内容
    void           closeDataSocket();                          // 关闭数据连接
    void           putLine(QString cmd);                       // 发送命令
    QString        getResp();                                  // 获取回应
    QString        putCMD(QString cmd);                        // 发送命令
    void           writeCMDLine(QString   str,
                                WRITETYPE type);               // 写入命令行
    static QString getSizeString(QString size);                // 获取大小的字符串表示

    // ftp命令
    void           ftpLogin(QString user,
                            QString passwd);        // 登录
    void           ftpABOR();                       // 中止
    void           ftpQUIT();                       // 退出
    void           ftpUSER(QString user);           // 用户名
    void           ftpPASS(QString passwd);         // 密码
    void           ftpSYST();                       // 显示系统信息
    void           ftpPWD();                        // 显示当前目录
    void           ftpTYPE();                       // 编码
    void           ftpPORT();                       // 主动模式
    void           ftpPASV();                       // 被动模式
    void           ftpLIST();                       // 显示当前目录下文件
    void           ftpCWD(QString dirname);         // 进入目录
    void           ftpMKD(QString newDirName);      // 新建文件夹
    void           ftpRMD(QString dirname);         // 删除文件夹
    void           ftpDELE(QString filename);       // 删除文件
    void           ftpRename(QString oldname,
                             QString newname);      // 重命名文件
    void           ftpRNFR(QString name);           // 重命名FROM
    void           ftpRNTO(QString name);           // 重命名TO
    void           ftpSTOR(QString localFileName,
                           QString remoteFileName); // 上传文件
    void           ftpRETR(QString localFileName,
                           QString remoteFileName); // 下载文件
};
#endif // CLIENT_H
