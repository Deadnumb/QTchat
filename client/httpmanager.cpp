#include "httpmanager.h"

HttpManager::~HttpManager()
{

}

HttpManager::HttpManager()
{
    connect(this, &HttpManager::sig_http_finish, this, &HttpManager::slot_http_finish);
}

/**
 * @brief 发送HTTP POST请求
 * @param url 目标URL地址
 * @param json 要发送的JSON数据
 * @param req_id 请求ID，用于标识不同的请求
 * @param mod 模块标识，标识请求来自哪个业务模块
 *
 * 功能：异步发送HTTP POST请求，请求完成后通过信号通知调用方
 * 特点：非阻塞、异步回调、自动资源清理
 */
void HttpManager::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{

    QByteArray data = QJsonDocument(json).toJson();


    QNetworkRequest request(url);

    // 设置HTTP请求头
    // Content-Type: application/json - 告诉服务器请求体是JSON格式
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Content-Length: 数据长度 - 标准HTTP头，表示请求体的大小
    // QByteArray::number将数字转换为字符串
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    auto self = shared_from_this();  // 值捕获，会增加引用计数


    QNetworkReply *reply = _manager.post(request, data);
    // 此时HTTP请求已进入队列，会在后台线程执行


    QObject::connect(reply, &QNetworkReply::finished,[self, reply, req_id, mod]() {
        if(reply->error() != QNetworkReply::NoError) {
            // 打印错误信息到控制台（调试用）
            qDebug() << "[HTTP Error]" << reply->errorString()
                     << "ReqID:" << static_cast<int>(req_id)
                     << "Module:" << static_cast<int>(mod);

             // 发射信号通知调用方：请求失败
             // 参数：请求ID, 空响应, 错误码, 模块标识
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERROR_NETWORK, mod);

             // 安全删除reply对象
             // deleteLater()会将删除操作放入Qt事件循环，在适当的时候执行
             // 直接delete reply可能引发崩溃（如果reply还在被使用）
            reply->deleteLater();

             // 返回，不再执行后面的成功逻辑
            return;
         }

         // ---------- 5.2 请求成功，处理响应 ----------
         // 读取服务器返回的所有数据
         // readAll()返回QByteArray，这里隐式转换为QString（UTF-8编码）
        QString res = reply->readAll();

         // 可以添加更多响应处理，例如：
         // 1. 检查HTTP状态码
         // int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
         // 2. 解析响应头
         // QVariant contentType = reply->header(QNetworkRequest::ContentTypeHeader);

         // 打印成功日志（调试用）
        qDebug() << "[HTTP Success] ReqID:" << static_cast<int>(req_id)
                 << "Module:" << static_cast<int>(mod)
                  << "Response size:" << res.size() << "bytes";

         // 发射信号通知调用方：请求成功
         // 注意：通过self指针发射信号，而不是直接使用this
         // 因为Lambda可能在不同线程执行，this可能已无效
         // 参数：请求ID, 响应数据, 成功码, 模块标识
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);

         // ---------- 5.3 清理资源 ----------
         // 安全删除reply对象
        reply->deleteLater();
    }
    );


}

void HttpManager::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if (mod == Modules::REGISTERMOD) {
        emit sig_reg_mod_finish(id, res, err);
    } else if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }
}
