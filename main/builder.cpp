#include "builder.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "texture.hpp"
#include "utf.hpp"
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
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
    void Builder::setImageFileFormat(ImageFileFormat format)
    {
        _fileformat = format;
    }
    void Builder::setMultiChannelEnable(bool v)
    {
        _multichannel = v;
    }
    bool Builder::build(const std::string_view path,
        uint32_t texture_width, uint32_t texture_height, uint32_t texture_edge,
        uint32_t glyph_edge)
    {
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
            // basic
            uint32_t code;
            uint32_t width;
            uint32_t height;
            uint32_t font;
            // on texture
            uint32_t channel;
            float uv_x;
            float uv_y;
            float uv_width;
            float uv_height;
            // on drawing
            float draw_width;
            float draw_height;
            float h_pen_x;
            float h_pen_y;
            float h_advance;
            float v_pen_x;
            float v_pen_y;
            float v_advance;
        };
        std::vector<GlyphInfo> glyphlist_;
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
                        GlyphInfo info_ = {};
                        info_.code = c;
                        info_.width = ft_.face[idx]->glyph->bitmap.width,
                        info_.height = ft_.face[idx]->glyph->bitmap.rows,
                        info_.font = idx;
                        glyphlist_.push_back(info_);
                    }
                }
                else
                {
                    logger::warn("font \"%s\": glyph %u not found\n", _fontlist[idx]->name.c_str(), c);
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
        GlyphInfoComparer comparer_;
        std::sort(glyphlist_.begin(), glyphlist_.end(), comparer_);
        
        // generate font atlas
        {
            uint32_t image = 1;
            fontatlas::Texture tex(texture_width, texture_height);
            uint32_t x = texture_edge;
            uint32_t y = texture_edge;
            uint32_t down = 0;
            uint32_t channel = 0; // 0 r 1 g 2 b 3 a
            uint32_t image_glyphs = 0;
            auto save_image = [&]()
            {
                char buffer_[256] = {};
                switch(_fileformat)
                {
                case ImageFileFormat::BMP:
                    snprintf(buffer_, 256, "%s%u.bmp", path.data(), image);
                    tex.save(buffer_, ImageFileFormat::BMP);
                    logger::info("%u.bmp: %u glyphs\n", image, image_glyphs);
                    break;
                case ImageFileFormat::PNG:
                default:
                    snprintf(buffer_, 256, "%s%u.png", path.data(), image);
                    tex.save(buffer_, ImageFileFormat::PNG);
                    logger::info("%u.png: %u glyphs\n", image, image_glyphs);
                    break;
                }
                tex.clear();
                image += 1;
                image_glyphs = 0;
            };
            auto upload_bitmap = [&](GlyphInfo& info, FT_GlyphSlot& glyph, FT_Bitmap& bitmap)
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
                        if (!_multichannel)
                        {
                            
                            // next image
                            save_image();
                        }
                        else
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
                            if (!_multichannel)
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
                    // save data
                    info.channel = channel;
                    info.uv_x = (float)x;
                    info.uv_y = (float)y;
                    info.uv_width  = (float)glyphx;
                    info.uv_height = (float)glyphy;
                    const float offset_xy = (float)glyph_edge;
                    info.draw_width  = (float)glyph->metrics.width  / 64.0f + 2.0f * offset_xy;
                    info.draw_height = (float)glyph->metrics.height / 64.0f + 2.0f * offset_xy;
                    info.h_pen_x = (float)glyph->metrics.horiBearingX / 64.0f - offset_xy;
                    info.h_pen_y = (float)glyph->metrics.horiBearingY / 64.0f + offset_xy;
                    info.h_advance = (float)glyph->metrics.horiAdvance / 64.0f;
                    info.v_pen_x = (float)glyph->metrics.vertBearingX / 64.0f - offset_xy;
                    info.v_pen_y = (float)glyph->metrics.vertBearingY / 64.0f + offset_xy;
                    info.v_advance = (float)glyph->metrics.vertAdvance / 64.0f;
                    // move to right
                    x += (glyphx + texture_edge);
                    down = std::max(down, glyphy);
                }
            };
            auto all_glyph = [&]()
            {
                for (auto& v : glyphlist_)
                {
                    FT_UInt cidx = FT_Get_Char_Index(ft_.face[v.font], v.code);
                    fterr_ = FT_Load_Glyph(ft_.face[v.font], cidx, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
                    assert(fterr_ == FT_Err_Ok);
                    if (fterr_ == FT_Err_Ok)
                    {
                        upload_bitmap(v, ft_.face[v.font]->glyph, ft_.face[v.font]->glyph->bitmap);
                        image_glyphs += 1;
                    }
                }
            };
            std::filesystem::create_directories(toWide(path));
            all_glyph();
            save_image();
        }
        
        // get all glyph info all sort
        std::vector<std::vector<GlyphInfo*>> fontlist_(_fontlist.size());
        for (auto& v : glyphlist_)
        {
            fontlist_[v.font].push_back(&v);
        }
        struct PGlyphInfoComparer
        {
            bool operator()(const GlyphInfo* a, const GlyphInfo* b) const
            {   
                return a->code < b->code;
            }
        };
        PGlyphInfoComparer pcomparer_;
        for (auto& v : fontlist_)
        {
            std::sort(v.begin(), v.end(), pcomparer_);
        }
        
        // generate index file
        {
            char fmtbuf_[1024] = {};
            std::wstring wpath_ = toWide(path) + L"\\index.lua";
            std::ofstream file_(wpath_, std::ios::binary |std::ios::out | std::ios::trunc);
            if (file_.is_open())
            {
                file_.write("local font = {}\n", 16);
                for (uint32_t idx = 0; idx < fontlist_.size(); idx += 1)
                {
                    file_.write("font[\"", 6);
                    file_.write(_fontlist[idx]->name.data(), _fontlist[idx]->name.size());
                    file_.write("\"] = {\n", 7);
                    {
                        int n = std::snprintf(fmtbuf_, 1024,
                            "  multi_channel=%s,\n"
                            "  ascender=%g,\n"
                            "  descender=%g,\n"
                            "  height=%g,\n"
                            "  max_advance=%g,\n",
                            _multichannel ? "true" : "false",
                            (float)ft_.face[idx]->size->metrics.ascender / 64.0f,
                            (float)ft_.face[idx]->size->metrics.descender / 64.0f,
                            (float)ft_.face[idx]->size->metrics.height / 64.0f,
                            (float)ft_.face[idx]->size->metrics.max_advance / 64.0f);
                        file_.write(fmtbuf_, n);
                    }
                    for (uint32_t i = 0; i < fontlist_[idx].size(); i += 1)
                    {
                        auto& v = *fontlist_[idx][i];
                        if (!_multichannel)
                        {
                            int n = std::snprintf(fmtbuf_, 1024,
                                "  [%u]={"
                                "%g,%g,%g,%g"
                                ",%g,%g"
                                ",%g,%g,%g"
                                ",%g,%g,%g"
                                "},\n",
                                v.code,
                                v.uv_x, v.uv_y, v.uv_width, v.uv_height,
                                v.draw_width, v.draw_height,
                                v.h_pen_x, v.h_pen_y, v.h_advance,
                                v.v_pen_x, v.v_pen_y, v.v_advance);
                            file_.write(fmtbuf_, n);
                        }
                        else
                        {
                            int n = std::snprintf(fmtbuf_, 1024,
                                "  [%u]={"
                                "%u,%g,%g,%g,%g"
                                ",%g,%g"
                                ",%g,%g,%g"
                                ",%g,%g,%g"
                                "},\n",
                                v.code,
                                v.channel, v.uv_x, v.uv_y, v.uv_width, v.uv_height,
                                v.draw_width, v.draw_height,
                                v.h_pen_x, v.h_pen_y, v.h_advance,
                                v.v_pen_x, v.v_pen_y, v.v_advance);
                            file_.write(fmtbuf_, n);
                        }
                    }
                    file_.write("}\n", 2);
                }
                file_.write("return font\n", 12);
                file_.close();
            }
        }
        
        return true;
    }
}
