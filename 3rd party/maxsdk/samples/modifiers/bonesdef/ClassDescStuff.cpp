/**********************************************************************

FILE: ClassdescStuff.cpp

DESCRIPTION:  Contains all our class desc stuff

CREATED BY: Peter Watje

HISTORY: 12/12/01

*>	Copyright (c) 1998, All Rights Reserved.

**********************************************************************/
#include "max.h"
#include "mods.h"
#include "iparamm.h"
#include "shape.h"
#include "spline3d.h"
#include "splshape.h"
#include "linshape.h"
#include "surf_api.h"
#include "polyobj.h"

// This uses the linked-list class templates
#include "linklist.h"
#include "bonesdef.h"
#include "macrorec.h"
#include "modstack.h"
#include "ISkin.h"
#include "ISkinEngine.h"
#include "MaxIcon.h"



// parameter setter callback, reflect any ParamBlock-mediated param setting in instance data members.
// JBW: since the old path controller kept all parameters as instance data members, this setter callback
// is implemented to to reduce changes to existing code
class BonesDefPBAccessor : public PBAccessor
{
public:
	TSTR GetLocalName(ReferenceMaker* owner, ParamID id, int tabIndex)
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		TSTR out;
		switch (id)
		{
		case skin_gizmos_list:
			{
				ReferenceTarget *ref;
				ref = p->pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,tabIndex);
				GizmoClass *gizmo = (GizmoClass *)ref;
				if (gizmo)
				{
					const TCHAR *name = gizmo->GetName();
					out.printf(_T("%s"),name);
				}
				break;
			}
		}
		return out;
	}

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid)
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		int val;
		switch (id)
		{
		case skin_draw_all_envelopes:
			if (p->pblock_display)
			{
				p->pblock_display->GetValue(skin_display_draw_all_envelopes,t,val,FOREVER);
				v.i = val;
			}
			break;
		case skin_draw_vertices:
			if (p->pblock_display)
			{
				p->pblock_display->GetValue(skin_display_draw_vertices,t,val,FOREVER);
				v.i = val;
			}
			break;
		case skin_ref_frame:
			if (p->pblock_advance)
			{
				p->pblock_advance->GetValue(skin_advance_ref_frame,t,val,FOREVER);
				v.i = val;
			}
			break;
		case skin_always_deform:
			if (p->pblock_advance)
			{
				p->pblock_advance->GetValue(skin_advance_always_deform,t,val,FOREVER);
				v.i = val;
			}
			break;
		}
	}

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		int val;
		switch (id)
		{
		case skin_filter_vertices:
			if (p->ip)
			{
				if (v.i == 0)
				{
					EnableWindow(GetDlgItem(p->hParam,IDC_RING),FALSE);
					EnableWindow(GetDlgItem(p->hParam,IDC_LOOP),FALSE);
					EnableWindow(GetDlgItem(p->hParam,IDC_GROW),FALSE);
					EnableWindow(GetDlgItem(p->hParam,IDC_SHRINK),FALSE);

					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_RING),FALSE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_LOOP),FALSE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_GROW),FALSE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_SHRINK),FALSE);
				}
				else
				{
					EnableWindow(GetDlgItem(p->hParam,IDC_RING),TRUE);
					EnableWindow(GetDlgItem(p->hParam,IDC_LOOP),TRUE);
					EnableWindow(GetDlgItem(p->hParam,IDC_GROW),TRUE);
					EnableWindow(GetDlgItem(p->hParam,IDC_SHRINK),TRUE);

					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_RING),TRUE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_LOOP),TRUE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_GROW),TRUE);
					EnableWindow(GetDlgItem(p->weightToolHWND,IDC_SHRINK),TRUE);
				}
			}
			break;
		case skin_filter_bones:
			if (v.i==0)
			{
				if (p->IsValidBoneIndex(p->ModeBoneIndex))
				{
					BoneDataClass &boneData = p->BoneData[p->ModeBoneIndex];
					boneData.end1Selected = FALSE;
					boneData.end2Selected = FALSE;
					p->ModeBoneEndPoint = -1;
				}
			}
			break;
		case skin_draw_all_envelopes:
			if (p->pblock_display)
			{
				val = v.i;
				p->pblock_display->SetValue(skin_display_draw_all_envelopes,t,val);
			}
			break;
		case skin_draw_vertices:
			if (p->pblock_display)
			{
				val = v.i;
				p->pblock_display->SetValue(skin_display_draw_vertices,t,val);
			}
			break;
		case skin_ref_frame:
			if (p->pblock_advance)
			{
				val = v.i;
				p->pblock_advance->SetValue(skin_advance_ref_frame,t,val);
			}
			break;
		case skin_always_deform:
			if (p->pblock_advance)
			{
				// only change the value if it has to be changed
				p->pblock_advance->GetValue(skin_advance_always_deform,t,val,FOREVER);
				if ( val != v.i )
				{
					val = v.i;
					p->pblock_advance->SetValue(skin_advance_always_deform,t,val);
					// This is hit when user changes the flag using MaxScript.
					p->ForceRecomuteBaseNode (TRUE);
					p->UpdateEndPointDelta();
				}
			}
			break;
		case skin_paintblendmode:
			p->OnBrushPresetContextUpdated();
			break;
		}
	}
};

static BonesDefPBAccessor bonesdef_accessor;

// parameter setter callback, reflect any ParamBlock-mediated param setting in instance data members.
// JBW: since the old path controller kept all parameters as instance data members, this setter callback
// is implemented to to reduce changes to existing code
class BonesDefDisplayPBAccessor : public PBAccessor
{
public:

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		int val;
		switch (id)
		{
		case skin_display_shadeweights:
			if (p->pblock_display)
			{
				val = v.i;
				if (val == FALSE)
				{
					if ((p->ip)  && (p->ip->GetSubObjectLevel() == 1)  )
					{
						p->RestoreVCMode();
					}
				}
				else
				{
					if ((p->ip)  && (p->ip->GetSubObjectLevel() == 1)  )
					{
						p->SetVCMode();
					}
				}
			}
			break;
		}
	}
};

static BonesDefDisplayPBAccessor bonesdefdisplay_accessor;

class BonesDefAdvancePBAccessor : public PBAccessor
{
public:

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		switch (id)
		{
		case skin_advance_bonelimit:
			p->InvalidateWeightCache();
			p->Reevaluate(TRUE);
			break;
		}
	}
};

static BonesDefAdvancePBAccessor bonesdefadvance_accessor;

static BonesDefClassDesc bonesDefDesc;
extern ClassDesc* GetBonesDefModDesc() {return &bonesDefDesc;}

//static int outputIDs[] = {IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_RADIO5,IDC_RADIO6};

//
// Parameters

#define PARAMDESC_LENGTH	18

//MIRROR
class BonesDefMirrorPBAccessor : public PBAccessor
{
public:

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		BonesDefMod* p = (BonesDefMod*)owner;
		switch (id)
		{
		case skin_mirrormanualupdate:
			if (!v.i)
			{
				p->mirrorData.BuildBonesMirrorData();
				p->mirrorData.BuildVertexMirrorData();
			}
			break;

		case skin_mirrorplane:
		case skin_mirroroffset:
		case skin_mirrorinitialtm:
		case skin_mirrorthreshold:
			{
				BOOL manualUpdate;
				p->pblock_mirror->GetValue(skin_mirrormanualupdate,0,manualUpdate,FOREVER);

				if (!manualUpdate)
				{
					p->mirrorData.BuildBonesMirrorData();
					p->mirrorData.BuildVertexMirrorData();
				}
				break;
			}
		case skin_mirrorenabled:
			if (v.i)
			{
				p->mirrorData.InitializeData(p);
				p->mirrorData.BuildBonesMirrorData();
				p->mirrorData.BuildVertexMirrorData();
				p->mirrorData.EnableUIButton(TRUE);
			}
			else
			{
				p->mirrorData.EnableUIButton(FALSE);
				p->mirrorData.Free();
			}
			break;
		}
	}
};

static BonesDefMirrorPBAccessor bonesdefmirror_accessor;

