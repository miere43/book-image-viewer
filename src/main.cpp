#include <d2d1.h>
#include <shellapi.h>
#include <dwrite.h>
#include <wincodec.h>
#include <Shlwapi.h>
#include "common.hpp"
#include "epub.hpp"
#include "string.hpp"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

struct Image {
    String fileName;
    ID2D1Bitmap* bitmap = nullptr;
    float width = 0;
    float height = 0;
    int clientWidth = 0;
    int clientHeight = 0;
};

HWND hwnd = 0;
static ID2D1Factory* d2d1Factory = 0;
static IWICImagingFactory2* wicFactory = nullptr;
static ID2D1HwndRenderTarget* hwndRenderTarget = 0;
static EPub* currentEPub;
static std::vector<Image> currentImages;
static int currentImageIndex = 0;

static void initWindow(HINSTANCE hInstance);
static void initCom();
static void redraw();
static void updateTitle();
static LRESULT __stdcall windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow) {
    initCom();
	initWindow(hInstance);

    MSG msg;
    while (GetMessageW(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

	return 0;
}

static void initCom() {
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY | COINIT_DISABLE_OLE1DDE);
    verify(SUCCEEDED(hr));

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d1Factory);
    verify(SUCCEEDED(hr));

    hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (void**)&wicFactory);
    verify(SUCCEEDED(hr));
}

static void initWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = L"BookView";
    wc.hInstance = hInstance;
    wc.lpfnWndProc = windowProc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hCursor = LoadCursorW(0, IDC_ARROW);
    wc.hIcon = LoadIconW(0, IDI_APPLICATION);
    verify(RegisterClassExW(&wc));

    hwnd = CreateWindowExW(
        0,
        L"BookView",
        L"Book Image Viewer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        700,
        700,
        0,
        0,
        hInstance,
        nullptr);
    verify(hwnd);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    D2D1_SIZE_U clientPixelSize = D2D1::SizeU(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

    HRESULT hr = d2d1Factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, clientPixelSize, D2D1_PRESENT_OPTIONS_IMMEDIATELY),
        &hwndRenderTarget);
    verify(SUCCEEDED(hr));

    DragAcceptFiles(hwnd, true);
}

static ID2D1Bitmap* createBitmap(const String& imageData, int clientWidth, int clientHeight) {
    auto stream = SHCreateMemStream((BYTE*)imageData.chars, (UINT)imageData.count);
    verify(stream);

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = wicFactory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    verify(SUCCEEDED(hr));

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    verify(SUCCEEDED(hr));

    UINT width = 0;
    UINT height = 0;
    hr = frame->GetSize(&width, &height);
    verify(SUCCEEDED(hr));

    float scaleX = (float)clientWidth / width;
    float scaleY = (float)clientHeight / height;
    float scale = scaleX > scaleY ? scaleY : scaleX;
    float normWidth = scale * width;
    float normHeight = scale * height;

    IWICBitmapScaler* scaler = nullptr;
    hr = wicFactory->CreateBitmapScaler(&scaler);
    verify(SUCCEEDED(hr));

    hr = scaler->Initialize(frame, (int)normWidth, (int)normHeight, WICBitmapInterpolationModeHighQualityCubic);
    verify(SUCCEEDED(hr));

    IWICFormatConverter* converter = nullptr;
    hr = wicFactory->CreateFormatConverter(&converter);
    verify(SUCCEEDED(hr));

    hr = converter->Initialize(scaler, GUID_WICPixelFormat32bppPRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    verify(SUCCEEDED(hr));

    ID2D1Bitmap* bitmap = nullptr;
    hr = hwndRenderTarget->CreateBitmapFromWicBitmap(converter, &bitmap);
    verify(SUCCEEDED(hr));

    converter->Release();
    scaler->Release();
    frame->Release();
    decoder->Release();
    stream->Release();

    return bitmap;
}

static D2D1_RECT_F centerImage(float width, float height) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    float clientWidth = clientRect.right - clientRect.left;
    float clientHeight = clientRect.bottom - clientRect.top;

    float scaleX = clientWidth / width;
    float scaleY = clientHeight / height;
    float scale = scaleX > scaleY ? scaleY : scaleX;

    float normWidth = scale * width;
    float normHeight = scale * height;

    float x = clientWidth / 2 - normWidth / 2;
    float y = clientHeight / 2 - normHeight / 2;

    D2D1_RECT_F result;
    result.left = x;
    result.top = y;
    result.right = result.left + normWidth;
    result.bottom = result.top + normHeight;
    return result;
}

