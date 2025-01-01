#include "idatabase.h"
#include<qdebug.h>


bool IDatabase::validateUserLogin(const QString &userName, const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT userpassword FROM user WHERE username = :username");
    query.bindValue(":username", userName);

    if (!query.exec()) {
        qCritical() << "Database query failed:" << query.lastError();
        return false;
    }
    if (query.first()) {
        QString storedPassword = query.value("userpassword").toString();
        if (storedPassword == password) {
            return true;  // 密码匹配，登录成功
        }
    }

    return false;  // 用户名或密码错误
}

bool IDatabase::saveMessage(const QString &username, const QString &message)
{
    QSqlQuery query;
    query.prepare("INSERT INTO messages (username, message) VALUES (:username, :message)");
    query.bindValue(":username", username);
    query.bindValue(":message", message);

    if (!query.exec()) {
        qCritical() << "Failed to save message:" << query.lastError();
        return false;
    }

    return true;
}

QList<QPair<QString, QString>> IDatabase::getChatHistory(const QString &username)
{
    QList<QPair<QString, QString>> chatHistory;

    // 确认接收到的用户名
    qDebug() << "Fetching chat history for user: " << username;

    QSqlQuery query;

    // 只根据用户名查询该用户的所有消息
    query.prepare("SELECT username, message FROM messages WHERE username = :username");

    // 绑定参数
    query.bindValue(":username", username);

    // 执行查询
    if (query.exec()) {
        // 查询成功，遍历每一条记录
        while (query.next()) {
            QString user = query.value(0).toString();  // 获取发送消息的用户名
            QString message = query.value(1).toString();  // 获取消息内容
            chatHistory.append(qMakePair(user, message));  // 将用户名和消息内容存储到聊天记录列表中
        }
    } else {
        // 查询失败，打印错误信息
        qCritical() << "Failed to fetch chat history:" << query.lastError().text();
    }

    return chatHistory;
}




IDatabase::IDatabase(QObject *parent)
    : QObject{parent}
{
    initDatabase();
}

void IDatabase::initDatabase()
{
    database=QSqlDatabase::addDatabase("QSQLITE");
    QString aFile="E:/qt/dazuoye.db";
    database.setDatabaseName(aFile);
    if(!database.open()){
        qDebug()<<"failed to open database";
    } else
        qDebug()<<"open database is ok";
}
