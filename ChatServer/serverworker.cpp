#include "serverworker.h"
#include"QDataStream"
#include<QJsonObject>
#include<qjsondocument.h>
#include<QThread>

ServerWorker::ServerWorker(QObject *parent)
    : QObject{parent}
{
    m_serverSocket=new QTcpSocket(this);
    connect(m_serverSocket,&QTcpSocket::readyRead,this,&ServerWorker::onReadyRead);
    connect(m_serverSocket,&QTcpSocket::disconnected,this,&ServerWorker::disconnectedFromClient);


}

bool ServerWorker::setSocketDestriptor(qintptr socketDescriptor)
{
    return m_serverSocket->setSocketDescriptor(socketDescriptor);
}

QString ServerWorker::userName()
{
    return m_userName;
}

void ServerWorker::setUserName(QString user)
{
    m_userName=user;
}

QTcpSocket *ServerWorker::getSocket()
{
    return m_serverSocket;
}

void ServerWorker::run()
{
    while (m_serverSocket->state() == QAbstractSocket::ConnectedState) {
        // 如果有可读取的数据，处理数据
        if (m_serverSocket->waitForReadyRead()) {
            onReadyRead();  // 调用原来处理数据的函数
        }
    }

    // 客户端断开连接时退出循环并清理资源
    emit disconnectedFromClient();
    m_serverSocket->deleteLater();
}

void ServerWorker::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);
    for(;;){
        socketStream.startTransaction();
        socketStream>>jsonData;
        if(socketStream.commitTransaction()){
            QJsonParseError parseError;
            const QJsonDocument jsonDoc=QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error ==QJsonParseError::NoError) {
                if (jsonDoc.isObject()){
                    emit logMessage(QJsonDocument(jsonDoc).toJson(QJsonDocument::Compact));
                    emit jsonReceived(this, jsonDoc.object());
                }
            }
        }else{
            break;
        }
    }
}

void ServerWorker::sendMessage(const QString &text, const QString &type)
{
    if(m_serverSocket->state()!=QAbstractSocket::ConnectedState)
        return;
    if(!text.isEmpty()){
        QDataStream serverStream(m_serverSocket);
        serverStream.setVersion(QDataStream::Qt_5_12);
        QJsonObject message;
        message["type"]=type;
        message["text"]=text;
        serverStream<<QJsonDocument(message).toJson();
    }
}

void ServerWorker::sendJson(const QJsonObject &json)
{
    const QByteArray jsonData=QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit logMessage(QLatin1String("Sending to ")+ userName()+QLatin1String(" - ")+
                    QString::fromUtf8(jsonData));
    QDataStream socketStream(m_serverSocket);
    socketStream.setVersion(QDataStream::Qt_5_7);
    socketStream << jsonData;
}

void ServerWorker::disconnected()
{
    emit disconnectedFromClient();
    m_serverSocket->deleteLater();
}
