
/*Interface class and miscellaneous functions*/

def_visible_primitive			( ConfigureBitmapPaths,		"ConfigureBitmapPaths");
def_visible_primitive			( EditAtmosphere,			"EditAtmosphere");
def_visible_primitive_debug_ok	( CheckForSave,				"CheckForSave");


/* Object Class */ 
def_visible_primitive_debug_ok	( GetPolygonCount,			"GetPolygonCount");
def_visible_primitive_debug_ok	( GetTriMeshFaceCount,		"GetTriMeshFaceCount");
def_visible_primitive_debug_ok	( IsPointSelected,			"IsPointSelected");
def_visible_primitive_debug_ok	( NumMapsUsed,				"NumMapsUsed");
def_visible_primitive_debug_ok	( IsShapeObject,			"IsShapeObject");
def_visible_primitive_debug_ok	( PointSelection,			"PointSelection");
def_visible_primitive_debug_ok	( NumSurfaces,				"NumSurfaces");
def_visible_primitive_debug_ok	( IsSurfaceUVClosed,		"IsSurfaceUVClosed");

/* Miscellaneous functions */
def_visible_primitive			( DeselectHiddenEdges,		"DeselectHiddenEdges");
def_visible_primitive			( DeselectHiddenFaces,		"DeselectHiddenFaces");
def_visible_primitive_debug_ok	( AverageSelVertCenter,		"AverageSelVertCenter");
def_visible_primitive_debug_ok	( AverageSelVertNormal,		"AverageSelVertNormal");
def_visible_primitive_debug_ok	( MatrixFromNormal,			"MatrixFromNormal");

/* Patch Objects */
def_visible_primitive			( SetPatchSteps,			"SetPatchSteps");
def_visible_primitive_debug_ok	( GetPatchSteps,			"GetPatchSteps");

/* Euler Angles */
def_visible_primitive_debug_ok	( GetEulerQuatAngleRatio,	"GetEulerQuatAngleRatio");
def_visible_primitive_debug_ok	( GetEulerMatAngleRatio,	"GetEulerMatAngleRatio");

