#include "builder.hpp"
#include "common.hpp"
#include "texture.hpp"
#include "utf.hpp"
#include <cassert>
#include <algorithm>
#include <filesystem>
#include "ft2build.h"
#include FT_FREETYPE_H

namespace fontatlas
{
    bool Builder::addFont(const std::string_view name, const std::string_view path, uint32_t face, uint32_t size)
    {
        if (!std::filesystem::is_regular_file(toWide(path)))
        {
            return false;
        }
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
    bool Builder::addCode(const std::string_view name, uint32_t c)
    {
        return addRange(name, c, c);
    }
    bool Builder::addRange(const std::string_view name, uint32_t a, uint32_t b)
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
    bool Builder::addText(const std::string_view name, const std::string_view text)
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
    bool Builder::build(const std::string_view path,
        bool is_multi_channel, uint32_t texture_width, uint32_t texture_height, uint32_t texture_edge,
        uint32_t glyph_edge)
    {
        // create dir
        std::filesystem::create_directories(toWide(path));
        
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
        struct GlyphInfoComparer
        {
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
            uint32_t image = 1;
            fontatlas::Texture tex(texture_width, texture_height);
            uint32_t x = texture_edge;
            uint32_t y = texture_edge;
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
                    // real glyph size
                    uint32_t glyphx = bitmap.width + 2 * glyph_edge;
                    uint32_t glyphy = bitmap.rows  + 2 * glyph_edge;
                    // check horizontal space
                    if ((x + glyphx) > (texture_width - texture_edge))
                    {
                        // next line
                        x = texture_edge;
                        y += (down + texture_edge);
                        down = 0;
                    }
                    // check vertical space
                    if ((y + glyphy) > (texture_height - texture_edge))
                    {
                        if (is_multi_channel)
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
                        x = texture_edge;
                        y = texture_edge;
                        down = 0;
                    }
                    // copy pixel data
                    uint8_t* buffer = bitmap.buffer;
                    uint32_t startx = x + glyph_edge;
                    uint32_t starty = y + glyph_edge;
                    uint32_t endx = startx + bitmap.width;
                    uint32_t endy = starty + bitmap.rows;
                    for (uint32_t peny = starty; peny < endy; peny += 1)
                    {
                        uint32_t index = 0;
                        for (uint32_t penx = startx; penx < endx; penx += 1)
                        {
                            if (!is_multi_channel)
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
                    x += (glyphx + texture_edge);
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
}
