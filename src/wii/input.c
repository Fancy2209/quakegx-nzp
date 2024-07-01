/*
Quake GameCube port.
Copyright (C) 2007 Peter Mackay
Copyright (C) 2008 Eluan Miranda
Copyright (C) 2015 Fabio Olimpieri

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// ELUTODO: do something about lookspring and lookstrafe
// ELUTODO: keys to: nunchuk turn and nunchuk look up/down?
// ELUTODO: osk doesn't work if client disconnected

#include "../generic/quakedef.h"

cvar_t	osk_repeat_delay = {"osk_repeat_delay","0.25"};
cvar_t	kb_repeat_delay = {"kb_repeat_delay","0.1"};
cvar_t	nunchuk_stick_as_arrows = {"nunchuk_stick_as_arrows","0"};
cvar_t  rumble = {"rumble","1"};

char keycode_normal[256] = { 
	'\0', '\0', '\0', '\0', //0-3
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', //4-29
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', //30-39
	K_ENTER, K_ESCAPE, K_BACKSPACE, K_TAB, K_SPACE, //40-44
	'-', '=', '[', ']', '\0', '\\', ';', '\'', '`', ',', '.', '/', '\0', //45-57
	K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, '\0', '\0', K_PAUSE, //58-72
	'\0', '\0', '\0', '\0', '\0', '\0',//73-78
	K_RIGHTARROW, K_LEFTARROW, K_DOWNARROW, K_UPARROW, //79-82
	K_NUMLOCK, '/', '*', '-', '+', K_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', '\\', //83-100
	K_MENU
};

char keycode_shifted[256] = { 
	'\0', '\0', '\0', '\0', 
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
	K_ENTER, K_ESCAPE, K_BACKSPACE, K_TAB, K_SPACE,
	'_', '+', '{', '}', '\0', '|', ':', '"', '~', '<', '>', '?', '\0',
	K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, '\0', '\0', K_PAUSE,
	'\0', '\0', '\0', '\0', '\0', '\0', //73-78
	K_RIGHTARROW, K_LEFTARROW, K_DOWNARROW, K_UPARROW, //79-82
	K_NUMLOCK, '/', '*', '-', '+', K_ENTER, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.', '\\', //83-100
	K_MENU
};

bool keyboard_shifted = false;
u8 kb_last_selected = 0x0;

// pass these values to whatever subsystem wants it
float in_pitchangle;
float in_yawangle;
float in_rollangle;

// Are we inside the on-screen keyboard? (ELUTODO: refactor)
int in_osk = 0;

// \0 means not mapped...
// 5 * 15
char osk_normal[75] =
{
	'\'', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', ']', K_BACKSPACE,
	0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0, '[', K_ENTER, K_ENTER,
	0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0, '~', '/', K_ENTER, K_ENTER,
	0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', ';', K_ENTER, K_ENTER, K_ENTER,
	0 , 0, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, 0, 0
};

char osk_shifted[75] =
{
	'\"', '!', '@', '#', '$', '%', 0, '&', '*', '(', ')', '_', '+', '}', K_BACKSPACE,
	0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0, '{', K_ENTER, K_ENTER,
	0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0, '^', '?', K_ENTER, K_ENTER,
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', ':', K_ENTER, K_ENTER, K_ENTER,
	0 , 0, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, K_SPACE, 0, 0
};

char *osk_set;
int osk_selected;
int osk_last_selected;
int osk_coords[2];

float osk_last_press_time = 0.0f;

#include <ogc/pad.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#include "input_wiimote.h"

#define FORCE_KEY_BINDINGS 0

u32 wiimote_ir_res_x;
u32 wiimote_ir_res_y;

// wiimote info
u32 wpad_previous_keys = 0x0000;
u32 wpad_keys = 0x0000;

ir_t pointer;
orient_t orientation;
expansion_t expansion;
nunchuk_t nunchuk; //Shake
gforce_t gforce;

bool wiimote_connected = true;
bool nunchuk_connected = false;
bool classic_connected = false;
bool keyboard_connected = false;

typedef enum  {LEFT, CENTER_X, RIGHT} stick_x_st_t;
typedef enum   {UP, CENTER_Y, DOWN} stick_y_st_t;

stick_x_st_t stick_x_st = CENTER_X;
stick_y_st_t stick_y_st = CENTER_Y;

u16 pad_previous_keys = 0x0000;
u16 pad_keys = 0x0000;

int last_irx = -1, last_iry = -1;

static float clamp(float value, float minimum, float maximum)
{
	if (value > maximum)
	{
		return maximum;
	}
	else if (value < minimum)
	{
		return minimum;
	}
	else
	{
		return value;
	}
}

static void apply_dead_zone(float* x, float* y, float dead_zone)
{
	// Either stick out of the dead zone?
	if ((fabsf(*x) >= dead_zone) || (fabsf(*y) >= dead_zone))
	{
		// Nothing to do.
	}
	else
	{
		// Clamp to the dead zone.
		*x = 0.0f;
		*y = 0.0f;
	}
}

static s8 WPAD_StickX(u8 which)
{
	float mag = 0.0;
	float ang = 0.0;

	switch (expansion.type)
	{
		case WPAD_EXP_NUNCHUK:
		case WPAD_EXP_GUITARHERO3:
			if (which == 0)
			{
				mag = expansion.nunchuk.js.mag;
				ang = expansion.nunchuk.js.ang;
			}
			break;

		case WPAD_EXP_CLASSIC:
			if (which == 0)
			{
				mag = expansion.classic.ljs.mag;
				ang = expansion.classic.ljs.ang;
			}
			else
			{
				mag = expansion.classic.rjs.mag;
				ang = expansion.classic.rjs.ang;
			}
			break;

		default:
			break;
	}

	/* calculate X value (angle needs to be converted into radians) */
	if (mag > 1.0) mag = 1.0;
	else if (mag < -1.0) mag = -1.0;
	double val = mag * sin(M_PI * ang/180.0f);

	return (s8)(val * 128.0f);
}

