#include "common.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

Client::Client(QObject *parent) : QObject(parent)
{
    m_tcpSocket = new QTcpSocket(this);

    // 连接QTcpSocket的readyRead()信号到readMessage()槽函数
    connect(m_tcpSocket, &QTcpSocket::readyRead,
            this, &Client::readMessage);

    connect(m_tcpSocket, &QTcpSocket::connected,
            this,[=](){emit connectOK();});
}

void Client::disconnectFromServer()
{
    qDebug() << "Disconnect from server...";

    m_tcpSocket->close();
}

void Client::connectToServer(QString ipAddress, quint16 port)
{
    qDebug() << "Connect to server...";

    m_tcpSocket->connectToHost(ipAddress, port);
    if (m_tcpSocket->waitForConnected())
    {
        qDebug() << "Connet Succes";
    }
    else
    {
        qDebug() << "Error";
        emit connectERR();
    }
    return;
}

void Client::sendMessage(QString message)
{
    QByteArray data = message.toUtf8();

    qint64 bytes = m_tcpSocket->write(data);
    if (bytes != data.size()) {
        emit connectERR();
        qWarning() << "Send error: " << bytes << "/" << data.size();
    } else {
        qDebug() << "Send message: " << message;
    }
}

int DataPro(QByteArray data)
{
    QString loginOK = "200";
    int len = data.size();
    qDebug() << len;
    if (len < loginOK.length())
    {
        return 0;
    }
    int i = 0;
    char *data2 = data.data();
    for (i = 0; i < 3; i ++)
    {
        while (data2[i] != loginOK[i])
        {
            qDebug() << data2[i];
            return 0;
        }
    }
    return 1;
}

void Client::readMessage()
{
    QByteArray data = m_tcpSocket->readAll();
    qDebug() << "Receive message: " << data;
    int iret = 0;
    iret = DataPro(data);
    if (1 == iret)
    {
        m_tcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        emit loginOK();
        qDebug() << "OK";
    }
    else
    {
        char *data2 = data.data();
        if (data2[0] == '9')
        {
            if (data2[1] == '9')
            {
                emit RegFailed();
            }
            else
            {
                emit loginFailed();
            }
        }

        qDebug() << "Error";
    }
}

void Client::sendFile(QString path)
{
    QFile file(path);
    QFileInfo info(path);
    int fileSize = info.size();
    QString str = QString::number(fileSize);
    QString str1 = "sendF:" + str;
    sendMessage(str1);
    m_tcpSocket->flush();
    file.open(QFile::ReadOnly);
    int num = 0;
    while(!file.atEnd())
    {
        QByteArray line = file.read(1024);
        m_tcpSocket->write(line);
        num += line.size();
    }
    int percent = 0;
    percent = (num/fileSize)*100;
    file.close();
    return;
}
