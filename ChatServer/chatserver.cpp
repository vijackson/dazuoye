#include "chatserver.h"
#include "serverworker.h"
#include<QJsonValue>
#include<QJsonObject>
#include<QJsonArray>
#include<qdebug.h>

ChatServer::ChatServer(QObject *parent):
    QTcpServer(parent)
{

}

void ChatServer::incomingConnection(qintptr socketDescriptor)
{
    ServerWorker *worker=new ServerWorker(this);
    if(!worker->setSocketDestriptor(socketDescriptor)){
        worker->deleteLater();
        return;
    }
    connect(worker,&ServerWorker::logMessage,this,&ChatServer::logMessage);
    connect(worker,&ServerWorker::jsonReceived,this,&ChatServer::jsonReceived);
    connect(worker,&ServerWorker::disconnectedFromClient,this,
            std::bind(&ChatServer::userDisconnected,this,worker));


    m_clients.append(worker);
    emit logMessage("新的用户连接上了");
    // // 将ServerWorker对象提交给线程池
    // QThreadPool::globalInstance()->start(worker);
}

void ChatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
    for(ServerWorker *worker:m_clients){
        worker->sendJson(message);
    }
}

void ChatServer::stopServer()
{
    close();
}

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    qDebug() << "Received type:" << typeVal.toString();
    if (typeVal.isNull() || !typeVal.isString())
        return;

    if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0) {
        // 处理普通消息
        const QJsonValue textVal = docObj.value("text");
        if (textVal.isNull() || !textVal.isString())
            return;
        const QString text = textVal.toString().trimmed();
        if (text.isEmpty())
            return;

        QJsonObject message;
        message["type"] = "message";
        message["text"] = text;
        message["sender"] = sender->userName();
        broadcast(message, sender);
    }
    else if (typeVal.toString().compare("private_message", Qt::CaseInsensitive) == 0) {
        // 处理私聊消息
        const QJsonValue receiverVal = docObj.value("receiver");
        const QJsonValue textVal = docObj.value("text");
        if (receiverVal.isNull() || !receiverVal.isString() || textVal.isNull() || !textVal.isString())
            return;

        QString receiver = receiverVal.toString();
        QString text = textVal.toString();

        // 将私聊消息发送给接收者
        sendPrivateMessage(receiver, sender->userName(), text);
    }
    else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0) {
        // 处理用户登录
        const QJsonValue usernameVal = docObj.value("text");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;

        sender->setUserName(usernameVal.toString());

        // 广播新用户加入
        QJsonObject connectedMessage;
        connectedMessage["type"] = "newuser";
        connectedMessage["username"] = usernameVal.toString();
        broadcast(connectedMessage, sender);

        // 更新用户列表
        QJsonObject userListMessage;
        userListMessage["type"] = "userlist";
        QJsonArray userlist;
        for (ServerWorker *worker : m_clients) {
            if (worker == sender)
                userlist.append(worker->userName() + "*");  // 标记自己
            else
                userlist.append(worker->userName());
        }
        userListMessage["userlist"] = userlist;
        sender->sendJson(userListMessage);
    }else if(typeVal.toString().compare("kick", Qt::CaseInsensitive) == 0){
        // 处理踢人请求
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull() || !usernameVal.isString())
            return;

        QString username = usernameVal.toString();

        // 调用踢人方法
        kickUser(username);
    }
}


void ChatServer::userDisconnected(ServerWorker *sender)
{
    m_clients.removeAll(sender);
    const QString userName =sender->userName();
    if (!userName.isEmpty()){
        QJsonObject disconnectedMessage;
        disconnectedMessage["type"]="userdisconnected";
        disconnectedMessage["username"]=userName;
        broadcast(disconnectedMessage,nullptr);
        emit logMessage(userName +" disconnected");
    }
    sender->deleteLater();
}

void ChatServer::kickUser(const QString &username)
{
    for (ServerWorker *worker : m_clients) {
        if (worker->userName() == username) {
            // 向被踢的用户发送踢出消息
            QJsonObject kickMessage;
            kickMessage["type"] = "kicked";  // 定义消息类型为“kicked”
            kickMessage["username"] = username;
            qDebug() << "Sending kicked message to client:" << username;
            worker->sendJson(kickMessage);   // 发送消息给客户端

            // 发送消息并断开连接
            // worker->sendMessage("You have been kicked from the server.", "message");

            // // 断开与该客户端的连接
            // worker->getSocket()->disconnectFromHost();

            // // 广播该用户已被踢出
            // // QJsonObject userKickedMessage;
            // // userKickedMessage["type"] = "userdisconnected";
            // // userKickedMessage["username"] = username;
            // // broadcast(userKickedMessage, nullptr);
            // // 从客户端列表中移除该用户
            // m_clients.removeAll(worker);
            // worker->deleteLater();
            break;
        }
    }
}

void ChatServer::sendPrivateMessage(const QString &receiver, const QString &sender, const QString &text)
{
    for (ServerWorker *worker : m_clients) {
        if (worker->userName() == receiver) {
            QJsonObject privateMessage;
            privateMessage["type"] = "private_message";
            privateMessage["sender"] = sender;
            privateMessage["receiver"] = receiver;
            privateMessage["text"] = text;
            worker->sendJson(privateMessage);  // 只发送给接收者
            break;
        }
    }
}



