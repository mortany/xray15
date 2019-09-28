#include "RendType.h"

#define MXS_MACROSCRIPTS_DIR		100
#define MXS_WEB_DIR					101
#define MXS_MAX_SYSTEM_ICONS_DIR	102

#define def_boolMat_types()							\
	mxs_value_id_pair boolMatTypes[] = {					\
		{ BOOL_MAT_NO_MODIFY,	n_modify },			\
		{ BOOL_MAT_IDTOMAT,		n_idtomat },		\
		{ BOOL_MAT_MATTOID,		n_mattoid }			\
		{ BOOL_MAT_DISCARD_ORIG,n_discardOrig }		\
		{ BOOL_MAT_DISCARD_NEW,	n_discardNew }		\
	};

#define def_clone_types()							\
	mxs_value_id_pair cloneTypes[] = {						\
		{ BOOL_ADDOP_INSTANCE,	n_instance },		\
		{ BOOL_ADDOP_REFERENCE,	n_reference },		\
		{ BOOL_ADDOP_COPY,		n_copy }			\
		{ BOOL_ADDOP_MOVE,		n_move }			\
	};

#define def_marker_types()							\
	mxs_value_id_pair markerTypes[] = {						\
		{ POINT_MRKR,			n_point },			\
		{ HOLLOW_BOX_MRKR,		n_hollowBox },		\
		{ PLUS_SIGN_MRKR,		n_plusSign },		\
		{ ASTERISK_MRKR,		n_asterisk },		\
		{ X_MRKR,				n_xMarker },		\
		{ BIG_BOX_MRKR,			n_bigBox },			\
		{ CIRCLE_MRKR,			n_circle },			\
		{ TRIANGLE_MRKR,		n_triangle },		\
		{ DIAMOND_MRKR,			n_diamond },		\
		{ SM_HOLLOW_BOX_MRKR,	n_smallHollowBox },	\
		{ SM_CIRCLE_MRKR,		n_smallCircle },	\
		{ SM_TRIANGLE_MRKR,		n_smallTriangle },	\
		{ SM_DIAMOND_MRKR,		n_smallDiamond}		\
	};												

#define def_render_types()							\
	mxs_value_id_pair renderTypes[] = {						\
		{ GW_NO_ATTS,			n_noAtts },			\
		{ GW_WIREFRAME,			n_wireframe },		\
		{ GW_ILLUM,				n_illum },			\
		{ GW_FLAT,				n_flat },			\
		{ GW_SPECULAR,			n_specular },		\
		{ GW_TEXTURE,			n_texture },		\
		{ GW_Z_BUFFER,			n_zBuffer },		\
		{ GW_PERSP_CORRECT,		n_perspCorrect },	\
		{ GW_POLY_EDGES,		n_polyEdges },		\
		{ GW_BACKCULL,			n_backcull },		\
		{ GW_TWO_SIDED,			n_twoSided },		\
		{ GW_COLOR_VERTS,		n_colorVerts },		\
		{ GW_SHADE_CVERTS,		n_shadeCverts },	\
		{ GW_PICK,				n_pick },			\
		{ GW_BOX_MODE,			n_boxMode },		\
		{ GW_ALL_EDGES,			n_allEdges },		\
		{ GW_VERT_TICKS,		n_vertTicks },		\
		{ GW_LIGHTING,			n_lighting },		\
	};												

#define def_toolbtn_types()							\
	mxs_value_id_pair toolbtnTypes[] = {					\
		{ MOVE_BUTTON,			n_move },			\
		{ ROTATE_BUTTON,		n_rotate } ,		\
		{ NUSCALE_BUTTON,		n_nuscale },		\
		{ USCALE_BUTTON,		n_uscale },			\
		{ SQUASH_BUTTON,		n_squash },			\
		{ SELECT_BUTTON,		n_select },			\
		{ PLACE_BUTTON,			n_place },			\
	};

#define def_origin_types()							\
	mxs_value_id_pair originTypes[] = {						\
		{ ORIGIN_LOCAL,			n_local },			\
		{ ORIGIN_SELECTION,		n_selection },		\
		{ ORIGIN_SYSTEM,		n_system }			\
	};

 #define def_coord_types()							\
	mxs_value_id_pair coordTypes[] = {						\
		{ COORDS_HYBRID,		n_hybrid },			\
		{ COORDS_HYBRID,		n_view },			\
		{ COORDS_SCREEN,		n_screen },			\
		{ COORDS_WORLD,			n_world },			\
		{ COORDS_PARENT,		n_parent },			\
		{ COORDS_LOCAL,			n_local },			\
		{ COORDS_OBJECT,		n_object },			\
		{ COORDS_OBJECT,		n_grid },			\
		{ COORDS_GIMBAL,		n_gimbal },			\
		{ COORDS_WORKINGPIVOT,	n_working_pivot },	\
		{ COORDS_LOCAL_ALIGNED,	n_local_aligned },	\
	};

