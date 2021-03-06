
#include "sv_local.h"

/*
===================================================================================================

	OPERATOR CONSOLE ONLY COMMANDS

	These commands can only be entered from stdin or by a remote operator datagram

===================================================================================================
*/

/*
========================
SV_SetMaster_f

Specify a list of master servers
========================
*/
static void SV_SetMaster_f()
{
	int i, slot;

	// only dedicated servers send heartbeats
	if ( !dedicated->value ) {
		Com_Print( "Only dedicated servers use masters.\n" );
		return;
	}

	// make sure the server is listed public
	Cvar_Set( "public", "1" );

	for ( i = 1; i < MAX_MASTERS; i++ ) {
		memset( &master_adr[i], 0, sizeof( master_adr[i] ) );
	}

	slot = 1;		// slot 0 will always contain the id master
	for ( i = 1; i < Cmd_Argc(); i++ )
	{
		if ( slot == MAX_MASTERS )
			break;

		if ( !NET_StringToNetadr( Cmd_Argv( i ), master_adr[i] ) )
		{
			Com_Printf( "Bad address: %s\n", Cmd_Argv( i ) );
			continue;
		}
		if ( master_adr[slot].port == 0 )
			master_adr[slot].port = BigShort( PORT_MASTER );

		Com_Printf( "Master server at %s\n", NET_NetadrToString( master_adr[slot] ) );

		Com_Print( "Sending a ping.\n" );

		Netchan_OutOfBandPrint( NS_SERVER, master_adr[slot], "ping" );

		slot++;
	}

	svs.last_heartbeat = -9999999;
}

