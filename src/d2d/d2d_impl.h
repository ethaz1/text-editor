#ifndef DIRECT2D_TEXT_H
#define DIRECT2D_TEXT_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

	BOOL D2DText_Initialize( HWND hwnd );

	void D2DText_Resize( UINT width, UINT height );

	HRESULT D2DText_Render( const WCHAR* text, const UINT32 len, const BOOL cursor, const int cursor_pos );

	void D2DText_OffsetFontSize( float off );

	void D2DText_Shutdown( void );

#ifdef __cplusplus
}
#endif

#endif