#define def_axis_types()							\
	mxs_value_id_pair axisTypes[] = {						\
		{ NUMAXIS_ALL,			n_all },			\
		{ NUMAXIS_INDIVIDUAL,	n_individual },		\
	};

#define def_cursor_types()							\
	mxs_value_id_pair cursorTypes[] = {						\
		{ SYSCUR_MOVE,			n_move },			\
		{ SYSCUR_ROTATE,		n_rotate },			\
		{ SYSCUR_USCALE,		n_uscale },			\
		{ SYSCUR_NUSCALE,		n_nuscale },		\
		{ SYSCUR_SQUASH,		n_squash },			\
		{ SYSCUR_SELECT,		n_select },			\
		{ SYSCUR_DEFARROW,		n_arrow },			\
	};

#define def_browse_types()							\
	mxs_value_id_pair browseTypes[] = {						\
		{ BROWSE_MATSONLY,		n_mats },			\
		{ BROWSE_MAPSONLY,		n_maps },			\
		{ BROWSE_INCNONE,		n_incNone },		\
		{ BROWSE_INSTANCEONLY,	n_instanceOnly },	\
	};

#define def_status_types()							\
	mxs_value_id_pair statusTypes[] = {						\
		{ STATUS_UNIVERSE,		n_universe },		\
		{ STATUS_SCALE,			n_scale },			\
		{ STATUS_ANGLE,			n_angle },			\
		{ STATUS_OTHER ,		n_other }			\
	};

#define def_bkgor_types()							\
	mxs_value_id_pair bkgorTypes[] = {						\
		{ VIEWPORT_BKG_BLANK,	n_blank },			\
		{ VIEWPORT_BKG_HOLD,	n_hold },			\
		{ VIEWPORT_BKG_LOOP,	n_loop }			\
	};												\

#define def_bmp_types()								\
	mxs_value_id_pair bmpTypes[] = {						\
		{ BMM_NO_TYPE,			n_noType },			\
		{ BMM_LINE_ART,			n_lineArt },		\
		{ BMM_PALETTED,			n_paletted },		\
		{ BMM_GRAY_8,			n_gray8 },			\
		{ BMM_GRAY_16,			n_gray16 },			\
		{ BMM_TRUE_16,			n_true16 },			\
		{ BMM_TRUE_32,			n_true32 },			\
		{ BMM_TRUE_64,			n_true64 },			\
		{ BMM_TRUE_24,			n_true24 },			\
		{ BMM_TRUE_48,			n_true48 },			\
		{ BMM_YUV_422,			n_yuv422 },			\
		{ BMM_BMP_4,			n_bmp4 },			\
		{ BMM_PAD_24,			n_pad24 }			\
	};

#define def_dir_types()											\
	mxs_value_id_pair dirTypes[] = {									\
		{ APP_FONT_DIR,					n_font },				\
		{ APP_SCENE_DIR,				n_scene },				\
		{ APP_IMPORT_DIR,				n_import },				\
		{ APP_EXPORT_DIR,				n_export },				\
		{ APP_HELP_DIR,					n_help },				\
		{ APP_EXPRESSION_DIR,			n_expression },			\
		{ APP_PREVIEW_DIR,				n_preview },			\
		{ APP_IMAGE_DIR,				n_image },				\
		{ APP_SOUND_DIR,				n_sound },				\
		{ APP_PLUGCFG_DIR,				n_plugcfg },			\
		{ APP_MAXSTART_DIR,				n_maxstart },			\
		{ APP_VPOST_DIR,				n_vpost },				\
		{ APP_DRIVERS_DIR,				n_drivers },			\
		{ APP_AUTOBACK_DIR,				n_autoback },			\
		{ APP_MATLIB_DIR,				n_matlib },				\
		{ APP_SCRIPTS_DIR,				n_scripts },			\
		{ APP_STARTUPSCRIPTS_DIR,		n_startupScripts },		\
		{ APP_MARKETDEFAULTS_DIR,		n_defaults },			\
		{ APP_RENDER_PRESETS_DIR,		n_renderPresets },		\
		{ APP_UI_DIR,					n_ui },					\
		{ APP_MAX_SYS_ROOT_DIR,			n_maxroot },			\
		{ APP_RENDER_OUTPUT_DIR,		n_renderoutput },		\
		{ APP_ANIMATION_DIR,			n_animations },			\
		{ APP_ARCHIVES_DIR,				n_archives },			\
		{ APP_PHOTOMETRIC_DIR,			n_photometric },		\
		{ APP_RENDER_ASSETS_DIR,		n_renderassets },		\
		{ APP_USER_SCRIPTS_DIR,			n_userScripts },		\
		{ APP_USER_MACROS_DIR,			n_userMacros },			\
		{ APP_USER_STARTUPSCRIPTS_DIR,  n_userStartupScripts }, \
		{ APP_TEMP_DIR,					n_temp },				\
		{ APP_USER_ICONS_DIR,			n_userIcons },			\
		{ APP_MAXDATA_DIR,				n_maxData },			\
		{ APP_DOWNLOAD_DIR,				n_downloads },			\
		{ APP_PROXIES_DIR,				n_proxies },			\
		{ APP_MANAGED_ASSEMBLIES_DIR,	n_assemblies },			\
		{ APP_PAGE_FILE_DIR,			n_pageFile },			\
		{ APP_SHADER_CACHE_DIR,			n_hardwareShadersCache},\
		{ APP_PLUGCFG_LN_DIR,			n_plugcfg_ln },			\
		{ APP_UI_LN_DIR,				n_ui_ln },				\
		{ APP_AUTODESK_CLOUD_DIR,		n_autodeskcloud }, 		\
		{ APP_PLUGIN_PACKAGE_PRIVATE_DIR, n_privateExchangeStoreInstallPath },	\
		{ APP_PLUGIN_PACKAGE_PUBLIC_DIR, n_publicExchangeStoreInstallPath },	\
		{ APP_PLUGIN_PACKAGE_PRIVATE_DIR, n_privatePluginPackageInstallPath }, \
		{ APP_PLUGIN_PACKAGE_PUBLIC_DIR, n_publicPluginPackageInstallPath }, \
		{ APP_USER_STARTUP_TEMPLATE_DIR, n_userStartupTemplates }, 		\
		{ MXS_MACROSCRIPTS_DIR,			n_macroScripts }, 		\
		{ MXS_WEB_DIR,					n_web }, 		\
		{ MXS_MAX_SYSTEM_ICONS_DIR,		n_maxSysIcons }, 		\
		{ APP_CFD_DIR,					n_cfd },				\
		{ APP_SYSTEM_IMAGE_DIR,		n_systemImage },		\
		{ APP_SYSTEM_PHOTOMETRIC_DIR,	n_systemPhotometric },	\
		{ APP_SYSTEM_SOUND_DIR,			n_systemSound },		\
		{ APP_SYSTEM_CFD_DIR,			n_systemCFD },			\
		{ APP_FLUID_SIMULATION_DIR,		n_fluidSimulations },			\
		{ APP_USER_SETTINGS_DIR,		n_userSettings },		\
		{ APP_USER_TOOLS_DIR,			n_userTools },		\
};

