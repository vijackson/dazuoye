#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include<QTcpSocket>
#include <QRunnable>
#include <QJsonObject>

class ServerWorker : public QObject,public QRunnable  // 继承 QRunnable
{
    Q_OBJECT
public:
    explicit ServerWorker(QObject *parent = nullptr);
    virtual bool setSocketDestriptor(qintptr socketDescriptor);
    QString userName();
    void setUserName(QString user);
    QTcpSocket* getSocket() ;
    // 继承QRunnable的run方法，任务的执行入口
    void run() override;

signals:
    void logMessage(const QString &msg);
    void jsonReceived(ServerWorker *sender,const QJsonObject &docObj);
    void disconnectedFromClient();

private:
    QTcpSocket * m_serverSocket;
    QString m_userName;



public slots:
    void onReadyRead();
    void sendMessage(const QString &text,const QString &type ="mesage");
    void sendJson(const QJsonObject &json);
    void disconnected();
};

#endif // SERVERWORKER_H