/*
========================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
========================
*/
static bool SV_SetPlayer()
{
	client_t *cl;
	int i;
	int idnum;
	char *s;

	if ( Cmd_Argc() < 2 )
		return false;

	s = Cmd_Argv( 1 );

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' )
	{
		idnum = atoi( Cmd_Argv( 1 ) );
		if ( idnum < 0 || idnum >= maxclients->value )
		{
			Com_Printf( "Bad client slot: %i\n", idnum );
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if ( !sv_client->state )
		{
			Com_Printf( "Client %i is not active\n", idnum );
			return false;
		}
		return true;
	}

	// check for a name match
	for ( i = 0, cl = svs.clients; i < maxclients->value; i++, cl++ )
	{
		if ( !cl->state )
			continue;
		if ( !Q_strcmp( cl->name, s ) )
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_Printf( "Userid %s is not on the server\n", s );
	return false;
}

/*
===================================================================================================

	Savegame files

===================================================================================================
*/

/*
========================
SV_WipeSavegame

Delete save/<XXX>/
========================
*/
static void SV_WipeSavegame( const char *savename )
{
	char name[MAX_OSPATH];
	char *s;

	Com_DPrintf( "SV_WipeSaveGame(%s)\n", savename );

	Q_sprintf_s( name, "%s/save/%s/server.ssv", FS_Gamedir(), savename );
	Sys_DeleteFile( name );
	Q_sprintf_s( name, "%s/save/%s/game.ssv", FS_Gamedir(), savename );
	Sys_DeleteFile( name );

	Q_sprintf_s( name, "%s/save/%s/*.sav", FS_Gamedir(), savename );
	s = Sys_FindFirst( name, 0, 0 );
	while ( s )
	{
		Sys_DeleteFile( s );
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose();
	Q_sprintf_s( name, "%s/save/%s/*.sv2", FS_Gamedir(), savename );
	s = Sys_FindFirst( name, 0, 0 );
	while ( s )
	{
		Sys_DeleteFile( s );
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose();
}

/*
========================
SV_CopySaveGame
========================
*/
static void SV_CopySaveGame( const char *src, const char *dst )
{
	char name[MAX_OSPATH], name2[MAX_OSPATH];
	strlen_t l, len;
	char *found;

	Com_DPrintf( "SV_CopySaveGame(%s, %s)\n", src, dst );

	SV_WipeSavegame( dst );

	// copy the savegame over
	Q_sprintf_s( name, "%s/save/%s/server.ssv", FS_Gamedir(), src );
	Q_sprintf_s( name2, "%s/save/%s/server.ssv", FS_Gamedir(), dst );
	FS_CreatePath( name2 );
	Sys_CopyFile( name, name2 );

	Q_sprintf_s( name, "%s/save/%s/game.ssv", FS_Gamedir(), src );
	Q_sprintf_s( name2, "%s/save/%s/game.ssv", FS_Gamedir(), dst );
	Sys_CopyFile( name, name2 );

	Q_sprintf_s( name, "%s/save/%s/", FS_Gamedir(), src );
	len = Q_strlen( name );
	Q_sprintf_s( name, "%s/save/%s/*.sav", FS_Gamedir(), src );
	found = Sys_FindFirst( name, 0, 0 );
	while ( found )
	{
		strcpy( name + len, found + len );

		Q_sprintf_s( name2, "%s/save/%s/%s", FS_Gamedir(), dst, found + len );
		Sys_CopyFile( name, name2 );

		// change sav to sv2
		l = Q_strlen( name );
		strcpy( name + l - 3, "sv2" );
		l = Q_strlen( name2 );
		strcpy( name2 + l - 3, "sv2" );
		Sys_CopyFile( name, name2 );

		found = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose();
}

/*
========================
SV_WriteLevelFile
========================
*/
static void SV_WriteLevelFile()
{
	char name[MAX_OSPATH];
	FILE *f;

	Com_DPrintf( "SV_WriteLevelFile()\n" );

	Q_sprintf_s( name, "%s/save/current/%s.sv2", FS_Gamedir(), sv.name );
	f = fopen( name, "wb" );
	if ( !f )
	{
		Com_Printf( "Failed to open %s\n", name );
		return;
	}
	fwrite( sv.configstrings, sizeof( sv.configstrings ), 1, f );
	CM_WritePortalState( f );
	fclose( f );

	Q_sprintf_s( name, "%s/save/current/%s.sav", FS_Gamedir(), sv.name );
	ge->WriteLevel( name );
}

/*
========================
SV_ReadLevelFile
========================
*/
void SV_ReadLevelFile()
{
	char name[MAX_OSPATH];
	FILE *f;

	Com_DPrint( "SV_ReadLevelFile()\n" );

	Q_sprintf_s( name, "%s/save/current/%s.sv2", FS_Gamedir(), sv.name );
	f = fopen( name, "rb" );
	if ( !f )
	{
		Com_Printf( "Failed to open %s\n", name );
		return;
	}
	FS_Read( sv.configstrings, sizeof( sv.configstrings ), f );
	CM_ReadPortalState( f );
	fclose( f );

	Q_sprintf_s( name, "%s/save/current/%s.sav", FS_Gamedir(), sv.name );
	ge->ReadLevel( name );
}

/*
========================
SV_WriteServerFile
========================
*/
static void SV_WriteServerFile( qboolean autosave )
{
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf("SV_WriteServerFile(%s)\n", autosave ? "true" : "false");

	Q_sprintf_s (name, "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "wb");
	if (!f)
	{
		Com_Printf ("Couldn't write %s\n", name);
		return;
	}
	// write the comment field
	memset (comment, 0, sizeof(comment));

	if (!autosave)
	{
		time (&aclock);
		newtime = localtime (&aclock);
		Q_sprintf_s (comment, "%2i:%i%i %2i/%2i  ", newtime->tm_hour
			, newtime->tm_min/10, newtime->tm_min%10,
			newtime->tm_mon+1, newtime->tm_mday);
		strncat (comment, sv.configstrings[CS_NAME], sizeof(comment)-1-strlen(comment) );
	}
	else
	{	// autosaved
		Q_sprintf_s (comment, "ENTERING %s", sv.configstrings[CS_NAME]);
	}

	fwrite (comment, 1, sizeof(comment), f);

	// write the mapcmd
	fwrite (svs.mapcmd, 1, sizeof(svs.mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (var = cvar_vars ; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;
		if (strlen(var->name) >= sizeof(name)-1
			|| strlen(var->string) >= sizeof(string)-1)
		{
			Com_Printf ("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		memset (name, 0, sizeof(name));
		memset (string, 0, sizeof(string));
		strcpy (name, var->name);
		strcpy (string, var->string);
		fwrite (name, 1, sizeof(name), f);
		fwrite (string, 1, sizeof(string), f);
	}

	fclose (f);

	// write game state
	Q_sprintf_s (name, "%s/save/current/game.ssv", FS_Gamedir());
	ge->WriteGame (name, autosave);
}

/*
========================
SV_ReadServerFile
========================
*/
static void SV_ReadServerFile()
{
	FILE *f;
	char name[MAX_OSPATH], string[128];
	char comment[32];
	char mapcmd[MAX_TOKEN_CHARS];

	Com_DPrint( "SV_ReadServerFile()\n" );

	Q_sprintf_s( name, "%s/save/current/server.ssv", FS_Gamedir() );
	f = fopen( name, "rb" );
	if ( !f )
	{
		Com_Printf( "Couldn't read %s\n", name );
		return;
	}
	// read the comment field
	FS_Read( comment, sizeof( comment ), f );

	// read the mapcmd
	FS_Read( mapcmd, sizeof( mapcmd ), f );

	// read all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	while ( 1 )
	{
		if ( fread( name, 1, sizeof( name ), f ) == 0 ) {
			break;
		}
		FS_Read( string, sizeof( string ), f );
		Com_DPrintf( "Set %s = %s\n", name, string );
		Cvar_ForceSet( name, string );
	}

	fclose( f );

	// start a new game fresh with new cvars
	SV_InitGame();

	strcpy( svs.mapcmd, mapcmd );

	// read game state
	Q_sprintf_s( name, "%s/save/current/game.ssv", FS_Gamedir() );
	ge->ReadGame( name );
}

//=================================================================================================

/*
========================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
========================
*/
static void SV_DemoMap_f()
{
	SV_Map( true, Cmd_Argv( 1 ), false );
}

/*
========================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
========================
*/
static void SV_GameMap_f()
{
	char		*map;
	int			i;
	client_t	*cl;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: gamemap <map>\n");
		return;
	}

	Com_DPrintf("SV_GameMap(%s)\n", Cmd_Argv(1));

	FS_CreatePath (va("%s/save/current/", FS_Gamedir()));

	// check for clearing the current savegame
	map = Cmd_Argv(1);
	if (map[0] == '*')
	{
		// wipe all the *.sav files
		SV_WipeSavegame ("current");
	}
	else
	{	// save the map just exited
		if (sv.state == ss_game)
		{
			// clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			bool *savedInuse = (bool*)Mem_StackAlloc(maxclients->value * sizeof(bool));
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
			{
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
				cl->edict->inuse = savedInuse[i];
			//Mem_Free (savedInuse);
		}
	}

	// start up the next map
	SV_Map (false, Cmd_Argv(1), false );

	// archive server state
	Q_strcpy_s (svs.mapcmd, Cmd_Argv(1));

	// copy off the level to the autosave slot
	if (!dedicated->value)
	{
		SV_WriteServerFile (true);
		SV_CopySaveGame ("current", "save0");
	}
}

/*
========================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
========================
*/
static void SV_Map_f()
{
	char *map;
	char expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv( 1 );
	if ( !strstr( map, "." ) )
	{
		Q_sprintf_s( expanded, "maps/%s.bsp", map );
		if ( FS_LoadFile( expanded, NULL ) == -1 )
		{
			Com_Printf( "Can't find %s\n", expanded );
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame( "current" );
	SV_GameMap_f();
}

/*
===================================================================================================

	Savegames

===================================================================================================
*/

/*
========================
SV_Loadgame_f
========================
*/
static void SV_Loadgame_f()
{
	char name[MAX_OSPATH];
	FILE *f;
	char *dir;

	if ( Cmd_Argc() != 2 ) {
		Com_Print( "Usage: loadgame <directory>\n" );
		return;
	}

	Com_Print( "Loading game...\n" );

	dir = Cmd_Argv( 1 );
	if ( strstr( dir, ".." ) || strstr( dir, "/" ) || strstr( dir, "\\" ) ) {
		Com_Print( "Bad savedir.\n" );
	}

	// make sure the server.ssv file exists
	Q_sprintf_s( name, "%s/save/%s/server.ssv", FS_Gamedir(), Cmd_Argv( 1 ) );
	f = fopen( name, "rb" );
	if ( !f ) {
		Com_Printf( "No such savegame: %s\n", name );
		return;
	}
	fclose( f );

	SV_CopySaveGame( Cmd_Argv( 1 ), "current" );

	SV_ReadServerFile();

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map( false, svs.mapcmd, true );
}

/*
========================
SV_Savegame_f
========================
*/
static void SV_Savegame_f()
{
	if ( sv.state != ss_game ) {
		Com_Print( "You must be in a game to save.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Print( "Usage: savegame <directory>\n" );
		return;
	}

	if ( Cvar_VariableValue( "deathmatch" ) ) {
		Com_Print( "Can't savegame in a deathmatch\n" );
		return;
	}

	if ( Q_strcmp( Cmd_Argv( 1 ), "current" ) == 0 ) {
		Com_Print( "Can't save to 'current'\n" );
		return;
	}

	if ( maxclients->value == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0 ) {
		Com_Print( "\nCan't savegame while dead!\n" );
		return;
	}

	char *dir = Cmd_Argv( 1 );
	if ( strstr( dir, ".." ) || strstr( dir, "/" ) || strstr( dir, "\\" ) ) {
		Com_Print( "Bad savedir.\n" );
	}

	Com_Print( "Saving game...\n" );

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile();

	// save server state
	SV_WriteServerFile( false );

	// copy it off
	SV_CopySaveGame( "current", dir );

	Com_Print( "Done.\n" );
}

//=================================================================================================

/*
========================
SV_Kick_f

Kick a user off of the server
========================
*/
static void SV_Kick_f()
{
	if ( !svs.initialized ) {
		Com_Print( "No server running.\n" );
		return;
	}

	if ( Cmd_Argc() != 2 ) {
		Com_Print( "Usage: kick <userid>\n" );
		return;
	}

	if ( !SV_SetPlayer() ) {
		return;
	}

	SV_BroadcastPrintf( PRINT_HIGH, "%s was kicked\n", sv_client->name );
	// print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf( sv_client, PRINT_HIGH, "You were kicked from the game\n" );
	SV_DropClient( sv_client );
	sv_client->lastmessage = svs.realtime;	// min case there is a funny zombie
}

/*
========================
SV_Status_f
========================
*/
static void SV_Status_f()
{
	int			i, j, l;
	client_t	*cl;
	char		*s;
	int			ping;

	if (!svs.clients)
	{
		Com_Print ("No server running.\n");
		return;
	}
	Com_Printf ("map              : %s\n", sv.name);

	Com_Print ("num score ping name            lastmsg address               qport \n");
	Com_Print ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3i ", i);
		Com_Printf ("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == cs_connected)
			Com_Print ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Print ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
		l = 16 - strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Print (" ");

		Com_Printf ("%7i ", svs.realtime - cl->lastmessage );

		s = NET_NetadrToString ( cl->netchan.remote_address);
		Com_Print (s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Print (" ");
		
		Com_Printf ("%5i", cl->netchan.qport);

		Com_Print ("\n");
	}
	Com_Print ("\n");
}

/*
========================
SV_ConSay_f
========================
*/
static void SV_ConSay_f()
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < maxclients->value; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}

/*
========================
SV_Heartbeat_f
========================
*/
static void SV_Heartbeat_f()
{
	svs.last_heartbeat = -9999999;
}

/*
========================
SV_Serverinfo_f

Examine or change the serverinfo string
========================
*/
static void SV_Serverinfo_f()
{
	Com_Print( "Server info settings:\n" );
	Info_Print( Cvar_Serverinfo() );
}

/*
========================
SV_DumpUser_f

Examine all a users info strings
========================
*/
static void SV_DumpUser_f()
{
	if ( Cmd_Argc() != 2 ) {
		Com_Print( "Usage: info <userid>\n" );
		return;
	}

	if ( !SV_SetPlayer() ) {
		return;
	}

	Com_Print( "userinfo\n" );
	Com_Print( "--------\n" );
	Info_Print( sv_client->userinfo );
}

/*
========================
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
========================
*/
static void SV_ServerRecord_f()
{
	char	name[MAX_OSPATH];
	byte	buf_data[32768];
	sizebuf_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Com_Print ("serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_Print ("Already recording.\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_Print ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	Q_sprintf_s (name, "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("recording to %s.\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_Print ("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	SZ_Init (&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

	//
	// write a single giant fake message with all the startup info
	//
	SZ_Init (&buf, buf_data, sizeof(buf_data));

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawncount);
	// 2 means server demo
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);
	// send full levelname
	MSG_WriteString (&buf, sv.configstrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
	{
		if (sv.configstrings[i][0])
		{
			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configstrings[i]);
		}
	}

	// write it to the demo file
	Com_DPrintf ("signon message length: %i\n", buf.cursize);
	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, svs.demofile);
	fwrite (buf.data, buf.cursize, 1, svs.demofile);

	// the rest of the demo file will be individual frames
}

/*
========================
SV_ServerStop_f

Ends server demo recording
========================
*/
static void SV_ServerStop_f()
{
	if ( !svs.demofile ) {
		Com_Print( "Not doing a serverrecord.\n" );
		return;
	}

	fclose( svs.demofile );
	svs.demofile = NULL;
	Com_Print( "Recording completed.\n" );
}

/*
========================
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game
========================
*/
static void SV_KillServer_f()
{
	if ( !svs.initialized ) {
		return;
	}

	SV_Shutdown( "Server was killed.\n", false );
	NET_Config( false );	// close network sockets
}

/*
========================
SV_ServerCommand_f

Let the game dll handle a command
========================
*/
static void SV_ServerCommand_f()
{
	if ( !ge ) {
		Com_Print( "No game loaded.\n" );
		return;
	}

	ge->ServerCommand();
}

//=================================================================================================

/*
========================
SV_InitOperatorCommands
========================
*/
void SV_InitOperatorCommands()
{
	Cmd_AddCommand( "heartbeat", SV_Heartbeat_f );
	Cmd_AddCommand( "kick", SV_Kick_f );
	Cmd_AddCommand( "status", SV_Status_f );
	Cmd_AddCommand( "serverinfo", SV_Serverinfo_f );
	Cmd_AddCommand( "dumpuser", SV_DumpUser_f );

	Cmd_AddCommand( "map", SV_Map_f );
	Cmd_AddCommand( "demomap", SV_DemoMap_f );
	Cmd_AddCommand( "gamemap", SV_GameMap_f );
	Cmd_AddCommand( "setmaster", SV_SetMaster_f );

	if ( dedicated->value ) {
		Cmd_AddCommand( "say", SV_ConSay_f );
	}

	Cmd_AddCommand( "serverrecord", SV_ServerRecord_f );
	Cmd_AddCommand( "serverstop", SV_ServerStop_f );

	Cmd_AddCommand( "save", SV_Savegame_f );
	Cmd_AddCommand( "load", SV_Loadgame_f );

	Cmd_AddCommand( "killserver", SV_KillServer_f );

	Cmd_AddCommand( "sv", SV_ServerCommand_f );
}
