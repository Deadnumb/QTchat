#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"

template <typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator = (const Singleton<T>& st) = delete;
    static std::shared_ptr<T> _instance;// 智能指针，一个static变量一定要声明
public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;// 开始是false，在调用之后置为false，保证只会初始化一次
        std::call_once(s_flag,[&](){
            // 这里使用new调用对象，而不是用make_shared
            // 因为构造函数变成私有，make_shared会调用共有的构造函数
            _instance = std::shared_ptr<T>(new T);
        });

        return _instance;
    }

    void PrintAddress(){
        std::cout << _instance.get() << std::endl;
    }

    ~Singleton(){
        std::cout << "this is a singleton destruct" << std::endl;
    }
};


// 初始化static变量
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
