#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    Timestamp(int64_t sec_since_epoch);

    static Timestamp now();

    time_t to_int() const;                  // 整数表示的时间
    std::string to_string() const;          // 字符串表示的时间: yyyy-mm-dd hh24:mi:ss
    
private:
    time_t sec_since_epoch_;                // 时间, 从 1970 至今
};