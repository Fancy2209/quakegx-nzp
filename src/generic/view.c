/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// view.c -- player eye positioning

#include "quakedef.h"

sfx_t			*cl_sfx_step[5];

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

cvar_t		lcd_x = {"lcd_x","0"};
cvar_t		lcd_yaw = {"lcd_yaw","0"};

cvar_t	scr_ofsx = {"scr_ofsx","0", false};
cvar_t	scr_ofsy = {"scr_ofsy","0", false};
cvar_t	scr_ofsz = {"scr_ofsz","0", false};

cvar_t	cl_rollspeed = {"cl_rollspeed", "200"};
cvar_t	cl_rollangle = {"cl_rollangle", "2.0f"};

cvar_t	cl_bob = {"cl_bob","0.02", false};
cvar_t	cl_bobcycle = {"cl_bobcycle","0.06", false};
cvar_t	cl_bobup = {"cl_bobup","0.01", false};//BLUB changed to 0.02

cvar_t	cl_sidebobbing = {"cl_sidebobbing","1"};
cvar_t	cl_bobside = {"cl_bobside","0.1"};
cvar_t	cl_bobsidecycle = {"cl_bobsidecycle","0.9"};
cvar_t	cl_bobsideup = {"cl_bobsideup","0.5"};

cvar_t	v_kicktime = {"v_kicktime", "0.5", false};
cvar_t	v_kickroll = {"v_kickroll", "0.6", false};
cvar_t	v_kickpitch = {"v_kickpitch", "0.6", false};

cvar_t	cl_weapon_inrollangle = {"cl_weapon_inrollangle", "0", true};

cvar_t	v_iyaw_cycle = {"v_iyaw_cycle", "2", false};
cvar_t	v_iroll_cycle = {"v_iroll_cycle", "0.5", false};
cvar_t	v_ipitch_cycle = {"v_ipitch_cycle", "1", false};
cvar_t	v_iyaw_level = {"v_iyaw_level", "0.3", false}; //0.3
cvar_t	v_iroll_level = {"v_iroll_level", "0", false}; //0.1f
cvar_t	v_ipitch_level = {"v_ipitch_level", "0.3", false}; //0.3

cvar_t	v_idlescale = {"v_idlescale", "0", false};

cvar_t	crosshair = {"crosshair", "1", true};
cvar_t	cl_crossx = {"cl_crossx", "0", false};
cvar_t	cl_crossy = {"cl_crossy", "0", false};

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;

extern	int			in_forward, in_forward2, in_back;


/*
===============
V_CalcRoll

Used by view and sv_user
===============
*/
vec3_t	forward, right, up;

float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;
	
	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabsf(side);
	
	value = cl_rollangle.value;
//	if (cl.inwater)
//		value *= 6;

	if (side < cl_rollspeed.value)
		side = side * value / cl_rollspeed.value;
	else
		side = value;
	
	return side*sign;
	
}


/*
===============
V_CalcBob

===============
*/

// Blub's new V_CalcBob code, both side and pitch are in one, dictated by the (which) parameter
float V_CalcBob (float speed,float which)//0 = regular, 1 = side bobbing
{
	float bob = 0;
	float sprint = 1;

	if (cl.stats[STAT_ZOOM] == 2)
		return 0;

	// Bob idle-y, instead of presenting as if in-motion.
	if (speed < 0.1) {
		if(cl.stats[STAT_ZOOM] == 1)
			speed = 0.05;
		else
			speed = 0.25;

		if (which == 0)
            		bob = cl_bobup.value * 10 * speed * (sprint * sprint) * sin(cl.time * 3.25 * sprint);
        	else
            		bob = cl_bobside.value * 50 * speed * (sprint * sprint * sprint) * sin((cl.time * sprint) - (M_PI * 0.25));
	} 
	// Normal walk/sprint bob.
	else {
		if(cl.stats[STAT_ZOOM] == 3)
			// sB was 1.8. just testing a slightly less exagerated value for preference sake :)
			sprint = 1.6; //this gets sprinting speed in comparison to walk speed per weapon

		//12.048 -> 4.3 = 100 -> 36ish, so replace 100 with 36
		if(which == 0)
			bob = cl_bobup.value * 24 * speed * (sprint * sprint) * sin((cl.time * 12.5 * sprint));//Pitch Bobbing 10
		else if(which == 1)
			bob = cl_bobside.value * 24 * speed * (sprint * sprint * sprint) * sin((cl.time * 6.25 * sprint) - (M_PI * 0.25));//Yaw Bobbing 5
	}

	return bob;
}

