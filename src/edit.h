#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef struct TextEdit_t {
	int cur;				// cursor position. where the next character goes. visual cursor draws on top of this char.
	int last;				// position of the last character typed. -1 if the buffer is empty.
	unsigned int size;		// max number of characters that can fit in the input buffer.
	uint16_t* buf;			// where the text lives.
	int row;				// row of the cursor (i.e. the line number)
	int col;				// column of the cursor 
	int preferredColumn;
	bool preferredColumnValid;
	bool curFlash;
	bool inFocus;
	//uint32_t backCol;
	//uint32_t foreCol;
} TextEdit;

void TextEdit_Initialise( TextEdit* t );
void TextEdit_InsertChar( TextEdit* t, uint16_t ch, int pos );
void TextEdit_HandleCharCode( TextEdit* t, uint64_t key );
void TextEdit_MoveVertical( TextEdit* t, int direction );
void TextEdit_DeleteChar( TextEdit* t, int pos, bool backspace );
void TextEdit_SaveContents( TextEdit* t, uint16_t *path );
void TextEdit_CleanUp( TextEdit* t );