#define def_cpanel_types()							\
	mxs_value_id_pair cpanelTypes[] = {						\
		{ TASK_MODE_CREATE,		n_create },			\
		{ TASK_MODE_MODIFY,		n_modify },			\
		{ TASK_MODE_HIERARCHY,	n_hierarchy },		\
		{ TASK_MODE_MOTION,		n_motion },			\
		{ TASK_MODE_DISPLAY,	n_display },		\
		{ TASK_MODE_UTILITY,	n_utility },		\
	};

#define def_color_types()							\
	mxs_value_id_pair colorTypes[] = {						\
		{ LINE_COLOR,			n_line },			\
		{ FILL_COLOR,			n_fill },			\
		{ TEXT_COLOR,			n_text },			\
		{ CLEAR_COLOR,			n_clear },			\
	};

#define def_tbar_filter_types()							\
	mxs_value_id_pair tbarFiltTypes[] = {						\
		{ TRACKBAR_FILTER_ALL,		n_all },			\
		{ TRACKBAR_FILTER_TMONLY,	n_TMOnly },			\
		{ TRACKBAR_FILTER_CURRENTTM,n_currentTM },		\
		{ TRACKBAR_FILTER_OBJECT,	n_object },			\
		{ TRACKBAR_FILTER_MATERIAL,	n_mat },			\
	};

#define def_snap_types()								\
	mxs_value_id_pair snapTypes[] = {							\
		{ SNAP_IN_3D,				n_in3d },			\
		{ SNAP_IN_PLANE,			n_inPlane },		\
		{ SNAP_UNSEL_OBJS_ONLY,		n_unSelObjs },		\
		{ SNAP_SEL_OBJS_ONLY,		n_selObjs },		\
		{ SNAP_UNSEL_SUBOBJ_ONLY,	n_unSelSubobj },	\
		{ SNAP_SEL_SUBOBJ_ONLY,		n_selSubobj },		\
		{ SNAP_FORCE_3D_RESULT,		n_force3d },		\
	};



#define def_spt_types()									\
	mxs_value_id_pair sptTypes[] = {							\
		{ GW_SPT_TXT_CORRECT,		n_txtCorrect },		\
		{ GW_SPT_GEOM_ACCEL,		n_geomAccel },		\
		{ GW_SPT_TRI_STRIPS,		n_triStrips },		\
		{ GW_SPT_DUAL_PLANES,		n_dualPlanes },		\
		{ GW_SPT_SWAP_MODEL,		n_swapModel },		\
		{ GW_SPT_INCR_UPDATE,		n_incrUpdate },		\
		{ GW_SPT_1_PASS_DECAL,		n_passDecal },		\
		{ GW_SPT_DRIVER_CONFIG,		n_driverConfig },	\
		{ GW_SPT_TEXTURED_BKG,		n_texturedBkg },	\
		{ GW_SPT_VIRTUAL_VPTS,		n_virtualVpts },	\
		{ GW_SPT_PAINT_DOES_BLIT,	n_paintDoesBlit },	\
		{ GW_SPT_WIREFRAME_STRIPS,	n_wireframeStrips }	\
	};


