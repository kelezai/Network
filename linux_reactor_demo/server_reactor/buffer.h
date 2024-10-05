#pragma once
#include <string>
#include <iostream>

class Buffer {
public:
    Buffer(uint16_t sep = 1);
    ~Buffer() = default;

    void append(const char * data, size_t size);
    void erase(size_t pos, size_t size);
    void clear();

    size_t size();
    const char * data();

protected:
    std::string buf_;               // 存放数据
    const uint16_t sep_;            // 报文的分隔符: 
                                    //      0-无分隔符(固定长度); 
                                    //      1-四字节报头;
                                    //      2-"\r\n\r\n" 分隔符 (http协议)
};

class InputBuffer : public Buffer {
public:
    InputBuffer(uint16_t sep) : Buffer(sep) {}
    bool get_message(std::string& message);                 // 从 buffer 中提取报文
};

class OutputBuffer : public Buffer {
public:
    OutputBuffer(uint16_t sep) : Buffer(sep) {}
    void append_with_sep(const char * data, size_t size);   // 把数据追加到 buffer中
};