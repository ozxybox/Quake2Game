
#include "cmdlib.h"
#include "bspfile.h"
#include "polylib.h"
#include "threads.h"

#include <vector>
#include <climits>

typedef enum
{
	emit_surface,
	emit_point,
	emit_spotlight
} emittype_t;



typedef struct directlight_s
{
	struct directlight_s *next;
	emittype_t	type;

	float		intensity;
	int			style;
	vec3_t		origin;
	vec3_t		color;
	vec3_t		normal;		// for surfaces and spotlights
	float		stopdot;		// for spotlights
} directlight_t;


// the sum of all tranfer->transfer values for a given patch
// should equal exactly 0x10000, showing that all radiance
// reaches other patches
typedef struct
{
	int32	patch;
	int32	transfer;
} transfer_t;


#define	MIN_PATCHES	65536			// we reserve this in main()
#define	MAX_PATCHES	INT32_MAX

typedef struct patch_s
{
	winding_t	*winding;
	struct patch_s		*next;		// next in face
	int			numtransfers;
	transfer_t	*transfers;

	int			cluster;			// for pvs checking
	vec3_t		origin;
	dplane_t	*plane;

	qboolean	sky;

	vec3_t		totallight;			// accumulated by radiosity
									// does NOT include light
									// accounted for by direct lighting
	float		area;

	// illuminance * reflectivity = radiosity
	vec3_t		reflectivity;
	vec3_t		baselight;			// emissivity only

	// each style 0 lightmap sample in the patch will be
	// added up to get the average illuminance of the entire patch
	vec3_t		samplelight;
	int			samples;		// for averaging direct light
} patch_t;

extern	patch_t		*face_patches[MAX_MAP_FACES];
extern	entity_t	*face_entity[MAX_MAP_FACES];
extern	vec3_t		face_offset[MAX_MAP_FACES];		// for rotating bmodels

extern	std::vector<patch_t> g_patches;

extern	int		leafparents[MAX_MAP_LEAFS];
extern	int		nodeparents[MAX_MAP_NODES];

extern	float	lightscale;
extern	float	g_smoothing_threshold;

//==============================================

extern	float ambient, maxlight;

void LinkPlaneFaces (void);

extern	qboolean	extrasamples;
extern int numbounce;

extern	directlight_t	*directlights[MAX_MAP_LEAFS];

extern	byte	nodehit[MAX_MAP_NODES];

void BuildFacelights (int facenum);

void FinalLightFace (int facenum);

qboolean PvsForOrigin (vec3_t org, byte *pvs);

int TestLine_r (int node, vec3_t start, vec3_t stop);

void CreateDirectLights (void);

dleaf_t		*PointInLeaf (vec3_t point);


extern	dplane_t	backplanes[MAX_MAP_PLANES];
extern	int			fakeplanes;					// created planes for origin offset

extern	float	subdiv;

extern	float	direct_scale;
extern	float	entity_scale;

int	PointInLeafnum (vec3_t point);
void MakeTnodes (dmodel_t *bm);
void MakePatches (void);
void SubdividePatches (void);
void PairEdges (void);
void LoadMaterials (void);
