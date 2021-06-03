#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace fontatlas
{
    struct Color
    {
        union
        {
            struct
            {
                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t a;
            };
            uint32_t argb;
        };
        
        Color() : argb(0)
        {
        }
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
            : b(blue), g(green), r(red), a(alpha)
        {
        }
    };
    
    class Texture
    {
    private:
        uint32_t           _width  = 0;
        uint32_t           _height = 0;
        std::vector<Color> _pixels;
    public:
        uint32_t width();
        uint32_t height();
        Color& pixel(uint32_t x, uint32_t y);
        bool save(const std::string_view path);
        bool save(const std::wstring_view path);
        void clear(Color c = Color(0, 0, 0, 0));
    public:
        Texture(uint32_t width, uint32_t height);
    };
}
