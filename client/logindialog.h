#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMap>
#include <functional>

#include "global.h"

namespace Ui {
class LoginDialog;
}

struct LoginSession {
    int uid = 0;
    QString host;
    quint16 port = 0;
    QString token;

    bool isValid() const {
        return uid > 0 && !host.isEmpty() && port > 0 && !token.isEmpty();
    }
};

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void switchRegister();
    void loginSuccess(int uid, QString host, quint16 port, QString token);

private slots:
    void on_login_btn_clicked();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);

private:
    void initHttpHandlers();

    Ui::LoginDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    LoginSession _session;
};

#endif // LOGINDIALOG_H