static s8 WPAD_StickY(u8 which)
{
	float mag = 0.0;
	float ang = 0.0;

	switch (expansion.type)
	{
		case WPAD_EXP_NUNCHUK:
		case WPAD_EXP_GUITARHERO3:
			if (which == 0)
			{
				mag = expansion.nunchuk.js.mag;
				ang = expansion.nunchuk.js.ang;
			}
			break;

		case WPAD_EXP_CLASSIC:
			if (which == 0)
			{
				mag = expansion.classic.ljs.mag;
				ang = expansion.classic.ljs.ang;
			}
			else
			{
				mag = expansion.classic.rjs.mag;
				ang = expansion.classic.rjs.ang;
			}
			break;

		default:
			break;
	}

	/* calculate X value (angle need to be converted into radian) */
	if (mag > 1.0) mag = 1.0;
	else if (mag < -1.0) mag = -1.0;
	double val = mag * cos(M_PI * ang/180.0f);

	return (s8)(val * 128.0f);
}

void IN_Init (void)
{
#if FORCE_KEY_BINDINGS
	// Set up the key bindings.
	Cbuf_AddText("bind JOY0 +jump\n");
	Cbuf_AddText("bind JOY1 +attack\n");
	Cbuf_AddText("bind JOY8 +speed\n");
	Cbuf_AddText("bind JOY7 +mlook\n");
	Cbuf_AddText("bind JOY2 +showscores\n");
	Cbuf_AddText("bind RIGHTARROW \"impulse 10\"\n");	
#endif

	last_irx = -1;
	last_iry = -1;

	in_osk = 0;

	in_pitchangle = .0f;
	in_yawangle = .0f;
	in_rollangle = .0f;

	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, wiimote_ir_res_x, wiimote_ir_res_y);

	Cvar_RegisterVariable(&osk_repeat_delay);
	Cvar_RegisterVariable(&kb_repeat_delay);
	
	Cvar_RegisterVariable (&nunchuk_stick_as_arrows);
	Cvar_RegisterVariable (&rumble);

	keycode_normal[225] = K_LSHIFT;
	keycode_normal[229] = K_RSHIFT;

	keycode_shifted[225] = K_LSHIFT;
	keycode_shifted[229] = K_RSHIFT;
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
	// Fetch the pad state.
	PAD_ScanPads();
	WPAD_ScanPads();

	keyboard_event KB_event;

	// Usb keyboard managment 
	while(KEYBOARD_GetEvent(&KB_event) > 0)
	{
		switch(KB_event.type)
		{
			case KEYBOARD_CONNECTED:
				keyboard_connected = true;
				break;

			case KEYBOARD_DISCONNECTED:
				keyboard_connected = false;
				break;
	
			case KEYBOARD_PRESSED:
				if(!keyboard_shifted)
					Key_Event(keycode_normal[KB_event.keycode], true);

				else
					Key_Event(keycode_shifted[KB_event.keycode], true);

				if(keycode_normal[KB_event.keycode] == K_LSHIFT || keycode_normal[KB_event.keycode] == K_RSHIFT)
					keyboard_shifted = true;

				break;

			case KEYBOARD_RELEASED:
				if(!keyboard_shifted)
					Key_Event(keycode_normal[KB_event.keycode], false);

				else
					Key_Event(keycode_shifted[KB_event.keycode], false);

				if(keycode_normal[KB_event.keycode] == K_LSHIFT || keycode_normal[KB_event.keycode] == K_RSHIFT)
					keyboard_shifted = false;

				break;
		}
	}

	//It manages the nunchunk or classic controller connection
	//It assigns the pressed buttons to wpad_keys and to pad_keys
	
	u32 exp_type;
	if ( WPAD_Probe(WPAD_CHAN_0, &exp_type) != 0 )
		exp_type = WPAD_EXP_NONE;

	if(exp_type == WPAD_EXP_NUNCHUK)
	{
		if(!nunchuk_connected)
			wpad_previous_keys = 0x0000;

		nunchuk_connected = true;
		classic_connected = false;
		wpad_keys = WPAD_ButtonsHeld(WPAD_CHAN_0);
		pad_keys = 0x0000;
		pad_previous_keys = 0x0000;
	}

	else if(exp_type == WPAD_EXP_CLASSIC)
	{
		if(!classic_connected)
			wpad_previous_keys = 0x0000;

		nunchuk_connected = false;
		classic_connected = true;
		wpad_keys = WPAD_ButtonsHeld(WPAD_CHAN_0);
		pad_keys = 0x0000;
		pad_previous_keys = 0x0000;
	}

	else
	//Here neither the classic controller nor the nuncunk are connected
	{
		if(classic_connected || nunchuk_connected)
			wpad_previous_keys = 0x0000;

		nunchuk_connected = false;
		classic_connected = false;
		pad_keys = PAD_ButtonsHeld(PAD_CHAN0);
		wpad_keys = WPAD_ButtonsHeld(WPAD_CHAN_0);
	}
	
	WPAD_IR(WPAD_CHAN_0, &pointer);
	WPAD_Orientation(WPAD_CHAN_0, &orientation);
	WPAD_Expansion(WPAD_CHAN_0, &expansion);
	WPAD_GForce(WPAD_CHAN_0, &gforce); //Shake to reload

	//On screen keyboard
	if (wiimote_connected && (wpad_keys & WPAD_BUTTON_MINUS))
	{
		// ELUTODO: we are using the previous frame wiimote position... FIX IT
		in_osk = 1;
		int line = (last_iry - OSK_YSTART) / (osk_line_size * (osk_line_size / osk_charsize)) - 1;
		int col = (last_irx - OSK_XSTART) / (osk_col_size * (osk_col_size / osk_charsize)) - 1;

		osk_coords[0] = last_irx;
		osk_coords[1] = last_iry;

		line = clamp(line, 0, osk_num_lines);
		col = clamp(col, 0, osk_num_col);

		if (nunchuk_connected && (wpad_keys & WPAD_NUNCHUK_BUTTON_Z))
			osk_set = osk_shifted;
		else
			osk_set = osk_normal;

		osk_selected = osk_set[line * osk_num_col + col];

		if ((wpad_keys & WPAD_BUTTON_B) && osk_selected && (Sys_FloatTime() >= osk_last_press_time + osk_repeat_delay.value || osk_selected != osk_last_selected))
		{
			Key_Event((osk_selected), true);
			Key_Event((osk_selected), false);
			osk_last_selected = osk_selected;
			osk_last_press_time = Sys_FloatTime();
		}
	}
	else
	{
		// TODO: go back to the old method with buton mappings. The code was a lot cleaner that way
		in_osk = 0;
		
//Send the wireless classic controller buttons events
		if(classic_connected)
		{
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_LEFT) != (wpad_keys & WPAD_CLASSIC_BUTTON_LEFT))
			{
				// Send a press event.
				Key_Event(K_LEFTARROW, ((wpad_keys & WPAD_CLASSIC_BUTTON_LEFT) == WPAD_CLASSIC_BUTTON_LEFT));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_RIGHT) != (wpad_keys & WPAD_CLASSIC_BUTTON_RIGHT))
			{
				// Send a press event.
				Key_Event(K_RIGHTARROW, ((wpad_keys & WPAD_CLASSIC_BUTTON_RIGHT) == WPAD_CLASSIC_BUTTON_RIGHT));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_DOWN) != (wpad_keys & WPAD_CLASSIC_BUTTON_DOWN))
			{
				// Send a press event.
				Key_Event(K_DOWNARROW, ((wpad_keys & WPAD_CLASSIC_BUTTON_DOWN) == WPAD_CLASSIC_BUTTON_DOWN));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_UP) != (wpad_keys & WPAD_CLASSIC_BUTTON_UP))
			{
				// Send a press event.
				Key_Event(K_UPARROW, ((wpad_keys & WPAD_CLASSIC_BUTTON_UP) == WPAD_CLASSIC_BUTTON_UP));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_A) != (wpad_keys & WPAD_CLASSIC_BUTTON_A))
			{
				// Send a press event.
				Key_Event(K_JOY9, ((wpad_keys & WPAD_CLASSIC_BUTTON_A) == WPAD_CLASSIC_BUTTON_A));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_B) != (wpad_keys & WPAD_CLASSIC_BUTTON_B))
			{
				// Send a press event.
				Key_Event(K_JOY10, ((wpad_keys & WPAD_CLASSIC_BUTTON_B) == WPAD_CLASSIC_BUTTON_B));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_X) != (wpad_keys & WPAD_CLASSIC_BUTTON_X))
			{
				// Send a press event.
				Key_Event(K_JOY11, ((wpad_keys & WPAD_CLASSIC_BUTTON_X) == WPAD_CLASSIC_BUTTON_X));
			}
	
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_Y) != (wpad_keys & WPAD_CLASSIC_BUTTON_Y))
			{
				// Send a press event.
				Key_Event(K_JOY12, ((wpad_keys & WPAD_CLASSIC_BUTTON_Y) == WPAD_CLASSIC_BUTTON_Y));
			}
			
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_FULL_L) != (wpad_keys & WPAD_CLASSIC_BUTTON_FULL_L))
			{
				// Send a press event.
				Key_Event(K_JOY13, ((wpad_keys & WPAD_CLASSIC_BUTTON_FULL_L) == WPAD_CLASSIC_BUTTON_FULL_L));
			}

			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_FULL_R) != (wpad_keys & WPAD_CLASSIC_BUTTON_FULL_R))
			{
				// Send a press event.
				Key_Event(K_JOY14, ((wpad_keys & WPAD_CLASSIC_BUTTON_FULL_R) == WPAD_CLASSIC_BUTTON_FULL_R));
			}
			
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_ZL) != (wpad_keys & WPAD_CLASSIC_BUTTON_ZL))
			{
				// Send a press event.
				Key_Event(K_JOY15, ((wpad_keys & WPAD_CLASSIC_BUTTON_ZL) == WPAD_CLASSIC_BUTTON_ZL));
			}

			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_ZR) != (wpad_keys & WPAD_CLASSIC_BUTTON_ZR))
			{
				// Send a press event.
				Key_Event(K_JOY16, ((wpad_keys & WPAD_CLASSIC_BUTTON_ZR) == WPAD_CLASSIC_BUTTON_ZR));
			}
			
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_MINUS) != (wpad_keys & WPAD_CLASSIC_BUTTON_MINUS))
			{
				// Send a press event.
				Key_Event(K_JOY17, ((wpad_keys & WPAD_CLASSIC_BUTTON_MINUS) == WPAD_CLASSIC_BUTTON_MINUS));
			}
			
			if ((wpad_previous_keys & WPAD_CLASSIC_BUTTON_PLUS) != (wpad_keys & WPAD_CLASSIC_BUTTON_PLUS))
			{
				// Send a press event.
				Key_Event(K_JOY18, ((wpad_keys & WPAD_CLASSIC_BUTTON_PLUS) == WPAD_CLASSIC_BUTTON_PLUS));
			}
	
		}

		else
