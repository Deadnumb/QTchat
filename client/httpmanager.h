#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include "global.h"
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>


class HttpManager:public QObject, public Singleton<HttpManager>, public std::enable_shared_from_this<HttpManager>
{
    Q_OBJECT
public:
    ~HttpManager();// 这个析构会被Singleton里的share_ptr智能指针析构调用，所以要设成共有
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);
private:
    friend class Singleton<HttpManager>;// 为了让Singleton单例可以访问构造函数，这里声明为友元
    HttpManager();
    QNetworkAccessManager _manager;

    
private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err,Modules mod);


signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err,Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMANAGER_H






