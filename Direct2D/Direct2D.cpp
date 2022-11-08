#include "Direct2D.h"
#include <d2d1_3.h>
#include <windowsx.h>
#include <cmath>
#include <numbers>

Timer::Timer() {
    QueryPerformanceFrequency(&frequency);
}

double Timer::get_time(int seconds) {
    LARGE_INTEGER curr_counter;
    QueryPerformanceCounter(&curr_counter);

    auto difference = curr_counter.QuadPart % (frequency.QuadPart * seconds);

    return (double)difference / (double)frequency.QuadPart;
}

Direct2D::Direct2D() : hwnd(nullptr), d2d_factory(nullptr), d2d_render_target(nullptr), main_brush(nullptr),
main_rad_brush(nullptr), left_eye_brush(nullptr), right_eye_brush(nullptr), monster_path(nullptr) {}

Direct2D::~Direct2D() {
    if (d2d_factory) {
        d2d_factory->Release();
    }
    if (d2d_render_target) {
        d2d_render_target->Release();
    }
}

HRESULT Direct2D::Initialize(HINSTANCE instance, INT cmd_show) {
    auto hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory);

    // Register the window class.
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Direct2D::WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = instance;
    wcex.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
    wcex.lpszClassName = L"D2DDemoApp";

    ATOM register_result = RegisterClassEx(&wcex);
    if (register_result == 0) {
        return 1;
    }

    hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        L"D2DDemoApp",                     // Window class
        L"Learn to Program Direct2D",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        nullptr,       // Parent window    
        nullptr,       // Menu
        instance,  // Instance handle
        this        // Additional application data
    );

    if (hwnd == nullptr) {
        return 1;
    }

    ShowWindow(hwnd, cmd_show);

    return hr;
}

void Direct2D::RunMessageLoop() {
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

HRESULT Direct2D::CreateDeviceResources() {
    using D2D1::RadialGradientBrushProperties;
    using D2D1::ColorF;
    using D2D1::Point2F;

    HRESULT hr = S_OK;

    if (!d2d_render_target) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        auto size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        hr = d2d_factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &d2d_render_target
        );

        if (SUCCEEDED(hr)) {
            hr = d2d_render_target->CreateSolidColorBrush(brush_color, &main_brush);
        }

        ID2D1GradientStopCollection* rad_stops = nullptr;
        UINT const NUM_RAD_STOPS = 3;
        D2D1_GRADIENT_STOP rad_stops_data[NUM_RAD_STOPS];
        ID2D1GradientStopCollection* eye_stops = nullptr;
        UINT const NUM_EYE_STOPS = 2;
        D2D1_GRADIENT_STOP eye_stops_data[NUM_EYE_STOPS];

        if (SUCCEEDED(hr)) {
            rad_stops_data[0] =
            { .position = 0.0f, .color = ColorF(0.0f, 1.0f, 0.0f, 1.0f) };
            rad_stops_data[1] =
            { .position = 0.6f, .color = ColorF(0.0f, 0.7f, 0.0f, 1.0f) };
            rad_stops_data[2] =
            { .position = 1.0f, .color = ColorF(0.0f, 0.3f, 0.0f, 1.0f) };
            hr = d2d_render_target->CreateGradientStopCollection(
                rad_stops_data, NUM_RAD_STOPS, &rad_stops);
            if (SUCCEEDED(hr)) {
                hr = d2d_render_target->CreateRadialGradientBrush(
                    RadialGradientBrushProperties(Point2F(0, 0),
                        Point2F(0, 0), 150, 150),
                    rad_stops, &main_rad_brush);
            }
        }

        if (SUCCEEDED(hr)) {
            eye_stops_data[0] =
            { .position = 0.9f, .color = ColorF(1.0f, 1.0f, 1.0f, 1.0f) };
            eye_stops_data[1] =
            { .position = 1.0f, .color = ColorF(0.0f, 0.0f, 0.0f, 1.0f) };
            hr = d2d_render_target->CreateGradientStopCollection(
                eye_stops_data, NUM_EYE_STOPS, &eye_stops);
            if (SUCCEEDED(hr)) {
                hr = d2d_render_target->CreateRadialGradientBrush(
                    RadialGradientBrushProperties(Point2F(-43.0f, -12.0f),
                        Point2F(0, 0), 34, 34),
                    eye_stops, &left_eye_brush);
                hr = d2d_render_target->CreateRadialGradientBrush(
                    RadialGradientBrushProperties(Point2F(43.0f, -12.0f),
                        Point2F(0, 0), 34, 34),
                    eye_stops, &right_eye_brush);
            }
        }

        if (SUCCEEDED(hr)) {
            monster_path = create_monster();
        }
    }

    return hr;
}