//Send the wiimote button events if the classic controller is not connected	
		
		{
			if ((wpad_previous_keys & WPAD_BUTTON_LEFT) != (wpad_keys & WPAD_BUTTON_LEFT))
			{
				// Send a press event.
				Key_Event(K_LEFTARROW, ((wpad_keys & WPAD_BUTTON_LEFT) == WPAD_BUTTON_LEFT));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_RIGHT) != (wpad_keys & WPAD_BUTTON_RIGHT))
			{
				// Send a press event.
				Key_Event(K_RIGHTARROW, ((wpad_keys & WPAD_BUTTON_RIGHT) == WPAD_BUTTON_RIGHT));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_DOWN) != (wpad_keys & WPAD_BUTTON_DOWN))
			{
				// Send a press event.
				Key_Event(K_DOWNARROW, ((wpad_keys & WPAD_BUTTON_DOWN) == WPAD_BUTTON_DOWN));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_UP) != (wpad_keys & WPAD_BUTTON_UP))
			{
				// Send a press event.
				Key_Event(K_UPARROW, ((wpad_keys & WPAD_BUTTON_UP) == WPAD_BUTTON_UP));
			}
			
			if ((wpad_previous_keys & WPAD_BUTTON_A) != (wpad_keys & WPAD_BUTTON_A))
			{
				// Send a press event.
				Key_Event(K_JOY0, ((wpad_keys & WPAD_BUTTON_A) == WPAD_BUTTON_A));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_B) != (wpad_keys & WPAD_BUTTON_B))
			{
				// Send a press event.
				Key_Event(K_JOY1, ((wpad_keys & WPAD_BUTTON_B) == WPAD_BUTTON_B));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_1) != (wpad_keys & WPAD_BUTTON_1))
			{
				// Send a press event.
				Key_Event(K_JOY2, ((wpad_keys & WPAD_BUTTON_1) == WPAD_BUTTON_1));
			}
	
			if ((wpad_previous_keys & WPAD_BUTTON_2) != (wpad_keys & WPAD_BUTTON_2))
			{
				// Send a press event.
				Key_Event(K_JOY3, ((wpad_keys & WPAD_BUTTON_2) == WPAD_BUTTON_2));
			}
			
			if ((wpad_previous_keys & WPAD_BUTTON_PLUS) != (wpad_keys & WPAD_BUTTON_PLUS))
			
			{
				// Send a press event.
				Key_Event(K_JOY5, ((wpad_keys & WPAD_BUTTON_PLUS) == WPAD_BUTTON_PLUS));
			}
			
			
