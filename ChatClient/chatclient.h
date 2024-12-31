#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>

class ChatClient : public QObject
{
    Q_OBJECT
public:
    explicit ChatClient(const QString &username,QObject *parent = nullptr);
    void setCurrentUsername(const QString &username);
    QString getCurrentUsername() const;

signals:
    void connected();
    void messageReceived(const QString &text);
    void jsonReceived(const QJsonObject &docObj);
    void returnToLoginPage();  // 定义信号，返回登录页面
    void privateMessageReceived(const QString &sender, const QString &text);

private:
    QString currentUsername;  // 存储当前用户名
    QTcpSocket * m_clientSocket;

public slots:
    void onReadyRead();
    void sendMessage(const QString &text,const QString &type ="message");
    void connectToServer(const QHostAddress &address,quint16 port);
    void disconnectFromHost();
    void sendJson(const QJsonObject &json); // 添加 sendJson 方法声明
    void onJsonReceived(const QJsonObject &json);  // 处理服务器返回的 JSON 数据
    void sendPrivateMessage(const QString &receiver, const QString &text);//发送私聊信息


};

#endif // CHATCLIENT_H