#define def_bmmerror_types()							\
	mxs_value_id_pair bmmerrorTypes[] = {						\
		{ BMMRES_SUCCESS,			n_success },		\
		{ BMMRES_ERRORTAKENCARE,	n_errortakencare },	\
		{ BMMRES_FILENOTFOUND,		n_FILENOTFOUND },	\
		{ BMMRES_MEMORYERROR,		n_MEMORYERROR },	\
		{ BMMRES_NODRIVER,			n_NODRIVER },		\
		{ BMMRES_IOERROR,			n_IOERROR },		\
		{ BMMRES_INVALIDFORMAT,		n_INVALIDFORMAT },	\
		{ BMMRES_CORRUPTFILE,		n_CORRUPTFILE },	\
		{ BMMRES_SINGLEFRAME,		n_SINGLEFRAME },	\
		{ BMMRES_INVALIDUSAGE,		n_INVALIDUSAGE },	\
		{ BMMRES_RETRY,				n_RETRY },			\
		{ BMMRES_NUMBEREDFILENAMEERROR, n_NUMBEREDFILENAMEERROR },	\
		{ BMMRES_INTERNALERROR,		n_INTERNALERROR },	\
		{ BMMRES_BADFILEHEADER,		n_BADFILEHEADER },	\
		{ BMMRES_CANTSTORAGE,		n_CANTSTORAGE },	\
		{ BMMRES_RETRY,				n_RETRY },			\
		{ BMMRES_BADFRAME,			n_BADFRAME },		\
	};

#define def_enumfile_types()							\
	mxs_value_id_pair enumfileTypes[] = {						\
		{ FILE_ENUM_INACTIVE,		n_inactive },		\
		{ FILE_ENUM_VP,				n_vp },				\
		{ FILE_ENUM_RENDER,			n_render },			\
		{ FILE_ENUM_ALL,			n_all },			\
		{ FILE_ENUM_MISSING_ONLY,	n_missingOnly },	\
		{ FILE_ENUM_1STSUB_MISSING,	n_1stMissing },		\
		{ FILE_ENUM_DONT_RECURSE,	n_dontRecurse },	\
		{ FILE_ENUM_CHECK_AWORK1,	n_checkAwork1 },	\
	};

#define def_bkgaspect_types()							\
	mxs_value_id_pair bkgaspectTypes[] = {						\
		{ VIEWPORT_BKG_ASPECT_VIEW,		n_view },		\
		{ VIEWPORT_BKG_ASPECT_BITMAP,	n_bitmap },		\
		{ VIEWPORT_BKG_ASPECT_OUTPUT,	n_output }		\
	};

#define def_propertyset_types()							\
	mxs_value_id_pair propertysetTypes[] = {					\
		{ PROPSET_SUMMARYINFO,			n_summary },	\
		{ PROPSET_DOCSUMMARYINFO,		n_contents },	\
		{ PROPSET_USERDEFINED,			n_custom }		\
	};

// AF -- (04/30/02) Removed the Trackview in a viewport feature ECO #831
#ifdef TRACKVIEW_IN_A_VIEWPORT

	#define def_view_types()                         \
		mxs_value_id_pair viewTypes[] = {                    \
			{ VIEW_LEFT,        n_view_left},        \
			{ VIEW_RIGHT,       n_view_right},       \
			{ VIEW_TOP,         n_view_top},         \
			{ VIEW_BOTTOM,      n_view_bottom},      \
			{ VIEW_FRONT,       n_view_front},       \
			{ VIEW_BACK,        n_view_back},        \
			{ VIEW_ISO_USER,    n_view_iso_user},    \
			{ VIEW_PERSP_USER,  n_view_persp_user},  \
			{ VIEW_CAMERA,      n_view_camera},      \
			{ VIEW_GRID,        n_view_grid},        \
			{ VIEW_NONE,        n_view_none},        \
			{ VIEW_TRACK,       n_view_track},       \
			{ VIEW_SPOT,        n_view_spot},        \
			{ VIEW_SHAPE,       n_view_shape},       \
		};

#else //TRACKVIEW_IN_A_VIEWPORT
	#define def_view_types()                         \
		mxs_value_id_pair viewTypes[] = {                    \
			{ VIEW_LEFT,        n_view_left},        \
			{ VIEW_RIGHT,       n_view_right},       \
			{ VIEW_TOP,         n_view_top},         \
			{ VIEW_BOTTOM,      n_view_bottom},      \
			{ VIEW_FRONT,       n_view_front},       \
			{ VIEW_BACK,        n_view_back},        \
			{ VIEW_ISO_USER,    n_view_iso_user},    \
			{ VIEW_PERSP_USER,  n_view_persp_user},  \
			{ VIEW_CAMERA,      n_view_camera},      \
			{ VIEW_GRID,        n_view_grid},        \
			{ VIEW_NONE,        n_view_none},        \
			{ VIEW_SPOT,        n_view_spot},        \
			{ VIEW_SHAPE,       n_view_shape},       \
		};