//===================================================== View Bobbing =====================================================
static int lastSound;
float PlayStepSound(void)
{
	float		num;
	int sound = 0;
	while(1)
	{
		num = (rand ()&0x7fff) / ((float)0x7fff);
		sound = (int)(num * 5);
		sound++;
		if(sound != lastSound)
			break;
	}

	if(sound == 1)
        S_StartSound (cl.viewentity, 0, cl_sfx_step[0], vec3_origin, 1.0f, 1.0f);
	else if(sound == 2)
        S_StartSound (cl.viewentity, 0, cl_sfx_step[1], vec3_origin, 1.0f, 1.0f);
	else if(sound == 3)
        S_StartSound (cl.viewentity, 0, cl_sfx_step[2], vec3_origin, 1.0f, 1.0f);
	else if(sound == 4)
        S_StartSound (cl.viewentity, 0, cl_sfx_step[3], vec3_origin, 1.0f, 1.0f);
	else if(sound == 5)
        S_StartSound (cl.viewentity, 0, cl_sfx_step[4], vec3_origin, 1.0f, 1.0f);

	lastSound = sound;
	return sound;
}
static int canStep;

float V_CalcVBob(float speed, float which)
{
	float bob = 0;
	//float speedMulti = (0.2 + sqrt((cl.velocity[0] * cl.velocity[0])	+	(cl.velocity[1] * cl.velocity[1])))/97; 	We're moving this to parent function to save calculations...
	//was going to multiply by speed in the sine function to step faster when you're moving faster... but it skipped around on points of the sine curve too much
	//It's be much more efficient to just have a constant step speed, though it would look really weird, it's the only way to have a constant step

	float sprint = 1;

	if(cl.stats[STAT_ZOOM] == 3)
		sprint = 1.8;

	if(cl.stats[STAT_ZOOM] == 2)
		return 0;

	if(sprint == 1)
	{
		if(which == 0)
			bob = speed * 8.6 * (1/sprint) * sin((cl.time * 12.5 * sprint));//10
		else if(which == 1)
			bob = speed * 8.6 * (1/sprint) * sin((cl.time * 6.25 * sprint) - (M_PI * 0.25));//5
		else if(which == 2)
			bob = speed * 8.6 * (1/sprint) * sin((cl.time * 6.25 * sprint) - (M_PI * 0.25));//5
	}
	else
	{
		if(which == 0)
			bob = speed * 8.6 * (1/sprint) * cos((cl.time * 6.25 * sprint));
		else if(which == 1)
			bob = speed * 8.6 * (1/sprint) * cos((cl.time * 12.5 * sprint));
		else if(which == 2)
			bob = speed * 8.6 * (1/sprint) * cos((cl.time * 6.25 * sprint));
	}


	if(speed > 0.1 && which == 0)
	{
		if(canStep && sin(cl.time * 12.5 * sprint) < -0.8)
		{
			PlayStepSound();
			canStep = 0;
		}
		if(sin(cl.time * 12.5 * sprint) > 0.9)
		{
			canStep = 1;
		}
	}
	return bob;
}


//=============================================================================


cvar_t	v_centermove = {"v_centermove", "0.15", false};
cvar_t	v_centerspeed = {"v_centerspeed","500"};


void V_StartPitchDrift (void)
{
#if 1
	if (cl.laststop == cl.time)
	{
		return;		// something else is keeping it from drifting
	}
#endif
	if (cl.nodrift || !cl.pitchvel)
	{
		cl.pitchvel = v_centerspeed.value;
		cl.nodrift = false;
		cl.driftmove = 0;
	}
}

void V_StopPitchDrift (void)
{
	cl.laststop = cl.time;
	cl.nodrift = true;
	cl.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when 
===============
*/
void V_DriftPitch (void)
{
	float		delta, move;

	if (noclip_anglehack || !cl.onground || cls.demoplayback )
	{
		cl.driftmove = 0;
		cl.pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (cl.nodrift)
	{
		if ( fabsf(cl.cmd.forwardmove) < cl_forwardspeed)
			cl.driftmove = 0;
		else
			cl.driftmove += host_frametime;
	
		if ( cl.driftmove > v_centermove.value)
		{
			V_StartPitchDrift ();
		}
		return;
	}
	
	delta = cl.idealpitch - cl.viewangles[PITCH];

	if (!delta)
	{
		cl.pitchvel = 0;
		return;
	}

	move = host_frametime * cl.pitchvel;
	cl.pitchvel += host_frametime * v_centerspeed.value;
	
//Con_Printf ("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			cl.pitchvel = 0;
			move = delta;
		}
		cl.viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			cl.pitchvel = 0;
			move = -delta;
		}
		cl.viewangles[PITCH] -= move;
	}
}





