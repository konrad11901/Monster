#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d2d1_3.h>
#include "Timer.h"

class Direct2D {
public:
	Direct2D();
	~Direct2D();

	HRESULT Initialize(HINSTANCE instance, INT cmd_show);
	void RunMessageLoop();
private:
	HWND hwnd;
	ID2D1Factory7* d2d_factory;
	ID2D1HwndRenderTarget* d2d_render_target;
	ID2D1SolidColorBrush* main_brush;
	ID2D1RadialGradientBrush* main_rad_brush;
	ID2D1RadialGradientBrush* left_eye_brush;
	ID2D1RadialGradientBrush* right_eye_brush;
	ID2D1PathGeometry* monster_path;
	ID2D1PathGeometry* nose_path;
	Timer timer;

	FLOAT angle = 0.0f;
	INT mouse_x = 0, mouse_y = 0;
	bool mouse_down = false;

	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();

	// Draw content.
	HRESULT OnRender();

	// Resize the render target.
	void OnResize(
		UINT width,
		UINT height
	);

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	ID2D1PathGeometry* create_monster();
	ID2D1PathGeometry* create_nose();
	ID2D1PathGeometry* create_smile();
	ID2D1PathGeometry* create_sad();

	D2D1_ELLIPSE create_ball(const D2D1::Matrix3x2F& transformation, bool left_eye);

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
};

INT WINAPI wWinMain(_In_ HINSTANCE instance,
	_In_opt_ HINSTANCE prev_instance,
	_In_ PWSTR cmd_line,
	_In_ INT cmd_show);
