#include "unwrap.h"
#include "modsres.h"
#include <helpsys.h>
//#include <new>

#include "3dsmaxport.h"
#include "../../../Include/uvwunwrap/uvwunwrapNewAPIs.h"

/*************************************************************



**************************************************************/
class RenderUVDlgProc : public ParamMap2UserDlgProc
{
public:	
	enum class TileSelection
	{
		Default = 0,
		All,
		FixedItemNum,
	};

	UnwrapMod *mod;

	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; };

	static bool renderAllTiles;
	static Bitmap *renderAllTilesMap;

	struct RenderAllTilesMapStatusHolder
	{
		RenderAllTilesMapStatusHolder()
		{
			renderAllTiles = true;
			renderAllTilesMap = nullptr;
		}

		~RenderAllTilesMapStatusHolder()
		{
			renderAllTiles = false;
		}
	};

private:
	void UpdateTileCombo();
	void RenderTile( UnwrapMod *mod, int UTile, int VTile );
};

bool RenderUVDlgProc::renderAllTiles = false;
Bitmap* RenderUVDlgProc::renderAllTilesMap = nullptr;

INT_PTR RenderUVDlgProc::DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
		case WM_INITDIALOG:
			{
				UnwrapMod::renderUVWindow = hWnd;
				UpdateTileCombo();
				::SetWindowContextHelpId(hWnd, idh_unwrap_renderuvw);
			}
			break;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_CONTEXTHELP) 
			{
				MaxSDK::IHelpSystem::GetInstance()->ShowProductHelpForTopic(idh_unwrap_renderuvw); 
			}
			return FALSE;
			break;
		case WM_CLOSE:
			if (UnwrapMod::renderUVWindow )
			{
				UnwrapMod::renderUVWindow = NULL;
				DestroyWindow(hWnd);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_GUESS_BUTTON:
					{
						if ( mod )
						{
		 					mod->GuessAspectRatio();
						}
					}
					break;
				case IDC_TILECOMBO:
					{
						if ( CBN_DROPDOWN == HIWORD(wParam) )
						{   // update tiles before dropdown
							UpdateTileCombo();
						}
					}
					break;
				case IDC_RENDERBUTTON:
					{
						HWND hTileCombo = GetDlgItem( UnwrapMod::renderUVWindow, IDC_TILECOMBO );
						if ( hTileCombo && mod )
						{
							const int selection = ComboBox_GetCurSel(hTileCombo);
							if ( CB_ERR != selection )
							{
								const auto &tilesContainer = Painter2D::Singleton().GetTilesContainer();
								if ( TileSelection::Default == static_cast<TileSelection>(selection) )
								{
									mod->fnRenderUV();

									TSTR mstr = mod->GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap5.renderUV"));
									macroRecorder->FunctionCall(mstr, 1, 0, mr_string, mod->renderUVbi.Filename());
									macroRecorder->EmitScript();
								}
								else if ( TileSelection::All == static_cast<TileSelection>(selection) )
								{
									RenderAllTilesMapStatusHolder holder;
									for ( const auto &tile : tilesContainer)
									{
										RenderTile( mod, tile.first, tile.second );
									}
								}
								else
								{
									const int index = selection - static_cast<int>(TileSelection::FixedItemNum);
									if ( (index >= 0) && (index < tilesContainer.size()) )
									{
                                        auto it = tilesContainer.begin();
                                        std::advance(it, index);
                                        auto tile = *it;
										RenderTile( mod, tile.first, tile.second );
									}
								}
							}
						}
					}
					break;
				case IDC_RENDER_FILES:
					{
						if ( mod )
						{
							TheManager->SelectFileOutput( &mod->renderUVbi, UnwrapMod::renderUVWindow, GetString(IDS_RENDERUV_RENDERFILE));
							DisplayStringWithEllipses( mod->renderUVbi.Name(), GetDlgItem(UnwrapMod::renderUVWindow, IDC_RENDER_FILENAME) );
						}
					}
					break;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void RenderUVDlgProc::UpdateTileCombo()
{
	HWND hTileCombo = GetDlgItem( UnwrapMod::renderUVWindow, IDC_TILECOMBO );
	if ( hTileCombo )
	{
		// clear content first
		ComboBox_ResetContent(hTileCombo);

		// Append fixed items
		ComboBox_AddString(hTileCombo, GetString(IDS_RENDERUV_TILE_DEFAULT));
		ComboBox_AddString(hTileCombo, GetString(IDS_RENDERUV_TILE_ALL));

		// Append tiles
		TSTR buffer;
		const auto &tilesContainer = Painter2D::Singleton().GetTilesContainer();
		for ( const auto uvTile : tilesContainer )
		{
			buffer.printf( _T("U%d_V%d"), 
				(uvTile.first < 0) ? uvTile.first : uvTile.first + 1, 
				(uvTile.second < 0) ? uvTile.second : uvTile.second + 1 );
			ComboBox_AddString(hTileCombo, buffer.data());
		}

		ComboBox_SetCurSel(hTileCombo, TileSelection::Default);
	}
}

void RenderUVDlgProc::RenderTile( UnwrapMod *mod, int UTile, int VTile )
{
	TSTR oldName = mod->AppendUVSuffixToBitmap( UTile, VTile );
	BOOL doRender = TRUE;
	MaxSDK::Util::Path path( mod->renderUVbi.Name() );
	if ( !path.IsEmpty() && path.Exists() )
	{
		doRender = RenderUV_ShowFileOverwriteDlg( mod->renderUVbi.Name(), UnwrapMod::renderUVWindow);
	}

	if ( !doRender )
	{
		// restore renderUVbi name and return
		if ( !oldName.isNull() )
		{
			mod->renderUVbi.SetName( oldName );
		}
		return;
	}

	mod->RenderUV( UTile, VTile );

	TSTR mstr = mod->GetMacroStr( _T("modifiers[#unwrap_uvw].unwrap5.renderUV") );
	macroRecorder->FunctionCall( mstr, 3, 0, 
		mr_string, mod->renderUVbi.Filename(),
		mr_int, (UTile < 0) ? UTile : UTile + 1,	// follow Mudbox UV pattern format, start from U1V1
		mr_int, (VTile < 0) ? VTile : VTile + 1 );
	macroRecorder->EmitScript();

	if ( !oldName.isNull() )
	{
		mod->renderUVbi.SetName( oldName );
	}
}

// ----------------------------------------------------------------------------

void UnwrapMod::GuessAspectRatio()
{
	float x = 0.0f;
	float y = 0.0f;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		if ( !ld ) 
		{ 
			continue; 
		}

		Tab<int> hiddenEdgeCount;
		const int fCount = ld->GetNumberFaces();
		hiddenEdgeCount.SetCount(fCount);
		for (int i = 0; i < fCount; i++)
		{
			hiddenEdgeCount[i] = 0;
		}
		for (int i = 0; i < ld->GetNumberGeomEdges(); i++) //TVMaps.gePtrList.Count(); i++)
		{
			const BOOL hidden = ld->GetGeomEdgeHidden(i);//TVMaps.gePtrList[i]->flags & FLAG_HIDDENEDGEA;
			if (hidden)
			{
				const int faceCount = ld->GetGeomEdgeNumberOfConnectedFaces(i);//TVMaps.gePtrList[i]->faceList.Count();
				for (int j = 0; j < faceCount; j++)
				{
					const int faceIndex = ld->GetGeomEdgeConnectedFace(i,j);//TVMaps.gePtrList[i]->faceList[j];
					hiddenEdgeCount[faceIndex] += 1;
				}
			}
		}

		for (int i = 0; i < ld->GetNumberTVEdges(); i++)//TVMaps.ePtrList.Count(); i++)
		{
			//line up the face
			int a = ld->GetTVEdgeVert(i,0);//TVMaps.ePtrList[i]->a;
			int b = ld->GetTVEdgeVert(i,1);//TVMaps.ePtrList[i]->b;

			int ga = ld->GetTVEdgeGeomVert(i,0);//TVMaps.ePtrList[i]->ga;
			int gb = ld->GetTVEdgeGeomVert(i,1);//TVMaps.ePtrList[i]->gb;

			if ( ld->IsTVVertVisible(a) && 
				 ld->IsTVVertVisible(b) && 
				!ld->GetTVEdgeHidden(i) )//TVMaps.ePtrList[i]->flags & FLAG_HIDDENEDGEA)))
			{
				Point3 pa = ld->GetTVVert(a);//TVMaps.v[a].p;
				Point3 pb = ld->GetTVVert(b);//TVMaps.v[b].p;
				Point3 uvec = pb - pa;
				Point3 nuvec = Normalize(pb - pa);

				Point3 pga = ld->GetGeomVert(ga);//TVMaps.geomPoints[ga];
				Point3 pgb = ld->GetGeomVert(gb);// TVMaps.geomPoints[gb];
				Point3 gvec = pgb - pga;
				Point3 ngvec = Normalize(pgb - pga);

				float uang = PI;
				if (uvec.x > 0.0f)
					uang = fabs(ld->AngleFromVectors(nuvec,Point3(1.0f,0.0f,0.0f)));
				else 
					uang = fabs(ld->AngleFromVectors(nuvec,Point3(-1.0f,0.0f,0.0f)));

				float vang = PI;
				if (uvec.y > 0.0f)
					vang = fabs(ld->AngleFromVectors(nuvec,Point3(0.0f,1.0f,0.0f)));
				else 
					vang = fabs(ld->AngleFromVectors(nuvec,Point3(0.0f,-1.0f,0.0f)));
				
				if (uang < (10.0f * DEG_TO_RAD))
				{
					float uwidth = fabs(uvec.x);
					if (uwidth != 0.0f) 
					{
						float gwidth = Length(gvec);
						float tx = gwidth / uwidth;
						if (tx > x)
							x = tx;
					}
				}

				if (vang < (10.0f * DEG_TO_RAD))
				{
					float uheight = fabs(uvec.y);
					if (uheight != 0.0f) 
					{
						float gheight = Length(gvec);
						float ty = gheight / uheight;
						if (ty > y)
							y = ty;
					}
				}
			}
		}
	}

	if ((x != 0.0f) && (y != 0.0f))
	{
		int width = 0;
		pblock->GetValue(unwrap_renderuv_width,0,width,FOREVER);
		int height = 0;
		pblock->GetValue(unwrap_renderuv_height,0,height,FOREVER);

		int newHeight = width * (y / x);
		pblock->SetValue(unwrap_renderuv_height,0,newHeight);
	}
}

