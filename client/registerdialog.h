#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>
#include "global.h"
namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

    static QString getRegex_Email();

signals:
    void switchLogin();

private slots:
    void on_get_code_btn_clicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);


    void on_confirm_btn_clicked();

private:
    void showTip(QString str, bool isOK);
    void initHttpHandlers();
    Ui::RegisterDialog *ui;
    // 从请求id映射到对应实现的map
    // 对应的实现使用的lambda表达式，在Init初始化里被insert到映射表里，达到回调函数的效果
    QMap<ReqId, std::function<void(const QJsonObject)>> _handlers;

};

#endif // REGISTERDIALOG_H