/*
============================================================================== 
 
						PALETTE FLASHES 
 
============================================================================== 
*/ 
 
 
cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

cvar_t		v_gamma = {"gamma", "1", true};

byte		gammatable[256];	// palette is sent through this

void BuildGammaTable (float g)
{
	int		i, inf;
	
	if (g == 1.0f)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}
	
	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * powf ( (i+0.5f)/255.5f , g ) + 0.5f;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}

/*
=================
V_CheckGamma
=================
*/
qboolean V_CheckGamma (void)
{
	static float oldgammavalue;
	
	if (v_gamma.value == oldgammavalue)
		return false;
	oldgammavalue = v_gamma.value;
	
	BuildGammaTable (v_gamma.value);
	vid.recalc_refdef = 1;				// force a surface cache flush
	
	return true;
}



/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (void)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	entity_t	*ent;
	float	side;
	float	count;
	
	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	count = blood*0.5f + armor*0.5f;
	if (count < 10)
		count = 10;

	cl.faceanimtime = cl.time + 0.2f;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)		
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

//
// calculate view angle kicks
//
	ent = &cl_entities[cl.viewentity];
	
	VectorSubtract (from, ent->origin, from);
	VectorNormalize (from);
	
	AngleVectors (ent->angles, forward, right, up);

	side = DotProduct (from, right);
	v_dmg_roll = count*side*v_kickroll.value;
	
	side = DotProduct (from, forward);
	v_dmg_pitch = count*side*v_kickpitch.value;

	v_dmg_time = v_kicktime.value;
}


/*
==================
V_cshift_f
==================
*/
void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = atoi(Cmd_Argv(1));
	cshift_empty.destcolor[1] = atoi(Cmd_Argv(2));
	cshift_empty.destcolor[2] = atoi(Cmd_Argv(3));
	cshift_empty.percent = atoi(Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	cl.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	cl.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	cl.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	cl.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case CONTENTS_LAVA:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		cl.cshifts[CSHIFT_CONTENTS] = cshift_water;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift (void)
{
	
}

/*
=============
V_HealthCshift
=============
*/
void V_HealthCshift (void)
{
	int pulse_value, pulseadd;
	float tempi1, tempi2, tempi3, tempi4;
	if (cl.stats[STAT_HEALTH] < 100 && cl.stats[STAT_HEALTH])
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;

		if (cl.stats[STAT_HEALTH] < 50)
		{
			pulse_value = abs(((int)(realtime*100)&100) - 50);
			pulseadd = 50;
		}
		else
		{
			pulse_value = abs(((int)(realtime*50)&20) - 10);
			pulseadd = 10;
		}
		tempi1 = cl.stats[STAT_HEALTH] + pulse_value;
		tempi2 = 100 + pulseadd;
		tempi3 = tempi1/tempi2;
		tempi4 = 200 - (tempi3*255);
		if (tempi4 < 0)
			tempi4 = 0;
		cl.cshifts[CSHIFT_DAMAGE].percent = (int)tempi4;
	}
	else
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
}

/* 
============================================================================== 
 
						VIEW RENDERING 
 
============================================================================== 
*/ 

float angledelta (float a)
{
	a = anglemod(a);
	if (a > 180)
		a -= 360;
	return a;
}
// in in_rollangle
// out last_roll
// amt 0.01

float lin_lerp (float lerp_in, float lerp_out, float smooth_amt) {
	return lerp_in + (smooth_amt * (lerp_in/lerp_out));
}

/*
==================
CalcGunAngle
==================
*/

//static float OldYawTheta;
//static float OldPitchTheta;