void UnwrapMod::fnRenderUVDialog()
{
	if (!WtIsEnabled(ID_RENDERUV))
		return;
	RenderUVDlgProc* paramMapDlgProc = new RenderUVDlgProc;
	if ( paramMapDlgProc )
	{
		paramMapDlgProc->mod = this;
	}

	if (renderUVWindow == nullptr)
	{
		renderUVMap = CreateModelessParamMap2(
			unwrap_renderuv_params, //param map ID constant
			pblock,
			0,
			hInstance,
			MAKEINTRESOURCE(IDD_UNWRAP_RENDERUVS_PARAMS),
			hDialogWnd,
			paramMapDlgProc);
	}
}

void UnwrapMod::fnRenderUV()
{
	if (!WtIsEnabled(ID_RENDERUV))
		return;
	RenderUVCurUTile = 0;
	RenderUVCurVTile = 0;

	BOOL doRender = TRUE;
	MaxSDK::Util::Path path( renderUVbi.Name() );
	if ( !path.IsEmpty() && path.Exists() )
	{
		doRender = RenderUV_ShowFileOverwriteDlg( renderUVbi.Name(), UnwrapMod::renderUVWindow );
	}

	if ( doRender )
	{
		RenderUV(renderUVbi);
	}
}

void UnwrapMod::RenderUV(int UTile, int VTile)
{
	RenderUVCurUTile = UTile;
	RenderUVCurVTile = VTile;
	RenderUV(renderUVbi);
}

void UnwrapMod::fnRenderUV(const TCHAR *name, int UTile, int VTile)
{
	if (!WtIsEnabled(ID_RENDERUV))
		return;
	// Apply Mudbox UV pattern format, UV<0, 0> = U1V1
	// U0V0 is the same as U1V1
	RenderUVCurUTile = (UTile > 0) ? UTile - 1 : UTile;
	RenderUVCurVTile = (VTile > 0) ? VTile - 1 : VTile;
	renderUVbi.SetName(name);
	RenderUV(renderUVbi);
}

TSTR UnwrapMod::AppendUVSuffixToBitmap( int UTile, int VTile )
{
	TSTR fullPath( renderUVbi.Name() );
	if ( fullPath.isNull() )
	{
		return nullptr;
	}

	MCHAR dir[_MAX_DIR];
	MCHAR fileName[_MAX_FNAME];
	MCHAR fileExt[_MAX_EXT];
	BMMSplitFilename( fullPath.data(), dir, fileName, fileExt );

	TSTR newName;
	newName.printf( _T("%s_U%d_V%d"), fileName, 
		(UTile < 0) ? UTile : UTile + 1, 
		(VTile < 0) ? VTile : VTile + 1 );

	MCHAR newFullPath[_MAX_PATH];
	errno_t ret = _tmakepath_s( newFullPath, nullptr, dir, newName.data(), fileExt );
	if ( 0 != ret ) {
		return nullptr;
	}
	renderUVbi.SetName( newFullPath );
	return fullPath;
}

void UnwrapMod::BXPLine2(long x1,long y1,long x2,long y2,
						 WORD r, WORD g, WORD b, WORD alpha,
						 Bitmap *map, BOOL wrap)
{
	BMM_Color_64 bit(r, g, b, alpha);

	const long dx = x2 - x1;
	const long sdx = (dx > 0) ? 1 : -1;
	const long dxabs = abs(dx);

	const long dy = y2 - y1;
	const long sdy = (dy > 0) ? 1 : -1;
	const long dyabs = abs(dy);

	long x = 0;
	long y = 0;
	long px = x1;
	long py = y1;

	if (dxabs >= dyabs)
	{
		for ( long i = 0; i <= dxabs; i++ )
		{
			y += dyabs;
			if (y >= dxabs)
			{
				y -= dxabs;
				py += sdy;
			}

			if (wrap)
			{
				if (px >= map->Width())
				{
					px = 0;
				}
				if (px < 0)
				{
					px = map->Width()-1;
				}
				if (py >= map->Height())
				{
					py = 0;
				}
				if (py < 0)
				{
					px = map->Height()-1;
				}
				map->PutPixels(px,py,1,&bit);
			}
			else
			{
				if ( (px>=0) && (px<map->Width()) &&
					 (py>=0) && (py<map->Height()) )
				{
					map->PutPixels(px,py,1,&bit);
				}
			}
			px += sdx;
		}
	}
	else
	{
		for ( long i = 0; i <= dyabs; i++ )
		{
			x += dxabs;
			if (x >= dyabs)
			{
				x -= dyabs;
				px += sdx;
			}

			if (wrap)
			{
				if (px >= map->Width())
				{
					px = 0;
				}
				if (px < 0)
				{
					px = map->Width()-1;
				}
				if (py >= map->Height())
				{
					py = 0;
				}
				if (py < 0)
				{
					px = map->Height()-1;
				}
				map->PutPixels(px,py,1,&bit);
			}
			else
			{
				if ( (px>=0) && (px<map->Width()) &&
					 (py>=0) && (py<map->Height()) )
				{
					map->PutPixels(px,py,1,&bit);
				}
			}
			map->PutPixels(px,py,1,&bit);
			py += sdy;
		}
	}
}

