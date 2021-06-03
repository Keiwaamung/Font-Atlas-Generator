#include "common.hpp"
#include <cassert>
#define  WIN32_LEAN_AND_MEAN
#define  NOMINMAX
#include <Windows.h>
#include <wrl.h>

namespace fontatlas
{
    std::wstring toWide(const std::string_view str)
    {
        const int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.length(), NULL, 0);
        if (size <= 0)
        {
            return L"";
        }
        std::wstring buffer(size, 0);
        const int result = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.length(), buffer.data(), size);
        if (size <= 0)
        {
            return L"";
        }
        return std::move(buffer);
    }
    std::string toUTF8(const std::wstring_view str)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.length(), NULL, 0, NULL, NULL);
        if (size <= 0)
        {
            return "";
        }
        std::string buffer(size, 0);
        const int result = WideCharToMultiByte(CP_UTF8, 0, str.data(), str.length(), buffer.data(), size, NULL, NULL);
        if (size <= 0)
        {
            return "";
        }
        return std::move(buffer);
    }
    
    Buffer readFile(const std::string_view  path)
    {
        std::wstring wpath = std::move(toWide(path));
        return std::move(readFile(wpath));
    }
    Buffer readFile(const std::wstring_view path)
    {
        Buffer buffer;
        
        Microsoft::WRL::Wrappers::FileHandle file;
        file.Attach(CreateFileW(
            path.data(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        ));
        
        if (file.IsValid())
        {
            LARGE_INTEGER file_size = {};
            GetFileSizeEx(file.Get(), &file_size);
            
            LARGE_INTEGER read_pos = {};
            SetFilePointerEx(file.Get(), read_pos, NULL, FILE_BEGIN);
            
            assert(file_size.HighPart == 0);
            buffer.resize(file_size.LowPart);
            DWORD read_size = 0;
            ReadFile(file.Get(), buffer.data(), file_size.LowPart, &read_size, NULL);
            assert(file_size.LowPart == read_size);
            
            file.Close();
        }
        
        return std::move(buffer);
    }
}
