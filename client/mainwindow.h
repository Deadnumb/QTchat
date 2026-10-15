#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "chatdialog.h"
/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      主窗口
 *
 * @author     Deadnumb
 * @date       2026/03/26
 * @history
 *****************************************************************************/
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void SlotSwitchRegister();
    void SlotSwitchLogin();
    void SlotSwitchChat();

private:
    Ui::MainWindow *ui;
    LoginDialog *_login_dlg;// 感觉这个下划线开头不是很规范
    RegisterDialog *_reg_dlg;
    ChatDialog *_chat_dlg;
};
#endif // MAINWINDOW_H










