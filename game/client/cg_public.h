
#pragma once

#include "../../common/q_shared.h"

struct material_t;
struct model_t;
struct sfx_t;

struct sizebuf_t;

#define	CGAME_API_VERSION	1

#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

struct centity_t
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	int			trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	int			fly_stoptime;
};

//=============================================================================

struct cgame_import_t
{
	// Logging
	void	( *Print ) ( const char *msg );
	void	( *DPrint ) ( const char *msg );
	void	( *Error ) ( const char *msg );
	void	( *FatalError ) ( const char *msg );

	cvar_t	*( *Cvar_Get ) ( const char *var_name, const char *value, uint32 flags );

	void	( *StartSound ) ( vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs );

	sfx_t		*( *RegisterSound ) ( const char *sample );
	material_t	*( *RegisterPic )( const char *name );
	model_t		*( *RegisterModel )( const char *name );

	void	( *AddEntity ) ( entity_t *ent );
	void	( *AddParticle ) ( vec3_t org, int color, float alpha );
	void	( *AddLight ) ( vec3_t org, float intensity, float r, float g, float b );
	void	( *AddLightStyle ) ( int style, float r, float g, float b );

	int		( *ReadByte ) ( sizebuf_t *sb );
	int		( *ReadShort ) ( sizebuf_t *sb );
	int		( *ReadLong ) ( sizebuf_t *sb );
	void	( *ReadPos ) ( sizebuf_t *sb, vec3_t pos );
	void	( *ReadDir ) ( sizebuf_t *sb, vec3_t vector );

	int		( *HeadnodeForBox ) ( vec3_t mins, vec3_t maxs );
	int		( *TransformedPointContents ) ( vec3_t p, int headnode, vec3_t origin, vec3_t angles );
	int		( *PointContents ) ( vec3_t p, int headnode );
	trace_t ( *BoxTrace ) ( vec3_t start, vec3_t end,
			vec3_t mins, vec3_t maxs, int headnode, int brushmask );
	void	( *TransformedBoxTrace ) ( vec3_t start, vec3_t end,
			vec3_t mins, vec3_t maxs, int headnode, int brushmask,
			vec3_t origin, vec3_t angles, trace_t &trace );

	// Client hooks

	centity_t		*( *GetEntityAtIndex ) ( int index );

	int				( *time ) ( void );				// cl
	int				( *servertime ) ( void );		// cl.frame
	int				( *serverframe ) ( void );		// cl.frame
	int				( *frametime ) ( void );		// cls
	float			( *lerpfrac ) ( void );			// cl
	int				( *playernum ) ( void );		// cl
	player_state_t	*( *playerstate ) ( void );		// cl.frame
	vec3_t			*( *vieworg ) ( void );			// cl.refdef
	vec3_t			*( *v_forward ) ( void );		// cl
	vec3_t			*( *v_right ) ( void );			// cl
	vec3_t			*( *v_up ) ( void );			// cl

	vec3_t		*bytedirs;
	sizebuf_t	*net_message;
};

struct cgame_export_t
{
	int			apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void		( *Init ) ( void );
	void		( *Shutdown ) ( void );

	void		( *RegisterTEntSounds ) ( void );
	void		( *RegisterTEntModels ) ( void );

	void		( *AddEntities ) ( void );
	void		( *Frame ) ( void );
	void		( *ClearState ) ( void );

	void		( *EntityEvent ) ( entity_state_t *ent );
	void		( *TeleporterParticles ) ( entity_state_t *ent );
	void		( *RunParticles ) ( int &i, uint effects, centity_t *cent, entity_t *ent, entity_state_t *s1 );

	void		( *ParseTEnt ) ( void );
	void		( *ParseMuzzleFlash ) ( void );
	void		( *ParseMuzzleFlash2 ) ( void );

	void		( *SetLightstyle ) ( int i, const char *s );

	// Slart: Try to write some code to prevent having to export this...
	void		( *Pmove ) ( pmove_t *pm );


};
