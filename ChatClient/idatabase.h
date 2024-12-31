#ifndef IDATABASE_H
#define IDATABASE_H

#include <QObject>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QItemSelectionModel>

class IDatabase : public QObject
{
    Q_OBJECT
public:
    static IDatabase &getInstance()
    {
        static IDatabase instance;
        return instance;
    }

    bool validateUserLogin(const QString &userName, const QString &password);
    //保存消息到数据库
    bool saveMessage(const QString &username, const QString &message);
    QList<QPair<QString, QString>> getChatHistory(const QString &username);

private:
    explicit IDatabase(QObject *parent = nullptr);
    IDatabase(IDatabase const &) = delete;
    void operator=(IDatabase const &) = delete;

    // 初始化数据库连接
    void initDatabase();

    QSqlDatabase database;

    // 模型和选择模型
    QSqlTableModel *patientTabModel;
    QItemSelectionModel *thePatientSelction;

signals:
    void loginSuccessful();
    void loginFailed();
};

#endif // IDATABASE_H
