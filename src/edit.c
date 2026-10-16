#include "edit.h"
#include "text_file.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define INPUT_SIZE 1'000'000

#define KEY_BACKSPACE	0x08
#define KEY_ENTER		0x0D
#define KEY_ESCAPE		0x1B
#define KEY_TAB			0x09
#define KEY_SAVE        0x13

void TextEdit_Initialise( TextEdit *t ) {
	t->size = INPUT_SIZE;
	t->cur = 0;
	t->last = -1;
    t->first = 0;
	t->buf = calloc( INPUT_SIZE, sizeof( WCHAR ) );
    t->preferredColumn = 0;
    t->preferredColumnValid = false;
    t->curFlash = true;
    t->inFocus = false;

	if ( t->buf == NULL ) {
		MessageBoxA( NULL, "Out of memory.", "Error", MB_ICONERROR );
		abort( );
	}
}

void TextEdit_HandleCharCode( TextEdit* t, uint64_t key ) {
	switch ( key ) {
		case KEY_BACKSPACE: {
            TextEdit_DeleteChar( t, t->cur, true );
			break;
		}
		case KEY_ENTER: {
			TextEdit_InsertChar( t, '\n', t->cur );
			break;
		}
        case KEY_SAVE: {
            TextEdit_SaveContents( t, L"test.c" );
            break;
        }
        case KEY_TAB: {
            TextEdit_InsertChar( t, ( uint16_t ) key, t->cur );     // TODO: allow user to do 4 spaces instead
            break;
        }
		default: {
            // drop all other special control keys
            if ( key < 32 && key != '\b' && key != '\r' )
                return 0;
			TextEdit_InsertChar( t, (uint16_t)key, t->cur );
			break;
		}
	}



}

void TextEdit_DeleteChar( TextEdit* t, int pos, bool backspace )
{
    assert( t != NULL );
    assert( pos >= 0 );

    if ( t == NULL || pos < 0 )
        return;

    int length = t->last + 1;

    assert( pos <= length );

    if ( pos > length )
        return;

    int deletePos;

    if ( backspace )
    {
        if ( pos == 0 )
            return; // Nothing to delete before the beginning.

        deletePos = pos - 1;
    }
    else
    {
        if ( pos >= length )
            return; // Nothing to delete at or after the end.

        deletePos = pos;
    }

    // Shift everything after the deleted character left.
    int count = length - deletePos - 1;

    if ( count > 0 )
    {
        memmove(
            &t->buf[deletePos],
            &t->buf[deletePos + 1],
            ( size_t ) count * sizeof( t->buf[0] )
        );
    }

    t->last--;

    // Keep the cursor position consistent with the deletion.
    if ( t->cur > deletePos )
        t->cur--;

    t->preferredColumnValid = false;
}

void TextEdit_MoveVertical( TextEdit* t, int direction )
{
    assert( t != NULL );
    assert( direction == -1 || direction == 1 );

    if ( t == NULL || ( direction != -1 && direction != 1 ) )
        return;

    int length = t->last + 1;

    if ( length <= 0 )
        return;

    if ( t->cur < 0 )
        t->cur = 0;
    if ( t->cur > length )
        t->cur = length;

    // Find the current logical line's boundaries.
    int lineStart = t->cur;
    while ( lineStart > 0 && t->buf[lineStart - 1] != '\n' )
        lineStart--;

    int lineEnd = t->cur;
    while ( lineEnd < length && t->buf[lineEnd] != '\n' )
        lineEnd++;

    // Remember the intended column on the first vertical move.
    if ( !t->preferredColumnValid )
    {
        t->preferredColumn = t->cur - lineStart;
        t->preferredColumnValid = true;
    }

    int column = t->preferredColumn;

    if ( direction < 0 )
    {
        // Already on the first line.
        if ( lineStart == 0 )
            return;

        int previousEnd = lineStart - 1; // Previous '\n'.
        int previousStart = previousEnd;

        while ( previousStart > 0 &&
            t->buf[previousStart - 1] != '\n' )
        {
            previousStart--;
        }

        int previousLength = previousEnd - previousStart;

        if ( column > previousLength )
            column = previousLength;

        t->cur = previousStart + column;
    }
    else
    {
        // Already on the last line.
        if ( lineEnd >= length )
            return;

        int nextStart = lineEnd + 1;
        int nextEnd = nextStart;

        while ( nextEnd < length && t->buf[nextEnd] != '\n' )
            nextEnd++;

        int nextLength = nextEnd - nextStart;

        if ( column > nextLength )
            column = nextLength;

        t->cur = nextStart + column;
    }
}

