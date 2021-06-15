#include "logger.hpp"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <Windows.h>

namespace
{
    static constexpr const char* _level_head[5] = {
        "[D] ",
        "[I] ",
        "[W] ",
        "[E] ",
        "[F] ",
    };
};

void logger::write(const char* str) noexcept
{
    return write(str, std::strlen(str));
}
void logger::write(const char* str, size_t len) noexcept
{
    if (len == 0)
    {
        return;
    }
    assert(str != nullptr || len == 0);
    OutputDebugStringA(str);
    if (_file != static_cast<void*>(INVALID_HANDLE_VALUE))
    {
        DWORD cnt_ = 0;
        WriteFile(static_cast<HANDLE>(_file), str, 0xFFFFFFFF & len, &cnt_, nullptr);
        FlushFileBuffers(static_cast<HANDLE>(_file));
    }
}
void logger::writef(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    writefv(fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::writefv(const char* fmt, void* arg) noexcept
{
    const int size_ = std::vsnprintf(nullptr, 0, fmt, static_cast<va_list>(arg));
    if (size_ > 0 && size_ < 64)
    {
        char buffer_[64] = {};
        std::vsnprintf(buffer_, 64, fmt, static_cast<va_list>(arg));
        write(buffer_, size_);
    }
    else if (size_ >= 64)
    {
        const size_t heap_size_ = size_ + 1;
        char* heap_ = static_cast<char*>(::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, heap_size_));
        if (heap_ != NULL)
        {
            std::vsnprintf(heap_, heap_size_, fmt, static_cast<va_list>(arg));
            write(heap_, size_);
            ::HeapFree(::GetProcessHeap(), 0, heap_);
        }
        else
        {
            log(level::error, "out of memory");
        }
    }
    // else ???
}
void logger::log(level lv, const char* str) noexcept
{
    return log(lv, str, std::strlen(str));
}
void logger::log(level lv, const char* str, size_t len) noexcept
{
    if (len == 0)
    {
        return;
    }
    assert(static_cast<int>(lv) >= 0 && static_cast<int>(lv) <= 5);
    assert(str != nullptr || len == 0);
    OutputDebugStringA(_level_head[static_cast<int>(lv)]);
    OutputDebugStringA(str);
    if (_file != static_cast<void*>(INVALID_HANDLE_VALUE))
    {
        DWORD cnt_ = 0;
        WriteFile(static_cast<HANDLE>(_file), _level_head[static_cast<int>(lv)], 4, &cnt_, nullptr);
        WriteFile(static_cast<HANDLE>(_file), str, 0xFFFFFFFF & len, &cnt_, nullptr);
        FlushFileBuffers(static_cast<HANDLE>(_file));
    }
}
void logger::logf(level lv, const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    logfv(lv, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::logfv(level lv, const char* fmt, void* arg) noexcept
{
    const int size_ = std::vsnprintf(nullptr, 0, fmt, static_cast<va_list>(arg));
    if (size_ > 0 && size_ < 64)
    {
        char buffer_[64] = {};
        std::vsnprintf(buffer_, 64, fmt, static_cast<va_list>(arg));
        log(lv, buffer_, size_);
    }
    else if (size_ >= 64)
    {
        const size_t heap_size_ = size_ + 1;
        char* heap_ = static_cast<char*>(::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, heap_size_));
        if (heap_ != NULL)
        {
            std::vsnprintf(heap_, heap_size_, fmt, static_cast<va_list>(arg));
            log(lv, heap_, size_);
            ::HeapFree(::GetProcessHeap(), 0, heap_);
        }
        else
        {
            log(level::error, "out of memory");
        }
    }
    // else ???
}

void logger::debug(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    get().logfv(level::debug, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::info(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    get().logfv(level::info, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::warn(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    get().logfv(level::warn, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::error(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    get().logfv(level::error, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}
void logger::fatal(const char* fmt, ...) noexcept
{
    va_list arg_{};
    va_start(arg_, fmt);
    get().logfv(level::fatal, fmt, static_cast<void*>(arg_));
    va_end(arg_);
}

logger::logger() : _file(static_cast<void*>(INVALID_HANDLE_VALUE))
{
    HANDLE file_h_ = CreateFileW(
        L"build.log",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (file_h_ != INVALID_HANDLE_VALUE)
    {
        _file = static_cast<void*>(file_h_);
    }
    else
    {
        log(level::error, "create file \"build.log\" failed");
    }
}
logger::~logger()
{
    if (_file != static_cast<void*>(INVALID_HANDLE_VALUE))
    {
        CloseHandle(static_cast<HANDLE>(_file));
        _file = static_cast<void*>(INVALID_HANDLE_VALUE);
    }
}
logger& logger::get()
{
    static logger instance_;
    return instance_;
}
