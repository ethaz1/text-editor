#pragma once

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

// Returns a malloc-allocated UTF-16 buffer.
// outLength receives the number of UTF-16 code units,
// excluding the terminating null character.
//
// The caller must free() the returned buffer.
// Returns NULL on failure.
uint16_t* TextFile_ReadUtf16(
    const wchar_t* path,
    size_t* outLength
);