void UnwrapMod::BXPLineFloat(long x1,long y1,long x2,long y2,
					   WORD r, WORD g, WORD b, WORD alpha,
					   Bitmap *map)
{
	BMM_Color_64 bit( r, g, b, alpha );

	float xinc = 0.0f;
 	float yinc = 0.0f;
	
	const float xDist = x2-x1;
	const float yDist = y2-y1;
 	const int width = map->Width();
	const int height = map->Height();

	int ix,iy;
 	if (fabs(xDist) > fabs(yDist))
	{
		if (xDist >= 0.0f)
			xinc = 1.0f;
		else 
			xinc = -1.0f;
		yinc = yDist/fabs(xDist);

		float fx = x1;
		float fy = y1;
		int ct = fabs(xDist);
		for (int i = 0; i <= ct; i++)
		{
			ix = fx;
			iy = fy;

			if ((fx - floor(fx)) >= 0.5f) ix += 1;
			if ((fy - floor(fy)) >= 0.5f) iy += 1;

			if ( (ix >= 0) && (ix < width) && 
				 (iy >= 0) && (iy < height) )
			{
				map->PutPixels(ix,iy,1,&bit);
			}
			fx += xinc;
			fy += yinc;
		}
	}
	else
	{
		if(yDist >= 0.0f)
			yinc = 1.0f;
		else 
			yinc = -1.0f;
		xinc = xDist/fabs(yDist);

		float fx = x1;
		float fy = y1;
		int ct = fabs(yDist);
		for (int i = 0; i <= ct; i++)
		{
			ix = fx;
			iy = fy;

			if ((fx - floor(fx)) >= 0.5f) ix += 1;
			if ((fy - floor(fy)) >= 0.5f) iy += 1;

			if ( (ix >= 0) && (ix < width) && 
				 (iy >= 0) && (iy < height) )
			{
				map->PutPixels(ix,iy,1,&bit);
			}
			fx += xinc;
			fy += yinc;
		}
	}
}

BOOL UnwrapMod::BXPTriangleCheckOverlap( long x1,long y1,
					   long x2,long y2,
					   long x3,long y3,
					   Bitmap *map,
					   BitArray &processedPixels )
{
	//sort top to bottom
	int sx[3], sy[3];
    sx[0] = x1;
	sy[0] = y1;

	if (y2 < sy[0])
	{
		sx[0] = x2;
		sy[0] = y2;

		sx[1] = x1;
		sy[1] = y1;
	}
	else 
	{
		sx[1] = x2;
		sy[1] = y2;
	}

	if (y3 < sy[0])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = sx[0];
		sy[1] = sy[0];

		sx[0] = x3;
		sy[0] = y3;
	}
	else if (y3 < sy[1])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = x3;
		sy[1] = y3;
	}
	else
	{
		sx[2] = x3;
		sy[2] = y3;
	}

	int hit = 0;

	const int width = map->Width();
	const int height = map->Height();
	//if flat top
	if (sy[0] == sy[1])
	{
		const float yDist = DL_abs(sy[0] - sy[2]);
		const float xDist0 = sx[2] - sx[0];
		const float xDist1 = sx[2] - sx[1];
		const float xInc0 = xDist0 / yDist;
		const float xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[1];
		for (int i = sy[0]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{
				int index = j + i * width;
				if ( (j >= 0) && (j < width) && 
					 (i >= 0) && (i < height) )
				{
					if (processedPixels[index] && (j > (ix0+1)) && (j < (ix1-1)))
					{
						hit++;
					}
					processedPixels.Set(index,TRUE);
				}
			}
			cx0 += xInc0;
			cx1 += xInc1;
		}
	}
	//it flat bottom
 	else if (sy[1] == sy[2])
	{
		const float yDist = DL_abs(sy[0] - sy[2]);
		const float xDist0 = sx[1]- sx[0];
		const float xDist1 = sx[2] - sx[0];
		const float xInc0 = xDist0 / yDist;
		const float xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[0];
		for (int i = sy[0]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				int temp = ix0;
				ix0 = ix1;
				ix1 = temp;
			}
			for (int j = ix0; j <= ix1; j++)
			{
				int index = j + i * width;
				if ( (j >= 0) && (j < width) && 
					 (i >= 0) && (i < height))
				{
					if (processedPixels[index]&& (j > (ix0+1)) && (j < (ix1-1)))
					{
						hit++;
					}
					processedPixels.Set(index,TRUE);
				}
			}
			cx0 += xInc0;
			cx1 += xInc1;
		}
	}
	else
	{
		const float yDist0to1 = DL_abs(sy[1] - sy[0]);
		const float yDist0to2 = DL_abs(sy[2] - sy[0]);
		const float yDist1to2 = DL_abs(sy[2] - sy[1]);
		const float xDist1 = sx[1] - sx[0];
		const float xDist2 = sx[2] - sx[0];
		const float xDist3 = sx[2] - sx[1];
		const float xInc1 = xDist1 / yDist0to1;
		const float xInc2 = xDist2 / yDist0to2;
		const float xInc3 = xDist3 / yDist1to2;

		float cx0 = sx[0];
		float cx1 = sx[0];
		//go from s[0] to s[1]
		for (int i = sy[0]; i < sy[1]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				int index = j + i * width;
				if ( (j >= 0) && (j < width) && 
					 (i >= 0) && (i < height) )
				{
					if (processedPixels[index] &&  (j > (ix0+1)) && (j < (ix1-1)))
					{
						hit++;
					}
					processedPixels.Set(index,TRUE);
				}
			}
			cx0 += xInc1;
			cx1 += xInc2;
		}	
		//go from s[1] to s[2]
		for (int i = sy[1]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				int index = j + i * width;
				if ( (j >= 0) && (j < width) && 
					 (i >= 0) && (i < height))
				{
					if (processedPixels[index] && (j > (ix0+1)) && (j < (ix1-1)))
					{
						hit++;
					}
					processedPixels.Set(index,TRUE);
				}
			}
			cx0 += xInc3;
			cx1 += xInc2;
		}
	}

	return (hit > 1);
}

