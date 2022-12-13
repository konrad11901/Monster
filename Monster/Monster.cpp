#include "Monster.h"
#include <windowsx.h>
#include <cmath>
#include <numbers>
#include <dxgi1_6.h>

Monster::Monster() : hwnd(nullptr), center(), transformation() {}

void Monster::InitializeWindow(HINSTANCE instance, INT cmd_show) {
    CreateDeviceIndependentResources();

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Monster::WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = instance;
    wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
    wcex.lpszClassName = L"D2DMonster";

    winrt::check_bool(RegisterClassEx(&wcex));

    hwnd = CreateWindowEx(
        0,                      // Optional window styles
        L"D2DMonster",          // Window class
        L"Direct2D Monster",    // Window text
        WS_OVERLAPPEDWINDOW,    // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        nullptr,    // Parent window    
        nullptr,    // Menu
        instance,   // Instance handle
        this        // Additional application data
    );

    winrt::check_pointer(hwnd);

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();

    ShowWindow(hwnd, cmd_show);
}

void Monster::RunMessageLoop() {
    MSG msg;

    do {
        mouse_down = (GetAsyncKeyState(VK_LBUTTON) < 0);

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message != WM_QUIT) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            auto time = timer.get_time(2) * std::numbers::pi;
            angle = std::sin(time) * 10.0f;
            OnRender();
        }
    } while (msg.message != WM_QUIT);
}

void Monster::CreateDeviceIndependentResources() {
    D2D1_FACTORY_OPTIONS options{};

    // Initialize the Direct2D Factory.
    winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, d2d_factory.put()));

    monster_path = CreateMonster();
    nose_path = CreateNose();
    smile_path = CreateSmile();
    sad_path = CreateSad();
}

void Monster::CreateDeviceDependentResources() {
    using D2D1::RadialGradientBrushProperties;
    using D2D1::Point2F;

    // This flag adds support for surfaces with a different color channel ordering
    // than the API default. It is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    // This array defines the set of DirectX hardware feature levels this app will support.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> context;

    auto hr = D3D11CreateDevice(
        nullptr,                    // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,   // Create a device using the hardware graphics driver.
        0,                          // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        creationFlags,              // Set Direct2D compatibility flag.
        featureLevels,              // List of feature levels this app can support.
        ARRAYSIZE(featureLevels),   // Size of the list above.
        D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Runtime apps.
        device.put(),               // Returns the Direct3D device created.
        nullptr,                    // Don't store feature level of device created.
        context.put()               // Returns the device immediate context.
    );

    if (FAILED(hr))
    {
        // If the initialization fails, fall back to the WARP device.
        // For more information on WARP, see: 
        // http://go.microsoft.com/fwlink/?LinkId=286690
        winrt::check_hresult(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
            0,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            device.put(),
            nullptr,
            context.put()
        ));
    }

    // Store pointers to the Direct3D 11.1 API device and immediate context.
    device.as(d3d_device);
    context.as(d3d_context);

    // Create the Direct2D device object and a corresponding context.
    winrt::com_ptr<IDXGIDevice4> dxgi_device;
    d3d_device.as(dxgi_device);

    winrt::check_hresult(d2d_factory->CreateDevice(dxgi_device.get(), d2d_device.put()));
    winrt::check_hresult(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_context.put()));

    winrt::check_hresult(d2d_context->CreateSolidColorBrush(brush_color, main_brush.put()));

    winrt::com_ptr<ID2D1GradientStopCollection> rad_stops;
    winrt::check_hresult(d2d_context->CreateGradientStopCollection(rad_stops_data, 3, rad_stops.put()));
    winrt::check_hresult(d2d_context->CreateRadialGradientBrush(
        RadialGradientBrushProperties(Point2F(0, 0), Point2F(0, 0), 150, 150),
        rad_stops.get(), main_rad_brush.put()));

    winrt::com_ptr<ID2D1GradientStopCollection> eye_stops;
    winrt::check_hresult(d2d_context->CreateGradientStopCollection(eye_stops_data, 2, eye_stops.put()));
    winrt::check_hresult(d2d_context->CreateRadialGradientBrush(
        RadialGradientBrushProperties(Point2F(-43.0f, -12.0f), Point2F(0, 0), 34, 34),
        eye_stops.get(), left_eye_brush.put()));
    winrt::check_hresult(d2d_context->CreateRadialGradientBrush(
        RadialGradientBrushProperties(Point2F(43.0f, -12.0f), Point2F(0, 0), 34, 34),
        eye_stops.get(), right_eye_brush.put()));
}