void Direct2D::DiscardDeviceResources() {
    if (d2d_render_target) d2d_render_target->Release();
    if (main_brush) main_brush->Release();
    if (left_eye_brush) left_eye_brush->Release();
    if (right_eye_brush) right_eye_brush->Release();
    if (main_rad_brush) main_rad_brush->Release();
    if (monster_path) monster_path->Release();
}

INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE instance,
    _In_opt_ [[maybe_unused]] HINSTANCE prev_instance,
    _In_ [[maybe_unused]] PWSTR cmd_line,
    _In_ [[maybe_unused]] INT cmd_show) {
    Direct2D app;

    if (SUCCEEDED(app.Initialize(instance, cmd_show))) {
        app.RunMessageLoop();
    }

    return 0;
}

void Direct2D::OnResize(UINT width, UINT height) {
    if (d2d_render_target) {
        d2d_render_target->Resize(D2D1::SizeU(width, height));
    }
}

ID2D1PathGeometry* Direct2D::create_monster() {
    using D2D1::Point2F;
    using D2D1::BezierSegment;

    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    d2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

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

ID2D1PathGeometry* Direct2D::create_nose() {
    return nullptr;
}

ID2D1PathGeometry* Direct2D::create_smile() {
    using D2D1::Point2F;
    using D2D1::SizeF;
    using D2D1::ArcSegment;
    
    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    d2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(-40.0f, 75.0f), D2D1_FIGURE_BEGIN_HOLLOW);
    path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(10.0f, 7.0f), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    path_sink->Close();

    return path;
}

ID2D1PathGeometry* Direct2D::create_sad() {
    using D2D1::Point2F;
    using D2D1::SizeF;
    using D2D1::ArcSegment;

    ID2D1PathGeometry* path = nullptr;
    ID2D1GeometrySink* path_sink = nullptr;
    d2d_factory->CreatePathGeometry(&path);
    path->Open(&path_sink);

    path_sink->BeginFigure(Point2F(-40.0f, 75.0f), D2D1_FIGURE_BEGIN_HOLLOW);
    path_sink->AddArc(ArcSegment(Point2F(40.0f, 75.0f), SizeF(8.0f, 3.0f), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

    path_sink->EndFigure(D2D1_FIGURE_END_OPEN);
    path_sink->Close();

    return path;
}

D2D1_ELLIPSE Direct2D::create_ball(const D2D1::Matrix3x2F& transformation, bool left_eye) {
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

HRESULT Direct2D::OnRender() {
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();
    if (SUCCEEDED(hr)) {
        // Deklaracje użycia pomocniczych funkcji
        using D2D1::Point2F;
        using D2D1::BezierSegment;
        using D2D1::RectF;
        using D2D1::Matrix3x2F;
        using D2D1::ColorF;
        using D2D1::Ellipse;

        // - Macierz do połączenia transformacji
        auto transformation = Matrix3x2F::Scale(3.0f, 3.0f) * Matrix3x2F::Translation(d2d_render_target->GetSize().width / 2.0f, d2d_render_target->GetSize().height / 2.0f);
        auto mouth_transformation = Matrix3x2F::Rotation(angle) * transformation;

        auto left_ball = create_ball(transformation, true), right_ball = create_ball(transformation, false);
        auto mouth_path = mouse_down ? create_smile() : create_sad();

        d2d_render_target->BeginDraw();
        d2d_render_target->Clear(background_color);

        d2d_render_target->SetTransform(transformation);
        d2d_render_target->FillGeometry(monster_path, main_rad_brush);
        d2d_render_target->DrawGeometry(monster_path, main_brush);

        auto left_eye = Point2F(-EYE_X_OFFSET, EYE_Y_OFFSET);
        d2d_render_target->FillEllipse(Ellipse(left_eye, EYE_RADIUS, EYE_RADIUS), left_eye_brush);
        auto right_eye = Point2F(EYE_X_OFFSET, EYE_Y_OFFSET);
        d2d_render_target->FillEllipse(Ellipse(right_eye, EYE_RADIUS, EYE_RADIUS), right_eye_brush);

        d2d_render_target->FillEllipse(left_ball, main_brush);
        d2d_render_target->FillEllipse(right_ball, main_brush);

        d2d_render_target->SetTransform(mouth_transformation);
        d2d_render_target->DrawGeometry(mouth_path, main_brush, 3.0f);

        hr = d2d_render_target->EndDraw();

        if (mouth_path) mouth_path->Release();
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        DiscardDeviceResources();
    }

    return hr;
}

LRESULT CALLBACK Direct2D::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;

    if (message == WM_CREATE) {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Direct2D* direct2D = (Direct2D*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(direct2D)
        );

        result = 1;
    }
    else {
        Direct2D* direct2D = reinterpret_cast<Direct2D*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (direct2D) {
            switch (message) {
            case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                direct2D->OnResize(width, height);
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
                direct2D->OnRender();
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
                direct2D->mouse_x = GET_X_LPARAM(lParam);
                direct2D->mouse_y = GET_Y_LPARAM(lParam);
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