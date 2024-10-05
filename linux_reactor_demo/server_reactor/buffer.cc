#include "buffer.h"
#include <string.h>

Buffer::Buffer(uint16_t sep) : sep_(sep) {
}

void Buffer::append(const char * data, size_t size) {
    buf_.append(data, size);
}

void Buffer::erase(size_t pos, size_t size) {
    buf_.erase(pos, size);
}

void Buffer::clear() {
    buf_.clear();
}

size_t Buffer::size() {
    return buf_.size();
}

const char * Buffer::data() {
    return buf_.c_str();
}

bool InputBuffer::get_message(std::string& message) {
    if (buf_.empty()) {
        return false;
    }

    if (sep_ == 0) {                                            // 不需要分割
        message = buf_;
        return true;
    }
    else if (sep_ == 1) {                                       // 报文长度+报文内容
        int len = 0;
        if (size() < sizeof(len)) {
            return false;
        }
        
        memcpy(&len, data(), sizeof(len));                      // 报文头部
    
        if (size() < sizeof(len) + len) {
            return false;
        }

        message = std::string(data() + sizeof(len), len);       // 报文内容
        erase(0, len + sizeof(len));                            // 删除已经获取的报文
        return true;
    }
    else if (sep_ == 3) {
        size_t pos = buf_.find("\r\n\r\n");
        if (pos == std::string::npos) {
            return false;
        }

        message.append(data(), pos);
        erase(0, pos + 4);
        return true;
    }

    return true;
}

void OutputBuffer::append_with_sep(const char * data, size_t size) {
    if (sep_ == 0) {
        append(data, size);
    }
    else if (sep_ == 1) {
        append((char*)&size, 4);
        append(data, size);
    }
    else if (sep_ == 2) {
        append(data, size);
        append("\r\n\r\n", strlen("\r\n\r\n"));
    }
}