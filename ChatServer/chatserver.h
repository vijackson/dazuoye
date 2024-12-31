#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include "serverworker.h"
#include <QThreadPool>


class ChatServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ChatServer(QObject *parent=nullptr);


protected:
    void incomingConnection(qintptr socketDescriptor) override;
    QVector<ServerWorker *>m_clients;
    void broadcast(const QJsonObject &message,ServerWorker *exclude);

signals:
    void logMessage(const QString &msg);


public slots:
    void stopServer();
    void jsonReceived(ServerWorker *sender,const QJsonObject &docObj);
    void userDisconnected(ServerWorker *sender);
    void kickUser(const QString &username);  // 新增踢人功能
    void sendPrivateMessage(const QString &receiver, const QString &sender, const QString &text);

};

#endif // CHATSERVER_H
