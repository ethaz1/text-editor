#include "status.h"
#include "edit.h"

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static StatusMode current_mode = STATUS_MODE_NORMAL;
static TextEdit te;

#define NORMAL_TEXT		L"normal mode"
#define SAVE_TEXT		L"Save file to disk:\n"
#define OPEN_TEXT		L"Open file:\nc:/users/ewant/"

void on_normal( ) {
	current_mode = STATUS_MODE_NORMAL;

	// insert into stats
	int len = wcslen( NORMAL_TEXT );
	memcpy( te.buf, NORMAL_TEXT, len * sizeof( uint16_t ) );
	te.last = len;
	te.cur = te.last + 1;
}

void on_save( void )
{
	wchar_t cwd[512];

	DWORD cwd_len = GetCurrentDirectoryW(
		( DWORD ) ( sizeof( cwd ) / sizeof( cwd[0] ) ),
		cwd
	);

	// Failed, or the buffer was too small.
	if ( cwd_len == 0 ||
		cwd_len >= sizeof( cwd ) / sizeof( cwd[0] ) )
	{
		return;
	}

	size_t prefix_len = wcslen( SAVE_TEXT );
	size_t path_len = ( size_t ) cwd_len;

	// Add a trailing slash only if one isn't already present.
	int need_separator =
		path_len > 0 &&
		cwd[path_len - 1] != L'/' &&
		cwd[path_len - 1] != L'\\';

	size_t total_len = prefix_len + path_len + need_separator;

	// Reserve one slot for the null terminator.
	if ( te.buf == NULL || total_len >= te.size )
		return;

	// Copy the status prefix.
	for ( size_t i = 0; i < prefix_len; ++i )
		te.buf[i] = ( uint16_t ) SAVE_TEXT[i];

	// Copy the working directory, normalizing separators.
	for ( size_t i = 0; i < path_len; ++i )
	{
		wchar_t ch = cwd[i];

		te.buf[prefix_len + i] =
			( uint16_t ) ( ch == L'\\' ? L'/' : ch );
	}

	// Append the trailing slash if required.
	if ( need_separator )
		te.buf[prefix_len + path_len] = ( uint16_t ) L'/';

	// Null-terminate the text.
	te.buf[total_len] = 0;

	// Update TextEdit state.
	te.last = total_len > 0 ? ( int ) total_len - 1 : -1;
	te.cur = ( int ) total_len;
	te.first = 0;

	current_mode = STATUS_MODE_SAVE;
}

void on_open( ) {
	current_mode = STATUS_MODE_OPEN;

	// insert into stats
	int len = wcslen( OPEN_TEXT );
	memcpy( te.buf, OPEN_TEXT, len * sizeof( uint16_t ) );
	te.last = len;
	te.cur = te.last + 1;
}

TextEdit* StatusBar_Initialise( ) {
	TextEdit_Initialise( &te );

	on_normal( );

	return &te;
}

void StatusBar_ToggleFocus( ) {
	te.inFocus = !te.inFocus;
}

void StatusBar_HandleEvent( StatusEvent e, TextEdit *other ) {

	if ( e == STATUS_EVENT_RESET ) {
		on_normal( );
		return;
	}

	switch ( current_mode ) {
		case STATUS_MODE_NORMAL: {
			if ( e == STATUS_EVENT_SAVE ) on_save( );
			else if ( e == STATUS_EVENT_OPEN ) on_open( );
			break;
		}
		case STATUS_MODE_SAVE: {
			if ( e == STATUS_EVENT_SUBMIT ) {
				int off = wcslen( SAVE_TEXT );	// TODO: readonly status bar text
				int len = te.last - off;
				assert( len > 0 );
				uint16_t path[512];
				memset( path, 0, 512 );
				memcpy( path, te.buf + off, len * sizeof( uint16_t ) );
				
				OutputDebugStringW( path );

				if ( TextEdit_SaveContents( other, path ) != TE_SAVE_SUCCESS ) {
					MessageBoxA( NULL, "Failed to save file!", "text-editor", MB_ICONERROR );
				}

				StatusBar_HandleEvent( STATUS_EVENT_RESET, other );
				break;
			}
			else if ( e == STATUS_EVENT_RESET ) {
				current_mode = STATUS_MODE_NORMAL;
				on_normal( );
			}
			break;
		}
		default: break;
	}

}

void StatusBar_CleanUp( ) {
	TextEdit_CleanUp( &te );
}