#include "time_stamp.h"

Timestamp::Timestamp() {
    sec_since_epoch_ = time(0);                 // 系统当前时间
}

Timestamp::Timestamp(int64_t sec_since_epoch) {
    sec_since_epoch_ = sec_since_epoch;
}

Timestamp Timestamp::now() {
    return Timestamp();
}

// 整数表示的时间
time_t Timestamp::to_int() const {
    return sec_since_epoch_;
}

// 字符串表示的时间: yyyy-mm-dd hh24:mi:ss          
std::string Timestamp::to_string() const {
    tm * tm_time = localtime(&sec_since_epoch_);

    char buf[32] = { 0 };
    snprintf(buf, 32, "%4d-%02d-%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec);

    return buf;
}