//Send nunchunk button events
			if(nunchuk_connected)
			{
				if ((wpad_previous_keys & WPAD_NUNCHUK_BUTTON_Z) != (wpad_keys & WPAD_NUNCHUK_BUTTON_Z))
				{
					// Send a press event.
					Key_Event(K_JOY7, ((wpad_keys & WPAD_NUNCHUK_BUTTON_Z) == WPAD_NUNCHUK_BUTTON_Z));
				}
				
				if ((wpad_previous_keys & WPAD_NUNCHUK_BUTTON_C) != (wpad_keys & WPAD_NUNCHUK_BUTTON_C))
				{
					// Send a press event.
					Key_Event(K_JOY8, ((wpad_keys & WPAD_NUNCHUK_BUTTON_C) == WPAD_NUNCHUK_BUTTON_C));
				}
				
				//Con_Printf("xge:%f, acy:%f\n", expansion.nunchuk.gforce.x, expansion.nunchuk.accel.y);
				
				if(/*expansion.nunchuk.gforce.x > 0.8 && */expansion.nunchuk.gforce.y > 0.35) {
					Key_Event(K_SHAKE, true);
				} else {
					Key_Event(K_SHAKE, false);
				}
				
//Emulation of the wimote arrows with the nunchuk stick
				if(nunchuk_stick_as_arrows.value)
				{
					const s8 nunchuk_stick_x = WPAD_StickX(0);
					const s8 nunchuk_stick_y = WPAD_StickY(0);
					if (nunchuk_stick_x > 10) 
					{
						switch (stick_x_st)
						{
							
							case CENTER_X : Key_Event(K_RIGHTARROW, true);break;
							default : break;
						
						}
					stick_x_st = RIGHT;
					}
					
					else if (nunchuk_stick_x < -10) 
					{
						switch (stick_x_st)
						{
							case CENTER_X : Key_Event(K_LEFTARROW, true);break;
							default: break;
							
							
						}
					stick_x_st = LEFT;
					}
					
					else
					{
						switch (stick_x_st)
						{
							case LEFT :	Key_Event(K_LEFTARROW, false);break;							
							case RIGHT: Key_Event(K_RIGHTARROW, false);break;
							default: break;
						}
					stick_x_st = CENTER_X;
					}
					
					if (nunchuk_stick_y > 10) 
					{
						switch (stick_y_st)
						{
							case CENTER_Y : Key_Event(K_UPARROW, true); break;
							default: break;	
							
						}
					stick_y_st = UP;
					}
					
					else if (nunchuk_stick_y < -10) 
					{
						switch (stick_y_st)
						{
							
							case CENTER_Y : Key_Event(K_DOWNARROW, true);break;
							default: break;
						}
					stick_y_st = DOWN;
					}
					
					
					else
					{
						switch (stick_y_st)
						{
							case DOWN :	Key_Event(K_DOWNARROW, false);break;
							case UP: Key_Event(K_UPARROW, false);break;
							default: break;
						}
					stick_y_st = CENTER_Y;
					}
						
				}	
				
			}
		}
