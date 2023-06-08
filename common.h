#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QObject>
#include <QTcpSocket>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

signals:
    void connectOK();
    void connectERR();
    void loginOK();
    void loginFailed();
    void RegFailed();

public slots:
    void connectToServer(QString ipAddress, quint16 port);  // 连接服务器
    void disconnectFromServer();   // 断开连接
    void sendMessage(QString message);  // 发送消息

private:
    QTcpSocket* m_tcpSocket;

private slots:
    void readMessage();  // 响应通信消息的槽函数
};

#endif // COMMON_H