int lock_viewmodel; 
extern float centerdrift_offset_yaw;
extern float centerdrift_offset_pitch;
extern qboolean aimsnap;
static vec3_t cADSOfs;
void CalcGunAngle (void)
{	
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;
	
	//entity_t *vent, *vent2;
	
	//vent = &cl.viewent;
	//vent2 = &cl.viewent2;
	
	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4f;
	if (yaw > 10)
		yaw = 10;
	if (yaw < -10)
		yaw = -10;
	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4f;
	if (pitch > 10)
		pitch = 10;
	if (pitch < -10)
		pitch = -10;
	move = host_frametime*20;
	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}
	
	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}
	
	oldyaw = yaw;
	oldpitch = pitch;
	
	//=========Strafe-Roll=========
	//Creating backup
	CWeaponRot[PITCH] = cl.viewent.angles[PITCH] * -1;
	CWeaponRot[YAW] = cl.viewent.angles[YAW] * -1;
	CWeaponRot[ROLL] = cl.viewent.angles[ROLL] * -1;
	/*
	vec3_t	vieworgrest;
	entity_t *vieworg;
	
	vieworg = &cl_entities[cl.viewentity];
	
	// sBTODO test refdef vieworg
	vieworgrest[0] = vieworg->origin[0];
	vieworgrest[1] = vieworg->origin[1];
	vieworgrest[2] = vieworg->origin[2];
	
	vieworg->origin[0] = vieworg->origin[0] - 50.0f;
	vieworg->origin[1] = vieworg->origin[1] - 30.0f;
	
	//Con_Printf ("old: %f %f %f new: %f %f %f\n", vieworgrest[0], vieworgrest[1], vieworgrest[2], vieworg->origin[0], vieworg->origin[1], vieworg->origin[2]);
	*/
	float side;
	side = V_CalcRoll (cl_entities[cl.viewentity].angles, cl.velocity);
	cl.viewent.angles[ROLL] = angledelta(cl.viewent.angles[ROLL] - ((cl.viewent.angles[ROLL] - (side * 5)) * 0.5));
	
	float xcrossnormal;
	float ycrossnormal;
	
	xcrossnormal = (cl_crossx.value / (vid.width/2) * IR_YAWRANGE);
	ycrossnormal = (cl_crossy.value / (vid.height/2) * IR_PITCHRANGE);
	
	//Con_Printf ("x: %f", xcrossnormal);
	//Con_Printf ("  y: %f", ycrossnormal);
	
	float roll_og_pos;
	float inroll_smooth;
	float last_roll = 0.0f;
	static float smooth_amt = 0.001f;
	
	if (aimsnap == false && !(cl.stats[STAT_ZOOM] == 1 && ads_center.value) && lock_viewmodel != 1 && !(cl.stats[STAT_ZOOM] == 2 && sniper_center.value))
	{
		cl.viewent.angles[YAW] = (r_refdef.viewangles[YAW]/* + yaw*/) - (xcrossnormal);
		
		//cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + yaw - ((cl_crossx.value/scr_vrect.width * IR_YAWRANGE) * (centerdrift_offset_yaw) - OldYawTheta);
		
		//cl.viewent.angles[PITCH] = - r_refdef.viewangles[PITCH] + pitch + ((cl_crossy.value/scr_vrect.height * IR_PITCHRANGE) * (centerdrift_offset_pitch*-1));
		cl.viewent.angles[PITCH] = -(r_refdef.viewangles[PITCH]/* + pitch*/) + (ycrossnormal)*-1;
		//guMtxTrans(temp, viewmod->origin[0] - (r_refdef.viewangles[PITCH]/* + pitch*/) + (ycrossnormal * scr_vrect.width)*-1, viewmod->origin[1], viewmod->origin[2]);
		
		//Con_Printf("YAW:%f PITCH%f\n", cl.viewent.angles[YAW], -cl.viewent.angles[PITCH]);
		//Con_Printf ("viewent origin: %f %f %f\n", cl.viewent.origin[0], cl.viewent.origin[1], cl.viewent.origin[2]);
		
		if (cl_weapon_inrollangle.value) {
			if (cl.stats[STAT_ZOOM] == 0) {	
				
				roll_og_pos = in_rollangle;
				
				inroll_smooth = /*lin_lerp (in_rollangle, last_roll, smooth_amt)*/roll_og_pos;	
				
				if(inroll_smooth > 24.5f)
					inroll_smooth = 24.5f;
				else if(inroll_smooth < -24.5f)
					inroll_smooth = -24.5f;
				
				//Con_Printf("roll: %f\n", inroll_smooth);
				cl.viewent.angles[ROLL] = inroll_smooth;
				last_roll = roll_og_pos;
			}
		}
	}
	else
	{
		// ELUTODO: Maybe there are other cases besides demo playback
		if (lock_viewmodel != 1) {
			cl.viewent.angles[YAW] = r_refdef.viewangles[YAW] + yaw;
			cl.viewent.angles[PITCH] = - (r_refdef.viewangles[PITCH] + pitch);
			//Con_Printf ("viewent origin: %f %f %f\n", cl.viewent.origin[0], cl.viewent.origin[1], cl.viewent.origin[2]);
		} else {
			cl.viewent.angles[YAW] = r_refdef.viewangles[YAW];
			cl.viewent.angles[PITCH] = - (r_refdef.viewangles[PITCH]);
			//Con_Printf ("viewent origin: %f %f %f\n", cl.viewent.origin[0], cl.viewent.origin[1], cl.viewent.origin[2]);
		}	
	}
	