void UnwrapMod::BXPTriangleFloat( long x1,long y1,
					   long x2,long y2,
					   long x3,long y3,
					   WORD r1, WORD g1, WORD b1, WORD alpha1,
					   Bitmap *map )
{
	//sort top to bottom
	int sx[3], sy[3];
    sx[0] = x1;
	sy[0] = y1;

	if (y2 < sy[0])
	{
		sx[0] = x2;
		sy[0] = y2;

		sx[1] = x1;
		sy[1] = y1;
	}
	else 
	{
		sx[1] = x2;
		sy[1] = y2;
	}

	if (y3 < sy[0])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = sx[0];
		sy[1] = sy[0];

		sx[0] = x3;
		sy[0] = y3;
	}
	else if (y3 < sy[1])
	{
		sx[2] = sx[1];
		sy[2] = sy[1];

		sx[1] = x3;
		sy[1] = y3;
	}
	else
	{
		sx[2] = x3;
		sy[2] = y3;
	}

	BMM_Color_64 bit(r1, g1, b1, alpha1);

	//if flat top
	if (sy[0] == sy[1])
	{
		const float yDist = DL_abs(sy[0] - sy[2]);
		const float xDist0 = sx[2] - sx[0];
		const float xDist1 = sx[2] - sx[1];
		const float xInc0 = xDist0 / yDist;
		const float xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[1];
		for (int i = sy[0]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				map->PutPixels(j,i,1,&bit);
			}
			cx0 += xInc0;
			cx1 += xInc1;
		}
	}
	//it flat bottom
 	else if (sy[1] == sy[2])
	{
		const float yDist = DL_abs(sy[0] - sy[2]);
		const float xDist0 = sx[1]- sx[0];
		const float xDist1 = sx[2] - sx[0];
		const float xInc0 = xDist0 / yDist;
		const float xInc1 = xDist1 / yDist;

		float cx0 = sx[0];
		float cx1 = sx[0];
		for (int i = sy[0]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				map->PutPixels(j,i,1,&bit);
			}
			cx0 += xInc0;
			cx1 += xInc1;
		}
	}
	else
	{
		const float yDist0to1 = DL_abs(sy[1] - sy[0]);
		const float yDist0to2 = DL_abs(sy[2] - sy[0]);
		const float yDist1to2 = DL_abs(sy[2] - sy[1]);
		const float xDist1 = sx[1]- sx[0];
		const float xDist2 = sx[2] - sx[0];
		const float xDist3 = sx[2] - sx[1];
		const float xInc1 = xDist1 / yDist0to1;
		const float xInc2 = xDist2 / yDist0to2;
		const float xInc3 = xDist3 / yDist1to2;

		float cx0 = sx[0];
		float cx1 = sx[0];
		//go from s[0] to s[1]
		for (int i = sy[0]; i < sy[1]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				map->PutPixels(j,i,1,&bit);
			}
			cx0 += xInc1;
			cx1 += xInc2;
		}	
		//go from s[1] to s[2]
		for (int i = sy[1]; i < sy[2]; i++)
		{
			int ix0 = cx0;
			int ix1 = cx1;

			if (cx0 - floor(cx0) >= 0.5f) ix0 += 1;
			if (cx1 - floor(cx1) >= 0.5f) ix1 += 1;

			//line 
			if (ix0 > ix1)
			{
				std::swap( ix0, ix1 );
			}
			for (int j = ix0; j <= ix1; j++)
			{
				map->PutPixels(j,i,1,&bit);
			}
			cx0 += xInc3;
			cx1 += xInc2;
		}
	}
}

