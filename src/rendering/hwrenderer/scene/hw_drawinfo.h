#pragma once

#include <atomic>
#include <functional>
#include "vectors.h"
#include "r_defs.h"
#include "r_utility.h"
#include "hw_viewpointuniforms.h"
#include "v_video.h"
#include "hw_weapon.h"
#include "hw_drawlist.h"
#include "hw_renderstate.h"

EXTERN_CVAR(Bool, lm_always_update);
EXTERN_CVAR(Int, lm_max_updates);

enum EDrawMode
{
	DM_MAINVIEW,
	DM_OFFSCREEN,
	DM_PORTAL,
	DM_SKYPORTAL
};

struct FSectorPortalGroup;
struct FLinePortalSpan;
struct FFlatVertex;
class HWWall;
class HWFlat;
class HWSprite;
struct HWDecal;
class ShadowMap;
struct particle_t;
struct FDynLightData;
struct HUDSprite;
class ACorona;
class AFogball;
class Clipper;
class HWPortal;
class HWScenePortalBase;
class FRenderState;
class HWDrawContext;

// these are used to link faked planes due to missing textures to a sector
struct gl_subsectorrendernode
{
	gl_subsectorrendernode *	next;
	subsector_t *				sub;
	int							lightindex;
	int							vertexindex;
};

struct gl_floodrendernode
{
	gl_floodrendernode * next;
	seg_t *seg;
	int vertexindex;
	// This will use the light list of the originating sector.
};

enum area_t : int;

enum SectorRenderFlags
{
    // This is used to merge several subsectors into a single draw item
    SSRF_RENDERFLOOR = 1,
    SSRF_RENDERCEILING = 2,
    SSRF_RENDER3DPLANES = 4,
    SSRF_RENDERALL = 7,
    SSRF_PROCESSED = 8,
    SSRF_SEEN = 16,
    SSRF_PLANEHACK = 32,
    SSRF_FLOODHACK = 64
};

enum EPortalClip
{
	PClip_InFront,
	PClip_Inside,
	PClip_Behind,
};

enum DrawListType
{
	GLDL_PLAINWALLS,
	GLDL_PLAINFLATS,
	GLDL_MASKEDWALLS,
	GLDL_MASKEDFLATS,
	GLDL_MASKEDWALLSOFS,
	GLDL_MODELS,

	GLDL_TRANSLUCENT,
	GLDL_TRANSLUCENTBORDER,

	GLDL_TYPES,
};


struct HWDrawInfo
{
	struct wallseg
	{
		float x1, y1, z1, x2, y2, z2;
	};

	struct MissingTextureInfo
	{
		seg_t * seg;
		subsector_t * sub;
		float Planez;
		float Planezfront;
	};

	struct MissingSegInfo
	{
		seg_t * seg;
		int MTI_Index;    // tells us which MissingTextureInfo represents this seg.
	};

	struct SubsectorHackInfo
	{
		subsector_t * sub;
		uint8_t flags;
	};

	enum EFullbrightFlags
	{
		Fullbright = 1,
		Nightvision = 2,
		StealthVision = 4
	};

	bool isFullbrightScene() const { return !!(FullbrightFlags & Fullbright); }
	bool isNightvision() const { return !!(FullbrightFlags & Nightvision); }
	bool isStealthVision() const { return !!(FullbrightFlags & StealthVision); }
    
	HWDrawContext* drawctx = nullptr;

	HWDrawList drawlists[GLDL_TYPES];
	int vpIndex;
	ELightMode lightmode;

	FLevelLocals *Level;
	HWDrawInfo * outer = nullptr;
	int FullbrightFlags;
	std::atomic<int> spriteindex;
	HWPortal *mClipPortal;
	HWPortal *mCurrentPortal;
	Clipper *mClipper;
	FRenderViewpoint Viewpoint;
	HWViewpointUniforms VPUniforms;	// per-viewpoint uniform state
	TArray<HWPortal *> Portals;
	TArray<HWDecal *> Decals[2];	// the second slot is for mirrors which get rendered in a separate pass.
	TArray<HUDSprite> hudsprites;	// These may just be stored by value.
	TArray<AActor*> Coronas;
	TArray<Fogball> Fogballs;
	TArray<LevelMeshSurface*> VisibleSurfaces;
	uint64_t LastFrameTime = 0;

