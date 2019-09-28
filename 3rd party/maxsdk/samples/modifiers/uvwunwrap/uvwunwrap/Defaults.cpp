#include "unwrap.h"
#include <Util\IniUtil.h> // MaxSDK::Util::WritePrivateProfileString

#include <locale.h>

#define UNWRAPCONFIGNAME _T("unwrapuvw.ini")

//Modifier panel section
#define GETFACESELECTIONFROMSTACK		_T("GetFaceSelectionFromStack")
#define FORCEUPDATE						_T("ForceUpdate")
#define CHANNEL							_T("Channel")
#define GEOMELEMENTMODE					_T("GeomElementMode")
#define PLANARTHRESHOLD					_T("PlanarThreshold")
#define PLANARMODE						_T("PlanarMode")
#define IGNOREBACKFACECULL				_T("IgnoreBackFaceCull")
#define OLDSELMETHOD					_T("OldSelectionMethod")	
#define TVELEMENTMODE					_T("TVElementMode")
#define ALWAYSEDIT						_T("AlwaysEdit")
#define UVEDGEMMODE						_T("UVEdgeMode")
#define OPENEDGEMMODE					_T("OpenEdgeMode")
#define THICKOPENEDGES					_T("ThickOpenEdges")

//View tool section
#define WINDOWPOSX1						_T("WindposX1")
#define WINDOWPOSY1						_T("WindposY1")
#define WINDOWPOSX2						_T("WindposX2")
#define WINDOWPOSY2						_T("WindposY2")
#define SUBOBJECTLEVEL					_T("SubObjectLevel")
#define TRANSFORMMODE					_T("TransformMode")
#define LOCKASPECT						_T("LockAspect")
#define SHOWMAP							_T("ShowMap")
#define SHOWVERTS						_T("ShowVerts")
#define FALLOFFSTR						_T("FalloffStr")
#define FALLOFF							_T("Falloff")
#define FALLOFFSPACE					_T("FalloffSpace")
#define VIEWPORTOPENEDGES				_T("ViewportOpenEdges")
#define ENABLESOFTSELECTION				_T("EnableSoftSelection")
#define PAINTSIZE						_T("PaintSelectSize")
#define AUTOMAP							_T("AutoMap")
#define AUTOBACKGROUND					_T("EnableAutoBackground")
#define ABSOLUTETYPEIN					_T("AbsoluteTypeIn")
#define MOVE							_T("MoveState")
#define SCALE							_T("ScaleState")
#define MIRROR							_T("MirrorState")
#define UVW								_T("UVWState")
#define ZOOMEXTENT						_T("ZoomExtentsState")
#define ROTATIONSRESPECTASPECT			_T("RotationsRespectAspect")
#define RESETSELONPIVOT					_T("ResetSelOnPivot")
#define POLYMODE						_T("PolygonMode")
#define ALLOWSELECTIONINSIDEGIZMO		_T("AllowSelectionInsideGizmo")
#define WELDTHRESHOLD					_T("WeldThreshold")
#define CONSTANTUPDATE					_T("ConstantUpdate")
#define SNAPTOGGLE						_T("SnapToggle")
#define SNAPSTR							_T("SnapStrength")
#define PIXELCENTERSNAP					_T("PixelCenterSnap")
#define PIXELCORNERSNAP					_T("PixelCornerSnap")
#define GRIDSNAP						_T("GridSnap")
#define VERTEXSNAP						_T("VertexSnap")
#define EDGESNAP						_T("EdgeSnap")
#define AUTOPINMOVEDVERTEX				_T("AutoPinMovedVertex")
#define TYPEINLINKUV					_T("TypeInLinkUV")

//preferences dialog
#define LINECOLOR						_T("LineColor")
#define SELCOLOR						_T("SelColor")
#define OPENEDGECOLOR					_T("OpenEdgeColor")
#define HANDLECOLOR						_T("HandleColor")
#define TRANSFORMGIZMOCOLOR				_T("TransformGizmoColor")
#define SHAREDCOLOR						_T("SharedColor")
#define SHOWSHARED						_T("ShowShared")
#define DISPLAYOPENEDGES				_T("DisplayOpenEdges")
#define GRIDVISIBLE						_T("GridVisible")
#define TILEGRIDVISIBLE					_T("TileGridVisible")
#define FACEFILLMODE					_T("FaceFillMode")
#define RENDERW							_T("RenderW")
#define RENDERH							_T("RenderH")
#define TILELIMIT						_T("TileLimit")
#define TILEBRIGHTNESS					_T("TileBrightness")
#define USEBITMAPRES					_T("UseBitmapRes")
#define DISPLAYPIXELUNITS				_T("DisplayPixelUnits")
#define TILEON							_T("TileOn")
#define SYNCSELECTION					_T("SyncSelection")
#define BRIGHTCENTERTILE				_T("BrightnessAffectsCenterTile")
#define DISPLAYHIDDENEDGES				_T("DisplayHiddenEdges")
#define BLENDTILE						_T("BlendTilesToBackground")
#define FILTERMAP						_T("FilterMap")
#define CHECKERTILING					_T("CheckerTiling")
#define GRIDSIZE						_T("GridSize")
#define SELECTION_PREVIEW				_T("SelectionPreview")
#define SOFTSELLIMIT					_T("SoftSelLimit")
#define SOFTSELRANGE					_T("SoftSelRange")
#define HITSIZE							_T("HitSize")
#define TICKSIZE						_T("TickSize")
#define NONSQUAREAPPLYBITMAPRATIO		_T("NonSquareApplyBitmapRatio")
#define SHOW_FPS_IN_EDITOR				_T("ShowFPSinEditor")

