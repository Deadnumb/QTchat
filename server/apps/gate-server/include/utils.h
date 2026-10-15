#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <functional>

unsigned char FromHex(unsigned char x);

unsigned char ToHex(unsigned char x);

std::string UrlEncode(const std::string& str);

std::string UrlDecode(const std::string& str);

class Defer {
public:
    Defer(std::function<void()> func);
    ~Defer();

private:
    std::function<void()> func_;

};

#endif /* UTILS_H */