void TextEdit_InsertChar( TextEdit *t, uint16_t ch, int pos ) {
    assert( t != NULL );
    assert( pos >= 0 );

    if ( t == NULL || pos < 0 )
        return;

    // Valid insertion positions are [0, length].
    int length = t->last + 1;

    assert( pos <= length );

    if ( pos > length )
        return;

    // Inserting requires one additional slot, even when
    // inserting somewhere in the middle of the buffer.
    assert( ( unsigned int ) length < t->size );

    if ( ( unsigned int ) length >= t->size )
        return;

    // Shift existing characters right by one.
    memmove(
        &t->buf[pos + 1],
        &t->buf[pos],
        ( size_t ) ( length - pos ) * sizeof( t->buf[0] )
    );

    t->buf[pos] = ch;
    t->last++;

    // Assumes the cursor is a position between characters.
    if ( pos <= t->cur )
        t->cur++;

    t->preferredColumnValid = false;

    // TESTING
    if ( ch == '{' ) {
        TextEdit_InsertChar( t, '}', pos + 1 );
        t->cur--;
    }

}

void TextEdit_SaveContents( TextEdit* t, uint16_t *path ) {
    // UTF-16 to UTF-8
    int bytes = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        t->buf,
        t->last+1,
        NULL,
        0,
        NULL,
        NULL
    );

    char* utf8_str = malloc( bytes + 1 );
    if ( utf8_str == NULL ) return;

    int utf8_len = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        t->buf,
        t->last+1,
        utf8_str,
        bytes,
        NULL,
        NULL
    );

    assert( utf8_len == bytes );

    HANDLE file = CreateFile(
        path,
        GENERIC_WRITE,
        NULL,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if ( file == INVALID_HANDLE_VALUE ) {
        assert( 0 );    // TODO: handle bad file
    }

    DWORD written_to_file = 0;

    BOOL success = WriteFile(
        file,
        utf8_str,
        utf8_len,
        &written_to_file,
        NULL
    );

    if ( success == FALSE ) {
        assert( 0 );    // TODO: handle file write fail.
    }

    CloseHandle( file );
    free( utf8_str );
}

void TextEdit_OpenFile( TextEdit* t, uint16_t* path ) {
    size_t len = 0;
    char* data = TextFile_ReadUtf16( path, &len );
    if ( data != NULL ) {
        memset( t->buf, 0, t->size );
        memcpy( t->buf, data, len );
        t->last = len;
        free( data );
    }
}


void TextEdit_Scroll( TextEdit* t, float delta )
{
    assert( t != NULL );

    if ( t == NULL || delta == 0.0f )
        return;

    int length = t->last + 1;
    int oldFirst = t->first;

    if ( length <= 0 )
    {
        t->first = 0;
        return;
    }

    // Keep the scroll position within the buffer.
    if ( t->first < 0 )
        t->first = 0;

    if ( t->first > length )
        t->first = length;

    if ( delta > 0.0f )
    {
        // Find the beginning of the current line.
        int i = t->first;

        while ( i > 0 && t->buf[i - 1] != '\n' )
            i--;

        // Already on the first line.
        if ( i == 0 )
        {
            t->first = 0;
            return;
        }

        // Step over the newline preceding the current line.
        i--;

        // Find the beginning of the previous line.
        while ( i > 0 && t->buf[i - 1] != '\n' )
            i--;

        t->first = i;
    }
    else
    {
        // Find the next newline.
        int i = t->first;

        while ( i < length && t->buf[i] != '\n' )
            i++;

        // Move to the character after the newline.
        if ( i < length )
            t->first = i + 1;
    }

    if ( t->first != oldFirst )
    {
        //TextEdit_MoveVertical( t, delta > 0.0f ? -1 : 1 );
    }

}

void TextEdit_CleanUp( TextEdit *t ) {
	if ( t->buf != NULL ) free( t->buf );
}