//map section
#define FLATTENANGLE					_T("FlattenMapAngle")
#define FLATTENSPACING					_T("FlattenMapSpacing")
#define FLATTENNORMALIZE				_T("FlattenMapNormalize")
#define FLATTENROTATE					_T("FlattenMapRotate")
#define FLATTENCOLLAPSE					_T("FlattenMapCollapse")
#define FLATTENBYMATERIALID				_T("FlattenByMaterialID")
#define NORMALSPACING					_T("NormalMapSpacing")
#define NORMALNORMALIZE					_T("NormalMapNormalize")
#define NORMALROTATE					_T("NormalMapRotate")
#define NORMALALIGNWIDTH				_T("NormalMapAlignWidth")
#define NORMALMETHOD					_T("NormalMapMethod")
#define UNFOLDNORMALIZE					_T("UnfoldMapNormalize")
#define UNFOLDMETHOD					_T("UnfoldMapMethod")

//peel section
#define ABFERRORBOUND					_T("AbfErrorBound")
#define ABFMAXITNUM						_T("AbfMaxIterationNum")
#define SHOWPINS						_T("ShowPinPoints")
#define USESIMPLIFYMODEL				_T("UseSimplifyModel")

//pack section
#define PACKMETHOD						_T("PackMethod")
#define PACKSPACING						_T("Packpacing")
#define PACKNORMALIZE					_T("PackNormalize")
#define PACKROTATE						_T("PackRotate")
#define PACKCOLLAPSE					_T("PackCollapse")
#define PACKRESCALECLUSTER				_T("PackRescaleCluster")

//Relax section
#define RELAXTYPE						_T("RelaxType")
#define RELAXITERATIONS					_T("RelaxIterations")
#define RELAXAMOUNT						_T("RelaxAmount")
#define RELAXSTRETCH					_T("RelaxStretch")
#define RELAXBOUNDARY					_T("RelaxBoundary")
#define RELAXCORNER						_T("RelaxCorner")
#define RELAXBYSPRINGSTRETCH			_T("RelaxBySpringStretch")
#define RELAXBYSPRINGITERATION			_T("RelaxBySpringIteration")

//brush section
#define BRUSH_FULL_STRENGTH_SIZE		_T("BrushFullStrengthSize")
#define BRUSH_FALLOFF_SIZE				_T("BrushFallOffSize")
#define BRUSH_FALLOFF_TYPE				_T("BrushFallOffType")

//stitch section
#define STITCHBIAS						_T("StitchBias")
#define STITCHALIGN						_T("StitchAlign")
#define STITCHSCALE						_T("StitchScale")

//sketch section
#define SKETCHSELMODE					_T("SketchSelMode")
#define SKETCHTYPE						_T("SketchType")
#define SKETCHINTERACTIVE				_T("SketchInteactiveMode")
#define SKETCHDISPLAYPOINTS				_T("SketchDisplayPoints")

//Render template section
#define RENDERTEMPLATE_WIDTH					_T("RenderTemplateWidth")
#define RENDERTEMPLATE_HEIGHT					_T("RenderTemplateHeight")
#define RENDERTEMPLATE_EDGECOLOR				_T("RenderTemplateEdgeColor")
#define RENDERTEMPLATE_EDGEALPHA				_T("RenderTemplateEdgeAlpha")
#define RENDERTEMPLATE_SEAMCOLOR				_T("RenderTemplateSeamColor")
#define RENDERTEMPLATE_RENDERVISIBLEEDGES		_T("RenderTemplateRenderVisibleEdges")
#define RENDERTEMPLATE_RENDERINVISIBLEEDGES		_T("RenderTemplateRenderInvisibleEdges")
#define RENDERTEMPLATE_RENDERSEAMEDGES			_T("RenderTemplateRenderSeamEdges")
#define RENDERTEMPLATE_SHOWFRAMEBUFFER			_T("RenderTemplateShowFrameBuffer")
#define RENDERTEMPLATE_FORCE2SIDED				_T("RenderTemplateForce2Sided")
#define RENDERTEMPLATE_FILLMODE					_T("RenderTemplateFillMode")
#define RENDERTEMPLATE_FILLALPHA				_T("RenderTemplateFillAlpha")
#define RENDERTEMPLATE_FILLCOLOR				_T("RenderTemplateFillColor")
#define RENDERTEMPLATE_OVERLAP					_T("RenderTemplateOverlap")
#define RENDERTEMPLATE_OVERLAPCOLOR				_T("RenderTemplateOverlapColor")

// GetCfgFilename()
void UnwrapMod::GetCfgFilename( TCHAR *filename, size_t filenameSize ) 
	{
		//unwrapuvw.ini is moved to plugcfg_ln
	_tcscpy_s(filename, filenameSize, TheManager->GetDir(APP_PLUGCFG_LN_DIR));
	int len = static_cast<int>(_tcslen(filename));	// SR DCAST64: Downcast to 2G limit.
	if (len) 
		{
	   if (_tcscmp(&filename[len-1],_T("\\")))
		 _tcscat_s(filename, filenameSize, _T("\\"));

		}
	_tcscat_s(filename, filenameSize, UNWRAPCONFIGNAME);
	}


class NumericLocalGuard : public MaxSDK::Util::Noncopyable
{
public:
	NumericLocalGuard()
	{
		saved_lc = NULL;
		TCHAR* lc = _tsetlocale(LC_NUMERIC, NULL);
		if (lc)  
 			saved_lc = _tcsdup(lc);  
		_tsetlocale(LC_NUMERIC, _T("C"));  
	}
	~NumericLocalGuard()
	{
		if (saved_lc)  
 		{  
 			_tsetlocale(LC_NUMERIC, saved_lc);  
 			free(saved_lc);  
 			saved_lc = NULL;  
		}  
	}
private:
	TCHAR* saved_lc;
};