void Monster::CreateWindowSizeDependentResources() {
    using D2D1::Matrix3x2F;

    d2d_context->SetTarget(nullptr);
    d2d_target_bitmap = nullptr;

    if (swap_chain) {
        // If the swap chain already exists, resize it.
        auto hr = swap_chain->ResizeBuffers(
            2,
            0,
            0,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
        );
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else {
            winrt::check_hresult(hr);
        }
    }
    else {
        winrt::com_ptr<IDXGIDevice4> dxgi_device;
        d3d_device.as(dxgi_device);

        winrt::com_ptr<IDXGIAdapter> dxgi_adapter;
        winrt::check_hresult(dxgi_device->GetAdapter(dxgi_adapter.put()));

        winrt::com_ptr<IDXGIFactory7> dxgi_factory;
        winrt::check_hresult(dxgi_adapter->GetParent(IID_PPV_ARGS(dxgi_factory.put())));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

        swapChainDesc.Width = 0;
        swapChainDesc.Height = 0;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = 0;

        winrt::com_ptr<IDXGISwapChain1> swap_chain1;
        dxgi_factory->CreateSwapChainForHwnd(d3d_device.get(), hwnd, &swapChainDesc, nullptr, nullptr, swap_chain1.put());
        swap_chain1.as(swap_chain);
    }

    winrt::com_ptr<IDXGISurface2> dxgi_back_buffer;
    winrt::check_hresult(swap_chain->GetBuffer(0, IID_PPV_ARGS(dxgi_back_buffer.put())));

    // Get screen DPI
    FLOAT dpiX, dpiY;
    dpiX = (FLOAT)GetDpiForWindow(GetDesktopWindow());
    dpiY = dpiX;

    // Create a Direct2D surface (bitmap) linked to the Direct3D texture back buffer via the DXGI back buffer
    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);

    d2d_context->CreateBitmapFromDxgiSurface(dxgi_back_buffer.get(), &bitmapProperties, d2d_target_bitmap.put());

    d2d_context->SetTarget(d2d_target_bitmap.get());

    transformation = Matrix3x2F::Scale(3.0f, 3.0f) * Matrix3x2F::Translation(d2d_context->GetSize().width / 2.0f,
        d2d_context->GetSize().height / 2.0f);
}

void Monster::HandleDeviceLost() {
    swap_chain = nullptr;

    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

void Monster::OnRender() {
    using D2D1::Point2F;
    using D2D1::Matrix3x2F;
    using D2D1::Ellipse;

    auto mouth_transformation = Matrix3x2F::Rotation(angle) * transformation;
    auto left_ball = CreateBall(true), right_ball = CreateBall(false);

    d2d_context->BeginDraw();
    d2d_context->Clear(background_color);

    d2d_context->SetTransform(transformation);
    d2d_context->FillGeometry(monster_path.get(), main_rad_brush.get());
    d2d_context->DrawGeometry(monster_path.get(), main_brush.get());

    d2d_context->FillEllipse(left_eye, left_eye_brush.get());
    d2d_context->FillEllipse(right_eye, right_eye_brush.get());

    d2d_context->FillEllipse(left_ball, main_brush.get());
    d2d_context->FillEllipse(right_ball, main_brush.get());

    d2d_context->SetTransform(mouth_transformation);
    main_brush->SetColor(nose_color);
    d2d_context->FillGeometry(nose_path.get(), main_brush.get());
    main_brush->SetColor(brush_color);
    d2d_context->DrawGeometry(nose_path.get(), main_brush.get());
    d2d_context->DrawGeometry(mouse_down ? smile_path.get() : sad_path.get(), main_brush.get(), 3.0f);

    winrt::check_hresult(d2d_context->EndDraw());

    DXGI_PRESENT_PARAMETERS parameters = { 0 };
    parameters.DirtyRectsCount = 0;
    parameters.pDirtyRects = nullptr;
    parameters.pScrollRect = nullptr;
    parameters.pScrollOffset = nullptr;

    auto hr = swap_chain->Present1(1, 0, &parameters);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // If the device was removed for any reason, a new device and swap chain will need to be created.
        HandleDeviceLost();
    }
    else {
        winrt::check_hresult(hr);
    }
}

