#include "chatclient.h"
#include"QDataStream"
#include<QJsonObject>
#include<qjsondocument.h>
#include<QMessageBox>
#include"idatabase.h"

ChatClient::ChatClient(const QString &username,QObject *parent)
    : QObject{parent}, currentUsername(username)
{
    m_clientSocket=new QTcpSocket(this);

    connect(m_clientSocket,&QTcpSocket::connected,this,&ChatClient::connected);
    connect(m_clientSocket,&QTcpSocket::readyRead,this,&ChatClient::onReadyRead);


}

void ChatClient::setCurrentUsername(const QString &username)
{
    currentUsername = username;  // 设置当前用户名
}

QString ChatClient::getCurrentUsername() const
{
    return currentUsername;  // 获取当前用户名
}


void ChatClient::onReadyRead()
{
    QByteArray jsonData;
    QDataStream socketStream(m_clientSocket);
    socketStream.setVersion(QDataStream::Qt_5_12);
    for (;;) {
        socketStream.startTransaction();
        socketStream >> jsonData;
        if (socketStream.commitTransaction()) {
            qDebug() << "Received data:" << jsonData;
            QJsonParseError parseError;
            const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (jsonDoc.isObject()) {
                    emit jsonReceived(jsonDoc.object());
                    onJsonReceived(jsonDoc.object());
                }
            }
        } else {
            break;
        }
    }
}


void ChatClient::sendMessage(const QString &text, const QString &type)
{
    if (m_clientSocket->state() != QAbstractSocket::ConnectedState)
        return;

    if (!text.isEmpty()) {
        // 保存消息到数据库
        qDebug() << "用户名: " + currentUsername;  // 打印用户名（调试）
        qDebug() << "消息: " + text;
        if(type!="login"){
            IDatabase::getInstance().saveMessage(currentUsername, text);
        }
        // 发送消息到服务器
        QDataStream serverStream(m_clientSocket);
        serverStream.setVersion(QDataStream::Qt_5_12);
        QJsonObject message;
        message["type"] = type;
        message["text"] = text;
        serverStream << QJsonDocument(message).toJson();
    }
}


void ChatClient::connectToServer(const QHostAddress &address, quint16 port)
{
    m_clientSocket->connectToHost(address,port);
}

void ChatClient::disconnectFromHost()
{
    m_clientSocket->disconnectFromHost();
}

void ChatClient::sendJson(const QJsonObject &json)
{
    if (m_clientSocket->state() != QAbstractSocket::ConnectedState)
        return;

    QDataStream serverStream(m_clientSocket);
    serverStream.setVersion(QDataStream::Qt_5_12);
    serverStream << QJsonDocument(json).toJson();
}

void ChatClient::onJsonReceived(const QJsonObject &json)
{
    qDebug() << "onJsonReceived triggered";  // 调试输出
    const QString type = json["type"].toString();
    const QString username=json["username"].toString();
    qDebug() <<type;
    if (type == "kicked") {

        qDebug() << "Received kicked message";  // 调试输出
        //QMessageBox::information(nullptr, "被踢出", "你已被管理员踢出，返回登录页面");

        // 发射信号通知返回登录页面
        emit returnToLoginPage();
    }
    else if (type == "private_message") {
        const QString sender = json["sender"].toString();
        const QString text = json["text"].toString();

        // 发送私聊消息信号
        emit privateMessageReceived(sender, text);
    }
}

void ChatClient::sendPrivateMessage(const QString &receiver, const QString &text)
{
    if (m_clientSocket->state() != QAbstractSocket::ConnectedState)
        return;

    if (!text.isEmpty()) {
        qDebug() << "用户名: " + currentUsername;  // 打印用户名（调试）
        qDebug() << "消息: " + text;
        IDatabase::getInstance().saveMessage(currentUsername, text);

        QDataStream serverStream(m_clientSocket);
        serverStream.setVersion(QDataStream::Qt_5_12);
        QJsonObject message;
        message["type"] = "private_message";  // 私聊消息类型
        message["receiver"] = receiver;  // 指定接收者
        message["text"] = text;  // 私聊内容
        serverStream << QJsonDocument(message).toJson();
    }
}



