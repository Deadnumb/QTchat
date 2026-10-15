#include "logindialog.h"
#include "ui_logindialog.h"

#include "httpmanager.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QDebug>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);

    connect(ui->register_btn, &QPushButton::clicked,
            this, &LoginDialog::switchRegister);
    connect(HttpManager::GetInstance().get(),
            &HttpManager::sig_login_mod_finish,
            this, &LoginDialog::slot_login_mod_finish);

    initHttpHandlers();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_login_btn_clicked()
{
    const auto user = ui->user_edit->text().trimmed();
    const auto passwd = ui->pass_edit->text();

    if (user.isEmpty() || passwd.isEmpty()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("用户名和密码不能为空"));
        return;
    }

    QJsonObject request;
    request["user"] = user;
    request["passwd"] = passwd;

    HttpManager::GetInstance()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_login"),
        request,
        ReqId::ID_LOGIN_USER,
        Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(
    ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        QMessageBox::warning(this, tr("登录"),
                             tr("无法连接 GateServer"));
        return;
    }

    const auto document = QJsonDocument::fromJson(res.toUtf8());
    if (document.isNull() || !document.isObject()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("服务器返回不是有效 JSON"));
        return;
    }

    const auto handler = _handlers.find(id);
    if (handler == _handlers.end()) {
        QMessageBox::warning(this, tr("登录"),
                             tr("找不到登录回包处理器"));
        return;
    }

    handler.value()(document.object());
}

void LoginDialog::initHttpHandlers()
{
    _handlers.insert(
        ReqId::ID_LOGIN_USER,
        [this](const QJsonObject& response) {
            const int error = response["error"].toInt();
            if (error == GateErrorCode::GATE_SUCCESS) {
                _session.uid = response["uid"].toInt();
                _session.host = response["host"].toString();
                _session.port = static_cast<quint16>(
                    response["port"].toInt());
                _session.token = response["token"].toString();

                if (!_session.isValid()) {
                    QMessageBox::warning(
                        this, tr("登录"),
                        tr("登录交接数据不完整"));
                    return;
                }

                qDebug() << "HTTP login success:"
                         << "uid=" << _session.uid
                         << "host=" << _session.host
                         << "port=" << _session.port;
                emit loginSuccess(
                    _session.uid,
                    _session.host,
                    _session.port,
                    _session.token);
                QMessageBox::information(
                    this,
                    tr("登录"),
                    tr("登录成功，TCP 服务器将在下一步接入"));
                return;
            }

            if (error == GateErrorCode::GATE_LOGIN_FAILED) {
                QMessageBox::warning(this, tr("登录"),
                                     tr("用户名或密码错误"));
            } else if (error == GateErrorCode::GATE_REDIS_ERR) {
                QMessageBox::warning(this, tr("登录"),
                                     tr("登录服务暂时不可用，请稍后重试"));
            } else if (error == GateErrorCode::GATE_CONFIG_ERR) {
                QMessageBox::warning(this, tr("登录"),
                                     tr("ChatServer 地址配置错误"));
            } else if (error == GateErrorCode::GATE_DATABASE_ERR) {
                QMessageBox::warning(this, tr("登录"),
                                     tr("数据库暂时不可用，请稍后重试"));
            } else {
                QMessageBox::warning(this, tr("登录"),
                                     tr("登录失败，错误码 %1").arg(error));
            }
        });
}