#endif //TRACKVIEW_IN_A_VIEWPORT


#define def_vp_layouts()                     \
	mxs_value_id_pair vpLayouts[] = {				 \
		{ VP_LAYOUT_1,      n_layout_1 },    \
		{ VP_LAYOUT_2V,     n_layout_2v },   \
		{ VP_LAYOUT_2H,     n_layout_2h },   \
		{ VP_LAYOUT_2HT,    n_layout_2ht },  \
		{ VP_LAYOUT_2HB,    n_layout_2hb },  \
		{ VP_LAYOUT_3VL,    n_layout_3vl },  \
		{ VP_LAYOUT_3VR,    n_layout_3vr },  \
		{ VP_LAYOUT_3HT,    n_layout_3ht },  \
		{ VP_LAYOUT_3HB,    n_layout_3hb },  \
		{ VP_LAYOUT_4,      n_layout_4 },    \
		{ VP_LAYOUT_4VL,    n_layout_4vl },  \
		{ VP_LAYOUT_4VR,    n_layout_4vr },  \
		{ VP_LAYOUT_4HT,    n_layout_4ht },  \
		{ VP_LAYOUT_4HB,    n_layout_4hb }   \
	};

#define def_euler_angles()					    \
	mxs_value_id_pair eulerAngles[] = {				    \
		{ EULERTYPE_XYZ,	n_xyz			},  \
		{ EULERTYPE_XZY,    n_xzy			},  \
		{ EULERTYPE_YZX,	n_yzx			},  \
		{ EULERTYPE_YXZ,	n_yxz			},  \
		{ EULERTYPE_ZXY,	n_zxy			},  \
		{ EULERTYPE_ZYX,	n_zyx			},	\
		{ EULERTYPE_XYX,	n_xyx			},	\
		{ EULERTYPE_YZY,	n_yzy			},  \
		{ EULERTYPE_ZXZ,	n_zxz			},  \
	};

#define def_mtl_attach_types()						\
	mxs_value_id_pair mtlAttachTypes[] = {					\
		{ ATTACHMAT_IDTOMAT,	n_iDToMat },		\
		{ ATTACHMAT_MATTOID,	n_matToID } ,		\
		{ ATTACHMAT_NEITHER,	n_neither },		\
	};

#define def_stdcommandmode_types()					\
	mxs_value_id_pair stdcommandmodeTypes[] = {				\
		{ CID_OBJMOVE,			n_move },			\
		{ CID_OBJROTATE,		n_rotate } ,		\
		{ CID_OBJSCALE,			n_nuscale },		\
		{ CID_OBJUSCALE,		n_uscale },			\
		{ CID_OBJSQUASH,		n_squash },			\
		{ CID_OBJSELECT,		n_select }	,		\
		{ CID_OBJPLACE,			n_place },			\
	};

#define def_commandmode_types()						\
	mxs_value_id_pair commandmodeTypes[] = {				\
		{ MOVE_COMMAND,			n_move },			\
		{ ROTATE_COMMAND,		n_rotate },			\
		{ SCALE_COMMAND,		n_nuscale },		\
		{ USCALE_COMMAND,		n_uscale },			\
		{ SQUASH_COMMAND,		n_squash },			\
		{ VIEWPORT_COMMAND,		n_viewport },		\
		{ SELECT_COMMAND,		n_select },			\
		{ HIERARCHY_COMMAND,	n_hierarchy },		\
		{ CREATE_COMMAND,		n_create },			\
		{ MODIFY_COMMAND,		n_modify },			\
		{ MOTION_COMMAND,		n_motion },			\
		{ ANIMATION_COMMAND,	n_animation },		\
		{ CAMERA_COMMAND,		n_camera },			\
		{ NULL_COMMAND,			n_null },			\
		{ DISPLAY_COMMAND,		n_display },		\
		{ SPOTLIGHT_COMMAND,	n_spotlight },		\
		{ PICK_COMMAND,			n_pick },			\
		{ PICK_EX_COMMAND,		n_pick } ,			\
		{ PLACE_COMMAND,		n_place },			\
	};

#define def_axisconstraint_types()					\
	mxs_value_id_pair axisconstraintTypes[] = {				\
		{ AXIS_X,			n_X },					\
		{ AXIS_Y,			n_Y },					\
		{ AXIS_Z,			n_Z },					\
		{ AXIS_XY,			n_XY },					\
		{ AXIS_YZ,			n_YZ },					\
		{ AXIS_ZX,			n_ZX },					\
	};

#define def_autoedge_types()						\
	mxs_value_id_pair autoedgeTypes[] = {					\
		{ AUTOEDGE_SETCLEAR,	n_setClear },		\
		{ AUTOEDGE_SET,			n_set } ,			\
		{ AUTOEDGE_CLEAR,		n_clear },			\
	};

