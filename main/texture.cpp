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
    uint32_t Texture::width() { return _width; }
    uint32_t Texture::height() { return _height; }
    Color& Texture::pixel(uint32_t x, uint32_t y)
    {
        assert(x < _width && y < _height);
        return _pixels[y * _width + x];
    }
    bool Texture::save(const std::string_view path)
    {
        std::wstring wpath = std::move(toWide(path));
        return save(wpath);
    }
    bool Texture::save(const std::wstring_view path)
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