void UnwrapMod::BXPTriangleFloat( 
					   BXPInterpData c0, BXPInterpData c1, BXPInterpData c2,
					   WORD alpha,
					   Bitmap *map, 
					   Tab<Point3> &norms,
					   Tab<Point3> &pos )
{
	//sort top to bottom
	BXPInterpData c[3];
	c[0] = c0;
	if (c1.y < c[0].y)
	{
		c[0] = c1;
		c[1] = c0;
	}
	else 
	{
		c[1] =  c1;
	}

	if (c2.y < c[0].y)
	{
		c[2] = c[1];
		c[1] = c[0];
		c[0] = c2;
	}
	else if (c2.y < c[1].y)
	{
		c[2] = c[1];
		c[1] = c2;
	}
	else
	{
		c[2] = c2;
	}

	BMM_Color_64 bit;
	bit.a = alpha;

	const int width = map->Width();
	const int height = map->Height();

	//if flat top
	if (c[0].IntY() == c[1].IntY())
	{
		float yDist = fabs(c[0].y - c[2].y);

		BXPInterpData xInc0 = c[2] - c[0];
		BXPInterpData xInc1 = c[2] - c[1];
		xInc0 = xInc0 / yDist;
		xInc1 = xInc1 / yDist;
		
		BXPInterpData cx0 = c[0];
		BXPInterpData cx1 = c[1];
		
		int sy = c[0].IntY();
		int ey = c[2].IntY();
		for (int i = sy; i < ey; i++)
		{
			BXPInterpData ix0 = cx0;
			BXPInterpData ix1 = cx1;
			
			//line 
			if (ix0.IntX() > ix1.IntX())
			{
				std::swap( ix0, ix1 );
			}
			int sx = ix0.IntX();
			int ex = ix1.IntX();

			BXPInterpData subInc = ix1 -  ix0;
			subInc = subInc / (ix1.IntX()-ix0.IntX());
			BXPInterpData newc = ix0;
			
			for (int j = sx; j < ex; j++)
			{
				bit.r = static_cast<WORD>(newc.color.x);
				bit.g = static_cast<WORD>(newc.color.y);
				bit.b = static_cast<WORD>(newc.color.z);

				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					int index = i*width+j;
					if (norms[index] == Point3(0.0f,0.0f,0.0f))
					{
						map->PutPixels(j,i,1,&bit);
						norms[index] = newc.normal;
						pos[index] = newc.pos;
					}
					else
					{
						Point3 currentP = pos[index];
						Point3 newP = newc.pos;
						Point3 currentNorm = norms[index];
						Point3 newNorm = newc.normal;

						Point3 averageNormal = (currentNorm + newNorm) *0.5f;
						Matrix3 tm(1);
						MatrixFromNormal(averageNormal, tm);
						tm = Inverse(tm);
						currentP = currentP * tm;
						newP = newP * tm;
						if (newP.z > currentP.z)
						{
							map->PutPixels(j,i,1,&bit);
							norms[index] = newc.normal;
							pos[index] = newc.pos;
						}
					}
				}
				newc = newc + subInc;
			}
			cx0 = cx0 + xInc0;
			cx1 = cx1 + xInc1;
		}
	}

	//it flat bottom
 	else if (c[1].IntY() == c[2].IntY())
	{
		float yDist = fabs(c[0].y - c[2].y);
		
		BXPInterpData xInc0 = c[1] - c[0];
		BXPInterpData xInc1 = c[2] - c[0];
		xInc0 = xInc0 / yDist;
		xInc1 = xInc1 / yDist;
		
		BXPInterpData cx0 = c[0];
		BXPInterpData cx1 = c[0];
		
		int sy = c[0].IntY();
		int ey = c[2].IntY();

		for (int i = sy; i < ey; i++)
		{
			BXPInterpData ix0 = cx0;
			BXPInterpData ix1 = cx1;
			
			//line 
			if (ix0.IntX() > ix1.IntX())
			{
				std::swap( ix0, ix1 );
			}

			BXPInterpData subInc = (ix1-ix0)/(float)(ix1.IntX()-ix0.IntX());
			BXPInterpData newc = ix0;

			int sx = ix0.IntX();
			int ex = ix1.IntX();
			for (int j = sx; j < ex; j++)
			{
				bit.r = static_cast<WORD>(newc.color.x);
				bit.g = static_cast<WORD>(newc.color.y);
				bit.b = static_cast<WORD>(newc.color.z);

				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					int index = i*width+j;
					if (norms[index] == Point3(0.0f,0.0f,0.0f))
					{
						map->PutPixels(j,i,1,&bit);
						norms[index] = newc.normal;
						pos[index] = newc.pos;
					}
					else
					{
						Point3 currentP = pos[index];
						Point3 newP = newc.pos;
						Point3 currentNorm = norms[index];
						Point3 newNorm = newc.normal;

						Point3 averageNormal = (currentNorm + newNorm) *0.5f;
						Matrix3 tm(1);
						MatrixFromNormal(averageNormal, tm);
						tm = Inverse(tm);
						currentP = currentP * tm;
						newP = newP * tm;
						if (newP.z > currentP.z)
						{
							map->PutPixels(j,i,1,&bit);
							norms[index] = newc.normal;
							pos[index] = newc.pos;
						}
					}
				}
				newc = newc + subInc;
			}
			cx0 = cx0 + xInc0;
			cx1 = cx1 + xInc1;
		}
	}
	else
	{
		float yDist0to1 = fabs(c[1].y - c[0].y);
		float yDist0to2 = fabs(c[2].y - c[0].y);
		float yDist1to2 = fabs(c[2].y - c[1].y);

		BXPInterpData xInc1 = (c[1] - c[0]) / yDist0to1;
		BXPInterpData xInc2 = (c[2] - c[0]) / yDist0to2;
		BXPInterpData xInc3 = (c[2] - c[1]) / yDist1to2;

		BXPInterpData cx0 = c[0];
		BXPInterpData cx1 = c[0];

		//go from s[0] to s[1]
		int sy = c[0].IntY();
		int ey = c[1].IntY();
		for (int i = sy; i < ey; i++)
		{
			BXPInterpData ix0 = cx0;
			BXPInterpData ix1 = cx1;

			//line 
			if (ix0.IntX() > ix1.IntX())
			{
				std::swap( ix0, ix1 );
			}

			BXPInterpData subInc = (ix1-ix0)/(float)(ix1.IntX()-ix0.IntX());
			BXPInterpData newc = ix0;

			int sx = ix0.IntX();
			int ex = ix1.IntX();
			
			for (int j = sx; j <= ex; j++)
			{
				bit.r = static_cast<WORD>(newc.color.x);
				bit.g = static_cast<WORD>(newc.color.y);
				bit.b = static_cast<WORD>(newc.color.z);

				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{				
					int index = i*width+j;
					if (norms[index] == Point3(0.0f,0.0f,0.0f))
					{
						map->PutPixels(j,i,1,&bit);
						norms[index] = newc.normal;
						pos[index] = newc.pos;
					}
					else
					{
						Point3 currentP = pos[index];
						Point3 newP = newc.pos;
						Point3 currentNorm = norms[index];
						Point3 newNorm = newc.normal;

						Point3 averageNormal = (currentNorm + newNorm) *0.5f;
						Matrix3 tm(1);
						MatrixFromNormal(averageNormal, tm);
						tm = Inverse(tm);
						currentP = currentP * tm;
						newP = newP * tm;
						if (newP.z > currentP.z)
						{
							map->PutPixels(j,i,1,&bit);
							norms[index] = newc.normal;
							pos[index] = newc.pos;
						}
					}
				}
				newc = newc + subInc;
			}
			cx0 = cx0 + xInc1;
			cx1 = cx1 + xInc2;
		}
		//go from s[1] to s[2]
		sy = c[1].IntY();
		ey = c[2].IntY();
		for (int i = sy; i < ey; i++)
		{
			BXPInterpData ix0 = cx0;
			BXPInterpData ix1 = cx1;

			//line 
			if (ix0.IntX() > ix1.IntX())
			{
				std::swap( ix0, ix1 );
			}

			BXPInterpData subInc = (ix1-ix0)/(float)(ix1.IntX()-ix0.IntX());
			BXPInterpData newc = ix0;

			int sx = ix0.IntX();
			int ex = ix1.IntX();

			for (int j = sx; j <= ex; j++)
			{
				bit.r = (int)newc.color.x;
				bit.g = (int)newc.color.y;
				bit.b = (int)newc.color.z;
				if ((j >= 0) && (j < width) && (i >= 0) && (i < height))
				{
					int index = i*width+j;
					if (norms[index] == Point3(0.0f,0.0f,0.0f))
					{
						map->PutPixels(j,i,1,&bit);
						norms[index] = newc.normal;
						pos[index] = newc.pos;
					}
					else
					{
						Point3 currentP = pos[index];
						Point3 newP = newc.pos;
						Point3 currentNorm = norms[index];
						Point3 newNorm = newc.normal;

						Point3 averageNormal = (currentNorm + newNorm) *0.5f;
						Matrix3 tm(1);
						MatrixFromNormal(averageNormal, tm);
						tm = Inverse(tm);
						currentP = currentP * tm;
						newP = newP * tm;
						if (newP.z > currentP.z)
						{
							map->PutPixels(j,i,1,&bit);
							norms[index] = newc.normal;
							pos[index] = newc.pos;
						}
					}
				}
				newc = newc + subInc;
			}
			cx0 = cx0 + xInc3;
			cx1 = cx1 + xInc2;
		}
	}
}

enum class FillMode
{
	First = 0,
	None = First,
	Solid,
	Normal,
	Shaded,
	Last = Shaded,
	Num
};

// The function takes UV space vertex as input, offset it with current tile's UV value,
// and return image sapce vertex, which is scaled with bitmap width and height.
static IPoint2 TVVert2MapVert( const Point3 &tvVert, int mapWidth, int mapHeight )
{
	return IPoint2( mapWidth * (tvVert.x - UnwrapMod::RenderUVCurUTile), 
		(mapHeight - 1) * (1 - (tvVert.y - UnwrapMod::RenderUVCurVTile)) );
}

