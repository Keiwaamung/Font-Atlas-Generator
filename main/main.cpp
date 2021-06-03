#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include "lua.hpp"
#include "ft2build.h"
#include FT_FREETYPE_H
#define  NOMINMAX
#include <Windows.h>
#include <wrl.h>
#include <wincodec.h>

#include "common.hpp"
#include "texture.hpp"
#include "utf.hpp"

namespace fontatlas
{
    struct Config
    {
        std::string font;
        uint32_t face;
        uint32_t size;
        std::vector<std::pair<uint32_t, uint32_t>> ranges;
        std::vector<std::string> texts;
        std::string output;
    };
    
    using ConfigList = std::vector<Config>;
    
    void readConfigFromLua(lua_State* L, ConfigList& config)
    {
        const int lua_stack_size = lua_gettop(L);
        // lua stack = ?
        
        lua_newtable(L); // environment
        
        // lua stack = ? t
        
        Buffer config_file = std::move(readFile("config.lua"));
        const int compile_result = luaL_loadbuffer(L, (const char*)config_file.data(), config_file.size(), "config.lua");
        if (compile_result != LUA_OK)
        {
            // lua stack = ? t s
            
            const std::string_view error_message = luaL_checkstring(L, -1);
            std::cout << error_message << std::endl;
            
            lua_pop(L, 2); // remove environment & error message
            return;
        }
        
        // lua stack = ? t f
        
        lua_pushvalue(L, -2);
        
        // lua stack = ? t f t
        
        const std::string_view upvalue_name = lua_setupvalue(L, -2, 1); // set environment to loaded chunk
        
        // lua stack = ? t f
        
        const int call_result = lua_pcall(L, 0, 0, 0);
        if (call_result != LUA_OK)
        {
            // lua stack = ? t s
            
            const std::string_view error_message = luaL_checkstring(L, -1);
            std::cout << error_message << std::endl;
            
            lua_pop(L, 2); // remove environment & error message
            return;
        }
        
        // lua stack = ? t
        
        lua_getfield(L, -1, "config");
        
        // lua stack = ? t ?
        
        if (!lua_istable(L, -1))
        {
            // lua stack = ? t _
            
            std::cout << "invalid config data" << std::endl;
            
            lua_pop(L, 2); // remove environment & unknown value
            return;
        }
        
        // lua stack = ? t t
        
        const int config_size = luaL_len(L, -1);
        config.clear();
        config.resize(config_size);
        for (int idx = 0; idx < config_size; idx += 1)
        {
            lua_geti(L, -1, idx + 1);
            
            // lua stack = ? t t ?
            
            if (lua_istable(L, -1))
            {
                // lua stack = ? t t t
                
                lua_getfield(L, -1, "font");
                config[idx].font = luaL_checkstring(L, -1);
                lua_pop(L, 1);
                
                lua_getfield(L, -1, "face");
                config[idx].face = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
                
                lua_getfield(L, -1, "size");
                config[idx].size = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
                
                lua_getfield(L, -1, "output");
                config[idx].output = luaL_checkstring(L, -1);
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            
            // lua stack = ? t t
        }
        
        lua_pop(L, 2); // remove config table & environment
        
        // lua stack = ?
        assert(lua_gettop(L) == lua_stack_size);
        return;
    }
    
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
        bool addFont(const std::string_view name, const std::string_view path, uint32_t face, uint32_t size)
        {
            std::string name_;
            name_ = name;
            FontConfig cfg_ = {};
            cfg_.name = name;
            cfg_.path = path;
            cfg_.face = face;
            cfg_.size = size;
            if (_font.find(name_) == _font.end())
            {
                _font.emplace(name_, std::move(cfg_));
                auto it = _font.find(name_);
                _fontlist.push_back(&it->second);
                return true;
            }
            return false;
        }
        bool addCode(const std::string_view name, uint32_t c)
        {
            return addRange(name, c, c);
        }
        bool addRange(const std::string_view name, uint32_t a, uint32_t b)
        {
            std::string name_;
            name_ = name;
            auto it = _font.find(name_);
            if (it != _font.end())
            {
                for (uint32_t c = a; c <= b; c += 1)
                {
                    it->second.code.insert(c);
                }
                return true;
            }
            return false;
        }
        bool addText(const std::string_view name, const std::string_view text)
        {
            std::string name_;
            name_ = name;
            auto it = _font.find(name_);
            if (it != _font.end())
            {
                char32_t c = 0;
                utf::utf8reader reader(text.data(), text.size());
                while (reader(c))
                {
                    it->second.code.insert((uint32_t)c);
                }
                return true;
            }
            return false;
        }
        bool save(const std::string_view path)
        {
            // create dir
            std::filesystem::create_directories(path); // might not unicode
            
            struct Wrapper
            {
                FT_Library library = NULL;
                std::vector<FT_Face> face;
                
                ~Wrapper()
                {
                    for (auto& f : face)
                    {
                        if (f)
                        {
                            FT_Done_Face(f);
                            f = NULL;
                        }
                    }
                    face.clear();
                    if (library)
                    {
                        FT_Done_FreeType(library);
                        library = NULL;
                    }
                }
            };
            
            FT_Error fterr_ = 0;
            Wrapper  ft_;
            
            // open freetype
            fterr_ = FT_Init_FreeType(&ft_.library);
            if (fterr_ != FT_Err_Ok)
            {
                return false;
            }
            
            // create all face
            for (auto v : _fontlist)
            {
                ft_.face.push_back(NULL);
                FT_Face& ftface_ = ft_.face.back();
                fterr_ = FT_New_Face(ft_.library, v->path.c_str(), v->face, &ftface_);
                if (fterr_ != FT_Err_Ok)
                {
                    return false;
                }
                fterr_ = FT_Set_Char_Size(ftface_, v->size * 64, v->size * 64, 72, 72);
                if (fterr_ != FT_Err_Ok)
                {
                    return false;
                }
            }
            
            // get all glyph size all sort
            struct GlyphInfo
            {
                uint32_t code;
                uint32_t width;
                uint32_t height;
                uint32_t font;
            };
            std::vector<GlyphInfo> _glyphlist;
            for (uint32_t idx = 0; idx < _fontlist.size(); idx += 1)
            {
                for (uint32_t c : _fontlist[idx]->code)
                {
                    FT_UInt cidx = FT_Get_Char_Index(ft_.face[idx], c);
                    if (cidx > 0)
                    {
                        fterr_ = FT_Load_Glyph(ft_.face[idx], cidx, FT_LOAD_DEFAULT);
                        if (fterr_ == FT_Err_Ok)
                        {
                            GlyphInfo info_ = {
                                c,
                                ft_.face[idx]->glyph->bitmap.width,
                                ft_.face[idx]->glyph->bitmap.rows,
                                idx,
                            };
                            _glyphlist.push_back(info_);
                        }
                    }
                }
            }
            struct GlyphInfoComparer {
                bool operator()(const GlyphInfo& a, const GlyphInfo& b) const
                {   
                    if (a.height != b.height)
                    {
                        return a.height < b.height;
                    }
                    else if (a.width != b.width)
                    {
                        return a.width < b.width;
                    }
                    else if (a.code != b.code)
                    {
                        return a.code < b.code;
                    }
                    else
                    {
                        return a.font < b.font;
                    }
                }
            };
            GlyphInfoComparer _comparer;
            std::sort(_glyphlist.begin(), _glyphlist.end(), _comparer);
            
            // generate font atlas
            {
                bool multi = true;
                uint32_t image = 1;
                uint32_t space = 1; // texture edge space & minimum space between glyphs
                uint32_t edge = 0; // glyph edge space
                uint32_t width = 2048;
                uint32_t height = 2048;
                fontatlas::Texture tex(width, height);
                uint32_t x = space;
                uint32_t y = space;
                uint32_t down = 0;
                uint32_t channel = 0; // 0 r 1 g 2 b 3 a
                auto save_image = [&]()
                {
                    char buffer_[256] = {};
                    snprintf(buffer_, 256, "%s%u.png", path.data(), image);
                    tex.save(buffer_);
                    tex.clear();
                    image += 1;
                };
                auto upload_bitmap = [&](FT_Bitmap& bitmap)
                {
                    if (bitmap.num_grays == 256)
                    {
                        // glyph size + edge size
                        uint32_t glyphx = bitmap.width + 2 * edge;
                        uint32_t glyphy = bitmap.rows  + 2 * edge;
                        // check horizontal space
                        if ((x + glyphx) > (width - space))
                        {
                            // next line
                            x = space;
                            y += (down + space);
                            down = 0;
                        }
                        // check vertical space
                        if ((y + glyphy) > (height - space))
                        {
                            if (multi)
                            {
                                if (channel >= 3)
                                {
                                    // next image
                                    save_image();
                                    channel = 0;
                                }
                                else
                                {
                                    // next channel
                                    channel += 1;
                                }
                            }
                            else
                            {
                                // next image
                                save_image();
                            }
                            // reset
                            x = space;
                            y = space;
                            down = 0;
                        }
                        // copy pixel data
                        uint8_t* buffer = bitmap.buffer;
                        uint32_t startx = x + edge;
                        uint32_t starty = y + edge;
                        uint32_t endx = startx + bitmap.width;
                        uint32_t endy = starty + bitmap.rows;
                        for (uint32_t peny = starty; peny < endy; peny += 1)
                        {
                            uint32_t index = 0;
                            for (uint32_t penx = startx; penx < endx; penx += 1)
                            {
                                if (!multi)
                                {
                                    tex.pixel(penx, peny) = fontatlas::Color(255, 255, 255, buffer[index]);
                                }
                                else
                                {
                                    switch(channel)
                                    {
                                    case 0:
                                        tex.pixel(penx, peny).r = buffer[index];
                                        break;
                                    case 1:
                                        tex.pixel(penx, peny).g = buffer[index];
                                        break;
                                    case 2:
                                        tex.pixel(penx, peny).b = buffer[index];
                                        break;
                                    case 3:
                                    default:
                                        tex.pixel(penx, peny).a = buffer[index];
                                        break;
                                    }
                                    //tex.pixel(penx, peny).a = 255; // debug
                                }
                                index += 1;
                            }
                            buffer += bitmap.pitch;
                        }
                        // move to right
                        x += (glyphx + space);
                        down = std::max(down, glyphy);
                    }
                };
                auto all_glyph = [&]()
                {
                    for (auto& v : _glyphlist)
                    {
                        FT_UInt cidx = FT_Get_Char_Index(ft_.face[v.font], v.code);
                        fterr_ = FT_Load_Glyph(ft_.face[v.font], cidx, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
                        assert(fterr_ == FT_Err_Ok);
                        if (fterr_ == FT_Err_Ok)
                        {
                            upload_bitmap(ft_.face[v.font]->glyph->bitmap);
                        }
                    }
                };
                all_glyph();
                save_image();
            }
            
            return true;
        }
    };
}

void test()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    fontatlas::ConfigList config;
    fontatlas::readConfigFromLua(L, config);
    
