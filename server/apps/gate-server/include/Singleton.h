#ifndef SINGLETON_H
#define SINGLETON_H

#include <iostream>
#include <memory>
#include <mutex>
/*
单例模式：
保护：
    设置正常构造不变(C++11语法)
    删除危险构造函数
    静态的实例变量

共有
    初始化实例，线程安全地
    析构

最后记得类外初始化实例为nullptr
*/

template<typename T>
class Singleton
{
protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator = (const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator = (const Singleton&&) = delete;
    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> get_instance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }
    ~Singleton(){
        std::cout << "This singleton is destructed." << std::endl;
    }

};

template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif /* SINGLETON_H */
