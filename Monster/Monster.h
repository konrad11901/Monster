#pragma once

#include "framework.h"
#include "Timer.h"
#include <d2d1_3.h>
#include <winrt/base.h>
#include <d3d11_4.h>

class Monster {
public:
    Monster();

    void InitializeWindow(HINSTANCE instance, INT cmd_show);
    void RunMessageLoop();

private:
    // Window handle.
    HWND hwnd;

    // Direct3D objects.
    winrt::com_ptr<ID3D11Device5> d3d_device;
    winrt::com_ptr<ID3D11DeviceContext4> d3d_context;
    winrt::com_ptr<IDXGISwapChain4> swap_chain;

    // Direct2D objects.
    winrt::com_ptr<ID2D1Factory7> d2d_factory;
    winrt::com_ptr<ID2D1Device6> d2d_device;
    winrt::com_ptr<ID2D1DeviceContext6> d2d_context;
    winrt::com_ptr<ID2D1Bitmap1> d2d_target_bitmap;

    winrt::com_ptr<ID2D1SolidColorBrush> main_brush;
    winrt::com_ptr<ID2D1RadialGradientBrush> main_rad_brush;
    winrt::com_ptr<ID2D1RadialGradientBrush> left_eye_brush;
    winrt::com_ptr<ID2D1RadialGradientBrush> right_eye_brush;
    winrt::com_ptr<ID2D1PathGeometry> monster_path;
    winrt::com_ptr<ID2D1PathGeometry> nose_path;
    Timer timer;

    D2D1::Matrix3x2F transformation;
    D2D1_POINT_2F center;

    FLOAT angle = 0.0f;
    INT mouse_x = 0, mouse_y = 0;
    bool mouse_down = false;

    static constexpr FLOAT EYE_X_OFFSET = 43.0f;
    static constexpr FLOAT EYE_Y_OFFSET = -12.0f;
    static constexpr FLOAT EYE_RADIUS = 34.0f;
    static constexpr FLOAT EYE_BALL_RADIUS = 8.0f;

    static constexpr D2D1_COLOR_F clear_color =
    { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
    static constexpr D2D1_COLOR_F background_color =
    { .r = 0.8f, .g = 0.76f, .b = 0.89f, .a = 1.0f };
    static constexpr D2D1_COLOR_F brush_color =
    { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f };
    static constexpr D2D1_COLOR_F nose_color =
    { .r = 0.4f, .g = 0.4f, .b = 0.4f, .a = 1.0f };

    static constexpr D2D1_GRADIENT_STOP rad_stops_data[] = {
        {.position = 0.0f, .color = {.r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f } },
        {.position = 0.6f, .color = {.r = 0.0f, .g = 0.7f, .b = 0.0f, .a = 1.0f } },
        {.position = 1.0f, .color = {.r = 0.0f, .g = 0.3f, .b = 0.0f, .a = 1.0f } }
    };
    static constexpr D2D1_GRADIENT_STOP eye_stops_data[] = {
        {.position = 0.9f, .color = {.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f } },
        {.position = 1.0f, .color = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f } },
    };

    void CreateDeviceDependentResources();
    void CreateDeviceIndependentResources();
    void CreateWindowSizeDependentResources();
    void HandleDeviceLost();

    void OnRender();
    void OnResize(UINT width, UINT height);

    winrt::com_ptr<ID2D1PathGeometry> CreateMonster();
    winrt::com_ptr<ID2D1PathGeometry> CreateNose();
    winrt::com_ptr<ID2D1PathGeometry> CreateSmile();
    winrt::com_ptr<ID2D1PathGeometry> CreateSad();
    D2D1_ELLIPSE CreateBall(bool left_eye);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};