	TArray<MissingTextureInfo> MissingUpperTextures;
	TArray<MissingTextureInfo> MissingLowerTextures;

	TArray<MissingSegInfo> MissingUpperSegs;
	TArray<MissingSegInfo> MissingLowerSegs;

	TArray<SubsectorHackInfo> SubsectorHacks;

    TMap<int, gl_subsectorrendernode*> otherFloorPlanes;
    TMap<int, gl_subsectorrendernode*> otherCeilingPlanes;
    TMap<int, gl_floodrendernode*> floodFloorSegs;
    TMap<int, gl_floodrendernode*> floodCeilingSegs;

	TArray<subsector_t *> HandledSubsectors;

	TArray<uint8_t> section_renderflags;
	TArray<uint8_t> ss_renderflags;
	TArray<uint8_t> no_renderflags;

	// This is needed by the BSP traverser.
	BitArray CurrentMapSections;	// this cannot be a single number, because a group of portals with the same displacement may link different sections.
	area_t	in_area;
	fixed_t viewx, viewy;	// since the nodes are still fixed point, keeping the view position  also fixed point for node traversal is faster.
	bool multithread;

	bool MeshBSP = false;
	bool MeshBuilding = false;

	HWDrawInfo(HWDrawContext* drawctx) : drawctx(drawctx) { for (HWDrawList& list : drawlists) list.drawctx = drawctx; }

	void WorkerThread();

	void UnclipSubsector(subsector_t *sub);
	
	void AddLine(seg_t *seg, bool portalclip);
	void PolySubsector(subsector_t * sub);
	void RenderPolyBSPNode(void *node);
	void AddPolyobjs(subsector_t *sub);
	void AddLines(subsector_t * sub, sector_t * sector);
	void AddSpecialPortalLines(subsector_t * sub, sector_t * sector, linebase_t *line);
	public:
	void RenderThings(subsector_t * sub, sector_t * sector);
	void RenderParticles(subsector_t *sub, sector_t *front);
	void DoSubsector(subsector_t * sub);
	void DrawPSprite(HUDSprite* huds, FRenderState& state);
	WeaponLighting GetWeaponLighting(sector_t* viewsector, const DVector3& pos, int cm, area_t in_area, const DVector3& playerpos);

	void PreparePlayerSprites2D(sector_t* viewsector, area_t in_area, FRenderState& state);
	void PreparePlayerSprites3D(sector_t* viewsector, area_t in_area, FRenderState& state);
public:

	void SetCameraPos(const DVector3 &pos)
	{
		VPUniforms.mCameraPos = { (float)pos.X, (float)pos.Z, (float)pos.Y, 0.f };
	}

	void SetClipHeight(float h, float d)
	{
		VPUniforms.mClipHeight = h;
		VPUniforms.mClipHeightDirection = d;
		VPUniforms.mClipLine.X = -1000001.f;
	}

	void SetClipLine(line_t *line)
	{
		VPUniforms.mClipLine = { (float)line->v1->fX(), (float)line->v1->fY(), (float)line->Delta().X, (float)line->Delta().Y };
		VPUniforms.mClipHeight = 0;
	}

	void PushVisibleSurface(LevelMeshSurface* surface)
	{
		if (outer)
		{
			outer->PushVisibleSurface(surface);
			return;
		}

		if (lm_always_update || surface->AlwaysUpdate)
		{
			surface->NeedsUpdate = true;
		}
		else if (VisibleSurfaces.Size() >= unsigned(lm_max_updates))
		{
			return;
		}

		if (surface->NeedsUpdate && !surface->portalIndex && !surface->bSky)
		{
			VisibleSurfaces.Push(surface);
		}
	}