//Send the gamecube controller button events in the case neither the nunchuk nor the classic controller is connected
		if(!nunchuk_connected && !classic_connected)
		{
			if ((pad_previous_keys & PAD_BUTTON_LEFT) != (pad_keys & PAD_BUTTON_LEFT))
			{
				// Send a press event.
				Key_Event(K_LEFTARROW, ((pad_keys & PAD_BUTTON_LEFT) == PAD_BUTTON_LEFT));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_RIGHT) != (pad_keys & PAD_BUTTON_RIGHT))
			{
				// Send a press event.
				Key_Event(K_RIGHTARROW, ((pad_keys & PAD_BUTTON_RIGHT) == PAD_BUTTON_RIGHT));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_DOWN) != (pad_keys & PAD_BUTTON_DOWN))
			{
				// Send a press event.
				Key_Event(K_DOWNARROW, ((pad_keys & PAD_BUTTON_DOWN) == PAD_BUTTON_DOWN));
			}
			if ((pad_previous_keys & PAD_BUTTON_UP) != (pad_keys & PAD_BUTTON_UP))
			{
				// Send a press event.
				Key_Event(K_UPARROW, ((pad_keys & PAD_BUTTON_UP) == PAD_BUTTON_UP));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_A) != (pad_keys & PAD_BUTTON_A))
			{
				// Send a press event.
				Key_Event(K_JOY20, ((pad_keys & PAD_BUTTON_A) == PAD_BUTTON_A));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_B) != (pad_keys & PAD_BUTTON_B))
			{
				// Send a press event.
				Key_Event(K_JOY21, ((pad_keys & PAD_BUTTON_B) == PAD_BUTTON_B));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_X) != (pad_keys & PAD_BUTTON_X))
			{
				// Send a press event.
				Key_Event(K_JOY22, ((pad_keys & PAD_BUTTON_X) == PAD_BUTTON_X));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_Y) != (pad_keys & PAD_BUTTON_Y))
			{
				// Send a press event.
				Key_Event(K_JOY23, ((pad_keys & PAD_BUTTON_Y) == PAD_BUTTON_Y));
			}
			
			if ((pad_previous_keys & PAD_TRIGGER_Z) != (pad_keys & PAD_TRIGGER_Z))
			{
				// Send a press event.
				Key_Event(K_JOY24, ((pad_keys & PAD_TRIGGER_Z) == PAD_TRIGGER_Z));
			}
			
			if ((pad_previous_keys & PAD_TRIGGER_R) != (pad_keys & PAD_TRIGGER_R))
			{
				// Send a press event.
				Key_Event(K_JOY25, ((pad_keys & PAD_TRIGGER_R) == PAD_TRIGGER_R));
			}
	
			if ((pad_previous_keys & PAD_TRIGGER_L) != (pad_keys & PAD_TRIGGER_L))
			{
				// Send a press event.
				Key_Event(K_JOY26, ((pad_keys & PAD_TRIGGER_L) == PAD_TRIGGER_L));
			}
	
			if ((pad_previous_keys & PAD_BUTTON_START) != (pad_keys & PAD_BUTTON_START))
			{
				// Send a press event.
				Key_Event(K_ESCAPE, ((pad_keys & PAD_BUTTON_START) == PAD_BUTTON_START));
			}

			pad_previous_keys = pad_keys;
		}
	}

	wpad_previous_keys = wpad_keys;
}