void Monster::OnResize(UINT width, UINT height) {
    CreateWindowSizeDependentResources();
}

winrt::com_ptr<ID2D1PathGeometry> Monster::CreateMonster() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> path_sink;
    d2d_factory->CreatePathGeometry(path.put());
    path->Open(path_sink.put());

    path_sink->BeginFigure(Point2F(-2.0f, -74.5f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(7.7f, -74.5f), Point2F(14.5f, -74.4f), Point2F(23.1f, -71.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(30.2f, -69.8f), Point2F(37.5f, -67.6f), Point2F(40.1f, -62.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(40.4f, -61.7f), Point2F(41.2f, -59.8f), Point2F(42.3f, -59.8f)));
    path_sink->AddBezier(BezierSegment(Point2F(43.9f, -59.8f), Point2F(44.9f, -63.6f), Point2F(45.6f, -65.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(48.6f, -73.1f), Point2F(60.3f, -79.3f), Point2F(71.1f, -78.8f)));
    path_sink->AddBezier(BezierSegment(Point2F(72.0f, -78.8f), Point2F(85.9f, -77.8f), Point2F(92.0f, -67.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(99.2f, -54.9f), Point2F(90.5f, -37.9f), Point2F(83.2f, -30.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(79.0f, -25.6f), Point2F(69.8f, -18.0f), Point2F(69.8f, -18.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(71.9f, -7.4f), Point2F(74.7f, 2.9f), Point2F(78.2f, 13.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(84.3f, 30.5f), Point2F(88.2f, 35.3f), Point2F(91.3f, 49.6f)));
    path_sink->AddBezier(BezierSegment(Point2F(94.0f, 61.8f), Point2F(96.2f, 71.7f), Point2F(92.3f, 83.2f)));
    path_sink->AddBezier(BezierSegment(Point2F(87.5f, 97.7f), Point2F(76.5f, 105.6f), Point2F(72.4f, 108.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(62.1f, 115.6f), Point2F(52.4f, 117.1f), Point2F(37.4f, 119.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(26.8f, 121.2f), Point2F(16.3f, 121.3f), Point2F(4.1f, 121.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(0.3f, 121.6f), Point2F(-4.0f, 121.6f), Point2F(-8.9f, 121.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(-21.2f, 121.3f), Point2F(-31.7f, 121.2f), Point2F(-42.3f, 119.5f)));
    path_sink->AddBezier(BezierSegment(Point2F(-56.1f, 117.4f), Point2F(-66.5f, 115.9f), Point2F(-77.3f, 108.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(-81.3f, 105.6f), Point2F(-92.4f, 97.7f), Point2F(-97.2f, 83.2f)));
    path_sink->AddBezier(BezierSegment(Point2F(-101.0f, 71.7f), Point2F(-98.9f, 61.8f), Point2F(-96.2f, 49.6f)));
    path_sink->AddBezier(BezierSegment(Point2F(-93.6f, 37.5f), Point2F(-89.4f, 27.8f), Point2F(-83.1f, 13.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(-77.3f, -0.7f), Point2F(-70.4f, -14.5f), Point2F(-74.6f, -18.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(-74.6f, -18.0f), Point2F(-74.6f, -18.0f), Point2F(-74.6f, -18.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(-80.2f, -22.6f), Point2F(-86.6f, -28.7f), Point2F(-88.0f, -30.1f)));
    path_sink->AddBezier(BezierSegment(Point2F(-92.9f, -35.0f), Point2F(-104.8f, -53.6f), Point2F(-96.8f, -67.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(-90.7f, -77.8f), Point2F(-76.8f, -78.7f), Point2F(-75.9f, -78.8f)));
    path_sink->AddBezier(BezierSegment(Point2F(-65.1f, -79.3f), Point2F(-53.4f, -73.1f), Point2F(-50.4f, -65.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(-49.8f, -63.6f), Point2F(-48.8f, -59.8f), Point2F(-47.2f, -59.8f)));
    path_sink->AddBezier(BezierSegment(Point2F(-46.0f, -59.8f), Point2F(-45.2f, -61.7f), Point2F(-44.9f, -62.4f)));
    path_sink->AddBezier(BezierSegment(Point2F(-42.4f, -67.6f), Point2F(-35.0f, -69.8f), Point2F(-27.9f, -71.9f)));
    path_sink->AddBezier(BezierSegment(Point2F(-19.0f, -74.5f), Point2F(-12.0f, -74.5f), Point2F(-2.0f, -74.55f)));

    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    path_sink->Close();

    return path;
}

winrt::com_ptr<ID2D1PathGeometry> Monster::CreateNose() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> path_sink;
    d2d_factory->CreatePathGeometry(path.put());
    path->Open(path_sink.put());

    path_sink->BeginFigure(Point2F(0.0f, 38.3f), D2D1_FIGURE_BEGIN_FILLED);
    path_sink->AddBezier(BezierSegment(Point2F(1.8f, 40.9f), Point2F(16.2f, 25.1f), Point2F(14.6f, 23.0f)));
    path_sink->AddBezier(BezierSegment(Point2F(15.3f, 18.9f), Point2F(-16.4f, 19.2f), Point2F(-16.3f, 22.3f)));
    path_sink->AddBezier(BezierSegment(Point2F(-16.7f, 26.2f), Point2F(-0.7f, 40.9f), Point2F(0.0f, 38.33f)));
    path_sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    path_sink->Close();

    return path;
}

winrt::com_ptr<ID2D1PathGeometry> Monster::CreateSmile() {
    using D2D1::Point2F;
    using D2D1::SizeF;
    using D2D1::ArcSegment;

    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> path_sink;
    d2d_factory->CreatePathGeometry(path.put());
    path->Open(path_sink.put());

    path_sink->BeginFigure(Point2F(-40.0f, 75.0f), D2D1_FIGURE_BEGIN_HOLLOW);
    path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(10.0f, 7.0f), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    path_sink->Close();

    return path;
}

winrt::com_ptr<ID2D1PathGeometry> Monster::CreateSad() {
    using D2D1::Point2F;
    using D2D1::SizeF;
    using D2D1::ArcSegment;

    winrt::com_ptr<ID2D1PathGeometry> path;
    winrt::com_ptr<ID2D1GeometrySink> path_sink;
    d2d_factory->CreatePathGeometry(path.put());
    path->Open(path_sink.put());

    path_sink->BeginFigure(Point2F(-40.0f, 75.0f), D2D1_FIGURE_BEGIN_HOLLOW);
    path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(8.0f, 3.0f), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    path_sink->Close();

    return path;
}

D2D1_ELLIPSE Monster::CreateBall(bool left_eye) {
    using D2D1::Point2F;
    using D2D1::Matrix3x2F;
    using D2D1::Ellipse;

    Matrix3x2F transformationCopy = transformation;
    if (transformationCopy.IsInvertible()) {
        transformationCopy.Invert();
    }

    auto mousePosTransformed = transformationCopy.TransformPoint(Point2F(mouse_x, mouse_y));

    auto eye_x = left_eye ? -EYE_X_OFFSET : EYE_X_OFFSET;
    auto eye_y = EYE_Y_OFFSET;
    auto dist_x = mousePosTransformed.x - eye_x, dist_y = mousePosTransformed.y - eye_y;

    auto dist_squared = dist_x * dist_x + dist_y * dist_y;

    auto ball_orbit = EYE_RADIUS - EYE_BALL_RADIUS;

    D2D1_POINT_2F ball_pos;
    if (dist_squared <= ball_orbit * ball_orbit) {
        ball_pos = Point2F(mousePosTransformed.x, mousePosTransformed.y);
    }
    else {
        auto fac = ball_orbit / std::sqrt(dist_squared);
        ball_pos = Point2F(eye_x + dist_x * fac, eye_y + dist_y * fac);
    }

    return Ellipse(ball_pos, EYE_BALL_RADIUS, EYE_BALL_RADIUS);
}

LRESULT Monster::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    if (message == WM_CREATE) {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Monster* clock = (Monster*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(clock)
        );

        result = 1;
    }
    else {
        Monster* monster = reinterpret_cast<Monster*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (monster) {
            switch (message) {
            case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                monster->OnResize(width, height);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DISPLAYCHANGE:
            {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_PAINT:
            {
                monster->OnRender();
                ValidateRect(hwnd, nullptr);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DESTROY:
            {
                PostQuitMessage(0);
            }
            result = 1;
            wasHandled = true;
            break;

            case WM_MOUSEMOVE:
            {
                monster->mouse_x = GET_X_LPARAM(lParam);
                monster->mouse_y = GET_Y_LPARAM(lParam);
            }
            result = 0;
            wasHandled = true;
            break;

            }
        }

        if (!wasHandled) {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}
