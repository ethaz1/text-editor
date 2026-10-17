#pragma once
#include "edit.h"


typedef enum  {
	STATUS_MODE_NORMAL,
	STATUS_MODE_SAVE,
	STATUS_MODE_OPEN
} StatusMode;

typedef enum {
	STATUS_EVENT_SAVE,
	STATUS_EVENT_OPEN,
	STATUS_EVENT_RESET,
	STATUS_EVENT_SUBMIT	// enter key pressed
} StatusEvent;


TextEdit* StatusBar_Initialise( );
void StatusBar_HandleEvent( StatusEvent e, TextEdit* other );
void StatusBar_CleanUp( );
