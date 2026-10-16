
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>		// dark mode

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#include <stdlib.h>
#include <assert.h>

#include "d2d/d2d_impl.h"
#include "edit.h"

#define ID_TIMER_CUSOR_FLASH 10001
#define INPUT_SIZE 4096

const int CURSOR_FLASH_INTERVAL_MS = 500;

TextEdit te;



LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam ) {
	
	switch ( msg ) {
		case WM_TIMER: {
			switch ( wparam ) {
				case ID_TIMER_CUSOR_FLASH: {
					te.curFlash = !te.curFlash;
					InvalidateRect( hwnd, NULL, TRUE );
					break;
				}
			}
			return 0;
		}
		case WM_KEYDOWN: {
			switch ( wparam ) {
				case VK_LEFT: {
					if ( te.cur > 0 )
						te.cur--;
					te.preferredColumnValid = false;
					break;
				}
				case VK_RIGHT: {
					if ( te.cur <= te.last )
						te.cur++;
					te.preferredColumnValid = false;
					break;
				}
				case VK_UP: {
					TextEdit_MoveVertical( &te, -1 );
					break;
				}
				case VK_DOWN: {
					TextEdit_MoveVertical( &te, 1 );
					break;
				}
				case VK_DELETE: {
					TextEdit_DeleteChar( &te, te.cur, false );
					break;
				}
				//default: return 0;
			}

			SetTimer( hwnd, ID_TIMER_CUSOR_FLASH, CURSOR_FLASH_INTERVAL_MS, NULL );
			te.curFlash = TRUE;
			InvalidateRect( hwnd, NULL, TRUE );
			return 0;
		}
		case WM_CHAR: {
			TextEdit_HandleCharCode( &te, wparam );
			return 0;
		}
		case WM_MOUSEWHEEL: {
			// resize the text with ctrl+mwheel
			if ( LOWORD( wparam ) == MK_CONTROL ) {
				float delta = GET_WHEEL_DELTA_WPARAM( wparam ) > 0 ? 1.0f : -1.0f;
				D2DText_OffsetFontSize( delta );
			}
			return 0;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hwnd, &ps );
			D2DText_Render( &te );
			EndPaint( hwnd, &ps );
			return 0;
		}
		case WM_SIZE: {
			D2DText_Resize( LOWORD( lparam ), HIWORD( lparam ) );
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage( 0 );
			return 0;
		}
		default: return DefWindowProc( hwnd, msg, wparam, lparam );
	}
}

int __stdcall wWinMain( HINSTANCE inst, HINSTANCE prev_inst, LPWSTR cmd_args, int cmd_show ) {

	TextEdit_Initialise( &te );

	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof( wcex );
	wcex.hInstance = inst;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = L"text-editor";

	if ( !RegisterClassEx( &wcex ) )
		return 1;

	HWND hwnd = CreateWindowEx(
		0,
		L"text-editor",
		L"text-editor",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 720,
		NULL,
		NULL,
		inst,
		0
	);

	if ( hwnd == NULL )
		return 1;

	if ( !D2DText_Initialize( hwnd ) )
		return 1;

	SetTimer( hwnd, ID_TIMER_CUSOR_FLASH, CURSOR_FLASH_INTERVAL_MS, NULL );

	BOOL value = TRUE;
	DwmSetWindowAttribute( hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof( value ) );

	ShowWindow( hwnd, cmd_show );

	MSG msg = { 0 };
	while ( GetMessage( &msg, NULL, 0, 0 ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	KillTimer( hwnd, ID_TIMER_CUSOR_FLASH );
	D2DText_Shutdown( );
	TextEdit_CleanUp( &te );
	return 0;
}