/*
	cl.viewent.angles[ROLL] -= v_idlescale.value * sinf(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	cl.viewent.angles[PITCH] -= v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	cl.viewent.angles[YAW] -= v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;

	//^^^ Model swaying
	if(cl.stats[STAT_ZOOM] == 1)
	{
		cl.viewent.angles[YAW] = (r_refdef.viewangles[YAW] + yaw) - ((angledelta((r_refdef.viewangles[YAW] + yaw) - OldYawTheta ) * 0.3));//0.6
	}
	else
	{
		cl.viewent.angles[YAW] = (r_refdef.viewangles[YAW] + yaw) - (angledelta((r_refdef.viewangles[YAW] + yaw) - OldYawTheta ) * 0.6);//0.6
	}

	cl.viewent.angles[PITCH] = -1 * ((r_refdef.viewangles[PITCH] + pitch) - (angledelta((r_refdef.viewangles[PITCH] + pitch) + OldPitchTheta ) * 0.2));
*/
	//BLUBS STOPS HERE
	
	//OldYawTheta = cl.viewent.angles[YAW];
	//OldPitchTheta = cl.viewent.angles[PITCH];
	
	//readd this
	cl.viewent2.angles[ROLL] = cl.viewent.angles[ROLL] -= v_idlescale.value * sinf(cl.time*v_iroll_cycle.value * 2) * v_iroll_level.value;
	cl.viewent2.angles[PITCH] = cl.viewent.angles[PITCH] -= v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value * 2) * v_ipitch_level.value;
	cl.viewent2.angles[YAW] = cl.viewent.angles[YAW] -= v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value * 2) * v_iyaw_level.value;
	
	
	//Evaluating total offset
	CWeaponRot[PITCH] -= cl.viewent.angles[PITCH];
	CWeaponRot[YAW] += cl.viewent.angles[YAW];
	CWeaponRot[ROLL] += cl.viewent.angles[ROLL];
	
	//reset vieworg after rotation
	/*
	vieworg->origin[0] = vieworgrest[0];
	vieworg->origin[1] = vieworgrest[1];
	*/
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (void)
{
	entity_t	*ent;
	
	ent = &cl_entities[cl.viewentity];

// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (r_refdef.vieworg[0] < ent->origin[0] - 14)
		r_refdef.vieworg[0] = ent->origin[0] - 14;
	else if (r_refdef.vieworg[0] > ent->origin[0] + 14)
		r_refdef.vieworg[0] = ent->origin[0] + 14;
	if (r_refdef.vieworg[1] < ent->origin[1] - 14)
		r_refdef.vieworg[1] = ent->origin[1] - 14;
	else if (r_refdef.vieworg[1] > ent->origin[1] + 14)
		r_refdef.vieworg[1] = ent->origin[1] + 14;
	if (r_refdef.vieworg[2] < ent->origin[2] - 22)
		r_refdef.vieworg[2] = ent->origin[2] - 22;
	else if (r_refdef.vieworg[2] > ent->origin[2] + 32)
		r_refdef.vieworg[2] = ent->origin[2] + 32;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (void)
{
	r_refdef.viewangles[ROLL] += v_idlescale.value * sinf(cl.time*v_iroll_cycle.value) * v_iroll_level.value;
	r_refdef.viewangles[PITCH] += v_idlescale.value * sinf(cl.time*v_ipitch_cycle.value) * v_ipitch_level.value;
	r_refdef.viewangles[YAW] += v_idlescale.value * sinf(cl.time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (void)
{
	float		side;
		
	side = V_CalcRoll (cl_entities[cl.viewentity].angles, cl.velocity);
	r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		r_refdef.viewangles[ROLL] += v_dmg_time/v_kicktime.value*v_dmg_roll;
		r_refdef.viewangles[PITCH] += v_dmg_time/v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= host_frametime;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef (void)
{
	entity_t	*ent, *view, *view2;
	float		old;

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;
	view2 = &cl.viewent2;

	VectorCopy (ent->origin, r_refdef.vieworg);
	VectorCopy (ent->angles, r_refdef.viewangles);
	view->model = NULL;
	view2->model = NULL;

// allways idle in intermission
	old = v_idlescale.value;
	v_idlescale.value = 1;
	V_AddIdle ();
	v_idlescale.value = old;
}

/*
==================
V_CalcRefdef

==================
*/
static float lastUpVelocity;
static float VerticalOffset;
static float cVerticalOffset;
vec3_t CWeaponOffset;//blubs declared this
vec3_t CWeaponRot;

extern double crosshair_spread_time;
void DropRecoilKick (void)
{
	float	len;

	if (crosshair_spread_time > sv.time)
		return;
	len = VectorNormalize (cl.gun_kick);

	//Con_Printf ("len = %f\n",len);
	len = len - 5*host_frametime;
	if (len < 0)
		len = 0;
	VectorScale (cl.gun_kick, len, cl.gun_kick);
	//Con_Printf ("len final = %f\n",len);
}

vec3_t lastPunchAngle;
void V_CalcRefdef (void)
{
	entity_t	*ent, *view, *view2;
	int			i;
	vec3_t		forward, right, up;
	vec3_t		angles;
	
	static float oldz = 0;

	//V_DriftPitch ();
	DropRecoilKick();

// ent is the player model (visible when out of body)
	ent = &cl_entities[cl.viewentity];
// view is the weapon model (only visible from inside body)
	view = &cl.viewent;
	view2 = &cl.viewent2;

// transform the view offset by the model's matrix to get the offset from
// model origin for the view
	ent->angles[YAW] = cl.viewangles[YAW];	// the model should face
										// the view dir
	ent->angles[PITCH] = -cl.viewangles[PITCH];	// the model should face
										// the view dir
	
	//Blubs Bobbing calculations were here, moved them down to be right above bob code block

// refresh position
	VectorCopy (ent->origin, r_refdef.vieworg);
	r_refdef.vieworg[2] += cl.viewheight;//blubs removed "+ bob", it's added again below actually...
	
// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	
	r_refdef.vieworg[0] += 1.0/32;
	r_refdef.vieworg[1] += 1.0/32;
	r_refdef.vieworg[2] += 1.0/32;
	

	VectorCopy (cl.viewangles, r_refdef.viewangles);
	//V_CalcViewRoll ();
	V_AddIdle ();

// offsets

	angles[PITCH] = -ent->angles[PITCH];	// because entity pitches are								
	angles[YAW] = ent->angles[YAW];			//  actually backward
	angles[ROLL] = ent->angles[ROLL];

	AngleVectors (angles, forward, right, up);

	for (i=0 ; i<3 ; i++)
		r_refdef.vieworg[i] += scr_ofsx.value*forward[i]
			+ scr_ofsy.value*right[i]
			+ scr_ofsz.value*up[i];
	
	V_BoundOffsets ();
		
// set up gun position

	VectorCopy (cl.viewangles, view->angles);
	//cl.oldviewangles = cl.viewangles;
	
	CalcGunAngle ();
	
	view->angles[PITCH] = view->angles[PITCH] - cl.gun_kick[PITCH];
	view->angles[YAW] = view->angles[YAW] + cl.gun_kick[YAW];
	VectorCopy (ent->origin, view->origin);
	view->origin[2] += cl.viewheight;
	
	//Storing base location, later to calculate total offset
		CWeaponOffset[0]= view->origin[0] * -1;
		CWeaponOffset[1]= view->origin[1] * -1;
		CWeaponOffset[2]= view->origin[2] * -1;
		
		
	//Angle Vectors used by landing and iron sights, so do it here
	vec3_t		temp_up,temp_right,temp_forward;
	AngleVectors (r_refdef.viewangles,temp_forward, temp_right, temp_up);
	//============================================================ Fall Landing Buffering ============================================================
	if(lastUpVelocity < cl.velocity[2] - 5)//We've had a dramatic change in velocity
	{
		VerticalOffset = (lastUpVelocity - cl.velocity[2])/25;
		if(VerticalOffset < -15)
		{
			VerticalOffset = -15;
		}
	}

	cVerticalOffset += (VerticalOffset - cVerticalOffset) * 0.3;

	temp_up[0] *= cVerticalOffset;
	temp_up[1] *= cVerticalOffset;
	temp_up[2] *= cVerticalOffset;

	view->origin[0] +=(temp_up[0]);
	view->origin[1] +=(temp_up[1]);
	view->origin[2] +=(temp_up[2]);

	if(cVerticalOffset > VerticalOffset - 2 && cVerticalOffset < VerticalOffset + 2)//Close enough to goal
	{
		VerticalOffset = 0;
	}
	lastUpVelocity = cl.velocity[2];

	//============================================================ Engine-Side Iron Sights ============================================================
	AngleVectors (r_refdef.viewangles, temp_forward, temp_right, temp_up);

	vec3_t ADSOffset;
	if(cl.stats[STAT_ZOOM] == 1 || cl.stats[STAT_ZOOM] == 2)
	{
		ADSOffset[0] = sv_player->v.ADS_Offset[0];
		ADSOffset[1] = sv_player->v.ADS_Offset[1];
		ADSOffset[2] = sv_player->v.ADS_Offset[2];
		
		ADSOffset[0] = ADSOffset[0]/1000;
		ADSOffset[1] = ADSOffset[1]/1000;
		ADSOffset[2] = ADSOffset[2]/1000;
	}
	else
	{
		ADSOffset[0] = 0;
		ADSOffset[1] = 0;
		ADSOffset[2] = 0;
	}
	//Side offset
	cADSOfs [0] += (ADSOffset[0] - cADSOfs[0]) * 0.25;
	cADSOfs [1] += (ADSOffset[1] - cADSOfs[1]) * 0.25;
	cADSOfs [2] += (ADSOffset[2] - cADSOfs[2]) * 0.25;
	
	temp_right[0] *= cADSOfs[0];
	temp_right[1] *= cADSOfs[0];
	temp_right[2] *= cADSOfs[0];

	temp_up[0] *= cADSOfs[1];
	temp_up[1] *= cADSOfs[1];
	temp_up[2] *= cADSOfs[1];

	temp_forward[0] *= cADSOfs[2];
	temp_forward[1] *= cADSOfs[2];
	temp_forward[2] *= cADSOfs[2];

	view->origin[0] +=(temp_forward[0] + temp_right[0] + temp_up[0]);
	view->origin[1] +=(temp_forward[1] + temp_right[1] + temp_up[1]);
	view->origin[2] +=(temp_forward[2] + temp_right[2] + temp_up[2]);
	
	float speed = (0.2 + sqrt((cl.velocity[0] * cl.velocity[0])	+	(cl.velocity[1] * cl.velocity[1])));
	speed = speed/190;

	float		bob, bobside = 0;

	if (cl_sidebobbing.value)
		bobside = V_CalcBob(speed,1);
	bob = V_CalcBob (speed,0);

	//============================ Weapon Bobbing Code Block=================================
	for (i=0 ; i<3 ; i++)
	{
		if (cl_sidebobbing.value)
		{
			view->origin[i] += right[i]*bobside*0.4;
			view->origin[i] += up[i]*bob*0.5;
//			view2->origin[i] += right[i]*bobside*0.2;
//			view2->origin[i] += up[i]*bob*0.2;
//			mz->origin[i] += right[i]*bobside*0.2;
//			mz->origin[i] += up[i]*bob*0.2;
		}
		else
		{
			view->origin[i] += forward[i]*bob*0.4;
//			view2->origin[i] += forward[i]*bob*0.4;
//			mz->origin[i] += forward[i]*bob*0.4;
		}
	}
	view->origin[2] += bob;

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV

//=============================== Added View Bobbing Code Block (Blubs wuz here)=======================
	vec3_t vbob;
	vbob[0] = V_CalcVBob(speed,0) * cl_bob.value * 30;//cl_bob * 50 undo each other, but we want to give some control to people to limit view bobbing
	vbob[1] = V_CalcVBob(speed,1) * cl_bob.value * 30;
	vbob[2] = V_CalcVBob(speed,2) * cl_bob.value * 30;

	r_refdef.viewangles[YAW] = angledelta(r_refdef.viewangles[YAW] + (vbob[0] * 0.1));
	r_refdef.viewangles[PITCH] = angledelta(r_refdef.viewangles[PITCH] + (vbob[1] * 0.1));
	r_refdef.viewangles[ROLL] = anglemod(r_refdef.viewangles[ROLL] + (vbob[2] * 0.07));
		
	//Here we finally set CWeaponOffset by the total weapon model offset, used for mzfl which uses CWeaponOffset variable.
		CWeaponOffset[0] += view->origin[0];
		CWeaponOffset[1] += view->origin[1];
		CWeaponOffset[2] += view->origin[2];
//I don't know what the comments below this are, but blubs didn't add them...

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV

	view->model = cl.model_precache[cl.stats[STAT_WEAPON]];
	view->frame = cl.stats[STAT_WEAPONFRAME];
	view->skinnum = cl.stats[STAT_WEAPONSKIN];
	view->colormap = vid.colormap;
	
	view2->model = cl.model_precache[cl.stats[STAT_WEAPON2]];
	view2->frame = cl.stats[STAT_WEAPON2FRAME];
	view2->skinnum = cl.stats[STAT_WEAPON2SKIN];
	view2->colormap = vid.colormap;
// set up the refresh position
	
	//blubs's punchangle interpolation
	lastPunchAngle[0] += (cl.punchangle[0] - lastPunchAngle[0]) * 0.5;
	lastPunchAngle[1] += (cl.punchangle[1] - lastPunchAngle[1]) * 0.5;
	lastPunchAngle[2] += (cl.punchangle[2] - lastPunchAngle[2]) * 0.5;
	//VectorCopy(cl.punchangle,lastPunchAngle);

	// set up the refresh position
	VectorAdd (r_refdef.viewangles, lastPunchAngle, r_refdef.viewangles);
	
	VectorAdd (r_refdef.viewangles, cl.gun_kick, r_refdef.viewangles);

	// smooth out stair step ups
	if (cl.onground && ent->origin[2] - oldz > 0)
	{
		float steptime;
	
		steptime = cl.time - cl.oldtime;
		if (steptime < 0) {
			//FIXME		I_Error ("steptime < 0");
			steptime = 0;
		}

		oldz += steptime * 80;
		if (oldz > ent->origin[2]) {
			oldz = ent->origin[2];
		}
		if (ent->origin[2] - oldz > 12) {
			oldz = ent->origin[2] - 12;
		}
		r_refdef.vieworg[2] += oldz - ent->origin[2];
		view->origin[2] += oldz - ent->origin[2];
	} else {
		oldz = ent->origin[2];

		if (chase_active.value)
			Chase_Update ();
		
		view2->origin[0] = view->origin[0];
		view2->origin[1] = view->origin[1];
		view2->origin[2] = view->origin[2];

		view2->angles[0] = view->angles[0];
		view2->angles[1] = view->angles[1];
		view2->angles[2] = view->angles[2];
	}

}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
void V_RenderView (void)
{
	if (con_forcedup)
		return;

	if (!cl.paused && key_dest == key_game)
		V_CalcRefdef ();

	R_PushDlights ();

	if (lcd_x.value)
	{
		//
		// render two interleaved views
		//
		int		i;

		vid.rowbytes <<= 1;
		vid.aspect *= 0.5f;

		r_refdef.viewangles[YAW] -= lcd_yaw.value;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] -= right[i]*lcd_x.value;
		R_RenderView ();

		vid.buffer += vid.rowbytes>>1;

		R_PushDlights ();

		r_refdef.viewangles[YAW] += lcd_yaw.value*2;
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg[i] += 2*right[i]*lcd_x.value;
		R_RenderView ();

		vid.buffer -= vid.rowbytes>>1;

		r_refdef.vrect.height <<= 1;

		vid.rowbytes >>= 1;
		vid.aspect *= 2;
	}
	else
	{
		R_RenderView ();
	}
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("v_cshift", V_cshift_f);	
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("centerview", V_StartPitchDrift);

	Cvar_RegisterVariable (&lcd_x);
	Cvar_RegisterVariable (&lcd_yaw);

	Cvar_RegisterVariable (&v_centermove);
	Cvar_RegisterVariable (&v_centerspeed);

	Cvar_RegisterVariable (&v_iyaw_cycle);
	Cvar_RegisterVariable (&v_iroll_cycle);
	Cvar_RegisterVariable (&v_ipitch_cycle);
	Cvar_RegisterVariable (&v_iyaw_level);
	Cvar_RegisterVariable (&v_iroll_level);
	Cvar_RegisterVariable (&v_ipitch_level);

	Cvar_RegisterVariable (&v_idlescale);
	Cvar_RegisterVariable (&crosshair);
	Cvar_RegisterVariable (&cl_crossx);
	Cvar_RegisterVariable (&cl_crossy);

	Cvar_RegisterVariable (&scr_ofsx);
	Cvar_RegisterVariable (&scr_ofsy);
	Cvar_RegisterVariable (&scr_ofsz);
	Cvar_RegisterVariable (&cl_rollspeed);
	Cvar_RegisterVariable (&cl_rollangle);
	Cvar_RegisterVariable (&cl_bob);
	Cvar_RegisterVariable (&cl_bobcycle);
	Cvar_RegisterVariable (&cl_bobup);

	Cvar_RegisterVariable (&v_kicktime);
	Cvar_RegisterVariable (&v_kickroll);
	Cvar_RegisterVariable (&v_kickpitch);

	Cvar_RegisterVariable (&cl_weapon_inrollangle);
	
	BuildGammaTable (1.0f);	// no gamma yet
	Cvar_RegisterVariable (&v_gamma);
}


