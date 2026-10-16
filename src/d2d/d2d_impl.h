#ifndef DIRECT2D_TEXT_H
#define DIRECT2D_TEXT_H
#include <windows.h>
#include "../edit.h"


typedef struct FontMetrics_t {
	float lineHeight;
	float dpi;
	float dpiScale;
} FontMetrics;

typedef struct RECTF_t {
	float left, top, bottom, right;
} RECTF;

#ifdef __cplusplus
extern "C" {
#endif

	BOOL D2DText_Initialize( HWND hwnd );

	void D2DText_Resize( UINT width, UINT height );

	void D2DText_StartRender( );
	void D2DText_Render( TextEdit *t, RECTF rect );
	void D2DText_FinishRender( );

	const FontMetrics* D2DText_GetFontMetrics( );

	void D2DText_OffsetFontSize( float off );

	void D2DText_Shutdown( void );

#ifdef __cplusplus
}
#endif

#endif