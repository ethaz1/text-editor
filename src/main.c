
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>		// dark mode

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "d2d/d2d_impl.h"
#include "edit.h"

#define ID_TIMER_CUSOR_FLASH 10001
#define INPUT_SIZE 1'000'000
#define STATUS_BAR_EXAMPLE_TEXT L"Save files to disk:\nc:/users/ewant/"

const int CURSOR_FLASH_INTERVAL_MS = 500;

TextEdit te, stats;
RECTF main_edit_rect, status_bar_rect;

TextEdit* te_in_focus = &te;



LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam ) {
	
	switch ( msg ) {
		case WM_TIMER: {
			switch ( wparam ) {
				case ID_TIMER_CUSOR_FLASH: {
					te_in_focus->curFlash = !te_in_focus->curFlash;
					InvalidateRect( hwnd, NULL, TRUE );
					break;
				}
			}
			return 0;
		}
		case WM_KEYDOWN: {
			switch ( wparam ) {
				case VK_LEFT: {
					if ( te_in_focus->cur > 0 )
						te_in_focus->cur--;
					te_in_focus->preferredColumnValid = false;
					break;
				}
				case VK_RIGHT: {
					if ( te_in_focus->cur <= te_in_focus->last )
						te_in_focus->cur++;
					te_in_focus->preferredColumnValid = false;
					break;
				}
				case VK_UP: {
					TextEdit_MoveVertical( te_in_focus, -1 );
					break;
				}
				case VK_DOWN: {
					TextEdit_MoveVertical( te_in_focus, 1 );
					break;
				}
				case VK_DELETE: {
					TextEdit_DeleteChar( te_in_focus, te_in_focus->cur, false );
					break;
				}
				case 192: {	// tilde
					if ( GetKeyState( VK_CONTROL ) & 0x8000 ) {
						// switch between statusbar and main edit
						te_in_focus->inFocus = !te_in_focus->inFocus;
						stats.inFocus = !stats.inFocus;
						te_in_focus = te_in_focus->inFocus ? &te : &stats;
					}
					break;
				}
				case 'O': {
					if ( GetKeyState( VK_CONTROL ) & 0x8000 ) {
						TextEdit_OpenFile( &te, L"edit_copy.c" );
					}
					break;
				}
				//default: return 0;
			}


			SetTimer( hwnd, ID_TIMER_CUSOR_FLASH, CURSOR_FLASH_INTERVAL_MS, NULL );
			te_in_focus->curFlash = TRUE;
			InvalidateRect( hwnd, NULL, TRUE );
			return 0;
		}
		case WM_CHAR: {
			TextEdit_HandleCharCode( te_in_focus, wparam );
			return 0;
		}
		case WM_MOUSEWHEEL: {
			// resize the text with ctrl+mwheel
			if ( LOWORD( wparam ) == MK_CONTROL ) {
				float delta = GET_WHEEL_DELTA_WPARAM( wparam ) > 0 ? 1.0f : -1.0f;
				D2DText_OffsetFontSize( delta );
			}
			else {
				TextEdit_Scroll( te_in_focus, ( float ) GET_WHEEL_DELTA_WPARAM( wparam ) / 120.0f );
				TextEdit_Scroll( te_in_focus, ( float ) GET_WHEEL_DELTA_WPARAM( wparam ) / 120.0f );
				TextEdit_Scroll( te_in_focus, ( float ) GET_WHEEL_DELTA_WPARAM( wparam ) / 120.0f );
				InvalidateRect( hwnd, NULL, TRUE );
			}
			return 0;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hwnd, &ps );

			D2DText_StartRender( );

			D2DText_Render( &te, main_edit_rect );
			D2DText_Render( &stats, status_bar_rect );
			
			D2DText_FinishRender( );

			EndPaint( hwnd, &ps );
			return 0;
		}
		case WM_SIZE: {
			int width = LOWORD( lparam );
			int height = HIWORD( lparam );

			D2DText_Resize( width, height );
			
			main_edit_rect.top = 0;
			main_edit_rect.left = 0;
			main_edit_rect.bottom = (float)height - ceilf(D2DText_GetFontMetrics( )->lineHeight * 2.0f);
			main_edit_rect.right = (float)width;

			status_bar_rect.top = main_edit_rect.bottom;
			status_bar_rect.left = 0.0f;
			status_bar_rect.bottom = ( float ) height;
			status_bar_rect.right = ( float ) width;

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
	te_in_focus->inFocus = true;

	TextEdit_Initialise( &stats );

	// insert into stats
	int size = wcslen( STATUS_BAR_EXAMPLE_TEXT ) * sizeof( uint16_t );
	memcpy( stats.buf, STATUS_BAR_EXAMPLE_TEXT, size);
	stats.last = size;
	stats.cur = stats.last + 1;
	

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

	// dark mode titlebar
	{
		BOOL value = TRUE;
		DwmSetWindowAttribute( hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof( value ) );
	}

	ShowWindow( hwnd, cmd_show );

	MSG msg = { 0 };
	while ( GetMessage( &msg, NULL, 0, 0 ) ) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	KillTimer( hwnd, ID_TIMER_CUSOR_FLASH );
	D2DText_Shutdown( );
	TextEdit_CleanUp( &te );
	TextEdit_CleanUp( &stats );
	return 0;
}