#include "global.h"


std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

QString gate_url_prefix = "";


// 对字符串进行异或操作，用来加密
std::function<QString(QString)> xorString = [](QString input)->QString {
    QString result = input;
    int length = input.length();
    length = length % 255;
    for(int i = 0; i < length; i++) {
        // 对每个字符进行异或操作
        // 这里假设字符都ASCII
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(length)));
        return result;
    }
};