// per instance param block
static ParamBlockDesc2 skin_param_blk ( skin_params, _T("Parameters"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_PARAM_REF,
	//rollout
	IDD_BONESDEFPARAM, IDS_PW_PARAMETER, 0, 0, NULL,
	// params
	skin_effect,  _T("effect"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_PW_EFFECT,
		p_default, 		1.0f,
		p_range, 		0.f, 1.0f,
//		p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EFFECT,IDC_EFFECTSPIN,  SPIN_AUTOSCALE,
		p_end,

	skin_filter_vertices, 	_T("filter_vertices"),		TYPE_BOOL, 		0,				IDS_PW_FILTERVERTICES,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FILTER_VERTICES_CHECK,
		p_enable_ctrls, 2, skin_backfacecull, skin_selectelement,
		p_accessor,		&bonesdef_accessor,
		p_end,
	skin_filter_bones, 	_T("filter_envelopes"),		TYPE_BOOL, 		0,				IDS_PW_FILTERBONES,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FILTER_BONES_CHECK,
		p_accessor,		&bonesdef_accessor,
		p_end,
	skin_filter_envelopes, 	_T("filter_cross_sections"),		TYPE_BOOL, 		0,			IDS_PW_FILTERENVELOPES	,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FILTER_ENVELOPES_CHECK,
		p_end,

	skin_draw_all_envelopes, 	_T("draw_all_envelopes"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DRAWALLENVELOPES,
		p_default, 		FALSE,
//		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_DRAWALL_ENVELOPES_CHECK,
		p_accessor,		&bonesdef_accessor,
		p_end,

	skin_draw_vertices, 	_T("draw_vertices"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_COLORVERTS,
		p_default, 		TRUE,
//		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_DRAW_VERTICES_CHECK,
		p_accessor,		&bonesdef_accessor,
		p_end,

	skin_ref_frame,  _T("ref_frame"),	TYPE_INT, 	P_RESET_DEFAULT, 	IDS_PW_REFFRAME,
		p_default, 		0,
//		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_REF_FRAME,IDC_REF_FRAME_SPIN,  SPIN_AUTOSCALE,
		p_accessor,		&bonesdef_accessor,
		p_end,
	skin_paint_radius,  _T("paint_radius"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_PW_RADIUS,
		p_default, 		20.0f,
		p_range, 		0.f, 5000.0f,
//		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_SRADIUS,IDC_SRADIUSSPIN,  SPIN_AUTOSCALE,
		p_end,

	skin_paint_feather,  _T("paint_feather"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_PW_FEATHER,
		p_default, 		0.7f,
		p_range, 		0.f, 1.0f,
//		p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FEATHER,IDC_FEATHERSPIN,  SPIN_AUTOSCALE,
		p_end,

	skin_cross_radius,  _T("cross_radius"),	TYPE_FLOAT, 	0, 	IDS_PW_RADIUS,
		p_default, 		10.f,
		p_range, 		0.f, 1000000.0f,
		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_ERADIUS,IDC_ERADIUSSPIN,  SPIN_AUTOSCALE,
		p_end,

	skin_always_deform, 	_T("always_deform"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_ALWAYS_DEFORM,
		p_default, 		TRUE,
		p_accessor,		&bonesdef_accessor,
//		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_ALWAYSDEFORM_CHECK,
		p_end,

	skin_paint_str,  _T("paint_str"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_PW_PAINTSTR,
		p_default, 		1.0f,
		p_range, 		0.f, 1.0f,
//		p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PAINT_STR2,IDC_PAINT_STR_SPIN2,  SPIN_AUTOSCALE,
		p_end,

	skin_local_squash,  _T("localSquash"),	TYPE_FLOAT_TAB,0, 	P_ANIMATABLE, 	IDS_PW_LOCALSQUASH,
		p_end,

	skin_initial_squash,  _T("initialSquash"),	TYPE_FLOAT_TAB,0, 	P_RESET_DEFAULT, 	IDS_PW_INITIALSQUASH,
		p_end,

	skin_initial_staticenvelope,  _T("initialStaticEnvelope"),	TYPE_BOOL, 	P_RESET_DEFAULT, 	0,
		p_default, 		FALSE,
		p_end,

	skin_initial_envelope_innerpercent,  _T("initialInnerEnvelopePercent"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	0,
		p_default, 		0.75f,
		p_end,
	skin_initial_envelope_outerpercent,  _T("initialOuterEnvelopePercent"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	0,
	 	p_default, 		3.5f,
		p_end,

	skin_initial_envelope_inner,  _T("initialEnvelopeInner"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	0,
		p_default, 		10.0f,
		p_end,
	skin_initial_envelope_outer,  _T("initialEnvelopeOuter"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	0,
		p_default, 		50.0f,
		p_end,

	skin_paintblendmode, 	_T("paintBlendMode"),		TYPE_BOOL, 		0 ,  IDS_PW_PAINTBLENDMODE,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FILTER_VERTICES_CHECK2,
		p_accessor,		&bonesdef_accessor,
		p_end,

	skin_backfacecull, 	_T("backFaceCull"),		TYPE_BOOL, 		P_RESET_DEFAULT ,  IDS_PW_BACKFACECULL,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_BACKFACECULL_CHECK,
		p_end,

	skin_weighttool_tolerance,  _T("weightTool_tolerance"),	TYPE_FLOAT, 	0, 	IDS_PW_TOLERANCE,
		p_default, 		0.1f,
		p_end,

	skin_weighttool_scale,  _T("weightTool_scale"),	TYPE_FLOAT, 	0, 	IDS_PW_SCALE,
		p_default, 		0.95f,
		p_end,

	skin_weighttool_weight,  _T("weightTool_weight"),	TYPE_FLOAT, 	0, 	IDS_PW_WEIGHT,
		p_default, 		0.5f,
		p_end,

	skin_selectelement, 	_T("selectElement"),		TYPE_BOOL, 		0 ,  IDS_PW_SELECTELEMENT,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_SELECTELEMENT_CHECK,
		p_end,

	p_end
);

//MIRROR
static ParamBlockDesc2 skin_mirror_blk ( skin_mirror, _T("Mirror Parameters"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_MIRROR_REF,
	//rollout
	IDD_BONESDEFMIRROR, IDS_PW_MIRRORPARAMETER, 0, 0, NULL,
	// params
	skin_mirrorplane, 			_T("mirrorPlane"), 		TYPE_INT, 	0, 	IDS_MIRRORPLANE,
		p_default, 		0,
		p_ui, TYPE_INTLISTBOX, IDC_MIRRORCOMBO, 3, IDS_X, IDS_Y, IDS_Z,
		p_accessor,		&bonesdefmirror_accessor,
		p_end,

	skin_mirroroffset,  _T("mirrorOffset"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_MIRROSOFFSET,
		p_default, 		0.0f,
		p_range, 		-100000.0f, 100000.0f,
		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_MIRROROFFSET,IDC_MIRROROFFSETSPIN,  SPIN_AUTOSCALE,
		p_accessor,		&bonesdefmirror_accessor,
		p_end,

	skin_mirrorinitialtm, 	_T("mirrorUseInitialTM"),		TYPE_BOOL, 		0,				IDS_USEINITIALTM,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_USEINTIALPOSE_CHECK,
		p_accessor,		&bonesdefmirror_accessor,
		p_end,

	skin_mirrorenabled, 	_T("mirrorEnabled"),		TYPE_BOOL, 		0,				IDS_MIRRORENABLED,
		p_default, 		FALSE,
		p_ui, 			TYPE_CHECKBUTTON, 	IDC_MIRRORMODE,
		p_accessor,		&bonesdefmirror_accessor,
		p_end,

	skin_mirrorthreshold,  _T("mirrorThreshold"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_MIRRORTHRESHOLD,
		p_default, 		0.5f,
		p_range, 		0.0f, 100000.0f,
		p_ui, 			TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_MIRRORTHRESHOLD,IDC_MIRRORTHRESHOLDSPIN,  SPIN_AUTOSCALE,
		p_accessor,		&bonesdefmirror_accessor,
		p_end,

	skin_mirrorprojection, 			_T("mirrorProjection"), 		TYPE_INT, 	0, 	IDS_MIRRORPROJECTION,
		p_default, 		0,
		p_ui, TYPE_INTLISTBOX, IDC_PROJECTIONCOMBO, 4, IDS_MIRRORNORMAL, IDS_MIRRORPOS,IDS_MIRRORNEG, IDS_NONE,
		p_end,

	skin_mirrormanualupdate, 	_T("manualUpdate"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_MIRRORMANUAL,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_MANUALCHECK,
		p_end,

	skin_mirrorfast, 	_T("mirrorFast"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_MIRRORFAST,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FASTCHECK,
		p_end,

	p_end
);

// per instance display block
static ParamBlockDesc2 skin_display_blk ( skin_display, _T("display"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_DISPLAY_REF,
	//rollout
	IDD_BONESDEFDISPLAY, IDS_PW_DISPLAY, 0, APPENDROLL_CLOSED, NULL,
	// params
	skin_display_draw_all_envelopes, 	_T("draw_all_envelopes"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DRAWALLENVELOPES,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_DRAWALL_ENVELOPES_CHECK,
		p_end,

	skin_display_draw_vertices, 	_T("draw_vertices"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_COLORVERTS,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_DRAW_VERTICES_CHECK,
		p_end,

	skin_display_all_gizmos, 	_T("draw_all_gizmos"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DRAWALLGIZMOS,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_SHOW_ALL_GIZMOS_CHECK,
		p_end,

	skin_display_all_vertices, 	_T("draw_all_vertices"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DRAWALLVERTS,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_SHOW_ALL_VERTICES_CHECK,
		p_end,

	skin_display_shadeweights, 	_T("shadeweights"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHADEWEIGTHS,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_DRAW_FACES_CHECK,
		p_accessor,		&bonesdefdisplay_accessor,
		p_end,

	skin_display_envelopesalwaysontop, 	_T("envelopesAlwaysOnTop"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_ENVONTOP,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_ENVELOPEONTOP_CHECK,
		p_end,

	skin_display_crosssectionsalwaysontop, 	_T("crossSectionsAlwaysOnTop"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_CROSSONTOP,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_CROSSONTOP_CHECK,
		p_end,

	skin_display_shownoenvelopes, 	_T("showNoEnvelopes"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHADEWEIGTHS,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 		IDC_SHOWNOENVELOPES_CHECK,
		p_end,

	skin_display_colorallweights, 	_T("colorAllWeights"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHADEWEIGTHS,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_COLORALLWEIGHTS_CHECK,
		p_end,

	skin_display_weightcolors,  _T("weightColors"),	TYPE_POINT3_TAB,0, 	0, 	IDS_PW_WEIGHTCOLORS,
		p_end,

	skin_display_showhiddenvertices, 	_T("showHiddenVertices"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWHIDDENVERTS,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_SHOWHIDDENVERTICES_CHECK,
		p_end,

	p_end
);

// per instance display block
static ParamBlockDesc2 skin_advance_blk ( skin_advance, _T("advance"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_ADVANCE_REF,
	//rollout
	IDD_BONESDEFADVANCE, IDS_PW_ADVANCE, 0, APPENDROLL_CLOSED, NULL,
	// params
	skin_advance_ref_frame,  _T("ref_frame"),	TYPE_INT, 	P_RESET_DEFAULT, 	IDS_PW_REFFRAME,
		p_default, 		0,
		p_range,		-100000,100000,
		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_REF_FRAME,IDC_REF_FRAME_SPIN,  SPIN_AUTOSCALE,
		p_end,
	skin_advance_always_deform, 	/*Trying to hide the advance one_T("always_deform")*/_T(""),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_ALWAYS_DEFORM,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_ALWAYSDEFORM_CHECK,
		p_end,

	skin_advance_rigid_verts, 	_T("rigid_vertices"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_RIGIDVERTS,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_RIGID_VERTICES_CHECK,
		p_end,

	skin_advance_rigid_handles, 	_T("rigid_handles"),		TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_RIGIDHANDLES,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_RIGID_HANDLES_CHECK,
		p_end,
	skin_advance_fast_update, 	_T("fast_update"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_FAST,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_FAST_CHECK,
		p_end,

	skin_advance_no_update, 	_T("no_update"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_NOUPDATE,
		p_default, 		FALSE,
		p_end,

	skin_advance_updateonmouseup, 	_T("updateOnMouseUp"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_UPDATEONMOUSEUP,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_UPDATEONMOUSEUP_CHECK,
		p_end,

	skin_advance_bonelimit,  _T("bone_Limit"),	TYPE_INT, 	P_RESET_DEFAULT, 	IDS_PW_BONELIMIT,
		p_default, 		20,
		p_range,		1,100,
		p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_BONELIMIT,IDC_BONELIMIT_SPIN,  SPIN_AUTOSCALE,
		p_accessor,		&bonesdefadvance_accessor,
		p_end,

	skin_advance_backtransform, 	_T("backTransform"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_BACKTRANSFORM,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_BACKTRANSFORM_CHECK,
		p_end,

	skin_advance_shortennames, 	_T("shortenBoneNames"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHORTENNAMES,
		p_default, 		FALSE,
		p_end,

	skin_advance_fastsubanims, 	_T("fastSubAnims"),	TYPE_BOOL, 		P_RESET_DEFAULT,  0,
		p_default, 		TRUE,
		p_end,

	skin_advance_fasttmcache, 	_T("fastTMCache"),	TYPE_BOOL, 		P_RESET_DEFAULT,  0,
		p_default, 		TRUE,
		p_end,

	skin_advance_fastvertexweighting, 	_T("fastVertexWeighting"),	TYPE_BOOL, 		P_RESET_DEFAULT,  0,
		p_default, 		TRUE,
		p_end,

	skin_advance_fastgizmo, 	_T("fastGizmos"),	TYPE_BOOL, 		P_RESET_DEFAULT,  0,
		p_default, 		TRUE,
		p_end,

//5.1.03
	skin_advance_ignorebonescale, 	_T("ignoreBoneScale"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_IGNOREBONESCALE,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_IGNOREBONESCALE_CHECK,
		p_end,

	skin_advance_animatable_envelopes, 	_T("animatableEnvelopes"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_ANIMATABLEENVELOPES,
		p_default, 		FALSE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_ANIMATABLE_ENVELOPES_CHECK,
		p_end,

	skin_advance_weightall, 	_T("weightAllVertices"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_WEIGHTALLVERTICES,
		p_default, 		TRUE,
		p_ui, 			TYPE_SINGLECHECKBOX, 	IDC_WEIGHT_ALL_VERTICES_CHECK,
		p_end,

	skin_advance_clearzerolimit,  _T("clearZeroLimit"),	TYPE_FLOAT, 	P_RESET_DEFAULT, 	IDS_PW_CLEARZEROLIMIT,
		p_default, 		0.0f,
		p_range,		0.0f,1.0f,
		p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_CLEARLIMIT,IDC_CLEARLIMIT_SPIN,  0.01,
		p_accessor,		&bonesdefadvance_accessor,
		p_end,
	skin_advance_enable_dq_blending, 	_T("enableDQ"),	TYPE_BOOL, 		P_RESET_DEFAULT,  0,  //toggles whether the DQ deformation is blended into the linear deformation, the vertex DQBlendWeight determines how much is blended in
		p_default, 		TRUE,
		p_end,
	p_end
);	

//WEIGHTTABLE

class WeightTablePBAccessor : public PBAccessor
{
public:

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		BonesDefMod* p = (BonesDefMod*)owner;

		if (id == skin_wt_showglobal)
		{
			if (v.i)
				p->pblock_weighttable->SetValue(skin_wt_jbuimethod,0,FALSE);
			else p->pblock_weighttable->SetValue(skin_wt_jbuimethod,0,TRUE);
		}

		if ( (id == skin_wt_showoptionui) || (id == skin_wt_showsetui))
		{
			if (v.i)
				p->pblock_weighttable->SetValue(skin_wt_tabley,0,120);
			else p->pblock_weighttable->SetValue(skin_wt_tabley,0,10);
		}

		switch (id)
		{
		case	skin_wt_flipui:
			{
				if (p->weightTableWindow.hWnd)
				{
#ifdef DEBUGMODE
					if (p->GetDebugMode()) ScriptPrint("Flip UI Starting Reset Scroll Bar function\n");
#endif

					p->weightTableWindow.ResetScrollBars();

#ifdef DEBUGMODE
					if (p->GetDebugMode()) ScriptPrint("Flip UI Starting Update UI function\n");
#endif
					p->weightTableWindow.UpdateUI();

#ifdef DEBUGMODE
					if (p->GetDebugMode()) ScriptPrint("Flip UI Invalidating the rect\n");
#endif
					InvalidateRect(p->weightTableWindow.hWnd,NULL,TRUE);

#ifdef DEBUGMODE
					if (p->GetDebugMode()) ScriptPrint("Flip UI Updating Window\n");
#endif
					UpdateWindow( p->weightTableWindow.hWnd);
#ifdef DEBUGMODE
					if (p->GetDebugMode()) ScriptPrint("Flip UI Done\n");
#endif
				}
			}
		case	skin_wt_affectselected:
		case	skin_wt_showattrib:
		case	skin_wt_showglobal:
		case	skin_wt_showoptionui:
		case	skin_wt_showsetui:
		case	skin_wt_shortenlabel:
		case	skin_wt_showexclusion:
		case	skin_wt_showlock:
		case	skin_wt_tabley:
		case	skin_wt_showcopypasteui:
		case	skin_wt_updateonmouseup:
		case	skin_wt_jbuimethod:
		case	skin_wt_fontsize:
		case	skin_wt_attriblabelheight:
			{
				if (p->weightTableWindow.hWnd)
				{
					p->weightTableWindow.ResetFont();
					p->weightTableWindow.SetFontSize();
					p->weightTableWindow.BringDownEditField();
					p->weightTableWindow.ResizeWindowControls();
					p->weightTableWindow.UpdateUI();
					p->weightTableWindow.InvalidateViews();
				}
				break;
			}
		case	skin_wt_currentvertexset:

			{
				if (p->weightTableWindow.hWnd)
				{
					if (v.i < 0) v.i = 0;
					if (v.i >= (p->weightTableWindow.CustomListCount()+3)) v.i = p->weightTableWindow.CustomListCount()+2;
					p->weightTableWindow.FillOutVertexList();
					p->weightTableWindow.UpdateDeleteButton();
					p->weightTableWindow.BringDownEditField();
					p->weightTableWindow.ResizeWindowControls();
					p->weightTableWindow.UpdateUI();
					p->weightTableWindow.InvalidateViews();
				}
				break;
			}
		case	skin_wt_showaffectbones:
			{
				if (p->weightTableWindow.hWnd)
				{
					p->weightTableWindow.RecomputeBones();
				}
			}
		}
	}
};

static WeightTablePBAccessor weighttable_accessor;

// per instance display block
static ParamBlockDesc2 skin_weighttable_blk ( skin_weighttable, _T("weightTable"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT , PBLOCK_WEIGHTTABLE_REF,

	// params

	skin_wt_affectselected, 	_T("wt_affectSelected"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_AFFECTSELECTED,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_showaffectbones, 	_T("wt_showAffectedBones"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWAFFECTEDBONES,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_updateonmouseup, 	_T("wt_updateOnMouseUp"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_UPDATEONMOUSEUP,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_flipui, 	_T("wt_flipUI"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_FLIPUI,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_showattrib, 	_T("wt_showAttributes"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWATTRIBUTES,
		p_default, 		TRUE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showglobal, 	_T("wt_showGlobal"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWGLOBAL,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_shortenlabel, 	_T("wt_shortenLabels"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHORTENLABELS,
		p_default, 		TRUE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showexclusion, 	_T("wt_showExclusions"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWEXLUSIONS,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showlock, 	_T("wt_showLocks"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWLOCKS,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_showoptionui, 	_T("wt_showOptionsUI"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWOPTIONSUI,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showsetui, 	_T("wt_showSetUI"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWSETUI,
		p_default, 		FALSE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showcopypasteui, 	_T("wt_showCopyPasteUI"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWCOPYPASTEUI,
		p_default, 		TRUE,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_showmenu, 	_T("wt_showMenu"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWMENU,
		p_default, 		TRUE,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_tabley, 	_T("wt_tableY"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_TABLEY,
		p_default, 		10,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_jbuimethod, 	_T(""),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_JBUIMETHOD,
		p_default, 		TRUE,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_precision, 	_T("wt_precision"),	TYPE_FLOAT, 		P_RESET_DEFAULT,  IDS_PW_PRECISION,
		p_default, 		0.01f,
		p_range,		0.001f,0.5f,
		p_accessor,		&weighttable_accessor,
		p_end,
	skin_wt_fontsize, 	_T("wt_fontSize"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_FONTSIZE,
		p_default, 		14,
		p_range,		8,24,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_xpos, 	_T("wt_winXPos"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_XPOS,
		p_default, 		0,
		p_end,

	skin_wt_ypos, 	_T("wt_winYPos"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_YPOS,
		p_default, 		0,
		p_end,

	skin_wt_width, 	_T("wt_winWidth"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_WIDTH,
		p_default, 		800,
		p_end,
	skin_wt_height,	_T("wt_winHeight"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_HEIGHT,
		p_default, 		500,
		p_end,

	skin_wt_dragleftright, 	_T("wt_dragLeftRightMode"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DRAGMODE,
		p_default, 		TRUE,
		p_end,

	skin_wt_currentvertexset, 	_T("wt_activeVertexSet"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_ACTIVEVERTSET,
		p_default, 		0,
		p_accessor,		&weighttable_accessor,
		p_end,

	skin_wt_showmarker,		_T("wt_showMarker"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_SHOWMARKER,
		p_default, 		TRUE,
		p_end,
	skin_wt_markertype,		_T("wt_markerType"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_MARKERTYPE,
		p_default, 		6,
		p_range,		0,14,
		p_end,
	skin_wt_markercolor,		_T("wt_markerColor"),	TYPE_RGBA, 		P_RESET_DEFAULT,  IDS_PW_MARKERCOLOR,
		p_default, Color(1.0f, 0.0f, 1.0f),
		p_end,

	skin_wt_debugmode,		_T("debugMode"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_DEBUGMODE,
		p_default, 		FALSE,
		p_end,

	skin_wt_attriblabelheight,		_T("wt_attribLabelHeight"),	TYPE_INT, 		P_RESET_DEFAULT,  IDS_PW_ATTRIBLABELHEIGHT,
		p_default, 		1,
		p_accessor,		&weighttable_accessor,
		p_end,

	//5.1.01 adds left/right justification
	skin_wt_rightjustify,		_T("rightJustifyBoneText"),	TYPE_BOOL, 		P_RESET_DEFAULT,  IDS_PW_RIGHTJUSTIFY,
		p_default, 		TRUE,
		p_end,

	p_end
);

// per instance gizmo block
static ParamBlockDesc2 skin_gizmos_blk ( skin_gizmos, _T("gizmos"),  0, &bonesDefDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, PBLOCK_GIZMOS_REF,
	//rollout
	IDD_BONESDEFGIZMOS, IDS_PW_GIZMOS, 0, APPENDROLL_CLOSED, NULL,
	// params
	skin_gizmos_list, 	_T("gizmos"),		TYPE_REFTARG_TAB, 	0,	P_SUBANIM|P_COMPUTED_NAME,  IDS_PW_GIZMOS,
		p_accessor,		&bonesdef_accessor,

		p_end,

	p_end
);

static ParamBlockDescID descVer0[17] =
{
	{ TYPE_FLOAT, NULL, FALSE,  skin_effect },		// Effect
	{ TYPE_INT,   NULL, FALSE, static_cast<DWORD>(-1) },		// Lock Bone
	{ TYPE_INT,   NULL, FALSE, static_cast<DWORD>(-1) },		// Absolute Influence
	{ TYPE_INT,   NULL, FALSE, skin_filter_vertices },		// Filter Vertices
	{ TYPE_INT,   NULL, FALSE, skin_filter_bones },		// Filter Bones
	{ TYPE_INT,   NULL, FALSE, skin_filter_envelopes },		// Filter Envelopes
	{ TYPE_INT,   NULL, FALSE, skin_draw_all_envelopes },		// Draw All envelopes
	{ TYPE_INT,   NULL, FALSE, skin_draw_vertices },		// Draw vertice
	{ TYPE_INT,   NULL, FALSE, skin_ref_frame },		// Ref Frame
	{ TYPE_FLOAT, NULL, FALSE,  skin_paint_feather},		// Radius
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1)},		// Project through
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// falloff
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// bone falloff
	{ TYPE_FLOAT, NULL, FALSE,  skin_paint_feather},		// feather
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// Draw bone envelope
	{ TYPE_FLOAT, NULL, FALSE,  skin_cross_radius},	// envelope raduis
	{ TYPE_INT, NULL, FALSE,  skin_always_deform}		// always deform
};

static ParamBlockDescID descVer1[18] =
{
	{ TYPE_FLOAT, NULL, FALSE,  skin_effect },		// Effect
	{ TYPE_INT,   NULL, FALSE, static_cast<DWORD>(-1) },		// Lock Bone
	{ TYPE_INT,   NULL, FALSE, static_cast<DWORD>(-1) },		// Absolute Influence
	{ TYPE_INT,   NULL, FALSE, skin_filter_vertices },		// Filter Vertices
	{ TYPE_INT,   NULL, FALSE, skin_filter_bones },		// Filter Bones
	{ TYPE_INT,   NULL, FALSE, skin_filter_envelopes },		// Filter Envelopes
	{ TYPE_INT,   NULL, FALSE, skin_draw_all_envelopes },		// Draw All envelopes
	{ TYPE_INT,   NULL, FALSE, skin_draw_vertices },		// Draw vertice
	{ TYPE_INT,   NULL, FALSE, skin_ref_frame },		// Ref Frame
	{ TYPE_FLOAT, NULL, FALSE,  skin_paint_feather},		// Radius
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// Project through
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// falloff
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// bone falloff
	{ TYPE_FLOAT, NULL, FALSE,  skin_paint_feather},		// feather
	{ TYPE_INT, NULL, FALSE,  static_cast<DWORD>(-1) },		// Draw bone envelope
	{ TYPE_FLOAT, NULL, FALSE,  skin_cross_radius},	// envelope raduis
	{ TYPE_INT, NULL, FALSE,  skin_always_deform},		// always deform
	{ TYPE_FLOAT, NULL, FALSE,  skin_paint_str}	// paint str
};

#define PBLOCK_LENGTH	18

#define CURRENT_VERSION	2

static ParamVersionDesc versions[] = {
	ParamVersionDesc(descVer0,17,0),
	ParamVersionDesc(descVer1,18,1)
};
#define NUM_OLDVERSIONS	2

// Current version
static ParamVersionDesc curVersion(descVer1,PBLOCK_LENGTH,CURRENT_VERSION);

IOResult BonesDefMod::Save(ISave *isave)
{
	if (hWeightTable)
	{
		if (!weightTableWindow.isDocked)
			weightTableWindow.SaveWindowState();
	}

	Modifier::Save(isave);
	ULONG nb;

	int c = BoneData.Count();
	isave->BeginChunk(BONE_COUNT_CHUNK);
	isave->Write(&c,sizeof(c),&nb);
	isave->EndChunk();

	//write bone chunks

	for (int i = 0; i < c; i++)
	{
		isave->BeginChunk(BONE_DATATM_CHUNK);
		BoneData[i].tm.Save(isave);
		isave->EndChunk();
	}

	for (int i = 0; i < c; i++)
	{
		isave->BeginChunk(BONE_INITDATATM_CHUNK);
		BoneData[i].InitNodeTM.Save(isave);
		isave->EndChunk();
	}
	//5.1.03
	if (hasStretchTM)
	{
		for (int i = 0; i < c; i++)
		{
			isave->BeginChunk(BONE_INITSTRETCHTM_CHUNK);
			BoneData[i].InitStretchTM.Save(isave);
			isave->EndChunk();
		}
	}

	isave->BeginChunk(BONE_DATA_CHUNK);
	for (int i = 0; i < c; i++)
	{
		Point3 save_pt;
		float save_f;
		BYTE save_b;
		int save_i;

		BoneDataClass &boneData = BoneData[i];
		save_i = boneData.CrossSectionList.Count();
		isave->Write(&save_i,sizeof(save_i),&nb);

		for (int j = 0; j < boneData.CrossSectionList.Count(); j++)
		{
			save_f = boneData.CrossSectionList[j].u;
			isave->Write(&save_f,sizeof(save_f),&nb);

			save_i = boneData.CrossSectionList[j].RefInnerID;
			isave->Write(&save_i,sizeof(save_i),&nb);

			save_i = boneData.CrossSectionList[j].RefOuterID;
			isave->Write(&save_i,sizeof(save_i),&nb);
		}

		save_b = boneData.flags;
		isave->Write(&save_b,sizeof(save_b),&nb);

		save_b = boneData.FalloffType;
		isave->Write(&save_b,sizeof(save_b),&nb);

		save_i = boneData.BoneRefID;
		isave->Write(&save_i,sizeof(save_i),&nb);

		save_i = boneData.RefEndPt1ID;
		isave->Write(&save_i,sizeof(save_i),&nb);

		save_i = boneData.RefEndPt2ID;
		isave->Write(&save_i,sizeof(save_i),&nb);
	}
	isave->EndChunk();

	for (int i = 0; i < c; i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if ((boneData.flags & BONE_SPLINE_FLAG) && (boneData.Node != NULL) )
		{
			isave->BeginChunk(BONE_SPLINE_CHUNK);
			boneData.referenceSpline.Save(isave);
			isave->EndChunk();
		}
	}

	if (bindNode)
	{
		isave->BeginChunk(BONE_BIND_CHUNK);
		initialXRefTM.Save(isave);

		ULONG id = isave->GetRefID(bindNode);
		isave->Write(&id,sizeof(ULONG),&nb);

		isave->EndChunk();
	}

	NameTab names;
	for (int i = 0; i < c; i++)
	{
		TSTR temp(BoneData[i].name);
		names.AddName(temp);
	}

	isave->BeginChunk(BONE_NAME_CHUNK);
	names.Save(isave);
	isave->EndChunk();

	c = endPointDelta.Count();
	isave->BeginChunk(DELTA_COUNT_CHUNK);
	isave->Write(&c,sizeof(c),&nb);
	isave->EndChunk();

	isave->BeginChunk(DELTA_DATA_CHUNK);
	for (int i = 0; i < c; i++)
	{
		Point3 p = endPointDelta[i];
		isave->Write(&p,sizeof(p),&nb);
	}
	isave->EndChunk();
	//need to update for each vers of max
	int cver = 5;
	isave->BeginChunk(VER_CHUNK);
	isave->Write(&cver,sizeof(cver),&nb);
	isave->EndChunk();

	if (namedSel.Count())
	{
		int ct = namedSel.Count();
		isave->BeginChunk(NAMEDSEL_STRINGCOUNT_CHUNK);
		isave->Write(&ct,sizeof(ct),&nb);
		isave->EndChunk();

		for (int i=0; i<ct; i++) {
			if (namedSel[i])
			{
				isave->BeginChunk(NAMEDSEL_STRINGID_CHUNK);
				isave->Write(&i,sizeof(i),&nb);
				isave->EndChunk();

				isave->BeginChunk(NAMEDSEL_STRING_CHUNK);
				isave->WriteWString(*namedSel[i]);
				isave->EndChunk();
			}
		}
	}

	isave->BeginChunk(WEIGHTTABLE_CHUNK);
	weightTableWindow.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(SHOWWEIGHTTOOL_CHUNK);
	isave->Write(&showWeightTool,sizeof(showWeightTool),&nb);
	isave->EndChunk();

	isave->BeginChunk(WEIGHTTOOLPOS_CHUNK);
	isave->Write(&weightToolWindowPos,sizeof(weightToolWindowPos),&nb);
	isave->EndChunk();

	return IO_OK;
}

class BonesDefModPostLoad : public PostLoadCallback {
public:
	BonesDefMod *n;int version;

	BonesDefModPostLoad(BonesDefMod *ns, int ver)
	{
		n = ns;
		version = ver;
	}
	void proc(ILoad *iload) {
		for (int  i = 0; i < n->pblock_gizmos->Count(skin_gizmos_list); i++)
		{
			ReferenceTarget *ref;
			ref = n->pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,i);
			GizmoClass *gizmo = (GizmoClass *)ref;
			gizmo->bonesMod = n;
		}
		if (version <5) //need to turn off backtransform
		{
			n->pblock_advance->SetValue(skin_advance_backtransform,0,FALSE);
		}

		if (n->pblock_display->Count(skin_display_weightcolors) == 0)
			n->SetupColors();
		n->stopEvaluation = FALSE;
		delete this;
	}
};

IOResult BonesDefMod::Load(ILoad *iload)
{
	stopEvaluation = TRUE;
	Modifier::Load(iload);
	IOResult res = IO_OK;
	int NodeID = 0;

	BoneData.New();
	int currentvt = -1;
	ULONG nb;
	int bonecount = 0;
	int MatrixCount = 0;

	int initMatrixCount = 0;

	reloadSplines = TRUE;

	int bct = 0;

	ver = 3;
	int lastID = 0;

	//5.1.03
	hasStretchTM = FALSE;
	int initStretchMatrixCount = 0;

	while (IO_OK==(res=iload->OpenChunk())) {
		int id = iload->CurChunkID();
		switch(id)  {
		case WEIGHTTOOLPOS_CHUNK:
			iload->Read(&weightToolWindowPos,sizeof(weightToolWindowPos),&nb);
			break;
		case SHOWWEIGHTTOOL_CHUNK:
			iload->Read(&showWeightTool,sizeof(showWeightTool),&nb);
			break;

		case NAMEDSEL_STRINGCOUNT_CHUNK:
			{
				int ct = 0;
				iload->Read(&ct,sizeof(ct), &nb);
				namedSel.SetCount(ct);
				for (int i =0; i < ct; i++)
					namedSel[i] = NULL;
				break;
			}
		case NAMEDSEL_STRINGID_CHUNK:
			{
				iload->Read(&lastID,sizeof(lastID), &nb);
				break;
			}
		case NAMEDSEL_STRING_CHUNK: {
			TCHAR *name;
			res = iload->ReadWStringChunk(&name);
			TSTR *newName = new TSTR(name);
			namedSel[lastID] = newName;
			break;
									}

		case BASE_TM_CHUNK:
			{
				OldBaseTM.Load(iload);
				OldInverseBaseTM = Inverse(OldBaseTM);
				break;
			}
		case VER_CHUNK:
			{
				iload->Read(&ver,sizeof(ver),&nb);
				break;
			}

		case BONE_COUNT_CHUNK:
			{
				int c;
				iload->Read(&c,sizeof(c),&nb);

				for (int i=0; i<c; i++)
				{
					BoneDataClass t;
					BoneData.Append(t);
					BoneData[i].Node = NULL;
					BoneData[i].EndPoint1Control = NULL;
					BoneData[i].EndPoint2Control = NULL;
					BoneData[i].CrossSectionList.ZeroCount();
				}
				break;
			}
		case BONE_DATATM_CHUNK:
			{
				BoneData[MatrixCount++].tm.Load(iload);
				BoneData[MatrixCount-1].InitObjectTM = Inverse(BoneData[MatrixCount-1].tm); //ns
				recompInitTM = true;		//ns
				break;
			}

		case BONE_INITDATATM_CHUNK:
			{
				BoneData[initMatrixCount++].InitNodeTM.Load(iload);
				recompInitTM = false;		//ns
				break;
			}
			//5.1.03
		case BONE_INITSTRETCHTM_CHUNK:
			{
				hasStretchTM = TRUE;
				BoneData[initStretchMatrixCount++].InitStretchTM.Load(iload);
				break;
			}

		case BONE_NAME_CHUNK:
			{
				NameTab names;
				int c = BoneData.Count();
				names.Load(iload);

				for (int i = 0; i < c; i++)
				{
					TSTR temp(names[i]);
					BoneData[i].name = temp;
				}

				break;
			}

		case BONE_DATA_CHUNK:
			{
				float load_f;
				Point3 load_p;
				int load_i;
				BYTE load_b;

				for (int i = 0; i < BoneData.Count(); i++)
				{
					BoneDataClass &boneData = BoneData[i];
					iload->Read(&load_i,sizeof(load_i),&nb);
					boneData.CrossSectionList.SetCount(load_i);
					for (int j=0;j<boneData.CrossSectionList.Count();j++)
					{
						iload->Read(&load_f,sizeof(load_f),&nb);
						boneData.CrossSectionList[j].u = load_f;
						iload->Read(&load_i,sizeof(load_i),&nb);
						boneData.CrossSectionList[j].RefInnerID = load_i;
						iload->Read(&load_i,sizeof(load_i),&nb);
						boneData.CrossSectionList[j].RefOuterID = load_i;

						boneData.CrossSectionList[j].InnerControl = NULL;
						boneData.CrossSectionList[j].OuterControl = NULL;

						boneData.CrossSectionList[j].outerSelected = FALSE;
						boneData.CrossSectionList[j].innerSelected = FALSE;
						boneData.name.Resize(0);
					}

					iload->Read(&load_b,sizeof(load_b),&nb);
					boneData.flags = load_b;

					iload->Read(&load_b,sizeof(load_b),&nb);
					boneData.FalloffType = load_b;

					iload->Read(&load_i,sizeof(load_i),&nb);
					boneData.BoneRefID = load_i;

					iload->Read(&load_i,sizeof(load_i),&nb);
					boneData.RefEndPt1ID = load_i;

					iload->Read(&load_i,sizeof(load_i),&nb);
					boneData.RefEndPt2ID = load_i;

					boneData.end1Selected = FALSE;
					boneData.end2Selected = FALSE;
				}

				break;
			}
		case BONE_SPLINE_CHUNK:
			{
				reloadSplines = FALSE;
				for (int i = bct; i < BoneData.Count(); i++)
				{
					BoneDataClass &boneData = BoneData[i];
					if (boneData.flags & BONE_SPLINE_FLAG)
					{
						boneData.referenceSpline.Load(iload);
						bct= i+1;
						i = BoneData.Count();
					}
				}
				break;
			}

		case VERTEX_COUNT_CHUNK:
			{
				int c;
				iload->Read(&c,sizeof(c),&nb);
				OldVertexDataCount = c;
				OldVertexData.ZeroCount();
				OldVertexData.SetCount(c);
				for (int i=0; i<c; i++) {
					VertexListClassOld *vc = new VertexListClassOld;
					OldVertexData[i] = vc;
					OldVertexData[i]->modified = FALSE;
					OldVertexData[i]->selected = FALSE;
					OldVertexData[i]->d.ZeroCount();
				}

				break;
			}
		case VERTEX_DATA_CHUNK:
			{
				for (int i=0; i < OldVertexDataCount; i++)
				{
					int c;
					BOOL load_b;
					iload->Read(&c,sizeof(c),&nb);
					OldVertexData[i]->d.SetCount(c);

					iload->Read(&load_b,sizeof(load_b),&nb);
					OldVertexData[i]->modified = load_b;
					float load_f;
					int load_i;
					Point3 load_p;
					for (int j=0; j<c; j++) {
						iload->Read(&load_i,sizeof(load_i),&nb);
						iload->Read(&load_f,sizeof(load_f),&nb);
						OldVertexData[i]->d[j].Bones = load_i;
						OldVertexData[i]->d[j].Influences =load_f;

						iload->Read(&load_i,sizeof(load_i),&nb);
						OldVertexData[i]->d[j].SubCurveIds =load_i;
						iload->Read(&load_i,sizeof(load_i),&nb);
						OldVertexData[i]->d[j].SubSegIds =load_i;

						iload->Read(&load_f,sizeof(load_f),&nb);
						OldVertexData[i]->d[j].u = load_f;

						iload->Read(&load_p,sizeof(load_p),&nb);
						OldVertexData[i]->d[j].Tangents = load_p;

						iload->Read(&load_p,sizeof(load_p),&nb);
						OldVertexData[i]->d[j].OPoints = load_p;
					}
				}

				break;
			}
		case BONE_BIND_CHUNK:
			{
				initialXRefTM.Load(iload);
				ULONG id;
				iload->Read(&id,sizeof(ULONG), &nb);
				if (id!=0xffffffff)
				{
					iload->RecordBackpatch(id,(void**)&bindNode);
				}
				break;
			}

		case DELTA_COUNT_CHUNK:
			{
				int c;
				iload->Read(&c,sizeof(c),&nb);
				endPointDelta.SetCount(c);
				break;
			}

		case DELTA_DATA_CHUNK:
			{
				for (int i = 0; i < endPointDelta.Count();i++)
				{
					Point3 p;
					iload->Read(&p,sizeof(p),&nb);
					endPointDelta[i] = p;
				}
				break;
			}
		case WEIGHTTABLE_CHUNK:
			{
				weightTableWindow.Load(iload);
				break;
			}
		}
		iload->CloseChunk();
		if (res!=IO_OK)
			return res;
	}

	//in R3 the bone and cross section ref ids followed right after the pblock and a point3 ref
	//this gives us 8 blank spaces before them for r4 for addin more pblocks and other refs that
	//might be needed that are not dynamically allocated
	if (ver<4)
	{
		for (int i=0;i < BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			boneData.RefEndPt1ID+=8;
			boneData.RefEndPt2ID+=8;
			boneData.BoneRefID+=8;
			for (int j=0;j < boneData.CrossSectionList.Count();j++)
			{
				boneData.CrossSectionList[j].RefInnerID+=8;
				boneData.CrossSectionList[j].RefOuterID+=8;
			}
		}
	}

	//build reftable
	int ref_size = 0;
	for (int i=0;i < BoneData.Count();i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneData.RefEndPt1ID > ref_size) ref_size = boneData.RefEndPt1ID;
		if (boneData.RefEndPt2ID > ref_size) ref_size = boneData.RefEndPt2ID;
		if (boneData.BoneRefID > ref_size) ref_size = boneData.BoneRefID;
		for (int j=0;j < boneData.CrossSectionList.Count();j++)
		{
			if (boneData.CrossSectionList[j].RefInnerID > ref_size) ref_size = boneData.CrossSectionList[j].RefInnerID;
			if (boneData.CrossSectionList[j].RefOuterID > ref_size) ref_size = boneData.CrossSectionList[j].RefOuterID;
		}
	}
	RefTable.SetCount(ref_size+BONES_REF);
	for (int i=0;i < RefTable.Count();i++)
		RefTable[i] = 0;
	int refID = 0;
	for (int i=0;i < BoneData.Count();i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneData.flags != BONE_DEAD_FLAG)
		{
			refID = boneData.RefEndPt1ID-BONES_REF;
			if ((refID >=0) && (refID < RefTable.Count())) RefTable[refID] = 1;
			else DebugPrint(_T("RefTable error %d\n"),refID);
			refID = boneData.RefEndPt2ID-BONES_REF;
			if ((refID >=0) && (refID < RefTable.Count())) RefTable[refID] = 1;
			else DebugPrint(_T("RefTable error %d\n"),refID);
			refID = boneData.BoneRefID-BONES_REF;
			if ((refID >=0) && (refID < RefTable.Count())) RefTable[refID] = 1;
			else DebugPrint(_T("RefTable error %d\n"),refID);
			for (int j=0;j < boneData.CrossSectionList.Count();j++)
			{
				refID = boneData.CrossSectionList[j].RefInnerID-BONES_REF;
				if ((refID >=0) && (refID < RefTable.Count())) RefTable[refID] = 1;
				else DebugPrint(_T("RefTable error %d\n"),refID);
				refID = boneData.CrossSectionList[j].RefOuterID-BONES_REF;
				if ((refID >=0) && (refID < RefTable.Count())) RefTable[refID] = 1;
				else DebugPrint(_T("RefTable error %d\n"),refID);
			}
		}
	}

	int ct = 1;

	for (int i = 0; i<BoneData.Count();i++)
	{
		BoneDataClass &boneData = BoneData[i];

		if (boneData.RefEndPt1ID > ct) ct = boneData.RefEndPt1ID;
		if (boneData.RefEndPt2ID > ct) ct = boneData.RefEndPt2ID;
		if (boneData.BoneRefID > ct) ct = boneData.BoneRefID;
		for (int j = 0; j<boneData.CrossSectionList.Count();j++)
		{
			if (boneData.CrossSectionList[j].RefInnerID > ct) ct = boneData.CrossSectionList[j].RefInnerID;
			if (boneData.CrossSectionList[j].RefOuterID > ct) ct = boneData.CrossSectionList[j].RefOuterID;
		}
	}
	ct++;
	refHandleList.SetCount(ct);
	for (int i =0; i < ct; i++)
	{
		refHandleList[i] = NULL;
	}

	ParamBlock2PLCB* plcb = new ParamBlock2PLCB(versions, NUM_OLDVERSIONS, &skin_param_blk, this, PBLOCK_PARAM_REF);
	iload->RegisterPostLoadCallback(plcb);

	iload->RegisterPostLoadCallback(new BonesDefModPostLoad(this,ver));

	return IO_OK;
}

void BonesDefMod::BeginEditParams(
	IObjParam  *ip, ULONG flags,Animatable *prev)
{
	inAddBoneMode = FALSE;
	pblock_display->GetValue(skin_display_shadeweights,0,shadeVC,FOREVER);

	editing = TRUE;
	this->ip = ip;
	this->flags = flags;
	this->prev = prev;

	editMod  = this;

	// Create sub object editing modes.
	moveMode    = new MoveModBoxCMode(this,ip);
	CrossSectionMode    = new CreateCrossSectionMode(this,ip);

	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);

	bonesDefDesc.BeginEditParams(ip, this, flags, prev);
	// install a callback for the type in.
	skin_param_blk.SetUserDlgProc(new MapDlgProc(this));
	skin_advance_blk.SetUserDlgProc(new AdvanceMapDlgProc(this));
	skin_gizmos_blk.SetUserDlgProc(new GizmoMapDlgProc(this));

	//MIRROR
	skin_mirror_blk.SetUserDlgProc(new MirrorMapDlgProc(this));
	pblock_mirror->SetValue(skin_mirrorenabled,0,FALSE);
	mirrorData.EnableMirrorButton(FALSE);

	ip->RegisterTimeChangeCallback(&boneTimeChangeCallback);
	boneTimeChangeCallback.mod = this;

	if (showWeightTool)
		BringUpWeightTool();

	EnableDQMaskEditing(false);
}

void BonesDefMod::EndEditParams(
	IObjParam *ip,ULONG flags,Animatable *next)
{
	this->ip = ip;

	if ((pPainterInterface) && (pPainterInterface->InPaintMode()))
	{
		pPainterInterface->StartPaintSession(); //this toggle the paint mode off, this is an override for 5.1 since we did not want to change the API
		pPainterInterface->InitializeCallback(NULL);
	}

	ip->ClearPickMode();
	inAddBoneMode = FALSE;

	editing = FALSE;
	if (pblock_gizmos->Count(skin_gizmos_list) > 0)
	{
		ReferenceTarget *ref;
		if (currentSelectedGizmo != -1)
		{
			ref = pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,currentSelectedGizmo);
			GizmoClass *gizmo = (GizmoClass *)ref;
			if (gizmo) gizmo->EndEditParams(ip, flags,prev);
		}
	}

	this->prev = NULL;
	editMod  = NULL;

	TimeValue t = ip->GetTime();

	// NOTE: This flag must be cleared before sending the REFMSG_END_EDIT
	ClearAFlag(A_MOD_BEING_EDITED);

	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);

	ip->DeleteMode(moveMode);
	if (moveMode) delete moveMode;
	moveMode = NULL;

	ip->DeleteMode(CrossSectionMode);
	if (CrossSectionMode) delete CrossSectionMode;
	CrossSectionMode = NULL;

	//	ip->DeleteMode(PaintMode);
	//	if (PaintMode) delete PaintMode;
	//	PaintMode = NULL;

	ReleaseICustButton(iCrossSectionButton);
	iCrossSectionButton = NULL;

	ReleaseICustButton(iEditEnvelopes);
	iEditEnvelopes = NULL;

	ReleaseICustButton(iWeightTool);
	iWeightTool = NULL;

	ReleaseICustButton(iWeightTable);
	iWeightTable = NULL;

	ReleaseICustButton(iSortText);
	iSortText = NULL;

	// Brush presets
	EndBrushPresetContext( this );

	ReleaseICustButton(iPaintButton);
	iPaintButton = NULL;

	ReleaseICustButton(iLock);
	iLock = NULL;

	ReleaseICustButton(iAbsolute);
	iAbsolute = NULL;

	ReleaseICustButton(iEnvelope);
	iEnvelope = NULL;

	ReleaseICustButton(iFalloff);
	iFalloff = NULL;

	ReleaseICustButton(iCopy);
	iCopy = NULL;

	ReleaseICustButton(iPaste);
	iPaste = NULL;

	ReleaseICustToolbar(iParams);
	iParams = NULL;

	ReleaseICustToolbar(iMirrorParams);
	iMirrorParams = NULL;

	bonesDefDesc.EndEditParams(ip, this, flags, next);

	ip->UnRegisterTimeChangeCallback(&boneTimeChangeCallback);

	RestoreVCMode();

	if ((hWeightTable) && (!weightTableWindow.isDocked))
	{
		weightTableWindow.ClearMod();
		DestroyWindow(hWeightTable);
	}

	//MIRROR
	pblock_mirror->SetValue(skin_mirrorenabled,0,FALSE);

	if (weightToolHWND)
	{
		DestroyWindow(weightToolHWND);
		showWeightTool = TRUE;
	}
	else showWeightTool = FALSE;

	EnableDQMaskEditing(false);

//make sure to close our weighting tool dialogs
	if (mVoxelDlg)
	{
		DestroyWindow(mVoxelDlg);
		mVoxelDlg = nullptr;
	}
	if (mHeatMapDlg)
	{
		DestroyWindow(mHeatMapDlg);
		mHeatMapDlg = nullptr;
	}

	this->ip = NULL;
}

int BonesDefMod::SubNumToRefNum(int subNum)
{
	int ct = 0;
	for (int i = 0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneData.Node)
		{
			if (ct == subNum) return boneData.RefEndPt1ID;//start point
			ct++;
			if (ct == subNum) return boneData.RefEndPt2ID;//end point
			ct++;
			for (int j = 0; j < boneData.CrossSectionList.Count(); j++)
			{
				if (ct == subNum) return boneData.CrossSectionList[j].RefInnerID;
				ct++;
				if (ct == subNum) return boneData.CrossSectionList[j].RefOuterID;
				ct++;
			}
		}
	}
	return -1;
}

int BonesDefMod::RemapRefOnLoad(int iref)
{
	if (ver < 4)
	{
		if (iref > 1)
			iref += 8;
	}
	return iref;
}

RefTargetHandle BonesDefMod::GetReference(int i)

{
	if (i==PBLOCK_PARAM_REF)
	{
		return (RefTargetHandle)pblock_param;
	}
	else if (i==PBLOCK_DISPLAY_REF)
	{
		return (RefTargetHandle)pblock_display;
	}
	else if (i==PBLOCK_GIZMOS_REF)
	{
		return (RefTargetHandle)pblock_gizmos;
	}
	else if (i==PBLOCK_ADVANCE_REF)
	{
		return (RefTargetHandle)pblock_advance;
	}
	//WEIGHTTABLE
	else if (i==PBLOCK_WEIGHTTABLE_REF)
	{
		return (RefTargetHandle)pblock_weighttable;
	}
	//MIRROR
	else if (i==PBLOCK_MIRROR_REF)
	{
		return (RefTargetHandle)pblock_mirror;
	}

	else if (i == POINT1_REF)
	{
		return (RefTargetHandle)p1Temp;
	}
	else
	{
		RefTargetHandle t,t2;
		t = NULL;
		t2 = NULL;
		if (i < refHandleList.Count())
			return refHandleList[i];

		else
		{
			for (int ct = 0; ct < BoneData.Count(); ct++)
			{
				BoneDataClass &boneData = BoneData[ct];
				if (i == boneData.BoneRefID)
				{
					return (RefTargetHandle)boneData.Node;
				}
				else if (i == boneData.RefEndPt1ID)
				{
					return (RefTargetHandle)boneData.EndPoint1Control;
				}
				else if (i == boneData.RefEndPt2ID)
				{
					return (RefTargetHandle)boneData.EndPoint2Control;
				}
				else
				{
					for (int j=0;j<boneData.CrossSectionList.Count();j++)
					{
						if (i == boneData.CrossSectionList[j].RefInnerID)
						{
							return (RefTargetHandle)boneData.CrossSectionList[j].InnerControl;
						}
						else if (i == boneData.CrossSectionList[j].RefOuterID)
						{
							return (RefTargetHandle)boneData.CrossSectionList[j].OuterControl;
						}
					}
				}
			}
		}
	}

	return NULL;
}

void BonesDefMod::AddToRefHandleList(int id, RefTargetHandle rtarg)
{
	if (id < refHandleList.Count())
		refHandleList[id] = rtarg;
	else
	{
		Tab<RefTargetHandle> newList;
		newList.SetCount(id+1);
		for (int i =0; i < (id+1); i++)
			newList[i] = NULL;
		for (int i =0; i < refHandleList.Count(); i++)
			newList[i] = refHandleList[i];
		refHandleList.SetCount(id+1);
		for (int i =0; i < (id+1); i++)
			refHandleList[i] = newList[i];
	}

	rebuildSubAnimList = TRUE;
}

void BonesDefMod::SetReference(int i, RefTargetHandle rtarg)
{
	if (i==PBLOCK_PARAM_REF)
	{
		pblock_param = (IParamBlock2*)rtarg;
	}
	else if (i==PBLOCK_DISPLAY_REF)
	{
		pblock_display = (IParamBlock2*)rtarg;
	}
	else if (i==PBLOCK_GIZMOS_REF)
	{
		pblock_gizmos = (IParamBlock2*)rtarg;
	}
	else if (i==PBLOCK_ADVANCE_REF)
	{
		pblock_advance = (IParamBlock2*)rtarg;
	}
	//WEIGHTTABLE
	else if (i==PBLOCK_WEIGHTTABLE_REF)
	{
		pblock_weighttable = (IParamBlock2*)rtarg;
	}
	//MIRROR
	else if (i==PBLOCK_MIRROR_REF)
	{
		pblock_mirror = (IParamBlock2*)rtarg;
	}

	else if (i == POINT1_REF)
	{
		p1Temp     = (Control*)rtarg;
	}
	else
	{
		AddToRefHandleList(i, rtarg);
		for (int ct = 0; ct < BoneData.Count(); ct++)
		{
			BoneDataClass &boneData = BoneData[ct];
			if (i == boneData.BoneRefID)
			{
				boneData.Node = (INode*)rtarg;
				// Recalculate the Bonemap, since the BoneData has changed ! ns
				recompBoneMap = true;
			}
			if (i == boneData.RefEndPt1ID)
			{
				boneData.EndPoint1Control = (Control*)rtarg;
			}
			if (i == boneData.RefEndPt2ID)
			{
				boneData.EndPoint2Control = (Control*)rtarg;
			}
			for (int j=0;j<boneData.CrossSectionList.Count();j++)
			{
				if (i == boneData.CrossSectionList[j].RefInnerID)
				{
					boneData.CrossSectionList[j].InnerControl  = (Control*)rtarg ;
				}
				if (i == boneData.CrossSectionList[j].RefOuterID)
				{
					boneData.CrossSectionList[j].OuterControl  = (Control*)rtarg ;
				}
			}
		}
	}
}

void BonesDefMod::RebuildSubAnimList()
{
	subAnimList.ZeroCount();
	for (int i = 0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneData.Node)
		{
			Animatable *an = (Animatable*)boneData.EndPoint1Control;
			subAnimList.Append(1,&an,300);//start point
			an = (Animatable*)boneData.EndPoint2Control;
			subAnimList.Append(1,&an,300);//end point
			for (int j = 0; j < boneData.CrossSectionList.Count(); j++)
			{
				an = (Animatable*)boneData.CrossSectionList[j].InnerControl;
				subAnimList.Append(1,&an,300);
				an = (Animatable*)boneData.CrossSectionList[j].OuterControl;
				subAnimList.Append(1,&an,300);
			}
		}
	}

	rebuildSubAnimList = FALSE;
}

int BonesDefMod::NumSubs()
{
	if (enableFastSubAnimList)
	{
		if (rebuildSubAnimList) RebuildSubAnimList();
		return subAnimList.Count();
	}
	else
	{
		int ct = 0;
		for (int i = 0; i < BoneData.Count(); i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node)
			{
				ct += 2; //start and end
				ct += boneData.CrossSectionList.Count() * 2; //2 anims per cross section
			}
		}
		return ct;
	}
}
Animatable* BonesDefMod::SubAnim(int index)
{
	if (enableFastSubAnimList)
	{
		if (rebuildSubAnimList) RebuildSubAnimList();
		return subAnimList[index];
	}
	else
	{
		int ct = 0;
		for (int i = 0; i < BoneData.Count(); i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node)
			{
				if (ct == index) return boneData.EndPoint1Control;//start point
				ct++;
				if (ct == index) return boneData.EndPoint2Control;//end point
				ct++;
				for (int j = 0; j < boneData.CrossSectionList.Count(); j++)
				{
					if (ct == index) return boneData.CrossSectionList[j].InnerControl;
					ct++;
					if (ct == index) return boneData.CrossSectionList[j].OuterControl;
					ct++;
				}
			}
		}
		return NULL;
	}
}

TSTR BonesDefMod::SubAnimName(int index)
{
	int ct = 0;
	TSTR animName;
	for (int i = 0; i < BoneData.Count(); i++)
	{
		BoneDataClass &boneData = BoneData[i];
		if (boneData.Node)
		{
			if (ct == index)
			{
				animName.printf(_T("%s(%s)"),GetString(IDS_BONESTART),boneData.Node->GetName());
				return animName;
			}
			ct++;
			if (ct == index)
			{
				animName.printf(_T("%s(%s)"),GetString(IDS_BONEEND),boneData.Node->GetName());
				return animName;
			}
			ct++;
			for (int j = 0; j < boneData.CrossSectionList.Count(); j++)
			{
				if (ct == index)
				{
					animName.printf(_T("    %s(%0.2f)"),GetString(IDS_INNNERRADIUS),boneData.CrossSectionList[j].u);
					return animName;
				}
				ct++;
				if (ct == index)
				{
					animName.printf(_T("    %s(%0.2f)"),GetString(IDS_OUTERRADIUS),boneData.CrossSectionList[j].u);
					return animName;
				}
				ct++;
			}
		}
	}
	return animName;
}

RefResult BonesDefMod::NotifyRefChanged(
	const Interval& changeInt,RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate)
{
	Interface *tip;

	switch (message) {
		//watje Defect 683932 we need to prevent this from propogating since we only are dependant on the tm and this
		//can cause an objects cache to get flushed and crash the renderer.
	case REFMSG_OBJECT_CACHE_DUMPED:
		return REF_STOP;
		break;

	case REFMSG_CHANGE:

		if (stopMessagePropogation)
			return REF_STOP;

		if ((editMod==this) && (hTarget == pblock_param))
		{
			ParamID changing_param = pblock_param->LastNotifyParamID();
			skin_param_blk.InvalidateUI(changing_param);
		}
		else if ((editMod==this) && (hTarget == pblock_display))
		{
			ParamID changing_param = pblock_display->LastNotifyParamID();
			skin_display_blk.InvalidateUI(changing_param);
		}
		else if ((editMod==this) && (hTarget == pblock_gizmos))
		{
			ParamID changing_param = pblock_gizmos->LastNotifyParamID();
			skin_gizmos_blk.InvalidateUI(changing_param);
		}
		else if ((editMod==this) && (hTarget == pblock_advance))
		{
			ParamID changing_param = pblock_advance->LastNotifyParamID();
			skin_advance_blk.InvalidateUI(changing_param);
			if (changing_param == skin_advance_weightall)
			{
				Reevaluate(TRUE);
			}
		}
		//WEIGHTTABLE
		else if (hTarget == pblock_weighttable)
		{
			this->weightTableWindow.InvalidateViews();
		}
		else if ((editMod==this) && (ip && ip->GetSubObjectLevel() == 1))
		{
			ReevaluateActiveBone();
		}

		if ((resolvedModify) && ((!AlwaysDeform) || (splinePresent)))
		{
			tip = GetCOREInterface();
			if (tip != NULL)
			{
				for (int i =0;i<BoneData.Count();i++)
				{
					BoneDataClass &boneData = BoneData[i];
					if ((boneData.Node != NULL) &&
						(boneData.Node == hTarget) &&
						(tip->GetTime() == RefFrame) )
					{
						BoneMoved = TRUE;
					}
					if ((boneData.Node != NULL) &&
						(boneData.Node == hTarget)
						)
					{
						//check if bone was spline
						if (boneData.flags & BONE_SPLINE_FLAG)
						{
							splineChanged = TRUE;
							whichSplineChanged = i;
						}
					}
				}
			}
		}
		if (resolvedModify)
		{
			resolvedModify = FALSE;
		}

		break;

	case REFMSG_TARGET_DELETED: {
		for (int j =0;j<BoneData.Count();j++)
		{
			if (hTarget==BoneData[j].Node)
			{
				RemoveBone(j);
			}
		}
		break;
								}
	}
	return REF_SUCCEED;
}

SvGraphNodeReference BonesDefMod::SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags)
{
	SvGraphNodeReference nodeRef;
	if( gom->TestFilter(SV_FILTER_SKIN_DETAILS) )
		nodeRef = Modifier::SvTraverseAnimGraph( gom, owner, id, flags );
	else {
		gom->PushLevel(this);
		nodeRef = gom->AddAnimatable(this, owner, id, flags);
		gom->PopLevel();
	}

	if( nodeRef.stat == SVT_PROCEED ) {
		for( int i=0; i<GetNumBones(); i++ ) {
			gom->AddRelationship( nodeRef.gNode, GetBone(i), i, RELTYPE_MODIFIER );
		}
	}

	return nodeRef;
}

TSTR BonesDefMod::SvGetRelTip(IGraphObjectManager *gom, IGraphNode *gNodeTarger, int id, IGraphNode *gNodeMaker)
{
	return SvGetName(gom, gNodeMaker, false) + _T(" -> ") + gNodeTarger->GetAnim()->SvGetName(gom, gNodeTarger, false);
}