	HWPortal * FindPortal(const void * src);
	void RenderBSPNode(void *node, FRenderState& state);
	void RenderBSP(void *node, bool drawpsprites, FRenderState& state);

	static HWDrawInfo *StartDrawInfo(HWDrawContext* drawctx, FLevelLocals *lev, HWDrawInfo *parent, FRenderViewpoint &parentvp, HWViewpointUniforms *uniforms);
	void StartScene(FRenderViewpoint &parentvp, HWViewpointUniforms *uniforms);
	void ClearBuffers();
	HWDrawInfo *EndDrawInfo();
	void SetViewArea();
	int SetFullbrightFlags(player_t *player);

	void DrawScene(int drawmode, FRenderState& state);
	void CreateScene(bool drawpsprites, FRenderState& state);
	void RenderScene(FRenderState &state);
	void RenderTranslucent(FRenderState &state);
	void RenderPortal(HWPortal *p, FRenderState &state, bool usestencil);
	void EndDrawScene(sector_t * viewsector, FRenderState &state);
	void DrawEndScene2D(sector_t * viewsector, FRenderState &state);
	void Set3DViewport(FRenderState &state);
	void ProcessScene(bool toscreen, FRenderState& state);

	bool DoOneSectorUpper(subsector_t * subsec, float planez, area_t in_area);
	bool DoOneSectorLower(subsector_t * subsec, float planez, area_t in_area);
	bool DoFakeBridge(subsector_t * subsec, float planez, area_t in_area);
	bool DoFakeCeilingBridge(subsector_t * subsec, float planez, area_t in_area);

	bool CheckAnchorFloor(subsector_t * sub);
	bool CollectSubsectorsFloor(subsector_t * sub, sector_t * anchor);
	bool CheckAnchorCeiling(subsector_t * sub);
	bool CollectSubsectorsCeiling(subsector_t * sub, sector_t * anchor);

	void DispatchRenderHacks(FRenderState& state);
	void AddUpperMissingTexture(side_t * side, subsector_t *sub, float backheight);
	void AddLowerMissingTexture(side_t * side, subsector_t *sub, float backheight);
	void HandleMissingTextures(area_t in_area, FRenderState& state);
	void PrepareUnhandledMissingTextures(FRenderState& state);
	void PrepareUpperGap(seg_t * seg, FRenderState& state);
	void PrepareLowerGap(seg_t * seg, FRenderState& state);
	void CreateFloodPoly(wallseg * ws, FFlatVertex *vertices, float planez, sector_t * sec, bool ceiling);
	void CreateFloodStencilPoly(wallseg * ws, FFlatVertex *vertices);

	void AddHackedSubsector(subsector_t * sub);
	void HandleHackedSubsectors(FRenderState& state);

	void ProcessActorsInPortal(FLinePortalSpan *glport, area_t in_area, FRenderState& state);

	void AddOtherFloorPlane(int sector, gl_subsectorrendernode * node, FRenderState& state);
	void AddOtherCeilingPlane(int sector, gl_subsectorrendernode * node, FRenderState& state);

	void GetDynSpriteLight(AActor *self, float x, float y, float z, FLightNode *node, int portalgroup, float *out);
	void GetDynSpriteLight(AActor *thing, particle_t *particle, float *out);

	void PreparePlayerSprites(sector_t * viewsector, area_t in_area, FRenderState& state);
	void PrepareTargeterSprites(double ticfrac, FRenderState& state);

	void UpdateCurrentMapSection();
	void SetViewMatrix(const FRotator &angles, float vx, float vy, float vz, bool mirror, bool planemirror);
	void SetupView(FRenderState &state, float vx, float vy, float vz, bool mirror, bool planemirror);
	angle_t FrustumAngle();

	void DrawDecals(FRenderState &state, TArray<HWDecal *> &decals);
	void DrawPlayerSprites(bool hudModelStep, FRenderState &state);
	void DrawCoronas(FRenderState& state);
	void DrawCorona(FRenderState& state, AActor* corona, float coronaFade, double dist);

