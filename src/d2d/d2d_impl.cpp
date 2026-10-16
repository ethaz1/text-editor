#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <cwchar>

#include "d2d_impl.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

static HWND g_hwnd = nullptr;

static ID2D1Factory* g_d2dFactory = nullptr;
static IDWriteFactory* g_writeFactory = nullptr;
static IDWriteTextFormat* g_textFormat = nullptr;

static ID2D1HwndRenderTarget* g_renderTarget = nullptr;
static ID2D1SolidColorBrush* g_text_brush = nullptr;
static ID2D1SolidColorBrush* g_current_line_brush = nullptr;

FontMetrics font_metrics;

static float g_font_size = 24.0f;

// Release resources associated with the render target.
static void ReleaseDeviceResources( )
{
    if ( g_text_brush )
    {
        g_text_brush->Release( );
        g_text_brush = nullptr;
    }

    if ( g_current_line_brush )
    {
        g_current_line_brush->Release( );
        g_current_line_brush = nullptr;
    }

    if ( g_renderTarget )
    {
        g_renderTarget->Release( );
        g_renderTarget = nullptr;
    }
}


// Create the window render target and its brush.
static HRESULT CreateDeviceResources( )
{
    if ( g_renderTarget )
        return S_OK;

    if ( !g_hwnd || !g_d2dFactory )
        return E_UNEXPECTED;

    RECT rc;
    if ( !GetClientRect( g_hwnd, &rc ) )
        return HRESULT_FROM_WIN32( GetLastError( ) );

    UINT width = static_cast< UINT >( rc.right - rc.left );
    UINT height = static_cast< UINT >( rc.bottom - rc.top );

    // Avoid creating a zero-sized render target.
    if ( width == 0 ) width = 1;
    if ( height == 0 ) height = 1;

    HRESULT hr = g_d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties( ),
        D2D1::HwndRenderTargetProperties(
            g_hwnd,
            D2D1::SizeU( width, height )
        ),
        &g_renderTarget
    );

    if ( SUCCEEDED( hr ) )
    {
        hr = g_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF( D2D1::ColorF::Wheat ),
            &g_text_brush
        );

        hr = g_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF( 0.15f, 0.15f, 0.15f ),
            &g_current_line_brush
        );
    }

    if ( FAILED( hr ) )
        ReleaseDeviceResources( );

    return hr;
}


// Initialize Direct2D and DirectWrite.
extern "C" BOOL D2DText_Initialize( HWND hwnd )
{
    if ( !hwnd )
        return FALSE;

    D2DText_Shutdown( );

    g_hwnd = hwnd;

    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &g_d2dFactory
    );

    if ( SUCCEEDED( hr ) )
    {
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof( IDWriteFactory ),
            reinterpret_cast< IUnknown** >( &g_writeFactory )
        );
    }

    if ( SUCCEEDED( hr ) )
        hr = CreateDeviceResources( );

    if ( SUCCEEDED( hr ) )
    {
        D2DText_OffsetFontSize( 0.0 );
    }

    if ( FAILED( hr ) )
    {
        D2DText_Shutdown( );
        return FALSE;
    }

    return TRUE;
}


// Resize the render target when the window changes size.
extern "C" void D2DText_Resize( UINT width, UINT height )
{
    if ( !g_renderTarget || width == 0 || height == 0 )
        return;

    HRESULT hr = g_renderTarget->Resize(
        D2D1::SizeU( width, height )
    );

    if ( FAILED( hr ) )
        ReleaseDeviceResources( );
}


// Render the specified text.
/*
extern "C" HRESULT D2DText_Render( const WCHAR* text, const UINT32 len, const BOOL cursor, const int cursor_pos )
{
    if ( !text || !g_hwnd || !g_textFormat )
        return E_INVALIDARG;

    HRESULT hr = CreateDeviceResources( );

    if ( FAILED( hr ) )
        return hr;

    D2D1_SIZE_F size = g_renderTarget->GetSize( );

    g_renderTarget->BeginDraw( );

    g_renderTarget->SetTransform(
        D2D1::Matrix3x2F::Identity( )
    );

    // Clear the background.
    g_renderTarget->Clear(
        D2D1::ColorF( 0.1f, 0.1f, 0.1f )
    );


    static IDWriteTextLayout* layout = nullptr;
    g_writeFactory->CreateTextLayout(
        text,
        len,
        g_textFormat,
        size.width,
        size.height,
        &layout
    );
    
    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    DWRITE_HIT_TEST_METRICS metrics{};
    
    hr = layout->HitTestTextPosition(
        cursor_pos,
        FALSE,   // Leading edge of this text position
        &x,
        &y,
        &metrics
    );
    
    D2D1_RECT_F line_rect = D2D1::RectF(
        0,
        y,
        size.width,
        y + metrics.height
    );
    
    D2D1_RECT_F cursor_rect = D2D1::RectF(
        x,
        y,
        x + 3.f,
        y + metrics.height //- 2.0f
    );
    
    g_renderTarget->FillRectangle( &line_rect, g_current_line_brush );
    
    if (cursor)
        g_renderTarget->FillRectangle( &cursor_rect, g_text_brush );
    
    layout->Release( );

    // Draw white text with a 20-DIP margin.
    g_renderTarget->DrawText(
            text,
            len,
            g_textFormat,
            D2D1::RectF(
                0,
                0,
                size.width,
                size.height
            ),
            g_text_brush,
            D2D1_DRAW_TEXT_OPTIONS_NONE,
            DWRITE_MEASURING_MODE_NATURAL
        );

    hr = g_renderTarget->EndDraw( );

    // The underlying graphics device may need its
    // render target and brush recreated.
    if ( hr == D2DERR_RECREATE_TARGET )
    {
        ReleaseDeviceResources( );
        return S_OK;
    }

    return hr;
}
*/

