#include "texture.hpp"
#include "common.hpp"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#define  WIN32_LEAN_AND_MEAN
#define  NOMINMAX
#include <Windows.h>
#include <wrl.h>
#include <wincodec.h>
#include <cinttypes>

//#define TEST_ISTREAM

namespace
{
    // in memory IStream adapter
    class Win32Stream : public IStream
    {
    private:
        ULONG  _refcnt  = 1;
        BYTE*  _buffer  = NULL;
        SIZE_T _size    = 0;
        SIZE_T _pointer = 0;
    private:
        void DebugPrint(const char* fmt, ...)
        {
            #if 1
            va_list arg {};
            va_start(arg, fmt);
            const int size = vsnprintf(NULL, 0, fmt, arg);
            va_end(arg);
            std::string buffer(size + 1, 0);
            va_start(arg, fmt);
            vsnprintf(buffer.data(), buffer.size(), fmt, arg);
            va_end(arg);
            OutputDebugStringW(fontatlas::toWide(buffer).c_str());
            #endif
        }
    public:
        HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject)
        {
            if (ppvObject == NULL)
            {
                DebugPrint("Win32Stream::QueryInterface(*, NULL)\n");
                return E_POINTER;
            }
            if (IID_IUnknown == riid)
            {
                DebugPrint("Win32Stream::QueryInterface(IID_IUnknown, *)\n");
                AddRef();
                *ppvObject = dynamic_cast<IUnknown*>(this);
                return S_OK;
            }
            else if (IID_ISequentialStream == riid)
            {
                DebugPrint("Win32Stream::QueryInterface(IID_ISequentialStream, *)\n");
                AddRef();
                *ppvObject = dynamic_cast<ISequentialStream*>(this);
                return S_OK;
            }
            else if (IID_IStream == riid)
            {
                DebugPrint("Win32Stream::QueryInterface(IID_IStream, *)\n");
                AddRef();
                *ppvObject = dynamic_cast<IStream*>(this);
                return S_OK;
            }
            else
            {
                DebugPrint("Win32Stream::QueryInterface(?, *)\n");
                *ppvObject = NULL;
                return E_NOINTERFACE;
            }
        }
        ULONG WINAPI AddRef()
        {
            DebugPrint("Win32Stream::AddRef (%u -> %u)\n", _refcnt, _refcnt + 1);
            _refcnt += 1;
            const ULONG refcnt = _refcnt;
            return refcnt;
        }
        ULONG WINAPI Release()
        {
            assert(_refcnt > 0);
            DebugPrint("Win32Stream::Release (%u -> %u)\n", _refcnt, _refcnt - 1);
            _refcnt -= 1;
            const ULONG refcnt = _refcnt;
            if (refcnt <= 0)
            {
                delete this;
            }
            return refcnt;
        }
    public:
        HRESULT WINAPI Read(void* pv, ULONG cb, ULONG *pcbRead)
        {
            const ULONG allow_ = _size - _pointer;
            if (allow_ >= cb)
            {
                std::memcpy(pv, _buffer + _pointer, cb);
                _pointer += cb;
                if (pcbRead)
                {
                    *pcbRead = cb;
                }
                DebugPrint("Win32Stream::Read(*, %u, %u)\n", cb, cb);
                return S_OK;
            }
            else if (allow_ > 0)
            {
                std::memcpy(pv, _buffer + _pointer, allow_);
                _pointer += allow_;
                if (pcbRead)
                {
                    *pcbRead = allow_;
                }
                DebugPrint("Win32Stream::Read(*, %u, %u)\n", cb, allow_);
                return S_OK;
            }
            else
            {
                if (pcbRead)
                {
                    *pcbRead = 0;
                }
                DebugPrint("Win32Stream::Read(*, %u, 0)\n", cb);
                return S_OK;
            }
        }
        HRESULT WINAPI Write(const void *pv, ULONG cb, ULONG *pcbWritten)
        {
            const ULONG allow_ = _size - _pointer;
            if (allow_ < cb)
            {
                if (_buffer == nullptr)
                {
                    _size   = cb;
                    _buffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _size);
                }
                else
                {
                    _size   = _pointer + cb;
                    _buffer = (BYTE*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _buffer, _size);
                }
            }
            std::memcpy(_buffer + _pointer, pv, cb);
            _pointer += cb;
            if (pcbWritten)
            {
                *pcbWritten = cb;
            }
            DebugPrint("Win32Stream::Write(*, %u, %u)\n", cb, cb);
            return S_OK;
        }
    public:
        HRESULT WINAPI Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
        {
            plibNewPosition->QuadPart = 0;
            switch(dwOrigin)
            {
            case STREAM_SEEK_SET:
                if (dlibMove.QuadPart < 0 || dlibMove.QuadPart > _size)
                {
                    DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_SET, *)\n", dlibMove.QuadPart);
                    return E_INVALIDARG;
                }
                _pointer = dlibMove.QuadPart;
                if (plibNewPosition)
                {
                    plibNewPosition->QuadPart = _pointer;
                }
                DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_SET, %lld)\n", dlibMove.QuadPart, dlibMove.QuadPart);
                return S_OK;
            case STREAM_SEEK_END:
                if ((-dlibMove.QuadPart) < 0 || (-dlibMove.QuadPart) > _size)
                {
                    DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_END, *)\n", dlibMove.QuadPart);
                    return E_INVALIDARG;
                }
                _pointer = _size + dlibMove.QuadPart;
                if (plibNewPosition)
                {
                    plibNewPosition->QuadPart = _pointer;
                }
                DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_END, %lld)\n", dlibMove.QuadPart, dlibMove.QuadPart);
                return S_OK;
            case STREAM_SEEK_CUR:
                if ((dlibMove.QuadPart > 0) && ((_pointer + dlibMove.QuadPart) > _size))
                {
                    DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_CUR, *)\n", dlibMove.QuadPart);
                    return E_INVALIDARG;
                }
                if ((dlibMove.QuadPart < 0) && ((-dlibMove.QuadPart) > _pointer))
                {
                    DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_CUR, *)\n", dlibMove.QuadPart);
                    return E_INVALIDARG;
                }
                _pointer = _size + dlibMove.QuadPart;
                if (plibNewPosition)
                {
                    plibNewPosition->QuadPart = _pointer;
                }
                DebugPrint("Win32Stream::Seek(%lld, STREAM_SEEK_CUR, %lld)\n", dlibMove.QuadPart, dlibMove.QuadPart);
                return S_OK;
            default:
                DebugPrint("Win32Stream::Seek(%lld, ?, *)\n", dlibMove.QuadPart);
                return E_INVALIDARG;
            }
        }
        HRESULT WINAPI SetSize(ULARGE_INTEGER libNewSize)
        {
            #ifdef _WIN64
                DebugPrint("Win32Stream::SetSize(%" PRIi64 ")\n", libNewSize.QuadPart);
                _size   = libNewSize.QuadPart;
            #else
                assert(libNewSize.HighPart == 0);
                //DebugPrint("Win32Stream::SetSize(%" PRIu32 ")\n", libNewSize.LowPart);
                _size   = libNewSize.LowPart;
            #endif
            if (_buffer == nullptr)
            {
                _buffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _size);
            }
            else
            {
                _buffer = (BYTE*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _buffer, _size);
            }
            return S_OK;
        }
        HRESULT WINAPI CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
        {
            DebugPrint("Win32Stream::CopyTo\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI Commit(DWORD grfCommitFlags)
        {
            DebugPrint("Win32Stream::Commit\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI Revert()
        {
            DebugPrint("Win32Stream::Revert\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        {
            DebugPrint("Win32Stream::LockRegion\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
        {
            DebugPrint("Win32Stream::UnlockRegion\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI Stat(STATSTG *pstatstg, DWORD grfStatFlag)
        {
            DebugPrint("Win32Stream::Stat\n");
            return E_NOTIMPL;
        }
        HRESULT WINAPI Clone(IStream **ppstm)
        {
            DebugPrint("Win32Stream::Clone\n");
            *ppstm = nullptr;
            return E_NOTIMPL;
        }
    public:
        Win32Stream()
        {
            DebugPrint("Win32Stream::Win32Stream()\n");
        }
        Win32Stream(const Win32Stream& other)
        {
            DebugPrint("Win32Stream::Win32Stream(other)\n");
            _size    = other._size;
            _pointer = other._pointer;
            _buffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _size);
            std::memcpy(_buffer, other._buffer, _size);
        }
        ~Win32Stream()
        {
            DebugPrint("Win32Stream::~Win32Stream\n");
            if (_buffer)
            {
                HeapFree(GetProcessHeap(), 0, _buffer);
            }
            _buffer  = nullptr;
            _size    = 0;
            _pointer = 0;
        }
    };
}

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
        
        #ifdef TEST_ISTREAM
        Microsoft::WRL::ComPtr<IStream> userstram;
        *userstram.GetAddressOf() = dynamic_cast<IStream*>(new Win32Stream);
        #endif
        
        // create output stream
        Microsoft::WRL::ComPtr<IWICStream> wicstram;
        hr = wicfac->CreateStream(wicstram.GetAddressOf());
        if (hr != S_OK)
        {
            return false;
        }
        #ifdef TEST_ISTREAM
        hr = wicstram->InitializeFromIStream(userstram.Get());
        if (hr != S_OK)
        {
            return false;
        }
        #else
        hr = wicstram->InitializeFromFilename(path.data(), GENERIC_WRITE);
        if (hr != S_OK)
        {
            return false;
        }
        #endif
        
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
