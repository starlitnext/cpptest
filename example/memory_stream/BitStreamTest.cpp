/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-30 15:40:59
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-09-30 16:58:09
 */

#include <iostream>
#include "MemoryBitStream.hpp"

int main(int argc, char const *argv[])
{
    OutputMemoryBitStream writer{};
    writer.Write(11, 5);
    writer.Write(52, 6);
    std::string s("hello");
    writer.Write(s);
    writer >> 1000 >> 1024;
    std::cout << writer.GetBitLength() << std::endl;

    char buffer[1024];
    memcpy(buffer, writer.GetBufferPtr(), writer.GetByteLength());
    InputMemoryBitStream reader{buffer, writer.GetBitLength()};
    // 注意要设为0初始化，否则结果未知
    int a = 0, b = 0;
    std::string ss;
    reader.Read(a, 5);
    reader.Read(b, 6);
    reader.Read(ss);
    int c = 0, d = 0;
    reader >> c >> d;
    std::cout << "a: " << a << std::endl;
    std::cout << "b: " << b << std::endl;
    std::cout << "ss:" << ss << std::endl;
    std::cout << "c:" << c << std::endl;
    std::cout << "d:" << d << std::endl;
    
    return 0;
}