#define def_displayunit_types()				\
	mxs_value_id_pair displayunitTypes[] = {		\
		{ UNITDISP_GENERIC,	n_generic },	\
		{ UNITDISP_METRIC,	n_metric } ,	\
		{ UNITDISP_US,		n_us },			\
		{ UNITDISP_CUSTOM,	n_custom },		\
	};

#define def_systemunit_types()						\
	mxs_value_id_pair systemunitTypes[] = {					\
		{ UNITS_INCHES,			n_inches },			\
		{ UNITS_FEET,			n_feet } ,			\
		{ UNITS_MILES,			n_miles },			\
		{ UNITS_MILLIMETERS,	n_millimeters },	\
		{ UNITS_CENTIMETERS,	n_centimeters },	\
		{ UNITS_METERS,			n_meters },			\
		{ UNITS_KILOMETERS,		n_kilometers },		\
	};

#define def_metricunit_types()						\
	mxs_value_id_pair metricunitTypes[] = {					\
		{ UNIT_METRIC_DISP_MM,	n_millimeters },	\
		{ UNIT_METRIC_DISP_CM,	n_centimeters },	\
		{ UNIT_METRIC_DISP_M,	n_meters },			\
		{ UNIT_METRIC_DISP_KM,	n_kilometers },		\
	};

#define def_usunit_types()							\
	mxs_value_id_pair usunitTypes[] = {						\
		{ UNIT_US_DISP_FRAC_IN,		n_frac_In },	\
		{ UNIT_US_DISP_DEC_IN,		n_dec_In },		\
		{ UNIT_US_DISP_FRAC_FT,		n_frac_Ft },	\
		{ UNIT_US_DISP_DEC_FT,		n_dec_Ft },		\
		{ UNIT_US_DISP_FT_FRAC_IN,	n_ft_Frac_In },	\
		{ UNIT_US_DISP_FT_DEC_IN,	n_ft_Dec_In },	\
	};

#define def_usfrac_types()					\
	mxs_value_id_pair usfracTypes[] = {				\
		{ UNIT_FRAC_1_1,	n_frac_1_1 },	\
		{ UNIT_FRAC_1_2,	n_frac_1_2 },	\
		{ UNIT_FRAC_1_4,	n_frac_1_4 },	\
		{ UNIT_FRAC_1_8,	n_frac_1_8 },	\
		{ UNIT_FRAC_1_10,	n_frac_1_10 },	\
		{ UNIT_FRAC_1_16,	n_frac_1_16 },	\
		{ UNIT_FRAC_1_32,	n_frac_1_32 },	\
		{ UNIT_FRAC_1_64,	n_frac_1_64 },	\
		{ UNIT_FRAC_1_100,	n_frac_1_100 },	\
	};

#define def_uvwmap_types()						\
	mxs_value_id_pair uvwmapTypes[] ={					\
		{ MAP_PLANAR,		n_planar },			\
		{ MAP_CYLINDRICAL,	n_cylindrical },	\
		{ MAP_SPHERICAL,	n_spherical },		\
		{ MAP_BALL,			n_ball },			\
		{ MAP_BOX,			n_box },			\
		{ MAP_FACE,			n_face },			\
	};

#define def_perdata_types()						\
	mxs_value_id_pair perdataTypes[] = {				\
		{ PERDATA_TYPE_FLOAT,	n_float },		\
	};

#define def_process_priority_types()			\
	mxs_value_id_pair processPriorityTypes[] = {		\
		{ HIGH_PRIORITY_CLASS,		n_high },	\
		{ NORMAL_PRIORITY_CLASS,	n_normal } ,\
		{ IDLE_PRIORITY_CLASS,		n_low },	\
		{ ABOVE_NORMAL_PRIORITY_CLASS, n_aboveNormal }, \
		{ BELOW_NORMAL_PRIORITY_CLASS, n_belowNormal }, \
		{ PROCESS_MODE_BACKGROUND_BEGIN, n_backgroundBegin }, \
		{ PROCESS_MODE_BACKGROUND_END, n_backgroundEnd }, \
		{ REALTIME_PRIORITY_CLASS, n_realtime }, \
	};

#define def_emesh_subdiv_method_types()			\
	mxs_value_id_pair emeshsubdivmethodtypes[] = {		\
		{ TESS_REGULAR,		n_regular },		\
		{ TESS_SPATIAL,		n_spatial },		\
		{ TESS_CURVE,		n_curvature },		\
		{ TESS_LDA,			n_spatialAndCurvature },	\
	};

#define def_emesh_subdiv_style_types()			\
	mxs_value_id_pair emeshsubdivstyletypes[] = {		\
		{ SUBDIV_TREE,		n_tree },			\
		{ SUBDIV_GRID,		n_grid },			\
		{ SUBDIV_DELAUNAY,	n_delaunay },		\
	};