extern bool croshhairmoving;
extern float crosshair_opacity;

// Some things here rely upon IN_Move always being called after IN_Commands on the same frame
void IN_Move (usercmd_t *cmd)
{
	const float dead_zone = 0.3f;

	float x1;
	float y1;
	float x2;
	float y2;

	// TODO: sensor bar position correct? aspect ratio correctly set? etc...
	// In "pointer" variable there are the IR values
	int last_wiimote_ir_x = pointer.x;
	int last_wiimote_ir_y = pointer.y;
	int wiimote_ir_x = 0, wiimote_ir_y = 0;


	if (pointer.x < 1 || (unsigned int)pointer.x > pointer.vres[0] - 1)
		wiimote_ir_x = last_wiimote_ir_x;
	else
		wiimote_ir_x = pointer.x;
	if (pointer.y < 1 || (unsigned int)pointer.y > pointer.vres[1] - 1)
		wiimote_ir_y = last_wiimote_ir_y;
	else
		wiimote_ir_y = pointer.y;

	last_wiimote_ir_x = wiimote_ir_x;
	last_wiimote_ir_y = wiimote_ir_y;

	if (in_osk || (cls.state == ca_connected && key_dest != key_game))
	{
		last_irx = wiimote_ir_x;
		last_iry = wiimote_ir_y;
		return;
	}
// Movement management of nunchuk stick (x1/y1) and of IR (x2/y2) if the nunchuk is connected
	if(nunchuk_connected && !nunchuk_stick_as_arrows.value)
		{
		const s8 nunchuk_stick_x = WPAD_StickX(0);
		const s8 nunchuk_stick_y = WPAD_StickY(0);

		x1 = clamp(((float)nunchuk_stick_x / 128.0f) * 1.5, -1.0f, 1.0f);
		y1 = clamp(((float)nunchuk_stick_y / (128.0f)) * 1.5, -1.0f, 1.0f);

		x2 = clamp((float)wiimote_ir_x / (pointer.vres[0] / 2.0f) - 1.0f, -1.0f, 1.0f);
		// Move the cross position
		Cvar_SetValue("cl_crossx", scr_vrect.width / 2 * x2);

		y2 = clamp((float)wiimote_ir_y / (pointer.vres[1] / 2.0f) - 1.0f, -1.0f, 1.0f);
		Cvar_SetValue("cl_crossy", scr_vrect.height / 2 * y2);
		}
		
// Movement management of 2 classic controller sticks (x1/y1) and (y1/y2) if the cc is connected

	else if(classic_connected)
	{
		const s8 left_stick_x = WPAD_StickX(0);
		const s8 left_stick_y = WPAD_StickY(0);

		const s8 right_stick_x = WPAD_StickX(1);
		const s8 right_stick_y = WPAD_StickY(1);

		x1 = clamp(((float)left_stick_x / 128.0f) * 1.5, -1.0f, 1.0f);
		y1 = clamp(((float)left_stick_y / (128.0f)) * 1.5, -1.0f, 1.0f);

		x2 = clamp(((float)right_stick_x / 128.0f) * 1.5, -1.0f, 1.0f);
		Cvar_SetValue("cl_crossx", /*(in_vlock.state & 1) ? */scr_vrect.width / 2 * x2/* : 0*/);
		y2 = clamp(((float)right_stick_y / (-128.0f)) * 1.5, -1.0f, 1.0f);
		Cvar_SetValue("cl_crossy", /*(in_vlock.state & 1) ? */scr_vrect.height / 2 * y2/* : 0*/);
	}
// Movement management of 2 gamecube controller sticks (x1/y1) e  (y1/y2) if neither the cc nor nn is connected
	else
	{
		const s8 stick_x = PAD_StickX(0);
		const s8 stick_y = PAD_StickY(0);

		const s8 sub_stick_x = PAD_SubStickX(0);
		const s8 sub_stick_y = PAD_SubStickY(0);

		x1 = clamp(stick_x / 90.0f, -1.0f, 1.0f);
		y1 = clamp(stick_y / 90.0f, -1.0f, 1.0f);

		x2 = clamp(sub_stick_x / 80.0f, -1.0f, 1.0f);
		Cvar_SetValue("cl_crossx", /*(in_vlock.state & 1) ? */scr_vrect.width / 2 * x2/* : 0*/);

		y2 = clamp(sub_stick_y / -80.0f, -1.0f, 1.0f);
		Cvar_SetValue("cl_crossy", /*(in_vlock.state & 1) ? */scr_vrect.height / 2 * y2/* : 0*/);
	}

	last_irx = wiimote_ir_x;
	last_iry = wiimote_ir_y;

	// Apply the dead zone.
	apply_dead_zone(&x1, &y1, dead_zone);
	apply_dead_zone(&x2, &y2, dead_zone);

	// Don't let the pitch drift back to centre if mouse look is on or the right stick is being used.
	//if ((in_mlook.state & 1) || (fabsf(y2) >= dead_zone)) Disabled, always very convenient with a gamepad or wiimote
	{
		V_StopPitchDrift();
	}

	// Lock view?
	if (in_vlock.state & 1)
	{
		x2 = 0;
		y2 = 0;
	}

	float yaw_rate;
	float pitch_rate;

	yaw_rate = x2;
	pitch_rate = y2;
	
	cl_backspeed = cl_forwardspeed = cl_sidespeed = sv_player->v.maxspeed;
	cl_sidespeed *= 0.8;
	cl_backspeed *= 0.7;

	// Move using the main stick.
	//Send the stick movement commands
	if (!(nunchuk_stick_as_arrows.value&&nunchuk_connected))
	{
		cmd->sidemove += cl_sidespeed * x1;
		if (y1>0) cmd->forwardmove += cl_forwardspeed * y1; /* TODO: use cl_backspeed when going backwards? */
			else cmd->forwardmove += cl_backspeed * y1; 

		//if the nunchuk c button is pressed it speeds up
		if (in_speed.state & 1)
		{
			if (cl_forwardspeed > 200)
			{
				cmd->forwardmove /= cl_movespeedkey.value;
				cmd->sidemove /= cl_movespeedkey.value;
			}
			else
			{
				cmd->forwardmove *= cl_movespeedkey.value;
				cmd->sidemove *= cl_movespeedkey.value; /* TODO: always seem to be at the max and I'm too sleepy now to figure out why */
			}
		}
	}

	// TODO: Use yawspeed and pitchspeed

	// Adjust the yaw.
	const float turn_rate = sensitivity.value * 40.0f;
	if (in_speed.state & 1)
	{
		if (cl_forwardspeed > 200)
			cl.viewangles[YAW] -= turn_rate * yaw_rate * host_frametime / cl_anglespeedkey.value;
		else
			cl.viewangles[YAW] -= turn_rate * yaw_rate * host_frametime * cl_anglespeedkey.value;
	}
	else
		cl.viewangles[YAW] -= turn_rate * yaw_rate * host_frametime;

	// How fast to pitch?
	float pitch_offset;
	if (in_speed.state & 1)
	{
		if (cl_forwardspeed > 200)
			pitch_offset = turn_rate * pitch_rate * host_frametime / cl_anglespeedkey.value;
		else
			pitch_offset = turn_rate * pitch_rate * host_frametime * cl_anglespeedkey.value;
	}
	else
		pitch_offset = turn_rate * pitch_rate * host_frametime;

	// Do the pitch.
	const bool	invert_pitch = m_pitch.value < 0;
	if (invert_pitch)
	{
		cl.viewangles[PITCH] -= pitch_offset;
	}
	else
	{
		cl.viewangles[PITCH] += pitch_offset;
	}

	// Don't look too far up or down.
	if (cl.viewangles[PITCH] > 80.0f)
	{
		cl.viewangles[PITCH] = 80.0f;
	}
	else if (cl.viewangles[PITCH] < -60.0f)
	{
		cl.viewangles[PITCH] = -60.0f;
	}

	if (wiimote_connected && nunchuk_connected && !nunchuk_stick_as_arrows.value)
	{
		in_pitchangle = orientation.pitch;
		in_yawangle = orientation.yaw;
		in_rollangle = orientation.roll;
	}
	else
	{
		in_pitchangle = .0f;
		in_yawangle = .0f;
		in_rollangle = .0f;
	}
	
	//Con_Printf("%f\n", x2);
	
	// crosshair stuff
	if (x2 < 0.065f && x2 > -0.065f && y2 < 0.065f && y2 > -0.065f) {
		croshhairmoving = false;

		crosshair_opacity += 22;

		if (crosshair_opacity >= 255)
			crosshair_opacity = 255;
	} else {
		croshhairmoving = true;
		crosshair_opacity -= 8;
		if (crosshair_opacity <= 128)
			crosshair_opacity = 128;
	}
	
}
