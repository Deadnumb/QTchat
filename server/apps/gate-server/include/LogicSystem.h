#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H

#include "Singleton.h"
#include <functional>
#include <map>
#include "global.h"
#include "RedisManager.h"
#include "MySQLManager.h"

class HttpConnection;

typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem:public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem();
    bool HandleGet(std::string path, std::shared_ptr<HttpConnection> );
    bool HandlePost(std::string path, std::shared_ptr<HttpConnection> con);
    void RegGet(std::string url, HttpHandler handler);
    void RegPost(std::string url, HttpHandler handler);
private:
    LogicSystem();
    std::map<std::string, HttpHandler> _get_handlers;
    std::map<std::string, HttpHandler> _post_handlers;


};


#endif /* LOGICSYSTEM_H */