#define def_poly_so_types()						\
	mxs_value_id_pair polysoTypes[] = {					\
		{ MNM_SL_OBJECT,	n_object },			\
		{ MNM_SL_VERTEX,	n_vertex } ,		\
		{ MNM_SL_EDGE,		n_edge },			\
		{ MNM_SL_FACE,		n_face },			\
	};

#define def_select_so_types()					\
	mxs_value_id_pair selectsoTypes[] = {				\
		{ IMESHSEL_OBJECT,	n_object },			\
		{ IMESHSEL_VERTEX,	n_vertex } ,		\
		{ IMESHSEL_EDGE,	n_edge },			\
		{ IMESHSEL_FACE,	n_face },			\
	};


#define def_max_render_types()						\
	mxs_value_id_pair renderTypes[] = {						\
		{ RENDER_VIEW,			n_view },			\
		{ RENDER_VIEW,			n_normal },			\
		{ RENDER_SELECTED,		n_selected } ,		\
		{ RENDER_SELECTED,		n_selection },		\
		{ RENDER_REGION,		n_region },			\
		{ RENDER_CROP,			n_crop },			\
		{ RENDER_CROP,			n_regionCrop },		\
		{ RENDER_BLOWUP_SELECTED,	n_boxselected },	\
		{ RENDER_BLOWUP,			n_blowup },			\
		{ RENDER_REGION_SELECTED,	n_regionselected },	\
		{ RENDER_CROP_SELECTED,		n_cropselected },	\
	};

// Viewport Rendering Levels from maxapi.h (ShadeType)
#define VP_SMOOTHHIGHLIGHTS		(SMOOTH_HIGHLIGHT)
#define VP_SMOOTH				(SMOOTH)
#define VP_FACETHIGHLIGHTS		(FACET_HIGHLITE)
#define VP_FACET				(FACET)
#define VP_FLAT					(CONSTANT)
#define VP_HIDDENLINE			(HIDDENLINE)
#define VP_LITWIREFRAME			(LITE_WIREFRAME)
#define VP_ZWIREFRAME			(Z_WIREFRAME)
#define VP_WIREFRAME			(WIREFRAME)
#define VP_BOX					(BOX)


#define def_vp_renderlevel()							\
	mxs_value_id_pair vpRenderLevel[] = {						\
		{ VP_SMOOTHHIGHLIGHTS,	n_smoothhighlights },	\
		{ VP_SMOOTH,			n_smooth },				\
		{ VP_FACETHIGHLIGHTS,   n_facethighlights },	\
		{ VP_FACET,				n_facet },				\
		{ VP_FLAT,				n_flat },				\
		{ VP_HIDDENLINE,		n_hiddenline },			\
		{ VP_LITWIREFRAME,		n_litwireframe },		\
		{ VP_ZWIREFRAME,		n_zwireframe },			\
		{ VP_WIREFRAME,			n_wireframe },			\
		{ VP_BOX,				n_box }	,				\
	};

// see class PreviewParams - member rndLevel
#define def_preview_renderlevel()		\
	mxs_value_id_pair previewRenderLevel[] = {		\
		{ 0,	n_smoothhighlights },	\
		{ 1,	n_smooth },				\
		{ 2,	n_facethighlights },	\
		{ 3,	n_facet },				\
		{ 4,	n_flat },				\
		{ 5,	n_hiddenline },			\
		{ 6,	n_litwireframe },		\
		{ 7,	n_wireframe },			\
		{ 8,	n_box }	,				\
	};

#define def_registry_value_types()									\
	mxs_value_id_pair registryValueTypes[] = {								\
		{ REG_BINARY,				n_REG_BINARY },					\
		{ REG_DWORD,				n_REG_DWORD },					\
		{ REG_DWORD_LITTLE_ENDIAN,	n_REG_DWORD_LITTLE_ENDIAN },	\
		{ REG_DWORD_BIG_ENDIAN,		n_REG_DWORD_BIG_ENDIAN },		\
		{ REG_EXPAND_SZ,			n_REG_EXPAND_SZ },				\
		{ REG_LINK,					n_REG_LINK },					\
		{ REG_MULTI_SZ,				n_REG_MULTI_SZ },				\
		{ REG_NONE,					n_REG_NONE },					\
		{ REG_QWORD,				n_REG_QWORD },					\
		{ REG_QWORD_LITTLE_ENDIAN,	n_REG_QWORD_LITTLE_ENDIAN },	\
		{ REG_RESOURCE_LIST,		n_REG_RESOURCE_LIST},			\
		{ REG_SZ,					n_REG_SZ },						\
	};

#define def_IK_joint_types()		\
	mxs_value_id_pair IKJointTypes[] = {	\
		{ 0,	n_rotational },		\
		{ 1,	n_sliding },		\
	};