void UnwrapMod::RenderUV(BitmapInfo bi)
{
	//get the width/height
	int width, height;
	pblock->GetValue(unwrap_renderuv_width,0,width,FOREVER);
	pblock->GetValue(unwrap_renderuv_height,0,height,FOREVER);
	static const int default_image_size = 512;
	if (height <= 0) width = default_image_size;
	if (height <= 0) height = default_image_size;

	float fEdgeAlpha;
	pblock->GetValue(unwrap_renderuv_edgealpha,0,fEdgeAlpha,FOREVER);
	if (fEdgeAlpha < 0.0f) fEdgeAlpha = 0.0f;
	if (fEdgeAlpha > 1.0f) fEdgeAlpha = 1.0f;
	const int edgeAlpha = int(0xFFFF * fEdgeAlpha);
	
	Color edgeColor;
	pblock->GetValue(unwrap_renderuv_edgecolor,0,edgeColor,FOREVER);
	edgeColor.ClampMinMax();
	const int edgeColorR = int(0xFFFF * edgeColor.r);
	const int edgeColorG = int(0xFFFF * edgeColor.g);
	const int edgeColorB = int(0xFFFF * edgeColor.b);

	Color seamColor;
	pblock->GetValue(unwrap_renderuv_seamcolor,0,seamColor,FOREVER);
	seamColor.ClampMinMax();
	const int seamColorR = int(0xFFFF * seamColor.r);
	const int seamColorG = int(0xFFFF * seamColor.g);
	const int seamColorB = int(0xFFFF * seamColor.b);

	BOOL drawSeams, drawHiddenEdge, drawEdges;
	pblock->GetValue(unwrap_renderuv_visible,0,drawEdges,FOREVER);
	pblock->GetValue(unwrap_renderuv_invisible,0,drawHiddenEdge,FOREVER);
	pblock->GetValue(unwrap_renderuv_seamedges,0,drawSeams,FOREVER);

	BOOL force2Sided;
	pblock->GetValue(unwrap_renderuv_force2sided,0,force2Sided,FOREVER);
	const BOOL backFaceCull = !force2Sided;

	int iVal;
	pblock->GetValue(unwrap_renderuv_fillmode,0,iVal,FOREVER);
	FillMode fillMode = static_cast<FillMode>(iVal);
	if (fillMode < FillMode::First) fillMode = FillMode::First;
	if (fillMode > FillMode::Last) fillMode = FillMode::Last;

	Color fillColor;
	pblock->GetValue(unwrap_renderuv_fillcolor,0,fillColor,FOREVER);
	fillColor.ClampMinMax();
	const int fillColorR = int(0xFFFF * fillColor.r);
	const int fillColorG = int(0xFFFF * fillColor.g);
	const int fillColorB = int(0xFFFF * fillColor.b);

	float fFillAlpha;
	pblock->GetValue(unwrap_renderuv_fillalpha,0,fFillAlpha,FOREVER);
	if (fFillAlpha < 0.0f) fFillAlpha = 0.0f;
	if (fFillAlpha > 1.0f) fFillAlpha = 1.0f;
	const int fillAlpha = int(0xFFFF * fFillAlpha);
	
	Color overlapColor;
	pblock->GetValue(unwrap_renderuv_overlapcolor,0,overlapColor,FOREVER);
	overlapColor.ClampMinMax();
	const int overlapColorR = int(0xFFFF * overlapColor.r);
	const int overlapColorG = int(0xFFFF * overlapColor.g);
	const int overlapColorB = int(0xFFFF * overlapColor.b);

	BOOL overlap;
	pblock->GetValue(unwrap_renderuv_overlap,0,overlap,FOREVER);

	bi.SetWidth(width);
	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetHeight(height);
	bi.SetType(BMM_TRUE_64);
	Bitmap *map = TheManager->Create(&bi);
	if (map == nullptr)
	{
		if (ip)
			ip->ReplacePrompt( GetString(IDS_RENDERMAP_FAILED) );
		return;
	}
	if ( RenderUVDlgProc::renderAllTiles && 
		(nullptr == RenderUVDlgProc::renderAllTilesMap) )
	{   // reuse one bitmap in render "All Tiles" option
		RenderUVDlgProc::renderAllTilesMap = map;
	}
	map->Fill(0,0,0,0);

	//draw our edges
	BitArray processedPixels;
	const int mapWidth = map->Width();
	const int mapHeight = map->Height();
	processedPixels.SetSize( mapWidth * mapHeight );
	processedPixels.ClearAll();

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		MeshTopoData *ld = mMeshTopoData[ldID];
		BitArray facingFaces;
		facingFaces.SetSize(ld->GetNumberFaces());//TVMaps.f.Count());
		facingFaces.SetAll();

		Tab<Point3> geomFaceNormals;
		geomFaceNormals.SetCount(ld->GetNumberFaces());
		for (int i =0; i < ld->GetNumberFaces(); i++)
		{
			geomFaceNormals[i] = ld->GetGeomFaceNormal(i);
			Point3 uvwNormal = ld->GetUVFaceNormal(i);
			if (uvwNormal.z < 0.0f) 
				facingFaces.Set(i,FALSE);
		}

		Tab<Point3> geomVertexNormals;
		geomVertexNormals.SetCount(ld->GetNumberGeomVerts());//TVMaps.geomPoints.Count());
		Tab<int> geomVertexNormalsCT;
		geomVertexNormalsCT.SetCount(ld->GetNumberGeomVerts());//(TVMaps.geomPoints.Count());
		for (int i =0; i < ld->GetNumberGeomVerts(); i++)
		{
			geomVertexNormals[i] = Point3(0.0f,0.0f,0.0f);
			geomVertexNormalsCT[i] = 0;
		}

		for (int i = 0; i < ld->GetNumberFaces(); i++)
		{
			for (int j = 0; j < ld->GetFaceDegree(i); j++)
			{
				const int index = ld->GetFaceGeomVert(i,j);//TVMaps.f[i]->v[j];
				geomVertexNormalsCT[index] = geomVertexNormalsCT[index] + 1;
				geomVertexNormals[index] += geomFaceNormals[i];
			}
		}

		for (int i =0; i < ld->GetNumberGeomVerts(); i++)
		{
			if (geomVertexNormalsCT[i] != 0)
				geomVertexNormals[i] = geomVertexNormals[i] / geomVertexNormalsCT[i];
			geomVertexNormals[i] = Normalize(geomVertexNormals[i]);
		}

		Tab<Point3> norms;
		Tab<Point3> pos;
		if (fillMode >= FillMode::Normal)
		{
			try
			{
				norms.SetCount( mapWidth * mapHeight );
			}
			catch( std::bad_alloc& )
			{
				if (ip)
					ip->DisplayTempPrompt(GetString(IDS_RENDERMAP_FAILED),20000 );
				map->DeleteThis();
				return;
			}

			for (int i = 0; i < norms.Count(); i++)
				norms[i] = Point3(0.0f,0.0f,0.0f);

			try
			{
				pos = norms;
			}
			catch( std::bad_alloc& )
			{
				if (ip)
					ip->DisplayTempPrompt(GetString(IDS_RENDERMAP_FAILED),20000 );
				map->DeleteThis();
				return;
			}
		}

		if (fillMode > FillMode::None)
		{
			Box3 bounds;
			bounds.Init();
			for (int i = 0; i < ld->GetNumberGeomVerts(); i++)
			{
				bounds += ld->GetGeomVert(i);//TVMaps.geomPoints[i];
			}
			bounds.Scale(4.0f);

			BitArray reCheckTheseFaces;
			reCheckTheseFaces.SetSize(ld->GetNumberFaces());//TVMaps.f.Count());
			reCheckTheseFaces.ClearAll();

	 		for (int i = 0; i < ld->GetNumberFaces(); i++)
			{
				BOOL drawFace = TRUE;
				if (backFaceCull)
				{
					drawFace = facingFaces[i];
				}
				if (ld->GetFaceDegree(i) < 3)
				{   // degenerate polygon
					drawFace = FALSE;
				}

				if ( drawFace && !ld->GetFaceHasVectors(i) )//TVMaps.f[i]->vecs == NULL))
				{
					const int id = ld->GetFaceTVVert(i,0);//TVMaps.f[i]->t[0];
					const Point3 Vert0 = ld->GetTVVert(id);//TVMaps.v[id].p;
					const IPoint2 pt0 = TVVert2MapVert( Vert0, mapWidth, mapHeight );

					const int numberOfTris = ld->GetFaceDegree(i) - 2;
					for (int j = 0; j < numberOfTris; j++)
					{
						int index[4];
						index[0] = ld->GetFaceTVVert(i,0);//TVMaps.f[i]->t[0];
						index[1] = ld->GetFaceTVVert(i,j+1);//TVMaps.f[i]->t[j + 1];
						index[2] = ld->GetFaceTVVert(i,j+2);//TVMaps.f[i]->t[j + 2]; 
						
						if (ld->IsTVVertVisible(index[0]) && 
							ld->IsTVVertVisible(index[1]) && 
							ld->IsTVVertVisible(index[2]))
						{
							int id = index[1];
							const Point3 VertA = ld->GetTVVert(id);//TVMaps.v[id].p;
							const IPoint2 pt1 = TVVert2MapVert( VertA, mapWidth, mapHeight );
							id = index[2];
							const Point3 VertB = ld->GetTVVert(id);//TVMaps.v[id].p;
							const IPoint2 pt2 = TVVert2MapVert( VertB, mapWidth, mapHeight );

							if (fillMode == FillMode::Solid)
							{
								if (overlap)
								{
									const BOOL hit = BXPTriangleCheckOverlap( pt0.x, pt0.y, 
										pt1.x, pt1.y, 
										pt2.x, pt2.y, 
										map, processedPixels );
									if (hit)
									{
										BXPTriangleFloat( pt0.x, pt0.y,
											pt1.x, pt1.y, 
											pt2.x, pt2.y, 
											overlapColorR, overlapColorG, overlapColorB, fillAlpha,
											map );
										reCheckTheseFaces.Set(i, TRUE);
									}
									else 
									{
										BXPTriangleFloat( pt0.x, pt0.y,
											pt1.x, pt1.y, 
											pt2.x, pt2.y, 
											fillColorR,fillColorG,fillColorB,fillAlpha,
											map );
									}
								}
								else 
								{
									BXPTriangleFloat( pt0.x, pt0.y,
										pt1.x, pt1.y, 
										pt2.x, pt2.y, 
										fillColorR,fillColorG,fillColorB,fillAlpha,
										map );
								}
							}
							else if (fillMode >= FillMode::Normal)
							{
								const int a = ld->GetFaceGeomVert(i,0);//TVMaps.f[i]->v[0];
								const int b = ld->GetFaceGeomVert(i,j+1);//TVMaps.f[i]->v[j + 1];
								const int c = ld->GetFaceGeomVert(i,j+2);//TVMaps.f[i]->v[j + 2]; 

								Point3 n1 = geomVertexNormals[a] ;
								Point3 n2 = geomVertexNormals[b] ;
								Point3 n3 = geomVertexNormals[c] ;

								if (!facingFaces[i])
								{
									n1 *= -1.0f;
									n2 *= -1.0f;
									n3 *= -1.0f;
								}

								BXPInterpData c0,c1,c2;
								c0.normal = n1;
								c1.normal = n2;
								c2.normal = n3;

								c0.pos = ld->GetGeomVert(a);//TVMaps.geomPoints[a];
								c1.pos = ld->GetGeomVert(b);//TVMaps.geomPoints[b];
								c2.pos = ld->GetGeomVert(c);//TVMaps.geomPoints[c];

								c0.x = pt0.x;
								c0.y = pt0.y;

								c1.x = pt1.x;
								c1.y = pt1.y;

								c2.x = pt2.x;
								c2.y = pt2.y;

								BOOL hit = FALSE;
								if (overlap)
								{
									hit = BXPTriangleCheckOverlap( pt0.x, pt0.y, 
										pt1.x, pt1.y, 
										pt2.x, pt2.y, 
										map, 
										processedPixels );
								}
								if (hit)
								{
									BXPTriangleFloat( pt0.x, pt0.y,
										pt1.x, pt1.y, 
										pt2.x, pt2.y, 
										overlapColorR, overlapColorG, overlapColorB, fillAlpha,
										map );
									reCheckTheseFaces.Set(i,TRUE);
								}
								else
								{
									if (fillMode == FillMode::Normal)
									{
										n1 = n1 + 1.0f;
										n1 *= 0.5f;
										n1 *= 0xfff0;

										n2 = n2 + 1.0f;
										n2 *= 0.5f;
										n2 *= 0xfff0;

										n3 = n3 + 1.0f;
										n3 *= 0.5f;
										n3 *= 0xfff0;

										c0.color = n1;
										c1.color = n2;
										c2.color = n3;
									}
									else if (fillMode == FillMode::Shaded)
									{
 										Point3 zPos(0.0f,0.0f,1.0f);
										Point3 zNeg(0.0f,0.0f,-1.0f);
										
 										const Point3 colorPos = fillColor;
										Point3 colorNeg = fillColor;
										colorNeg.x *= 0.5f;
										colorNeg.y *= 0.5f;

										zPos = c0.pos - bounds.pmax;
										zPos = Normalize(zPos);

										zNeg = c0.pos - bounds.pmin;
										zNeg = Normalize(zNeg);

										float dot = DotProd(zPos,n1);
										if (dot > 0.0f)
											c0.color += colorPos * dot;
										dot = DotProd(zNeg,n1);
										if (dot > 0.0f)
											c0.color += colorNeg * dot;

										zPos = c1.pos - bounds.pmax;
										zPos = Normalize(zPos);

										zNeg = c1.pos - bounds.pmin;
										zNeg = Normalize(zNeg);

										dot = DotProd(zPos,n2);
										if (dot > 0.0f)
											c1.color += colorPos * dot;
										dot = DotProd(zNeg,n2);
										if (dot > 0.0f)
											c1.color += colorNeg * dot;

										zPos = c2.pos - bounds.pmax;
										zPos = Normalize(zPos);

										zNeg = c2.pos - bounds.pmin;
										zNeg = Normalize(zNeg);

										dot = DotProd(zPos,n3);
										if (dot > 0.0f)
											c2.color += colorPos * dot;
										dot = DotProd(zNeg,n3);
										if (dot > 0.0f)
											c2.color += colorNeg * dot;

										c0.color *= 0xf000;
										c1.color *= 0xf000;
										c2.color *= 0xf000;
									}

									BXPTriangleFloat( c0, c1, c2, fillAlpha, map, norms, pos);
								}
							}
						}
					}
				}
			}

 			if (overlap && reCheckTheseFaces.NumberSet() > 0)
			{
				processedPixels.ClearAll();
	 			for (int i = (ld->GetNumberFaces()-1); i >= 0 ; i--)
				{
					BOOL drawFace = TRUE;
					if (backFaceCull)
					{
						drawFace = facingFaces[i];
					}
					if (ld->GetFaceDegree(i) < 3)
					{
						drawFace = FALSE;
					}

					if ( drawFace && !ld->GetFaceHasVectors(i) )//TVMaps.f[i]->vecs == NULL))
					{
						const int id = ld->GetFaceTVVert(i,0);//TVMaps.f[i]->t[0];
						const Point3 Vert0 = ld->GetTVVert(id);//TVMaps.v[id].p;
						const IPoint2 pt0 = TVVert2MapVert( Vert0, mapWidth, mapHeight );

						const int numberOfTris = ld->GetFaceDegree(i) - 2;
						for (int j = 0; j < numberOfTris; j++)
						{
							int index[4];
							index[0] = ld->GetFaceTVVert(i,0);//TVMaps.f[i]->t[0];
							index[1] = ld->GetFaceTVVert(i,j+1);//TVMaps.f[i]->t[j + 1];
							index[2] = ld->GetFaceTVVert(i,j+2);//TVMaps.f[i]->t[j + 2]; 
							
							if (ld->IsTVVertVisible(index[0]) && 
								ld->IsTVVertVisible(index[1]) && 
								ld->IsTVVertVisible(index[2]))
							{
								int id = index[1];
								const Point3 VertA = ld->GetTVVert(id);//TVMaps.v[id].p;
								const IPoint2 pt1 = TVVert2MapVert( VertA, mapWidth, mapHeight );
								id = index[2];
								const Point3 VertB = ld->GetTVVert(id);//TVMaps.v[id].p;
								const IPoint2 pt2 = TVVert2MapVert( VertB, mapWidth, mapHeight );

								if (fillMode == FillMode::Solid)
								{
									if (overlap)
									{
										const BOOL hit = BXPTriangleCheckOverlap( pt0.x, pt0.y, 
											pt1.x, pt1.y, 
											pt2.x, pt2.y, 
											map, processedPixels );
										if (hit)
										{
											BXPTriangleFloat( pt0.x, pt0.y,
												pt1.x, pt1.y, 
												pt2.x, pt2.y, 
												overlapColorR, overlapColorG, overlapColorB, fillAlpha,
												map );
										}
									}
								}
								else if (fillMode >= FillMode::Normal)
								{
									BOOL hit = FALSE;
									if (overlap)
									{
										hit = BXPTriangleCheckOverlap( pt0.x, pt0.y,
												pt1.x, pt1.y, 
												pt2.x, pt2.y, 
												map, processedPixels );
									}
									if (hit)
									{
										BXPTriangleFloat( pt0.x, pt0.y,
												pt1.x, pt1.y, 
												pt2.x, pt2.y, 
												overlapColorR, overlapColorG, overlapColorB, fillAlpha,
												map );
										reCheckTheseFaces.Set(i,TRUE);
									}
								}
							}
						}
					}
				}
			}	// if (overlap && reCheckTheseFaces.NumberSet() > 0)
		}	// if (fillMode > FillMode::None)
	
		for (int curEdgeIdx = 0; curEdgeIdx < ld->GetNumberTVEdges(); curEdgeIdx++)//TVMaps.ePtrList.Count();
		{
			const int a = ld->GetTVEdgeVert(curEdgeIdx,0);//TVMaps.ePtrList[curEdgeIdx]->a;
			const int b = ld->GetTVEdgeVert(curEdgeIdx,1);//TVMaps.ePtrList[curEdgeIdx]->b;
			if ( ld->IsTVVertVisible(a) && 
				 ld->IsTVVertVisible(b) )
			{
				BOOL draw = TRUE;
				if ( !drawHiddenEdge && ld->GetTVEdgeHidden(curEdgeIdx) )//TVMaps.ePtrList[curEdgeIdx]->flags & FLAG_HIDDENEDGEA;
				{
					draw = FALSE;
				}

				if (backFaceCull && draw)
				{
					int ct = ld->GetTVEdgeNumberTVFaces(curEdgeIdx);//TVMaps.ePtrList[curEdgeIdx]->faceList.Count();
					int facing = 0;
					for (int j = 0; j < ct; j++)
					{
						int faceIndex = ld->GetTVEdgeConnectedTVFace(curEdgeIdx,j);//TVMaps.ePtrList[curEdgeIdx]->faceList[j];
						if (facingFaces[faceIndex])
							facing++;
					}
					if (facing == 0)
						draw = FALSE;
				}
				if ( !draw ) { continue; }

				const int veca = ld->GetTVEdgeVec(curEdgeIdx,0);//TVMaps.ePtrList[curEdgeIdx]->avec;
				const int vecb = ld->GetTVEdgeVec(curEdgeIdx,1);//TVMaps.ePtrList[curEdgeIdx]->bvec;

				Point3 VertA = ld->GetTVVert(a);//TVMaps.v[a].p;
				Point3 VertB = ld->GetTVVert(b);//TVMaps.v[b].p;

				int cr = edgeColorR;
				int cg = edgeColorG;
				int cb = edgeColorB;

				const bool isSeam = (ld->GetTVEdgeNumberTVFaces(curEdgeIdx) == 1);//TVMaps.ePtrList[curEdgeIdx]->faceList.Count()
				if ( (veca != -1) && (vecb != -1) )
				{
					if (isSeam && drawSeams)
					{
						cr = seamColorR;
						cg = seamColorG;
						cb = seamColorB;
					}

					const Point3 pa = ld->GetTVVert(a);//TVMaps.v[a].p;
					const Point3 pb = ld->GetTVVert(b);//TVMaps.v[b].p;
 					const Point2 fpa(pa.x, pa.y);
					const Point2 fpb(pb.x, pb.y);

					const Point3 pvecA = ld->GetTVVert(veca);//TVMaps.v[veca].p;
					const Point3 pvecB = ld->GetTVVert(vecb);//TVMaps.v[vecb].p;
					const Point2 fpvecA(pvecA.x, pvecA.y);
					const Point2 fpvecB(pvecB.x, pvecB.y);

					VertA.x = pa.x;
					VertA.y = pa.y;

					static const int SEGMENT = 7;
 					for (int j = 1; j < SEGMENT; j++)
					{
						const float t = j / static_cast<float>(SEGMENT);
						const float s = 1.0f - t;
						const float t2 = t * t;
						const Point2 p = ( (s*fpa + (3.0f * t) * fpvecA) * s + (3.0f*t2)* fpvecB) * s + t * t2 * fpb;
						
						VertB.x = p.x;
						VertB.y = p.y;

						const IPoint2 pt1 = TVVert2MapVert( VertA, mapWidth, mapHeight );
						const IPoint2 pt2 = TVVert2MapVert( VertB, mapWidth, mapHeight );
						BXPLineFloat( pt1.x, pt1.y, pt2.x, pt2.y, 
							cr, cg, cb, edgeAlpha, map);
						VertA.x = VertB.x;
						VertA.y = VertB.y;
					}

					const IPoint2 pt1 = TVVert2MapVert( VertA, mapWidth, mapHeight );
					const IPoint2 pt2 = TVVert2MapVert( pb, mapWidth, mapHeight );
					BXPLineFloat( pt1.x, pt1.y, pt2.x, pt2.y, 
						cr, cg, cb, fEdgeAlpha, map);
				}
				else if ( (veca == -1) || (vecb == -1) )
				{
					const IPoint2 pt1 = TVVert2MapVert( VertA, mapWidth, mapHeight );
					const IPoint2 pt2 = TVVert2MapVert( VertB, mapWidth, mapHeight );
					if (isSeam && drawSeams)
					{
						cr = seamColorR;
						cg = seamColorG;
						cb = seamColorB;
						BXPLineFloat( pt1.x, pt1.y, pt2.x, pt2.y, 
							cr, cg, cb, edgeAlpha, map);
					}
					else if (drawEdges)
					{
						BXPLineFloat( pt1.x, pt1.y, pt2.x, pt2.y, 
							cr, cg, cb, edgeAlpha, map);
					}
				}
			}	// if ( ld->IsTVVertVisible(a) && ld->IsTVVertVisible(b) )
		}	// for (int curEdgeIdx = 0; curEdgeIdx < ld->GetNumberTVEdges(); curEdgeIdx++)//TVMaps.ePtrList.Count();
	}	// for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)

 	map->OpenOutput(&bi);
	map->Write(&bi);
	map->Close(&bi);

	BOOL showFrameBuffer;
	pblock->GetValue( unwrap_renderuv_showframebuffer, 0, showFrameBuffer, FOREVER );
	if ( showFrameBuffer )
	{
		if ( RenderUVDlgProc::renderAllTiles && (map != RenderUVDlgProc::renderAllTilesMap) )
		{
			RenderUVDlgProc::renderAllTilesMap->CopyImage(map, COPY_IMAGE_CROP, BMM_Color_64(0.0f, 0.0f, 0.0f, 0.0f) );
			map->DeleteThis();

			RenderUVDlgProc::renderAllTilesMap->RefreshWindow();
			HWND hMap = RenderUVDlgProc::renderAllTilesMap->GetWindow();
			if ( hMap ) 
			{
				UpdateWindow( hMap );
			}
		}
		else
		{
			map->Display( GetString(IDS_RENDERMAP), BMM_CN, TRUE );
		}
	}
	else
	{
		map->DeleteThis();
	}
}
