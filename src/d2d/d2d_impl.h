#ifndef DIRECT2D_TEXT_H
#define DIRECT2D_TEXT_H
#include <windows.h>
#include "../edit.h"


#ifdef __cplusplus
extern "C" {
#endif

	BOOL D2DText_Initialize( HWND hwnd );

	void D2DText_Resize( UINT width, UINT height );

	HRESULT D2DText_Render( TextEdit *t );

	void D2DText_OffsetFontSize( float off );

	void D2DText_Shutdown( void );

#ifdef __cplusplus
}
#endif

#endif