#define def_refmsg_parts()						\
	mxs_value_id_pair refmsgParts[] = {					\
		{ static_cast<int>(PART_TOPO),		n_topo },			\
		{ static_cast<int>(PART_GEOM),		n_geom },			\
		{ static_cast<int>(PART_TEXMAP),		n_texmap },			\
		{ static_cast<int>(PART_MTL),			n_mtl },			\
		{ static_cast<int>(PART_TM),  		n_TM },				\
		{ static_cast<int>(PART_OBJ),  		n_obj },			\
		{ static_cast<int>(PART_OBJECT_TYPE),	n_objectType },		\
		{ static_cast<int>(PART_SELECT),		n_select },			\
		{ static_cast<int>(PART_SUBSEL_TYPE), n_subSelType },		\
		{ static_cast<int>(PART_DISPLAY),    	n_display },		\
		{ static_cast<int>(PART_VERTCOLOR),	n_vertColor },		\
		{ static_cast<int>(PART_GFX_DATA),	n_gfxData },		\
		{ static_cast<int>(PART_DISP_APPROX),	n_dispApprox },		\
		{ static_cast<int>(PART_EXTENSION),	n_extension },		\
		{ static_cast<int>(PART_TM_CHAN),   	n_TMchannel },		\
		{ static_cast<int>(PART_MTL_CHAN),	n_MTLChannel },		\
		{ static_cast<int>(PART_ALL),			n_all },			\
		{ static_cast<int>(PART_PUT_IN_FG),	n_putInFG },		\
	};

#define def_bezier_tangent_types()				\
	mxs_value_id_pair bezierTangentTypes[] = {			\
		{ BEZKEY_SMOOTH,	n_smooth },			\
		{ BEZKEY_LINEAR,	n_linear },			\
		{ BEZKEY_STEP,		n_step },			\
		{ BEZKEY_FAST,		n_fast },			\
		{ BEZKEY_SLOW,		n_slow },			\
		{ BEZKEY_USER,		n_custom },			\
		{ BEZKEY_FLAT,		n_flat },			\
	};

#define def_TimeDisplayMode_types()					\
	mxs_value_id_pair timeDisplayModeTypes[] = {			\
		{ DISPTIME_FRAMES,		n_frames },			\
		{ DISPTIME_SMPTE,		n_smpte },			\
		{ DISPTIME_FRAMETICKS,	n_frameTicks },		\
		{ DISPTIME_TIMETICKS,	n_timeTicks },		\
	};

#define def_nodePropsToMatch_types()								\
	mxs_value_id_pair nodePropsToMatchTypes[] = {							\
		{ Interface10::kNodeProp_Material,		n_material },		\
		{ Interface10::kNodeProp_Layer,			n_layer },			\
		{ Interface10::kNodeProp_All,			n_all },			\
	};


// see enum EventTypeEnums in src\include\INodeEventManager.h
#define def_nodeeventcallback_event_types()			\
	mxs_value_id_pair nodeeventcallbackEventTypes[] = {		\
		{ 0,	n_added },							\
		{ 1,	n_deleted },						\
		{ 2,	n_linkChanged },					\
		{ 3,	n_layerChanged },					\
		{ 4,	n_groupChanged },					\
		{ 5,	n_hierarchyOtherEvent },			\
		{ 6,	n_modelStructured },				\
		{ 7,	n_geometryChanged },				\
		{ 8,	n_topologyChanged },				\
		{ 9,	n_mappingChanged },					\
		{ 10,	n_extentionChannelChanged },		\
		{ 11,	n_modelOtherEvent },				\
		{ 12,	n_materialStructured },				\
		{ 13,	n_materialOtherEvent },				\
		{ 14,	n_controllerStructured },			\
		{ 15,	n_controllerOtherEvent },			\
		{ 16,	n_nameChanged },					\
		{ 17,	n_wireColorChanged },				\
		{ 18,	n_renderPropertiesChanged },		\
		{ 19,	n_displayPropertiesChanged },		\
		{ 20,	n_userPropertiesChanged },			\
		{ 21,	n_propertiesOtherEvent },			\
		{ 22,	n_subobjectSelectionChanged },		\
		{ 23,	n_selectionChanged },				\
		{ 24,	n_hideChanged },					\
		{ 25,	n_freezeChanged },					\
		{ 26,	n_displayOtherEvent },				\
		{ 27,	n_callbackBegin },					\
		{ 28,	n_callbackEnd },					\
		{ 29,	n_all }						\
	};

#define def_mtlDlgMode_types()								\
	mxs_value_id_pair mtlDlgModeTypes[] = {							\
		{ Interface13::mtlDlgMode_Basic,	n_basic },		\
		{ Interface13::mtlDlgMode_Advanced,	n_advanced },	\
	};

#define def_logFileLongevity_types()					\
	mxs_value_id_pair logFileLongevity[] = {			\
		{ SYSLOG_LIFE_EVER,	n_forever },			\
		{ SYSLOG_LIFE_DAYS,	n_days },			\
		{ SYSLOG_LIFE_SIZE,	n_size },		\
	};

#define def_logEntry_types()					\
	mxs_value_id_pair logEntry[] = {			\
		{ SYSLOG_ERROR,	n_error },			\
		{ SYSLOG_WARN,		n_warning },			\
		{ SYSLOG_INFO,		n_info },		\
		{ SYSLOG_DEBUG,	n_debug },		\
	};

