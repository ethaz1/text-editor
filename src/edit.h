#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef struct TextEdit_t {
	int cur;				// cursor position. where the next character goes. visual cursor draws on top of this char.
	int first;				// position of the first character to appear. changes when scrolling.
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

enum TextEdit_SaveResult {
	TE_SAVE_SUCCESS = 0,
	TE_SAVE_UNKNOWN_ERROR = 1,
	TE_SAVE_ACCESS_DENIED = 2,
};

void TextEdit_Initialise( TextEdit* t );
void TextEdit_InsertChar( TextEdit* t, uint16_t ch, int pos );
void TextEdit_HandleCharCode( TextEdit* t, uint64_t key );
void TextEdit_MoveVertical( TextEdit* t, int direction );
void TextEdit_DeleteChar( TextEdit* t, int pos, bool backspace );
int TextEdit_SaveContents( TextEdit* t, uint16_t *path );
void TextEdit_OpenFile( TextEdit* t, uint16_t* path );
void TextEdit_Scroll( TextEdit* t, float delta );
void TextEdit_MoveHorizontal( TextEdit* t, int c );
void TextEdit_MoveByWord( TextEdit* t, int dir );
void TextEdit_CleanUp( TextEdit* t );

