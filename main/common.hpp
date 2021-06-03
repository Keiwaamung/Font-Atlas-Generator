#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace fontatlas
{
    using Buffer = std::vector<uint8_t>;
    
    std::wstring toWide(const std::string_view  str);
    std::string  toUTF8(const std::wstring_view str);
    
    Buffer readFile(const std::string_view  path);
    Buffer readFile(const std::wstring_view path);
    
    class ScopeCoInitialize
    {
    private:
        bool _init = false;
    public:
        ScopeCoInitialize();
        ~ScopeCoInitialize();
    };
}
