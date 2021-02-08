
/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
struct mvertex_t
{
	vec3_t		position;
};

struct mmodel_t
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
};


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWTURB		0x10
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80

// !!! if this is changed, it must be changed in asm_draw.h too !!!
struct medge_t
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
};

struct mtexinfo_t
{
	float		vecs[2][4];
	int			flags;
	int			numframes;
	mtexinfo_t	*next;		// animation chain
	material_t	*material;
};

#define	VERTEXSIZE	7

struct glpoly_t
{
	glpoly_t	*next;
	glpoly_t	*chain;
	int			numverts;
	int			flags;					// for SURF_UNDERWATER (not needed anymore?)
	float		verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
};

struct msurface_t
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates
	int			dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps

	glpoly_t	*polys;				// multiple if warped
	msurface_t	*texturechain;
	msurface_t	*lightmapchain;

	mtexinfo_t	*texinfo;

// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	byte		*samples;		// [numstyles*surfsize]
};

struct mnode_t
{
// common with leaf
	int			contents;		// -1, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	
	float		minmaxs[6];		// for bounding box culling

	mnode_t		*parent;

// node specific
	cplane_t	*plane;
	mnode_t		*children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};


struct mleaf_t
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	mnode_t		*parent;

// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
};


//-------------------------------------------------------------------------------------------------

//
// Whole model
//

enum modtype_t { mod_bad, mod_brush, mod_sprite, mod_alias, mod_studio };

struct model_t
{
	char		name[MAX_QPATH];

	int			registration_sequence;

	modtype_t	type;
	int			numframes;
	
	int			flags;

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping (unused)
//
//	qboolean	clipbox;
//	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;
	int			lightmap;		// only for submodels

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	byte		*lightdata;

	// for alias models and skins
	material_t	*skins[MAX_MD2SKINS];

	int			extradatasize;
	void		*extradata;
};

//-------------------------------------------------------------------------------------------------

extern	int registration_sequence;

void	Mod_Init (void);
mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model);
model_t *Mod_ForName (const char *name, qboolean crash);
byte	*Mod_ClusterPVS (int cluster, model_t *model);

void	Mod_Modellist_f (void);

void	*Hunk_Begin (int maxsize);
void	*Hunk_Alloc (int size);
int		Hunk_End (void);
void	Hunk_Free (void *base);

void	Mod_FreeAll (void);
void	Mod_Free (model_t *mod);