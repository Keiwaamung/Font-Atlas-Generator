#include "texture.hpp"
#include "common.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#define  WIN32_LEAN_AND_MEAN
#define  NOMINMAX
#include <Windows.h>
#include <wrl.h>
#include <wincodec.h>

namespace fontatlas
{
    bool Texture::_saveBMP(const std::wstring_view path)
    {
        // create file
        Microsoft::WRL::Wrappers::FileHandle file;
        file.Attach(CreateFileW(
            path.data(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        ));
        if (!file.IsValid())
        {
            return false;
        }
        
        // method
        auto write_ = [&](void* data, size_t size) -> bool {
            assert(size <= 0x7FFFFFFF);
            if (size > 0x7FFFFFFF)
            {
                return false;
            }
            DWORD write_size_ = 0;
            if (FALSE == WriteFile(file.Get(), data, size & 0x7FFFFFFF, &write_size_, NULL))
            {
                assert(false);
                return false;
            }
            assert(write_size_ == size);
            if (write_size_ != size)
            {
                return false;
            }
            return true;
        };
        
        // require file size
        const size_t total_file_size_   = sizeof(BITMAPFILEHEADER)
                                        + sizeof(BITMAPINFOHEADER)
                                        + sizeof(Color) * _width * _height; // always 4 byte align, so we can multiple them simply
        assert(total_file_size_ <= 0x7FFFFFFF);
        
        // head data
        BITMAPFILEHEADER bmp_file_head_ = {};
        bmp_file_head_.bfType = 0x4D42; // "BM"
        bmp_file_head_.bfSize = total_file_size_;
        bmp_file_head_.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        BITMAPINFOHEADER bmp_info_head_ = {};
        bmp_info_head_.biSize = sizeof(BITMAPINFOHEADER);
        bmp_info_head_.biWidth = _width;
        bmp_info_head_.biHeight = _height;
        bmp_info_head_.biPlanes = 1;
        bmp_info_head_.biBitCount = 32;
        bmp_info_head_.biCompression = BI_RGB;
        
        // write data
        if (!write_(&bmp_file_head_, sizeof(bmp_file_head_))) return false;
        if (!write_(&bmp_info_head_, sizeof(bmp_info_head_))) return false;
        Color* px = _pixels.data() + _height * _width;
        for (uint32_t v = _height; v > 0; v += 1)
        {
            px -= _width;
            if (!write_(px, sizeof(Color) * _width)) return false;
        }
        return true;
    }
    bool Texture::_savePNG(const std::wstring_view path)
    {
        HRESULT hr = 0;
        
        // create factory
        Microsoft::WRL::ComPtr<IWICImagingFactory> wicfac;
        hr = CoCreateInstance(CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(wicfac.GetAddressOf()));
        if (hr != S_OK)
        {
            return false;
        }
        
        // create output stream
        Microsoft::WRL::ComPtr<IWICStream> wicstram;
        hr = wicfac->CreateStream(wicstram.GetAddressOf());
        if (hr != S_OK)
        {
            return false;
        }
        std::filesystem::path winpath_(path, std::filesystem::path::native_format);
        hr = wicstram->InitializeFromFilename(winpath_.wstring().c_str(), GENERIC_WRITE);
        if (hr != S_OK)
        {
            return false;
        }
        
        // create encoder
        Microsoft::WRL::ComPtr<IWICBitmapEncoder> wicenc;
        hr = wicfac->CreateEncoder(GUID_ContainerFormatPng, NULL, wicenc.GetAddressOf());
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicenc->Initialize(wicstram.Get(), WICBitmapEncoderNoCache);
        if (hr != S_OK)
        {
            return false;
        }
        
        // create frame encoder
        Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> wicframe;
        Microsoft::WRL::ComPtr<IPropertyBag2> wicprop;
        hr = wicenc->CreateNewFrame(wicframe.GetAddressOf(), NULL);
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicframe->Initialize(NULL);
        if (hr != S_OK)
        {
            return false;
        }
        
        // write data
        WICPixelFormatGUID wicfmt = GUID_WICPixelFormat32bppBGRA;
        hr = wicframe->SetResolution(72.0, 72.0); // default dpi
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicframe->SetSize(_width, _height);
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicframe->SetPixelFormat(&wicfmt);
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicframe->WritePixels(_height, _width * sizeof(Color), _pixels.size() * sizeof(Color), (BYTE*)_pixels.data());
        if (hr != S_OK)
        {
            return false;
        }
        
        // commit
        hr = wicframe->Commit();
        if (hr != S_OK)
        {
            return false;
        }
        hr = wicenc->Commit();
        if (hr != S_OK)
        {
            return false;
        }
        
        return true;
    }
    uint32_t Texture::width() { return _width; }
    uint32_t Texture::height() { return _height; }
    Color& Texture::pixel(uint32_t x, uint32_t y)
    {
        assert(x < _width && y < _height);
        return _pixels[y * _width + x];
    }
    bool Texture::save(const std::string_view path, ImageFileFormat format)
    {
        std::wstring wpath = std::move(toWide(path));
        return save(wpath);
    }
    bool Texture::save(const std::wstring_view path, ImageFileFormat format)
    {
        switch(format)
        {
        case ImageFileFormat::BMP:
            return _saveBMP(path);
        case ImageFileFormat::PNG:
            return _savePNG(path);
        default:
            return false;
        }
    }
    void Texture::clear(Color c)
    {
        for (auto& px : _pixels)
        {
            px = c;
        }
    }
    Texture::Texture(uint32_t width, uint32_t height)
        : _width(width), _height(height), _pixels(width * height)
    {
        clear();
    }
}