void	UnwrapMod::fnSetAsDefaults()
{
	fnSetAsDefaults(MODIFIERPANELSECTION);
	fnSetAsDefaults(VIEWTOOLSECTION);
	fnSetAsDefaults(PREFERENCESSECTION);
	fnSetAsDefaults(MAPSECTION);
	fnSetAsDefaults(PEELSECTION);
	fnSetAsDefaults(PACKSECTION);
	fnSetAsDefaults(RELAXSECTION);
	fnSetAsDefaults(BRUSHSECTION);
	fnSetAsDefaults(STITCHSECTION);
	fnSetAsDefaults(SKETCHSECTION);
	fnSetAsDefaults(RENDERTEMPLATESECTION);
}

void	UnwrapMod::fnSetAsDefaults(TSTR sectionName)
{
	windowPos.length = sizeof(WINDOWPLACEMENT); 
	GetWindowPlacement(hDialogWnd,&windowPos);

	TCHAR filename[MAX_PATH];	
	GetCfgFilename(filename, _countof(filename));

	NumericLocalGuard theGuard;
	TSTR str;
	if(sectionName == MODIFIERPANELSECTION)
	{
		str.printf(_T("%d"),getFaceSelectionFromStack);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,GETFACESELECTIONFROMSTACK,str,filename);

		str.printf(_T("%d"),forceUpdate);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,FORCEUPDATE,str,filename);

		str.printf(_T("%d"),channel);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,CHANNEL,str,filename);

		str.printf(_T("%d"),geomElemMode);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,GEOMELEMENTMODE,str,filename);

		str.printf(_T("%f"),planarThreshold);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,PLANARTHRESHOLD,str,filename);

		str.printf(_T("%d"),planarMode);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,PLANARMODE,str,filename);

		str.printf(_T("%d"),ignoreBackFaceCull);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,IGNOREBACKFACECULL,str,filename);

		str.printf(_T("%d"),oldSelMethod);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,OLDSELMETHOD,str,filename);

		str.printf(_T("%d"),tvElementMode);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,TVELEMENTMODE,str,filename);

		str.printf(_T("%d"),alwaysEdit);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,ALWAYSEDIT,str,filename);

		str.printf(_T("%d"),uvEdgeMode);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,UVEDGEMMODE,str,filename);

		str.printf(_T("%d"),openEdgeMode);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,OPENEDGEMMODE,str,filename);

		str.printf(_T("%d"),thickOpenEdges);
		MaxSDK::Util::WritePrivateProfileString(MODIFIERPANELSECTION,THICKOPENEDGES,str,filename);
	}

	if(sectionName == VIEWTOOLSECTION)
	{
		str.printf(_T("%d"),windowPos.rcNormalPosition.left);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,WINDOWPOSX1,str,filename);

		str.printf(_T("%d"),windowPos.rcNormalPosition.bottom);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,WINDOWPOSY1,str,filename);

		str.printf(_T("%d"),windowPos.rcNormalPosition.right);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,WINDOWPOSX2,str,filename);

		str.printf(_T("%d"),windowPos.rcNormalPosition.top);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,WINDOWPOSY2,str,filename);

		str.printf(_T("%d"),mTVSubObjectMode);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SUBOBJECTLEVEL,str,filename);

		str.printf(_T("%d"),mode);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,TRANSFORMMODE,str,filename);

		str.printf(_T("%d"),lockAspect);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,LOCKASPECT,str,filename);

		str.printf(_T("%d"),showMap);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SHOWMAP,str,filename);

		str.printf(_T("%d"),showVerts);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SHOWVERTS,str,filename);

		str.printf(_T("%f"),falloffStr);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,FALLOFFSTR,str,filename);

		int iFalloff = fnGetFalloffType();
		str.printf(_T("%d"), iFalloff);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,FALLOFF,str,filename);

		str.printf(_T("%d"),falloffSpace);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,FALLOFFSPACE,str,filename);

		str.printf(_T("%d"),viewportOpenEdges);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,VIEWPORTOPENEDGES,str,filename);

		str.printf(_T("%d"),enableSoftSelection );
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,ENABLESOFTSELECTION,str,filename);

		str.printf(_T("%d"), MaxSDK::UIUnScaled(paintSize) );
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,PAINTSIZE,str,filename);

		str.printf(_T("%d"),autoMap );
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,AUTOMAP,str,filename);

		str.printf(_T("%d"),this->autoBackground);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,AUTOBACKGROUND,str,filename);

		str.printf(_T("%d"),this->absoluteTypeIn);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,ABSOLUTETYPEIN,str,filename);

		str.printf(_T("%d"),move);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,MOVE,str,filename);

		str.printf(_T("%d"),scale);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SCALE,str,filename);

		str.printf(_T("%d"),mirror);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,MIRROR,str,filename);

		str.printf(_T("%d"),uvw);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,UVW,str,filename);

		str.printf(_T("%d"),zoomext);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,ZOOMEXTENT,str,filename);

		str.printf(_T("%d"),rotationsRespectAspect);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,ROTATIONSRESPECTASPECT,str,filename);

		str.printf(_T("%d"),resetPivotOnSel);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,RESETSELONPIVOT,str,filename);

		str.printf(_T("%d"),polyMode);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,POLYMODE,str,filename);

		str.printf(_T("%d"),allowSelectionInsideGizmo);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,ALLOWSELECTIONINSIDEGIZMO,str,filename);

		str.printf(_T("%f"),weldThreshold);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,WELDTHRESHOLD,str,filename);

		str.printf(_T("%d"),update);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,CONSTANTUPDATE,str,filename);

		str.printf(_T("%d"),snapToggle );
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SNAPTOGGLE,str,filename);

		str.printf(_T("%f"),snapStrength );
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,SNAPSTR,str,filename);

		str.printf(_T("%d"),pixelCornerSnap);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,PIXELCORNERSNAP,str,filename);

		str.printf(_T("%d"),pixelCenterSnap);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION,PIXELCENTERSNAP,str,filename);

		str.printf(_T("%d"), GetGridSnap());
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION, GRIDSNAP, str, filename);

		str.printf(_T("%d"), GetVertexSnap());
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION, VERTEXSNAP, str, filename);

		str.printf(_T("%d"), GetEdgeSnap());
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION, EDGESNAP, str, filename);

		BOOL isAutoPinChecked = TRUE;
		pblock->GetValue(unwrap_autopin, 0, isAutoPinChecked, FOREVER);
		str.printf(_T("%d"), isAutoPinChecked);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION, AUTOPINMOVEDVERTEX, str, filename);

		int iTypeInLinkUV = (mTypeInLinkUV ? 1 : 0);
		str.printf(_T("%d"), iTypeInLinkUV);
		MaxSDK::Util::WritePrivateProfileString(VIEWTOOLSECTION, TYPEINLINKUV, str, filename);		
	}

	if(sectionName == PREFERENCESSECTION)
	{
		str.printf(_T("%d"),showShared );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,SHOWSHARED,str,filename);

		str.printf(_T("%d"),displayOpenEdges);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,DISPLAYOPENEDGES,str,filename);

		str.printf(_T("%d"),gridVisible );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,GRIDVISIBLE,str,filename);

		str.printf(_T("%d"),tileGridVisible );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,TILEGRIDVISIBLE,str,filename);

		str.printf(_T("%d"),fillMode);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,FACEFILLMODE,str,filename);

		str.printf(_T("%d"),rendW);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,RENDERW,str,filename);

		str.printf(_T("%d"),rendH);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,RENDERH,str,filename);

		str.printf(_T("%d"),iTileLimit);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,TILELIMIT,str,filename);

		str.printf(_T("%f"),fContrast);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,TILEBRIGHTNESS,str,filename);

		str.printf(_T("%d"),useBitmapRes);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,USEBITMAPRES,str,filename);

		str.printf(_T("%d"),displayPixelUnits);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,DISPLAYPIXELUNITS,str,filename);

		str.printf(_T("%d"),bTile);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,TILEON,str,filename);

		str.printf(_T("%d"), bShowFPSinEditor);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION, SHOW_FPS_IN_EDITOR, str, filename);

		str.printf(_T("%d"),syncSelection );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,SYNCSELECTION,str,filename);

		str.printf(_T("%d"),brightCenterTile );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,BRIGHTCENTERTILE,str,filename);

		str.printf(_T("%d"),displayHiddenEdges);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,DISPLAYHIDDENEDGES,str,filename);

		str.printf(_T("%d"),blendTileToBackGround );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,BLENDTILE,str,filename);

		str.printf(_T("%d"), filterMap);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION, FILTERMAP, str, filename);

		str.printf(_T("%f"),fCheckerTiling);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,CHECKERTILING,str,filename);

		str.printf(_T("%f"),gridSize );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,GRIDSIZE,str,filename);

		str.printf(_T("%d"), sSelectionPreview?1:0);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,SELECTION_PREVIEW,str,filename);

		str.printf(_T("%d"),limitSoftSel);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,SOFTSELLIMIT,str,filename);

		str.printf(_T("%d"),limitSoftSelRange);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,SOFTSELRANGE,str,filename);

		str.printf(_T("%d"),MaxSDK::UIUnScaled(hitSize));
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,HITSIZE,str,filename);

		str.printf(_T("%d"), MaxSDK::UIUnScaled(tickSize) );
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION,TICKSIZE,str,filename);

		str.printf(_T("%d"), mNonSquareApplyBitmapRatio);
		MaxSDK::Util::WritePrivateProfileString(PREFERENCESSECTION, NONSQUAREAPPLYBITMAPRATIO, str, filename);
	}

	if(sectionName == MAPSECTION)
	{
		str.printf(_T("%f"),flattenAngleThreshold);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENANGLE,str,filename);

		str.printf(_T("%f"),flattenSpacing);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENSPACING,str,filename);

		str.printf(_T("%d"),flattenNormalize);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENNORMALIZE,str,filename);

		str.printf(_T("%d"),flattenRotate);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENROTATE,str,filename);

		str.printf(_T("%d"),flattenCollapse);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENCOLLAPSE,str,filename);

		str.printf(_T("%d"),flattenByMaterialID);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,FLATTENBYMATERIALID,str,filename);
		
		str.printf(_T("%f"),normalSpacing);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,NORMALSPACING,str,filename);

		str.printf(_T("%d"),normalNormalize);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,NORMALNORMALIZE,str,filename);

		str.printf(_T("%d"),normalRotate);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,NORMALROTATE,str,filename);

		str.printf(_T("%d"),normalAlignWidth);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,NORMALALIGNWIDTH,str,filename);

		str.printf(_T("%d"),normalMethod);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,NORMALMETHOD,str,filename);

		str.printf(_T("%d"),unfoldNormalize);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,UNFOLDNORMALIZE,str,filename);

		str.printf(_T("%d"),unfoldMethod);
		MaxSDK::Util::WritePrivateProfileString(MAPSECTION,UNFOLDMETHOD,str,filename);
	}

	if(sectionName == PEELSECTION)
	{
		str.printf(_T("%f"),abfErrorBound);
		MaxSDK::Util::WritePrivateProfileString(PEELSECTION,ABFERRORBOUND,str,filename);

		str.printf(_T("%d"),abfMaxItNum);
		MaxSDK::Util::WritePrivateProfileString(PEELSECTION,ABFMAXITNUM,str,filename);

		str.printf(_T("%d"),showPins);
		MaxSDK::Util::WritePrivateProfileString(PEELSECTION,SHOWPINS,str,filename);

		str.printf(_T("%d"),useSimplifyModel);
		MaxSDK::Util::WritePrivateProfileString(PEELSECTION,USESIMPLIFYMODEL,str,filename);
	}

	if(sectionName == PACKSECTION)
	{
		str.printf(_T("%d"),packMethod);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKMETHOD,str,filename);

		str.printf(_T("%f"),packSpacing);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKSPACING,str,filename);

		str.printf(_T("%d"),packNormalize);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKNORMALIZE,str,filename);

		str.printf(_T("%d"),packRotate);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKROTATE,str,filename);

		str.printf(_T("%d"),packFillHoles);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKCOLLAPSE,str,filename);

		str.printf(_T("%d"),packRescaleCluster);
		MaxSDK::Util::WritePrivateProfileString(PACKSECTION,PACKRESCALECLUSTER,str,filename);
	}

	if(sectionName == RELAXSECTION)
	{
		str.printf(_T("%d"),this->relaxType);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXTYPE,str,filename);

		str.printf(_T("%d"),this->relaxIteration);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXITERATIONS,str,filename);

		str.printf(_T("%f"),this->relaxAmount);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXAMOUNT,str,filename);

		str.printf(_T("%f"),this->relaxStretch);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXSTRETCH,str,filename);

		str.printf(_T("%d"),this->relaxBoundary);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXBOUNDARY,str,filename);

		str.printf(_T("%d"),this->relaxSaddle);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXCORNER,str,filename);

		str.printf(_T("%f"),relaxBySpringStretch);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXBYSPRINGSTRETCH,str,filename);

		str.printf(_T("%d"),relaxBySpringIteration);
		MaxSDK::Util::WritePrivateProfileString(RELAXSECTION,RELAXBYSPRINGITERATION,str,filename);
	}

	if(sectionName == BRUSHSECTION)
	{
		str.printf(_T("%f"), fnGetPaintFullStrengthSize());
		MaxSDK::Util::WritePrivateProfileString(BRUSHSECTION,BRUSH_FULL_STRENGTH_SIZE,str,filename);

		str.printf(_T("%f"), fnGetPaintFallOffSize());
		MaxSDK::Util::WritePrivateProfileString(BRUSHSECTION,BRUSH_FALLOFF_SIZE,str,filename);

		str.printf(_T("%d"), fnGetPaintFallOffType());
		MaxSDK::Util::WritePrivateProfileString(BRUSHSECTION,BRUSH_FALLOFF_TYPE,str,filename);
	}

	if(sectionName == STITCHSECTION)
	{
		str.printf(_T("%d"),bStitchAlign);
		MaxSDK::Util::WritePrivateProfileString(STITCHSECTION,STITCHBIAS,str,filename);

		str.printf(_T("%f"),fStitchBias);
		MaxSDK::Util::WritePrivateProfileString(STITCHSECTION,STITCHALIGN,str,filename);

		str.printf(_T("%d"),bStitchScale);
		MaxSDK::Util::WritePrivateProfileString(STITCHSECTION,STITCHSCALE,str,filename);
	}

	if(sectionName == SKETCHSECTION)
	{
		str.printf(_T("%d"),sketchSelMode);
		MaxSDK::Util::WritePrivateProfileString(SKETCHSECTION,SKETCHSELMODE,str,filename);

		str.printf(_T("%d"),sketchType);
		MaxSDK::Util::WritePrivateProfileString(SKETCHSECTION,SKETCHTYPE,str,filename);

		str.printf(_T("%d"),sketchInteractiveMode);
		MaxSDK::Util::WritePrivateProfileString(SKETCHSECTION,SKETCHINTERACTIVE,str,filename);

		str.printf(_T("%d"),sketchDisplayPoints);
		MaxSDK::Util::WritePrivateProfileString(SKETCHSECTION,SKETCHDISPLAYPOINTS,str,filename);
	}

	if(sectionName == RENDERTEMPLATESECTION)
	{
		int width = 0;
		pblock->GetValue(unwrap_renderuv_width,0,width,FOREVER);
		str.printf(_T("%d"),width);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_WIDTH,str,filename);

		int height = 0;
		pblock->GetValue(unwrap_renderuv_height,0,height,FOREVER);
		str.printf(_T("%d"),height);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_HEIGHT,str,filename);

		Color edgeColor;
		pblock->GetValue(unwrap_renderuv_edgecolor,0,edgeColor,FOREVER);
		str.printf(_T("%f %f %f"),edgeColor.r,edgeColor.g,edgeColor.b);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_EDGECOLOR,str,filename);

		float edgeAlpha = 0.0f;
		pblock->GetValue(unwrap_renderuv_edgealpha,0,edgeAlpha,FOREVER);
		str.printf(_T("%f"),edgeAlpha);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_EDGEALPHA,str,filename);

		Color seamColor;
		pblock->GetValue(unwrap_renderuv_seamcolor,0,seamColor,FOREVER);
		str.printf(_T("%f %f %f"),seamColor.r,seamColor.g,seamColor.b);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_SEAMCOLOR,str,filename);

		int r = 0;
		pblock->GetValue(unwrap_renderuv_visible,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERVISIBLEEDGES,str,filename);

		pblock->GetValue(unwrap_renderuv_invisible,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERINVISIBLEEDGES,str,filename);

		pblock->GetValue(unwrap_renderuv_seamedges,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERSEAMEDGES,str,filename);

		pblock->GetValue(unwrap_renderuv_showframebuffer,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_SHOWFRAMEBUFFER,str,filename);

		pblock->GetValue(unwrap_renderuv_force2sided,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FORCE2SIDED,str,filename);

		pblock->GetValue(unwrap_renderuv_fillmode,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLMODE,str,filename);

		float fillAlpha = 0.0f;
		pblock->GetValue(unwrap_renderuv_fillalpha,0,fillAlpha,FOREVER);
		str.printf(_T("%f"),fillAlpha);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLALPHA,str,filename);

		Color fillColor;
		pblock->GetValue(unwrap_renderuv_fillcolor,0,fillColor,FOREVER);
		str.printf(_T("%f %f %f"),fillColor.r,fillColor.g,fillColor.b);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLCOLOR,str,filename);

		pblock->GetValue(unwrap_renderuv_overlap,0,r,FOREVER);
		str.printf(_T("%d"),r);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_OVERLAP,str,filename);

		Color overlapColor;
		pblock->GetValue(unwrap_renderuv_overlapcolor,0,overlapColor,FOREVER);
		str.printf(_T("%f %f %f"),overlapColor.r,overlapColor.g,overlapColor.b);
		MaxSDK::Util::WritePrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_OVERLAPCOLOR,str,filename);
	}
}

