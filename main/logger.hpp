#pragma once

class logger
{
private:
    void* _file = nullptr;
public:
    enum class level
    {
        debug = 0,
        info = 1,
        warn = 2,
        error = 3,
        fatal = 4,
    };
public:
    void write(const char* str) noexcept;
    void write(const char* str, size_t len) noexcept;
    void writef(const char* fmt, ...) noexcept;
    void writefv(const char* fmt, void* arg) noexcept;
    void log(level lv, const char* str) noexcept;
    void log(level lv, const char* str, size_t len) noexcept;
    void logf(level lv, const char* fmt, ...) noexcept;
    void logfv(level lv, const char* fmt, void* arg) noexcept;
public:
    static void debug(const char* fmt, ...) noexcept;
    static void info(const char* fmt, ...) noexcept;
    static void warn(const char* fmt, ...) noexcept;
    static void error(const char* fmt, ...) noexcept;
    static void fatal(const char* fmt, ...) noexcept;
public:
    logger();
    ~logger();
public:
    static logger& get();
};
