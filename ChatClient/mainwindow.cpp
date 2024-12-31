#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QHostAddress>
#include<QJsonValue>
#include<QJsonObject>
#include"idatabase.h"
#include<QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    m_chatClient =new ChatClient(currentUser,this);

    connect(m_chatClient,&ChatClient::connected,this,&MainWindow::connectedToServer);
    connect(m_chatClient,&ChatClient::jsonReceived,this,&MainWindow::jsonReceived);
    connect(m_chatClient, &ChatClient::returnToLoginPage, this, &MainWindow::onReturnToLoginPage);
    connect(m_chatClient, &ChatClient::privateMessageReceived, this, &MainWindow::privateMessageReceived);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginButton_clicked()
{
    QString userName = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();  // 假设你有一个密码输入框

    // 使用数据库验证用户登录
    bool loginValid = IDatabase::getInstance().validateUserLogin(userName, password);
    if(loginValid){
        m_chatClient->setCurrentUsername(userName);  // 设置当前用户名
        m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()),1967);
    }
    else {
        // 登录失败，弹出提示
        QMessageBox::warning(this, "登陆失败", "密码或用户名错误");
    }
    // 设置当前用户名

    // 如果是 admin 用户，启用踢人按钮
    if (userName == "admin") {
        ui->kickButton->setEnabled(true);  // 启用踢人按钮
    } else {
        ui->kickButton->setEnabled(false);  // 禁用踢人按钮
    }
}


void MainWindow::on_sayButton_clicked()
{
    if(!ui->sayLineEdit->text().isEmpty())
        m_chatClient->sendMessage(ui->sayLineEdit->text());
}


void MainWindow::on_logoutButton_clicked()
{
    m_chatClient->disconnectFromHost();
    ui->stackedWidget->setCurrentWidget(ui->loginPage);
    for(auto aItem:ui->userListWidget->findItems(ui->usernameEdit->text(),Qt::MatchExactly)){
        ui->userListWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::connectedToServer()
{
    ui->stackedWidget->setCurrentWidget(ui->chatPage);
    m_chatClient->sendMessage(ui->usernameEdit->text(),"login");

}

void MainWindow::messageReceived(const QString &sender, const QString &text)
{
    ui->roomTextEdit->append(QString("%1 : %2").arg(sender).arg(text));
}


void MainWindow::jsonReceived(const QJsonObject &docObj)
{
    const QJsonValue typeVal = docObj.value("type");
    if (typeVal.isNull()|| !typeVal.isString())
        return;
    if (typeVal.toString().compare("message", Qt:: CaseInsensitive)==0){
        const QJsonValue textVal=docObj.value("text");
        const QJsonValue senderVal=docObj.value("sender");
        if (textVal.isNull()|| !textVal.isString())
            return;

        if (senderVal.isNull()|| !senderVal.isString())
            return;

        messageReceived(senderVal.toString(),textVal.toString());
    }
    else if (typeVal.toString().compare("newuser",Qt::CaseInsensitive) ==0){
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull()||!usernameVal.isString())
            return;
        userJoined(usernameVal.toString());
    }
    else if (typeVal.toString().compare("userdisconnected",Qt::CaseInsensitive) ==0){
        const QJsonValue usernameVal = docObj.value("username");
        if (usernameVal.isNull()||!usernameVal.isString())
            return;
        userLeft(usernameVal.toString());
    }
    else if (typeVal.toString().compare("userlist",
                                        Qt::CaseInsensitive)==0){
        const QJsonValue userlistVal = docObj.value("userlist");
        if (userlistVal.isNull() ||!userlistVal.isArray())
            return;
        qDebug()<< userlistVal.toVariant().toStringList();
        userListReceived(userlistVal.toVariant().toStringList());
    }
}

void MainWindow::userJoined(const QString &user)
{
    ui->userListWidget->addItem(user);
}

void MainWindow::userLeft(const QString &user)
{
    for(auto aItem:ui->userListWidget->findItems(user,Qt::MatchExactly)){
        ui->userListWidget->removeItemWidget(aItem);
        delete aItem;
    }
}

void MainWindow::userListReceived(const QStringList &list)
{
    ui->userListWidget->clear();
    ui->userListWidget->addItems(list);

}


void MainWindow::on_kickButton_clicked()
{
    // 获取当前选中的用户名
    QListWidgetItem *selectedItem = ui->userListWidget->currentItem();
    if (selectedItem) {
        QString username = selectedItem->text();

        // 构造踢人请求
        QJsonObject kickRequest;
        kickRequest["type"] = "kick";  // 定义消息类型
        kickRequest["username"] = username;  // 传递要踢出的用户名

        // 发送踢人请求到服务器
        m_chatClient->sendJson(kickRequest);

    } else {
        // 如果没有选中用户，可以提示错误
        QMessageBox::warning(this, "错误", "请先选择一个用户进行踢出！");
    }
}

void MainWindow::onReturnToLoginPage()
{
    m_chatClient->disconnectFromHost();

    // 清理界面：切换到登录页面
    ui->stackedWidget->setCurrentWidget(ui->loginPage);

    // 清理当前登录用户信息
    ui->usernameEdit->clear();
    ui->passwordEdit->clear();

    // 清除当前的用户列表
    ui->userListWidget->clear();
}



void MainWindow::privateMessageReceived(const QString &sender, const QString &text)
{
    ui->roomTextEdit->append(QString("%1 : %2").arg("私聊for "+sender).arg(text));
}



void MainWindow::on_privateMessageButton_clicked()
{

    QListWidgetItem *selectedItem = ui->userListWidget->currentItem();
    if (selectedItem) {
        QString receiver = selectedItem->text();  // 选择的接收者
        QString message = ui->sayLineEdit->text();  // 输入的消息内容
        if (!message.isEmpty()) {
            // 发送私聊消息
            ui->roomTextEdit->append(QString("%1 : %2").arg("私聊to "+receiver).arg(message));
            m_chatClient->sendPrivateMessage(receiver, message);
        }
    } else {
        QMessageBox::warning(this, "错误", "请先选择一个用户进行私聊！");
    }
}


void MainWindow::on_findChatHistoryButton_clicked()
{
    // 获取当前选中的用户
    QListWidgetItem *selectedItem = ui->userListWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, "错误", "请先选择一个用户！");
        return;
    }

    QString username = selectedItem->text();  // 获取用户名

    // 查询该用户的聊天记录
    if (username.endsWith('*')) {
        username.chop(1);  // 删除最后一个字符 '*'
    }
    QList<QPair<QString, QString>> chatHistory = IDatabase::getInstance().getChatHistory(username);

    // 显示聊天记录
    ui->roomTextEdit->clear();  // 清空当前显示的聊天记录
    for (const auto &entry : chatHistory) {
        QString displayMessage = QString("%1 : %2").arg(entry.first).arg(entry.second);
        ui->roomTextEdit->append(displayMessage);  // 显示每条聊天记录
    }
}

