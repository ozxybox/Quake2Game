// client.h -- primary header for client

#pragma once

#include "../shared/engine.h"
#include "../renderer/ref_public.h"
#include "../../game/client/cg_public.h"

#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "input.h"
#include "cl_keys.h"
#include "cl_console.h"
#include "cdaudio.h"

#include "keycodes.h"

//=============================================================================

extern centity_t	cl_entities[MAX_EDICTS];

struct frame_t
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
};

#define MAX_CLIENTWEAPONMODELS		20		// PGM -- upped from 16 to fit the chainfist vwep

struct clientinfo_t
{
	char		name[MAX_QPATH];
	char		cinfo[MAX_QPATH];
	material_t	*skin;
	material_t	*icon;
	char		iconname[MAX_QPATH];
	model_t		*model;
	model_t		*weaponmodel[MAX_CLIENTWEAPONMODELS];
};

extern char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
extern int num_cl_weaponmodels;

//
// the client_state_t structure is wiped completely at every
// server map change
//
struct client_state_t
{
	int			timeoutcount;

	int			timedemo_frames;
	int			timedemo_start;

	qboolean	refresh_prepped;	// false if on new level or new ref dll
	qboolean	sound_prepped;		// ambient sounds can start
	qboolean	force_refdef;		// vid has changed, so we can't use a paused refdef

	int			parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings
	vec3_t		predicted_origins[CMD_BACKUP];	// for debug comparing against server

	float		predicted_step;				// for stair up smoothing
	unsigned	predicted_step_time;

	vec3_t		predicted_origin;	// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		prediction_error;

	frame_t		frame;				// received from server
	int			surpressCount;		// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			time;			// this is the time value that the client
								// is rendering at.  always <= cls.realtime
	float		lerpfrac;		// between oldframe and frame

	refdef_t	refdef;

	vec3_t		v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//
	char		layout[1024];		// general 2D overlay
	int			inventory[MAX_ITEMS];

	//
	// non-gameserver infornamtion
	// FIXME: move this cinematic stuff into the cin_t structure
	FILE		*cinematic_file;
	int			cinematictime;		// cls.realtime for first cinematic frame
	int			cinematicframe;
	byte		cinematicpalette[768];
	qboolean	cinematicpalette_active;

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;	// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	//
	model_t		*model_draw[MAX_MODELS];
	cmodel_t	*model_clip[MAX_MODELS];

	sfx_t		*sound_precache[MAX_SOUNDS];
//	image_t		*image_precache[MAX_IMAGES]; // SlartMaterialSystemTodo

	clientinfo_t	clientinfo[MAX_CLIENTS];
	clientinfo_t	baseclientinfo;
};

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

enum connstate_t {
	ca_uninitialized,
	ca_disconnected, 	// not talking to a server
	ca_connecting,		// sending request packets to the server
	ca_connected,		// netchan_t established, waiting for svc_serverdata
	ca_active			// game views should be displayed
};

enum dltype_t {
	dl_none,
	dl_model,
	dl_sound,
	dl_skin,
	dl_single
};				// download type

enum keydest_t {key_game, key_console, key_message, key_menu};

struct client_static_t
{
	connstate_t	state;
	keydest_t	key_dest;

	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametime;			// seconds since last frame

// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
	dltype_t	downloadtype;
	int			downloadpercent;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	FILE		*demofile;
};

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_add_blend;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_footsteps;
extern	cvar_t	*cl_noskins;
extern	cvar_t	*cl_autoskins;

extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;

extern	cvar_t	*sensitivity;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;

extern	cvar_t	*cl_lightlevel;	// FIXME HACK

extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;

extern	cvar_t	*cl_vwep;

// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;

void DrawString (int x, int y, const char *s);
void DrawAltString (int x, int y, const char *s);	// toggle high bit
qboolean	CL_CheckOrDownloadFile (const char *filename);

void CL_AddNetgraph (void);

//=================================================

int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, uint bits);
void CL_ParseFrame (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);

void CL_Quit_f (void);


//
// cl_cgame
//
extern cgame_export_t	*cge;

void CL_InitCGame();
void CL_ShutdownCGame();

//
// cl_main
//
void CL_Init (void);

void CL_FixUpGender(void);
void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_RequestNextDownload (void);

//
// cl_input
//
struct kbutton_t
{
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
};

extern kbutton_t in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);

void CL_ClearState (void);

void CL_ReadPackets (void);

void CL_BaseMove (usercmd_t *cmd);

void IN_CenterView (void);

float CL_KeyState (kbutton_t *key);
const char *Key_KeynumToString (int keynum);

//
// cl_ents.c
//
void CL_ParseFrame (void);
void CL_AddEntities (void);

//
// cl_demo.c
//
void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern	const char *svc_strings[256];

void CL_ParseServerMessage (void);
void CL_LoadClientinfo (clientinfo_t *ci, char *s);
void SHOWNET(const char *s);
void CL_ParseClientinfo (int player);
void CL_Download_f (void);

//
// cl_view.c
//
extern	int			gun_frame;
extern	model_t		*gun_model;

void V_Init (void);
void V_RenderView (void);
void V_AddEntity (entity_t *ent);
void V_AddParticle (vec3_t org, int color, float alpha);
void V_AddLight (vec3_t org, float intensity, float r, float g, float b);
void V_AddLightStyle (int style, float r, float g, float b);

//
// cl_pred.c
//
void CL_CheckPredictionError (void);
void CL_PredictMovement (void);

//
// menus
//
void M_Init (void);
void M_Shutdown (void);
void M_Keydown (int key);
void M_Draw (void);
void M_Menu_Main_f (void);
void M_ForceMenuOff (void);
void M_AddToServerList (netadr_t adr, char *info);

//
// cl_inv.c
//
void CL_ParseInventory (void);
void CL_DrawInventory (void);

//-------------------------------------------------------------------------------------------------
// Use defines for all exported functions
// This allows us to use the same names for both static links and dynamic links
// We could also use forceinline functions
//-------------------------------------------------------------------------------------------------

#define R_RegisterPic			Draw_FindPic

#define R_DrawGetPicSize		Draw_GetPicSize
#define R_DrawPic				Draw_Pic
#define R_DrawStretchPic		Draw_StretchPic
#define R_DrawChar				Draw_Char
#define R_DrawTileClear			Draw_TileClear
#define R_DrawFill				Draw_Fill
#define R_DrawFadeScreen		Draw_FadeScreen
#define R_DrawStretchRaw		Draw_StretchRaw

#define R_AppActivate			GLimp_AppActivate