const FontMetrics* D2DText_GetFontMetrics( )
{
    return &font_metrics;
}

HRESULT D2DText_Render( TextEdit* t, RECTF rect )
{
   HRESULT hr = CreateDeviceResources( );

    if ( FAILED( hr ) )
        return hr;

    //D2D1_SIZE_F size = g_renderTarget->GetSize( );
    D2D1_SIZE_F size = D2D1::SizeF(
        rect.right - rect.left,
        rect.bottom - rect.top
    );

    g_renderTarget->BeginDraw( );

    // Clear the background.
    g_renderTarget->Clear(
        D2D1::ColorF( 0.1f, 0.1f, 0.1f )
    );

    g_renderTarget->SetTransform(
        D2D1::Matrix3x2F::Translation( rect.left, rect.top )
    );



    g_renderTarget->PushAxisAlignedClip( D2D1::RectF(
        0,
        0,
        size.width / font_metrics.dpiScale,
        size.height / font_metrics.dpiScale
    ), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE );

 

    UINT32 len = t->last == -1 ? 0 : t->last + 1;

    static IDWriteTextLayout* layout = nullptr;
    g_writeFactory->CreateTextLayout(
        (const WCHAR*)t->buf,
        len,
        g_textFormat,
        size.width,
        size.height,
        &layout
    );

    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    DWRITE_HIT_TEST_METRICS metrics{};

    hr = layout->HitTestTextPosition(
        t->cur,
        FALSE,   // Leading edge of this text position
        &x,
        &y,
        &metrics
    );

    D2D1_RECT_F line_rect = D2D1::RectF(
        0,
        y,
        size.width,
        y + metrics.height
    );

    D2D1_RECT_F cursor_rect = D2D1::RectF(
        x,
        y,
        x + 3.f,
        y + metrics.height //- 2.0f
    );

    g_renderTarget->FillRectangle( &line_rect, g_current_line_brush );

    if ( t->curFlash )
        g_renderTarget->FillRectangle( &cursor_rect, g_text_brush );

    layout->Release( );

    // Draw white text with a 20-DIP margin.
    g_renderTarget->DrawText(
        (const WCHAR*)t->buf,
        len,
        g_textFormat,
        D2D1::RectF(
            0,
            0,
            rect.right,
            rect.bottom
        ),
        g_text_brush,
        D2D1_DRAW_TEXT_OPTIONS_CLIP,
        DWRITE_MEASURING_MODE_NATURAL
    );


    g_renderTarget->PopAxisAlignedClip( );
    hr = g_renderTarget->EndDraw( );


    // The underlying graphics device may need its
    // render target and brush recreated.
    if ( hr == D2DERR_RECREATE_TARGET )
    {
        ReleaseDeviceResources( );
        return S_OK;
    }

    return hr;
}

// make the font or resize it.
void D2DText_OffsetFontSize( float off )
{
    g_font_size += off;
    
    if ( g_textFormat )
        g_textFormat->Release( );

    g_writeFactory->CreateTextFormat(
        L"Courier New",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        g_font_size,
        L"en-us",
        &g_textFormat
    );

    g_textFormat->SetWordWrapping( DWRITE_WORD_WRAPPING_NO_WRAP );

    // get font metrics (ones that aren't context specific; global to the font, like font height).
    D2D1_SIZE_F size = g_renderTarget->GetSize( );

    static IDWriteTextLayout* layout = nullptr;
    g_writeFactory->CreateTextLayout(
        L"",
        0,
        g_textFormat,
        size.width,
        size.height,
        &layout
    );

    FLOAT x = 0.0f;
    FLOAT y = 0.0f;
    DWRITE_HIT_TEST_METRICS metrics{};

    layout->HitTestTextPosition(
        0,
        FALSE,   // Leading edge of this text position
        &x,
        &y,
        &metrics
    );

    font_metrics.lineHeight = metrics.height;
    font_metrics.dpi = GetDpiForWindow( g_renderTarget->GetHwnd( ) );
    font_metrics.dpiScale = font_metrics.dpi / (float)USER_DEFAULT_SCREEN_DPI;
}

// Release all resources.
extern "C" void D2DText_Shutdown( void )
{
    ReleaseDeviceResources( );

    if ( g_textFormat )
    {
        g_textFormat->Release( );
        g_textFormat = nullptr;
    }

    if ( g_writeFactory )
    {
        g_writeFactory->Release( );
        g_writeFactory = nullptr;
    }

    if ( g_d2dFactory )
    {
        g_d2dFactory->Release( );
        g_d2dFactory = nullptr;
    }

    g_hwnd = nullptr;
}