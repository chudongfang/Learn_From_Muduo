#include <iostream>
#include "Buffer.h"
using namespace std;
using namespace muduo;
using namespace net;
Buffer buf;
std::string sa;
int main() {

    cout  <<"writableBytes 可写字节数: "   <<buf.writableBytes() << endl;
    cout <<"internalCapacity 内存大小: " <<buf.internalCapacity() << endl;
    for(int i=0;i<10000;i++)
    {
        sa+='a'+i%10;
    }
    buf.append(sa.c_str());
    cout <<"writableBytes 可写字节数: " << buf.writableBytes() << endl;
    cout <<"internalCapacity 内存大小: " <<buf.internalCapacity() << endl;

    cout <<"readableBytes 可读字节数: " << buf.readableBytes() <<endl;

    cout <<"output  输出 : " << buf.retrieveAsString(10000)<<endl;

    cout <<"internalCapacity 内存大小 : " <<buf.internalCapacity() << endl;

    return 0;
}