    lua_close(L);
    L = NULL;
    
    FT_Error fterr = 0;
    
    FT_Library ftlib = NULL;
    fterr = FT_Init_FreeType(&ftlib);
    
    fterr = FT_Done_FreeType(ftlib);
    ftlib = NULL;
}

void test2()
{
    FT_Error fterr = 0;
    FT_Library ftlib = NULL;
    fterr = FT_Init_FreeType(&ftlib);
    {
        FT_Face ftface = NULL;
        fterr = FT_New_Face(ftlib, "C:\\Windows\\Fonts\\msyh.ttc", 0, &ftface);
        fterr = FT_Set_Char_Size(ftface, 32 * 64, 32 * 64, 72, 72);
        {
            bool multi = false;
            uint32_t image = 1;
            uint32_t space = 1; // texture edge space & minimum space between glyphs
            uint32_t edge = 0; // glyph edge space
            uint32_t width = 2048;
            uint32_t height = 2048;
            fontatlas::Texture tex(width, height);
            uint32_t x = space;
            uint32_t y = space;
            uint32_t down = 0;
            uint32_t channel = 0; // 0 r 1 g 2 b 3 a
            auto save_image = [&]()
            {
                char path[256] = {};
                snprintf(path, 256, "%u.png", image);
                tex.save(path);
                tex.clear();
                image += 1;
            };
            auto upload_bitmap = [&](FT_Bitmap& bitmap)
            {
                if (bitmap.num_grays == 256)
                {
                    // glyph size + edge size
                    uint32_t glyphx = bitmap.width + 2 * edge;
                    uint32_t glyphy = bitmap.rows  + 2 * edge;
                    // check horizontal space
                    if ((x + glyphx) > (width - space))
                    {
                        // next line
                        x = space;
                        y += (down + space);
                        down = 0;
                    }
                    // check vertical space
                    if ((y + glyphy) > (height - space))
                    {
                        if (multi)
                        {
                            if (channel >= 3)
                            {
                                // next image
                                save_image();
                                channel = 0;
                            }
                            else
                            {
                                // next channel
                                channel += 1;
                            }
                        }
                        else
                        {
                            // next image
                            save_image();
                        }
                        // reset
                        x = space;
                        y = space;
                        down = 0;
                    }
                    // copy pixel data
                    uint8_t* buffer = bitmap.buffer;
                    uint32_t startx = x + edge;
                    uint32_t starty = y + edge;
                    uint32_t endx = startx + bitmap.width;
                    uint32_t endy = starty + bitmap.rows;
                    for (uint32_t peny = starty; peny < endy; peny += 1)
                    {
                        uint32_t index = 0;
                        for (uint32_t penx = startx; penx < endx; penx += 1)
                        {
                            if (!multi)
                            {
                                tex.pixel(penx, peny) = fontatlas::Color(255, 255, 255, buffer[index]);
                            }
                            else
                            {
                                switch(channel)
                                {
                                case 0:
                                    tex.pixel(penx, peny).r = buffer[index];
                                    break;
                                case 1:
                                    tex.pixel(penx, peny).g = buffer[index];
                                    break;
                                case 2:
                                    tex.pixel(penx, peny).b = buffer[index];
                                    break;
                                case 3:
                                default:
                                    tex.pixel(penx, peny).a = buffer[index];
                                    break;
                                }
                                //tex.pixel(penx, peny).a = 255; // debug
                            }
                            index += 1;
                        }
                        buffer += bitmap.pitch;
                    }
                    // move to right
                    x += (glyphx + space);
                    down = std::max(down, glyphy);
                }
            };
            auto unicode_range = [&](uint32_t a, uint32_t b)
            {
                for (int32_t c = a; c <= b; c += 1)
                {
                    FT_UInt index = FT_Get_Char_Index(ftface, c);
                    fterr = FT_Load_Glyph(ftface, index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
                    upload_bitmap(ftface->glyph->bitmap);
                }
            };
            unicode_range(32, 126); // ascii
            unicode_range(0x4E00, 0x9FFF); // CJK unified ideographs
            save_image();
        }
        fterr = FT_Done_Face(ftface);
        ftface = NULL;
    }
    fterr = FT_Done_FreeType(ftlib);
    ftlib = NULL;
}

int main(int, char**)
{
    CoInitializeEx(0, COINIT_MULTITHREADED);
    
    fontatlas::Builder builder;
    
    builder.addFont("Sans24", "SourceHanSansSC-Regular.otf", 0, 24);
    builder.addRange("Sans24", 32, 126);
    builder.addRange("Sans24", 0x4E00, 0x9FFF);
    
    //builder.addFont("System24", "C:\\Windows\\Fonts\\msyh.ttc", 0, 24);
    //builder.addText("System24", "System.IO.FileStream: 无法打开文件");
    
    builder.save("font\\");
    
    CoUninitialize();
    return 0;
}
