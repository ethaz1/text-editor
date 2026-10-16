#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "text_file.h"

static uint16_t* MakeEmpty( size_t* outLength )
{
    uint16_t* result = malloc( sizeof( uint16_t ) );

    if ( !result )
        return NULL;

    result[0] = 0;

    if ( outLength )
        *outLength = 0;

    return result;
}


// Convert a multibyte encoding to UTF-16.
static uint16_t* ConvertToUtf16(
    const unsigned char* bytes,
    size_t byteCount,
    UINT codePage,
    DWORD flags,
    size_t* outLength )
{
    if ( byteCount == 0 )
        return MakeEmpty( outLength );

    if ( byteCount > INT_MAX )
        return NULL;

    int required = MultiByteToWideChar(
        codePage,
        flags,
        ( LPCCH ) bytes,
        ( int ) byteCount,
        NULL,
        0
    );

    if ( required <= 0 )
        return NULL;

    uint16_t* result =
        malloc( ( ( size_t ) required + 1 ) * sizeof( uint16_t ) );

    if ( !result )
        return NULL;

    int written = MultiByteToWideChar(
        codePage,
        flags,
        ( LPCCH ) bytes,
        ( int ) byteCount,
        ( LPWSTR ) result,
        required
    );

    if ( written != required )
    {
        free( result );
        return NULL;
    }

    result[written] = 0;

    if ( outLength )
        *outLength = ( size_t ) written;

    return result;
}


// Decode UTF-16 little-endian or big-endian bytes.
static uint16_t* DecodeUtf16(
    const unsigned char* bytes,
    size_t byteCount,
    int bigEndian,
    size_t* outLength )
{
    if ( byteCount % 2 != 0 )
        return NULL;

    size_t count = byteCount / 2;

    uint16_t* result =
        malloc( ( count + 1 ) * sizeof( uint16_t ) );

    if ( !result )
        return NULL;

    for ( size_t i = 0; i < count; i++ )
    {
        unsigned char a = bytes[i * 2];
        unsigned char b = bytes[i * 2 + 1];

        result[i] = bigEndian
            ? ( uint16_t ) ( ( ( uint16_t ) a << 8 ) | b )
            : ( uint16_t ) ( a | ( ( uint16_t ) b << 8 ) );
    }

    result[count] = 0;

    if ( outLength )
        *outLength = count;

    return result;
}


// Decode UTF-32 into UTF-16, handling supplementary characters.
static uint16_t* DecodeUtf32(
    const unsigned char* bytes,
    size_t byteCount,
    int bigEndian,
    size_t* outLength )
{
    if ( byteCount % 4 != 0 )
        return NULL;

    size_t count = byteCount / 4;

    uint16_t* result =
        malloc( ( count * 2 + 1 ) * sizeof( uint16_t ) );

    if ( !result )
        return NULL;

    size_t written = 0;

    for ( size_t i = 0; i < count; i++ )
    {
        const unsigned char* p = &bytes[i * 4];

        uint32_t cp;

        if ( bigEndian )
        {
            cp = ( ( uint32_t ) p[0] << 24 )
                | ( ( uint32_t ) p[1] << 16 )
                | ( ( uint32_t ) p[2] << 8 )
                | ( uint32_t ) p[3];
        }
        else
        {
            cp = ( ( uint32_t ) p[3] << 24 )
                | ( ( uint32_t ) p[2] << 16 )
                | ( ( uint32_t ) p[1] << 8 )
                | ( uint32_t ) p[0];
        }

        // Reject invalid Unicode scalar values.
        if ( cp > 0x10FFFF ||
            ( cp >= 0xD800 && cp <= 0xDFFF ) )
        {
            free( result );
            return NULL;
        }

        if ( cp <= 0xFFFF )
        {
            result[written++] = ( uint16_t ) cp;
        }
        else
        {
            cp -= 0x10000;

            result[written++] =
                ( uint16_t ) ( 0xD800 + ( cp >> 10 ) );

            result[written++] =
                ( uint16_t ) ( 0xDC00 + ( cp & 0x3FF ) );
        }
    }

    result[written] = 0;

    if ( outLength )
        *outLength = written;

    return result;
}


