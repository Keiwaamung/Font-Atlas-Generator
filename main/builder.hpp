#pragma once
#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <unordered_map>

namespace fontatlas
{
    class Builder
    {
    public:
        struct FontConfig
        {
            std::string name;
            std::string path;
            uint32_t face;
            uint32_t size;
            std::set<uint32_t> code;
        };
    private:
        std::vector<FontConfig*> _fontlist;
        std::unordered_map<std::string, FontConfig> _font;
    public:
        bool addFont(const std::string_view name, const std::string_view path, uint32_t face, uint32_t size);
        bool addCode(const std::string_view name, uint32_t c);
        bool addRange(const std::string_view name, uint32_t a, uint32_t b);
        bool addText(const std::string_view name, const std::string_view text);
        bool build(const std::string_view path,
            bool is_multi_channel, uint32_t texture_width, uint32_t texture_height, uint32_t texture_edge,
            uint32_t glyph_edge);
    };
}
