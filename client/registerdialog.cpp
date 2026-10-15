#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "httpmanager.h"



RegisterDialog::RegisterDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->pass_confirm_edit->setEchoMode(QLineEdit::Password);
    ui->error_tip->setProperty("state","normal");
    repolish(ui->error_tip);

    // HTTP告诉注册模块，http收到回包
    connect(HttpManager::GetInstance().get(), &HttpManager::sig_reg_mod_finish,
            this, &RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers();
    connect(ui->cancel_btn, &QPushButton::clicked,
            this, &RegisterDialog::switchLogin);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_btn_clicked()
{
    // secondtonone@163.com
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    bool match = regex.match(email).hasMatch();
    if(match){
        // 发送http验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpManager::GetInstance()->PostHttpReq(
            QUrl(gate_url_prefix + "/get_verifycode"),
            json_obj, ReqId::ID_GET_VERIFY_CODE,
            Modules::REGISTERMOD
        );


    }else{
        // 抛出错误
        // 这个tr是干什么的不知道
        showTip(tr("邮箱地址不正确"),false);
    }

}

// 将QString序列化成字节流数组
// 字节流是数据链路层的数据，2进制表示
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误！"),false);
        return;
    }

    // 解析JSON字符串，res 转化为QbyteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    // json为空
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"),false);
        return;
    }
    // json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"),false);
        return;
    }

    // 转成json对象
    jsonDoc.object();
    _handlers[id](jsonDoc.object());
    return;

}


// 这个isOK可能换成枚举之类的功能会更全面
void RegisterDialog::showTip(QString str, bool isOK)
{
    if(isOK == true){
        ui->error_tip->setProperty("state","normal");
    }
    else{
        ui->error_tip->setProperty("state","error");
    }
    ui->error_tip->setText(str);

    repolish(ui->error_tip);
}

// 注册回包对应的回调函数
void RegisterDialog::initHttpHandlers()
{
    // 注册获得验证码回包的逻辑
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error == GateErrorCode::GATE_EMAIL_EXIST){
            showTip(tr("该邮箱已注册，请直接登录"), false);
            return;
        }
        if(error != GateErrorCode::GATE_SUCCESS){
            // GateServer 已在边界翻译错误码，这里非零就是上游服务出问题，不必细分
            showTip(tr("获取验证码失败，请稍后重试"), false);
            return;
        }

        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已经发送到邮箱，注意查收"), true);
        qDebug() << "Email is " << email;
    });

    // 注册用户回包的逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](const QJsonObject& jsonObj) {
        int error = jsonObj["error"].toInt();
        if(error == GateErrorCode::GATE_SUCCESS) {
            showTip(tr("注册成功"), true);
            qDebug() << "Register success, uid is " << jsonObj["uid"].toInt();
            return;
        }

        // 除成功之外，所有错误码对应的提示
        if(error == GateErrorCode::GATE_USER_EXIST){
            showTip(tr("用户名或邮箱已存在"), false);
        } else if(error == GateErrorCode::GATE_PASSWD_ERR){
            showTip(tr("密码与确认密码不一致"), false);
        } else if(error == GateErrorCode::GATE_VARIFY_ERR){
            showTip(tr("验证码错误"), false);
        } else if(error == GateErrorCode::GATE_VARIFY_EXPIRED){
            showTip(tr("验证码已过期，请重新获取"), false);
        } else if(error == GateErrorCode::GATE_DATABASE_ERR){
            showTip(tr("注册服务暂时不可用，请稍后重试"), false);
        } else {
            showTip(tr("注册失败，错误码 %1").arg(error), false);
        }
    });
}

// 确认注册按钮
void RegisterDialog::on_confirm_btn_clicked()
{
    if(ui->user_edit->text() == ""){
        showTip(tr("用户名不能为空"), false);
        return;
    }
    if(ui->email_edit->text() == ""){
        showTip(tr("邮箱不能为空"), false);
        return;
    }
    if(ui->pass_edit->text() == ""){
        showTip(tr("密码不能为空"), false);
        return;
    }
    if(ui->pass_confirm_edit->text() == ""){
        showTip(tr("确认密码不能为空"), false);
        return;
    }
    if(ui->pass_confirm_edit->text() != ui->pass_edit->text()){
        showTip(tr("密码和确认密码不匹配"), false);
        return;
    }
    if(ui->varify_edit->text() == ""){
        showTip(tr("验证码不能为空"), false);
        return;
    }
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = ui->pass_edit->text();
    json_obj["confirm"] = ui->pass_confirm_edit->text();
    json_obj["varifycode"] = ui->varify_edit->text();
    HttpManager::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                            json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}