// Heuristic for BOM-less UTF-16 text that mostly contains ASCII.
static int GuessUtf16(
    const unsigned char* bytes,
    size_t byteCount,
    int* bigEndian )
{
    if ( byteCount < 4 || byteCount % 2 != 0 )
        return 0;

    size_t pairs = byteCount / 2;
    size_t evenZeros = 0;
    size_t oddZeros = 0;

    for ( size_t i = 0; i < byteCount; i++ )
    {
        if ( bytes[i] == 0 )
        {
            if ( i % 2 == 0 )
                evenZeros++;
            else
                oddZeros++;
        }
    }

    // UTF-16LE: the high byte is often zero.
    if ( oddZeros * 2 >= pairs &&
        evenZeros * 10 < pairs )
    {
        *bigEndian = 0;
        return 1;
    }

    // UTF-16BE: the high byte is often zero at even offsets.
    if ( evenZeros * 2 >= pairs &&
        oddZeros * 10 < pairs )
    {
        *bigEndian = 1;
        return 1;
    }

    return 0;
}


uint16_t* TextFile_ReadUtf16(
    const wchar_t* path,
    size_t* outLength )
{
    if ( outLength )
        *outLength = 0;

    if ( !path )
        return NULL;

    HANDLE file = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if ( file == INVALID_HANDLE_VALUE )
        return NULL;

    LARGE_INTEGER fileSize;

    if ( !GetFileSizeEx( file, &fileSize ) ||
        fileSize.QuadPart < 0 ||
        fileSize.QuadPart > INT_MAX )
    {
        CloseHandle( file );
        return NULL;
    }

    size_t byteCount = ( size_t ) fileSize.QuadPart;

    unsigned char* bytes = malloc( byteCount ? byteCount : 1 );

    if ( !bytes )
    {
        CloseHandle( file );
        return NULL;
    }

    size_t offset = 0;

    while ( offset < byteCount )
    {
        DWORD readCount = 0;

        if ( !ReadFile(
            file,
            bytes + offset,
            ( DWORD ) ( byteCount - offset ),
            &readCount,
            NULL ) || readCount == 0 )
        {
            free( bytes );
            CloseHandle( file );
            return NULL;
        }

        offset += readCount;
    }

    CloseHandle( file );

    uint16_t* result = NULL;
    size_t start = 0;

    // Check UTF-32 before UTF-16 because their BOMs overlap.
    if ( byteCount >= 4 &&
        bytes[0] == 0xFF && bytes[1] == 0xFE &&
        bytes[2] == 0x00 && bytes[3] == 0x00 )
    {
        result = DecodeUtf32(
            bytes + 4, byteCount - 4, 0, outLength );
    }
    else if ( byteCount >= 4 &&
        bytes[0] == 0x00 && bytes[1] == 0x00 &&
        bytes[2] == 0xFE && bytes[3] == 0xFF )
    {
        result = DecodeUtf32(
            bytes + 4, byteCount - 4, 1, outLength );
    }
    else if ( byteCount >= 3 &&
        bytes[0] == 0xEF &&
        bytes[1] == 0xBB &&
        bytes[2] == 0xBF )
    {
        // UTF-8 with BOM.
        result = ConvertToUtf16(
            bytes + 3, byteCount - 3,
            CP_UTF8, MB_ERR_INVALID_CHARS, outLength );
    }
    else if ( byteCount >= 2 &&
        bytes[0] == 0xFF && bytes[1] == 0xFE )
    {
        // UTF-16 little-endian.
        result = DecodeUtf16(
            bytes + 2, byteCount - 2, 0, outLength );
    }
    else if ( byteCount >= 2 &&
        bytes[0] == 0xFE && bytes[1] == 0xFF )
    {
        // UTF-16 big-endian.
        result = DecodeUtf16(
            bytes + 2, byteCount - 2, 1, outLength );
    }
    else
    {
        // No BOM. First check for likely UTF-16.
        int bigEndian = 0;

        if ( GuessUtf16( bytes, byteCount, &bigEndian ) )
        {
            result = DecodeUtf16(
                bytes, byteCount, bigEndian, outLength );
        }
        else
        {
            // Prefer valid UTF-8.
            result = ConvertToUtf16(
                bytes, byteCount,
                CP_UTF8, MB_ERR_INVALID_CHARS, outLength );

            // Best-effort legacy Windows code-page fallback.
            if ( !result )
            {
                result = ConvertToUtf16(
                    bytes, byteCount,
                    CP_ACP, 0, outLength );
            }
        }
    }

    free( bytes );
    return result;
}