void UnwrapMod::fnLoadDefaults()
{
	fnLoadDefaults(MODIFIERPANELSECTION);
	fnLoadDefaults(VIEWTOOLSECTION);
	fnLoadDefaults(PREFERENCESSECTION);
	fnLoadDefaults(MAPSECTION);
	fnLoadDefaults(PEELSECTION);
	fnLoadDefaults(PACKSECTION);
	fnLoadDefaults(RELAXSECTION);
	fnLoadDefaults(BRUSHSECTION);
	fnLoadDefaults(STITCHSECTION);
	fnLoadDefaults(SKETCHSECTION);
	fnLoadDefaults(RENDERTEMPLATESECTION);
}

void UnwrapMod::fnLoadDefaults(TSTR sectionName)
{
	TCHAR filename[MAX_PATH];
	GetCfgFilename(filename, _countof(filename));
	TCHAR str[MAX_PATH];
	TSTR def(_T("DISCARD"));

	TCHAR* saved_lc = NULL;
	TCHAR* lc = _tsetlocale(LC_NUMERIC, NULL);
	if (lc)  
    saved_lc = _tcsdup(lc);  
	lc = _tsetlocale(LC_NUMERIC, _T("C"));

	int		res = 0;
	int		tmp = 0;
	float	ftmp = 0.0f;

	if(sectionName == MODIFIERPANELSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,GETFACESELECTIONFROMSTACK,def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&getFaceSelectionFromStack);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,FORCEUPDATE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&forceUpdate);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,CHANNEL,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&channel);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,GEOMELEMENTMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&geomElemMode);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,PLANARTHRESHOLD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&planarThreshold);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,PLANARMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&planarMode);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,IGNOREBACKFACECULL,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&ignoreBackFaceCull);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,OLDSELMETHOD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&oldSelMethod);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,TVELEMENTMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&tvElementMode);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,ALWAYSEDIT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&alwaysEdit);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,UVEDGEMMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&uvEdgeMode);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,OPENEDGEMMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&openEdgeMode);

		res = MaxSDK::Util::GetPrivateProfileString(MODIFIERPANELSECTION,THICKOPENEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&thickOpenEdges);
	}

	if(sectionName == VIEWTOOLSECTION)
	{
		int full = 0;
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,WINDOWPOSX1,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			_stscanf(str,_T("%ld"),&windowPos.rcNormalPosition.left);
			full++;
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,WINDOWPOSY1,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			_stscanf(str,_T("%ld"),&windowPos.rcNormalPosition.bottom);
			full++;
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,WINDOWPOSX2,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{	
			_stscanf(str,_T("%ld"),&windowPos.rcNormalPosition.right);
			full++;
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,WINDOWPOSY2,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			_stscanf(str,_T("%ld"),&windowPos.rcNormalPosition.top);
			full++;
		}
		if (full == 4) windowPos.length = sizeof(WINDOWPLACEMENT); 

		if ( windowPos.length && hDialogWnd ) {
			windowPos.flags = 0;
			windowPos.showCmd = SW_SHOWNORMAL;
			SetWindowPlacement(hDialogWnd,&windowPos);
		}
		
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SUBOBJECTLEVEL,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int iSubMode = TVFACEMODE;
			_stscanf(str,_T("%d"),&iSubMode);
			fnSetTVSubMode(iSubMode);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,TRANSFORMMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int iTransformMode = 0;
			_stscanf(str,_T("%d"),&iTransformMode);
			SetMode(iTransformMode);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,LOCKASPECT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&lockAspect);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SHOWMAP,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&showMap);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SHOWVERTS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&showVerts);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,FALLOFFSTR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			_stscanf(str,_T("%f"),&falloffStr);
			fnSetFalloffDist(falloffStr);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,FALLOFF,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str, def)))
		{
			int iFallOffType = 4;
			_stscanf(str, _T("%d"), &iFallOffType);
			fnSetFalloffType(iFallOffType);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,FALLOFFSPACE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&falloffSpace);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,VIEWPORTOPENEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&viewportOpenEdges);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,ENABLESOFTSELECTION,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&enableSoftSelection);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,PAINTSIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str, def)))
		{
			_stscanf(str, _T("%d"), &paintSize);
			paintSize = MaxSDK::UIScaled(paintSize);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,AUTOMAP,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&autoMap);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,AUTOBACKGROUND,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&autoBackground);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,ABSOLUTETYPEIN,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&absoluteTypeIn);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,MOVE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&move);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SCALE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&scale);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,MIRROR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&mirror);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,UVW,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&uvw);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,ZOOMEXTENT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&zoomext);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,ROTATIONSRESPECTASPECT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&rotationsRespectAspect);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,RESETSELONPIVOT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&resetPivotOnSel);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,POLYMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&polyMode);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,ALLOWSELECTIONINSIDEGIZMO,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&allowSelectionInsideGizmo);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,WELDTHRESHOLD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&weldThreshold);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,CONSTANTUPDATE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&update);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SNAPTOGGLE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&snapToggle);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,PIXELCORNERSNAP,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&pixelCornerSnap);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,PIXELCENTERSNAP,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&pixelCenterSnap);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION, GRIDSNAP, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def))) 
		{
			BOOL isGridSnap = TRUE;
			_stscanf(str, _T("%d"), &isGridSnap);
			SetGridSnap(isGridSnap);
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION, VERTEXSNAP, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def)))
		{
			BOOL isVertexSnap = TRUE;
			_stscanf(str, _T("%d"), &isVertexSnap);
			SetVertexSnap(isVertexSnap);
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION, EDGESNAP, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def)))
		{
			BOOL isEdgeSnap = TRUE;
			_stscanf(str, _T("%d"), &isEdgeSnap);
			SetEdgeSnap(isEdgeSnap);
		}
		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION,SNAPSTR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&snapStrength);

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION, AUTOPINMOVEDVERTEX, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def)))
		{
			BOOL isChecked = TRUE;
			_stscanf(str, _T("%d"), &isChecked);
			pblock->SetValue(unwrap_autopin, 0, isChecked);
		}

		res = MaxSDK::Util::GetPrivateProfileString(VIEWTOOLSECTION, TYPEINLINKUV, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def)))
		{
			int iLinked = 1;
			_stscanf(str, _T("%d"), &iLinked);
			mTypeInLinkUV = (iLinked == 1 ? true : false);
		}		
	}

	if(sectionName == PREFERENCESSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,SHOWSHARED,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&showShared);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,DISPLAYOPENEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&displayOpenEdges);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,GRIDVISIBLE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&gridVisible);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,TILEGRIDVISIBLE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&tileGridVisible);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,FACEFILLMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&fillMode);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,RENDERW,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&rendW);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,RENDERH,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&rendH);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,TILELIMIT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&iTileLimit);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,TILEBRIGHTNESS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&fContrast);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,USEBITMAPRES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&useBitmapRes);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,DISPLAYPIXELUNITS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&displayPixelUnits);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,TILEON,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&bTile);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION, SHOW_FPS_IN_EDITOR, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def))) _stscanf(str, _T("%d"), &bShowFPSinEditor);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,SYNCSELECTION,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&syncSelection);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,BRIGHTCENTERTILE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&brightCenterTile);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,DISPLAYHIDDENEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&displayHiddenEdges);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,BLENDTILE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&blendTileToBackGround);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION, FILTERMAP, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def))) _stscanf(str, _T("%d"), &filterMap);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,CHECKERTILING,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			float fTiling = 1.0;
			_stscanf(str,_T("%f"),&fTiling);
			fnSetCheckerTiling(fTiling);
		}

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,GRIDSIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&gridSize);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION, SELECTION_PREVIEW,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			_stscanf(str,_T("%d"),&tmp);
			sSelectionPreview = (tmp!=0)?true:false;
		}

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,SOFTSELLIMIT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&limitSoftSel);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,SOFTSELRANGE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&limitSoftSelRange);

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,HITSIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str, def)))
		{
			_stscanf(str, _T("%d"), &hitSize);
			hitSize = MaxSDK::UIScaled(hitSize);
		}

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION,TICKSIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str, def)))
		{
			_stscanf(str,_T("%d"),&tickSize);
			tickSize = MaxSDK::UIScaled(tickSize);
		}

		res = MaxSDK::Util::GetPrivateProfileString(PREFERENCESSECTION, NONSQUAREAPPLYBITMAPRATIO, def, str, MAX_PATH, filename);
		if ((res) && (_tcscmp(str, def))) _stscanf(str, _T("%d"), &mNonSquareApplyBitmapRatio);
	}

	if(sectionName == MAPSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENANGLE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&flattenAngleThreshold);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENSPACING,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&flattenSpacing);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENNORMALIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&flattenNormalize);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENROTATE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&flattenRotate);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENCOLLAPSE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&flattenCollapse);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,FLATTENBYMATERIALID,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&flattenByMaterialID);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,NORMALSPACING,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&normalSpacing);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,NORMALNORMALIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&normalNormalize);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,NORMALROTATE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&normalRotate);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,NORMALALIGNWIDTH,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&normalAlignWidth);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,NORMALMETHOD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&normalMethod);
	
		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,UNFOLDNORMALIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&unfoldNormalize);

		res = MaxSDK::Util::GetPrivateProfileString(MAPSECTION,UNFOLDMETHOD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&unfoldMethod);
	}

	if (sectionName == PEELSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(PEELSECTION,ABFERRORBOUND,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&abfErrorBound);

		res = MaxSDK::Util::GetPrivateProfileString(PEELSECTION,ABFMAXITNUM,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&abfMaxItNum);

		res = MaxSDK::Util::GetPrivateProfileString(PEELSECTION,SHOWPINS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&showPins);

		res = MaxSDK::Util::GetPrivateProfileString(PEELSECTION,USESIMPLIFYMODEL,  def,str,MAX_PATH,filename);
		
		if ((res) && (_tcscmp(str, def)))
		{
			int tmpInt = 0;
			_stscanf(str, _T("%d"), &tmpInt);
			useSimplifyModel = (tmpInt != 0);
		}
	}

	if(sectionName == PACKSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKMETHOD,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&packMethod);

		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKSPACING,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&packSpacing);

		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKNORMALIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&packNormalize);

		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKROTATE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&packRotate);

		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKCOLLAPSE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&packFillHoles);

		res = MaxSDK::Util::GetPrivateProfileString(PACKSECTION,PACKRESCALECLUSTER,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&packRescaleCluster);
	}

	if(sectionName == RELAXSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXTYPE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&relaxType);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXITERATIONS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&relaxIteration);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXAMOUNT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&relaxAmount);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXSTRETCH,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&relaxStretch);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXBOUNDARY,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&relaxBoundary);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXCORNER,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&relaxSaddle);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXBYSPRINGSTRETCH,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&relaxBySpringStretch);

		res = MaxSDK::Util::GetPrivateProfileString(RELAXSECTION,RELAXBYSPRINGITERATION,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&relaxBySpringIteration);
	}

	if(sectionName == BRUSHSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(BRUSHSECTION,BRUSH_FULL_STRENGTH_SIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			_stscanf(str,_T("%f"),&ftmp);
			fnSetPaintFullStrengthSize(ftmp);
		}

		res = MaxSDK::Util::GetPrivateProfileString(BRUSHSECTION,BRUSH_FALLOFF_SIZE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			_stscanf(str,_T("%f"),&ftmp);
			fnSetPaintFallOffSize(ftmp);
		}

		res = MaxSDK::Util::GetPrivateProfileString(BRUSHSECTION,BRUSH_FALLOFF_TYPE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def)))
		{
			_stscanf(str,_T("%d"),&tmp);
			fnSetPaintFallOffType(tmp);
		}
	}

	if(sectionName == STITCHSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(STITCHSECTION,STITCHBIAS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&bStitchAlign);

		res = MaxSDK::Util::GetPrivateProfileString(STITCHSECTION,STITCHALIGN,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%f"),&fStitchBias);

		res = MaxSDK::Util::GetPrivateProfileString(STITCHSECTION,STITCHSCALE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&bStitchScale);
	}

	if(sectionName == SKETCHSECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(SKETCHSECTION,SKETCHSELMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&sketchSelMode);

		res = MaxSDK::Util::GetPrivateProfileString(SKETCHSECTION,SKETCHTYPE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&sketchType);

		res = MaxSDK::Util::GetPrivateProfileString(SKETCHSECTION,SKETCHINTERACTIVE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&sketchInteractiveMode);

		res = MaxSDK::Util::GetPrivateProfileString(SKETCHSECTION,SKETCHDISPLAYPOINTS,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) _stscanf(str,_T("%d"),&sketchDisplayPoints);
	}

	if(sectionName == RENDERTEMPLATESECTION)
	{
		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_WIDTH,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int width;
			_stscanf(str,_T("%d"),&width);
			pblock->SetValue(unwrap_renderuv_width,0,width);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_HEIGHT,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int height;
			_stscanf(str,_T("%d"),&height);
			pblock->SetValue(unwrap_renderuv_height,0,height);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_EDGECOLOR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			Point3 c;
			_stscanf(str,_T("%f %f %f"),&c.x,&c.y,&c.z);
			pblock->SetValue(unwrap_renderuv_edgecolor,0,c);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_EDGEALPHA,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			float a;
			_stscanf(str,_T("%f"),&a);
			pblock->SetValue(unwrap_renderuv_edgealpha,0,a);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_SEAMCOLOR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			Point3 c;
			_stscanf(str,_T("%f %f %f"),&c.x,&c.y,&c.z);
			pblock->SetValue(unwrap_renderuv_seamcolor,0,c);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERVISIBLEEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_visible,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERINVISIBLEEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_invisible,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_RENDERSEAMEDGES,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_seamedges,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_SHOWFRAMEBUFFER,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_showframebuffer,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FORCE2SIDED,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_force2sided,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLMODE,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_fillmode,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLALPHA,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			float a;
			_stscanf(str,_T("%f"),&a);
			pblock->SetValue(unwrap_renderuv_fillalpha,0,a);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_FILLCOLOR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			Point3 c;
			_stscanf(str,_T("%f %f %f"),&c.x,&c.y,&c.z);
			pblock->SetValue(unwrap_renderuv_fillcolor,0,c);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_OVERLAP,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			int r;
			_stscanf(str,_T("%d"),&r);
			pblock->SetValue(unwrap_renderuv_overlap,0,r);
		}

		res = MaxSDK::Util::GetPrivateProfileString(RENDERTEMPLATESECTION,RENDERTEMPLATE_OVERLAPCOLOR,  def,str,MAX_PATH,filename);
		if ((res) && (_tcscmp(str,def))) 
		{
			Point3 c;
			_stscanf(str,_T("%f %f %f"),&c.x,&c.y,&c.z);
			pblock->SetValue(unwrap_renderuv_overlapcolor,0,c);
		}
	}

	if (saved_lc)
 	{  
 		lc = _tsetlocale(LC_NUMERIC, saved_lc);  
 		free(saved_lc);  
 		saved_lc = NULL;  
	}

	InvalidateView();
}