	void ProcessLowerMinisegs(TArray<seg_t *> &lowersegs, FRenderState& state);
    void AddSubsectorToPortal(FSectorPortalGroup *portal, subsector_t *sub);
    
    void AddWall(HWWall *w);
    void AddMirrorSurface(HWWall *w, FRenderState& state);
	void AddFlat(HWFlat *flat, bool fog);
	void AddSprite(HWSprite *sprite, bool translucent);

	void RenderThings(subsector_t* sub, sector_t* sector, FRenderState& state);
	void RenderParticles(subsector_t* sub, sector_t* front, FRenderState& state);
	void DoSubsector(subsector_t* sub, FRenderState& state);
	int SetupLightsForOtherPlane(subsector_t* sub, FDynLightData& lightdata, const secplane_t* plane, FRenderState& state);
	int CreateOtherPlaneVertices(subsector_t* sub, const secplane_t* plane, FRenderState& state);

    HWDecal *AddDecal(bool onmirror);

	void SetFallbackLightMode()
	{
		lightmode = ELightMode::Doom;
	}

private:
	// For ProcessLowerMiniseg
	bool inview;
	subsector_t* viewsubsector;
	TArray<seg_t*> lowersegs;

	subsector_t* currentsubsector;	// used by the line processing code.
	sector_t* currentsector;

	void AddLine(seg_t* seg, bool portalclip, FRenderState& state);
	void PolySubsector(subsector_t* sub, FRenderState& state);
	void RenderPolyBSPNode(void* node, FRenderState& state);
	void AddPolyobjs(subsector_t* sub, FRenderState& state);
	void AddLines(subsector_t* sub, sector_t* sector, FRenderState& state);
	void AddSpecialPortalLines(subsector_t* sub, sector_t* sector, linebase_t* line, FRenderState& state);

	void UpdateLightmaps();
};

void CleanSWDrawer();
sector_t* RenderViewpoint(FRenderViewpoint& mainvp, AActor* camera, IntRect* bounds, float fov, float ratio, float fovratio, bool mainview, bool toscreen);
void WriteSavePic(player_t* player, FileWriter* file, int width, int height);
sector_t* RenderView(player_t* player);


inline bool isSoftwareLighting(ELightMode lightmode)
{
	return lightmode == ELightMode::ZDoomSoftware || lightmode == ELightMode::DoomSoftware || lightmode == ELightMode::Build;
}

inline bool isBuildSoftwareLighting(ELightMode lightmode)
{
	return lightmode == ELightMode::Build;
}

inline bool isDoomSoftwareLighting(ELightMode lightmode)
{
	return lightmode == ELightMode::ZDoomSoftware || lightmode == ELightMode::DoomSoftware;
}

inline bool isDarkLightMode(ELightMode lightmode)
{
	return lightmode == ELightMode::Doom || lightmode == ELightMode::DoomDark;
}

int CalcLightLevel(ELightMode lightmode, int lightlevel, int rellight, bool weapon, int blendfactor);
PalEntry CalcLightColor(ELightMode lightmode, int light, PalEntry pe, int blendfactor);
float GetFogDensity(FLevelLocals* Level, ELightMode lightmode, int lightlevel, PalEntry fogcolor, int sectorfogdensity, int blendfactor);
bool CheckFog(FLevelLocals* Level, sector_t* frontsector, sector_t* backsector, ELightMode lightmode);
void SetColor(FRenderState& state, FLevelLocals* Level, ELightMode lightmode, int sectorlightlevel, int rellight, bool fullbright, const FColormap& cm, float alpha, bool weapon = false);
void SetShaderLight(FRenderState& state, FLevelLocals* Level, float level, float olight);
void SetFog(FRenderState& state, FLevelLocals* Level, ELightMode lightmode, int lightlevel, int rellight, bool fullbright, const FColormap* cmap, bool isadditive, bool inskybox);
