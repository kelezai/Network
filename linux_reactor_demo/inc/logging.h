#pragma once

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <unistd.h>
#include <syscall.h>
#include <mutex>

class FormatStreamStream : public std::stringstream {
public:
    FormatStreamStream() = default;

    FormatStreamStream & operator << (const char * value) {
        (*static_cast<std::stringstream*>(this)) << value;
        return *this;
    }

    template <typename T>
    FormatStreamStream & operator << (T value) {
        // (*static_cast<std::stringstream*>(this)) << value;
        static_cast<std::stringstream&>(*this) << value;
        return *this;
    }
};

inline std::string LineNoToStr(int lineno) {
    std::stringstream ss;
    ss << std::setw(2) << std::setfill(' ') << lineno;
    return ss.str(); 
}

inline std::string FileNameToStr(const char * filename) {
    std::string fn(filename);
    return fn.substr(fn.find_last_of("//") + 1);
}

class LogCall {
public:
    void operator&(const FormatStreamStream & ss) {
        log_mtx_.lock();
        std::cout << ss.str() << std::endl;
        log_mtx_.unlock();
    }
private:
    inline static std::mutex log_mtx_;
};

#define LOG_PRINT(tag)  LogCall() & FormatStreamStream()                    \
    << "[" << tag << "]"                                                    \
    << "[pid: " << getpid() << "|" << syscall(SYS_gettid) << "]"            \
    << "[ln: " << LineNoToStr(__LINE__) << "]"                              \
    << "[file: " << FileNameToStr(__FILE__) << "][fn: " << __func__ << "] "