static void paintWindow() {
    HRESULT hr;
    auto R = hwndRenderTarget;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    R->BeginDraw();
    R->Clear({ 0, 0, 0, 1 });

    if (currentImageIndex < currentImages.size()) {
        auto& image = currentImages[currentImageIndex];
        if (!image.bitmap || image.clientWidth != clientWidth || image.clientHeight != clientHeight) {
            if (image.bitmap) image.bitmap->Release();
            {
                auto imageData = currentEPub->readFile(image.fileName);
                image.bitmap = createBitmap(imageData, clientWidth, clientHeight);
                imageData.destroy();
            }
            D2D1_SIZE_F size = image.bitmap->GetSize();
            image.width = size.width;
            image.height = size.height;
            image.clientWidth = clientWidth;
            image.clientHeight = clientHeight;
        }

        auto rect = centerImage(image.width, image.height);
        R->DrawBitmap(image.bitmap, rect); // @TODO: Use WIC to resize image to correct size for quality result.
    }

    hr = R->EndDraw();
    verify(SUCCEEDED(hr));
}

static void loadEPub(const String& fileName) {
    auto content = new EPub();
    content->parse(fileName);

    std::vector<Image> images;
    for (const auto& imagePath : content->images) {
        Image parsedImage;
        parsedImage.fileName = imagePath;
        images.push_back(parsedImage);
    }

    currentImageIndex = 0;

    if (currentEPub) {
        currentEPub->destroy();
    }

    currentEPub = content;

    for (auto& image : currentImages) {
        if (image.bitmap) image.bitmap->Release();
    }

    currentImages = images;

    redraw();
    updateTitle();
}

static void resizeWindow() {
    RECT size;
    GetClientRect(hwnd, &size);
    
    D2D1_SIZE_U sizeu;
    sizeu.width = size.right - size.left;
    sizeu.height = size.bottom - size.top;

    if (sizeu.width > 0 && sizeu.height > 0) {
        HRESULT hr = hwndRenderTarget->Resize(sizeu);
        verify(SUCCEEDED(hr));
    }
}

template<typename T>
static T clamp(const T& value, const T& min, const T& max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static void handleKeyboard(int key) {
    switch (key) {
        case VK_LEFT: {
            currentImageIndex = clamp(currentImageIndex - 1, 0, (int)currentImages.size());
            redraw();
            updateTitle();
        } break;

        case VK_RIGHT: {
            currentImageIndex = clamp(currentImageIndex + 1, 0, (int)currentImages.size());
            redraw();
            updateTitle();
        } break;
    }
}

static void redraw() {
    InvalidateRect(hwnd, nullptr, false);
}

static void updateTitle() {
    wchar_t buffer[1024];

    if (currentEPub) {
        auto fileName = toUtf16(currentEPub->fileName, nullptr);
        swprintf_s(buffer, L"Book Image Viewer - (%d/%d) - %s", currentImageIndex, (int)currentImages.size(), fileName);
        delete fileName;
        SetWindowTextW(hwnd, buffer);
    } else {
        SetWindowTextW(hwnd, L"Book Image Viewer");
    }
}

static LRESULT __stdcall windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            return 0;
        } break;

        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        } break;

        case WM_SIZE: {
            resizeWindow();
            return 0;
        } break;

        case WM_PAINT: {
            paintWindow();
            ValidateRect(hwnd, nullptr);
            return 0;
        } break;

        case WM_ERASEBKGND: {
            return 1;
        } break;

        case WM_KEYDOWN: {
            handleKeyboard((int)wParam);
            return 0;
        } break;

        case WM_DROPFILES: {
            auto drop = (HDROP)wParam;
            wchar_t fileName[500];
            uint32_t fileNameCount = DragQueryFileW(drop, 0, fileName, _countof(fileName));
            if (fileNameCount > 0) {
                loadEPub(toUtf8(fileName, fileNameCount));
            }
            DragFinish(drop);
            return 0;
        } break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
