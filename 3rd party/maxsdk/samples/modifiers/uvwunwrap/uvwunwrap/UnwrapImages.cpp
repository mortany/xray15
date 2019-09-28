//
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.   
//
//

#include "unwrap.h"
#include "modsres.h"
#include "sctex.h"
#include "IDxMaterial.h"
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

void UnwrapMod::UpdateMapListBox()
{
	int ct = pblock->Count(unwrap_texmaplist);

	SendMessage(hTextures, CB_RESETCONTENT, 0, 0);

	HDC hdc = GetDC(hTextures);
	Rect rect;
	GetClientRectP( hTextures, &rect );
	SIZE size;
	int width = rect.w();

	dropDownListIDs.ZeroCount();
	if (ct > 0)
	{
		//texture checker 
		Mtl *stdMatTextureChecker = GetTextureCheckerMtl();
		if(nullptr == stdMatTextureChecker)
		{
			stdMatTextureChecker = CreateTextureCheckerMtl();
		}
		else
		{
			BitmapTex *textureChecker = (BitmapTex *)stdMatTextureChecker->GetSubTexmap(1);
			if (textureChecker == nullptr)
			{
				stdMatTextureChecker = CreateTextureCheckerMtl();
				textureChecker = (BitmapTex *)stdMatTextureChecker->GetSubTexmap(1);
			}
			if (textureChecker)
			{
				TSTR existingMapPathName = textureChecker->GetMapName();

				TCHAR expectedMapFile[1024];
				_tcscpy_s(expectedMapFile, _countof(expectedMapFile), GetCOREInterface()->GetDir(APP_MAX_SYS_ROOT_DIR));
				TSTR expectedMapPathName;
				expectedMapPathName.printf(_T("%s\\maps\\uvwunwrap\\UV_Checker.png"), expectedMapFile);
				GetFullPathName(expectedMapPathName, 1024, expectedMapFile, NULL);

				//If find the path is not the expected one, then create the texture checker
				if(_tcsicmp(existingMapPathName, expectedMapFile) != 0)
				{
					stdMatTextureChecker = CreateTextureCheckerMtl();
				}
			}
		}

		if(stdMatTextureChecker)
		{
			//The texture checker will take the first position in the dropdown list.
			BitmapTex *textureChecker = (BitmapTex *)stdMatTextureChecker->GetSubTexmap(1);
			if (textureChecker)
			{
				SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)(const TCHAR*)textureChecker->GetFullName());

				DLGetTextExtent(hdc, textureChecker->GetFullName(), &size);
				if (size.cx > width)
					width = size.cx;

				//set the index as -1
				int j = TEXTURECHECKERINDEX;
				dropDownListIDs.Append(1, &j, 100);
			}
		}
		
		//The regular checker will take the second position in the dropdown list.
		Texmap *map = NULL;
		int i = 0;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (const TCHAR*) map->GetFullName());		

		DLGetTextExtent(hdc, map->GetFullName(), &size);
		if ( size.cx > width ) 
			width = size.cx;

		//set the index as 0
		dropDownListIDs.Append(1,&i,100);
	}

	//now loop thru the custom texmaps on the object
	for (int i = 1; i < ct; i++)
	{
		Texmap *map = NULL;
		pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
		if (map != NULL) 
		{
			if (matid == -1)
			{
				SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (const TCHAR*) map->GetFullName());
				dropDownListIDs.Append(1,&i,100);
				DLGetTextExtent(hdc, map->GetFullName(), &size);
				if ( size.cx > width ) 
					width = size.cx;
			}
			else
			{
				int id = -1;
				pblock->GetValue(unwrap_texmapidlist,0,id,FOREVER,i);
				if (filterMatID[matid] == id)
				{
					SendMessage(hTextures, CB_ADDSTRING , 0, (LPARAM) (const TCHAR*) map->GetFullName());
					dropDownListIDs.Append(1,&i,100);
					DLGetTextExtent(hdc, map->GetFullName(), &size);
					if ( size.cx > width ) 
						width = size.cx;
				}
			}
		}
	}

	//display the distortion
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_ANGLE_DISTORTION));
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_AREA_DISTORTION));

	//
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_PICK));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_REMOVE));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)GetString(IDS_PW_RESET));	
	SendMessage(hTextures, CB_ADDSTRING, 0, (LPARAM)_T("---------------------"));

	//if we were using the grid bitmap keep it that way
	if (CurrentMap <= 0)
	{
		SendMessage(hTextures, CB_SETCURSEL, 0, CurrentMap+1);
	}
	//otherwise if we have bitmap use the first one
	else if (dropDownListIDs.Count() >= 3)
	{
		CurrentMap = dropDownListIDs[2];
		SendMessage(hTextures, CB_SETCURSEL, 2, 0);
	}
	//last case default to the grid bitmap
	else
	{
		CurrentMap = -1;
		SendMessage(hTextures, CB_SETCURSEL, 0, 0);
	}
	
	
		

	int iCount = SendMessage( hTextures, CB_GETCOUNT, 0, 0 );
	if(GetDistortionType() == eAngleDistortion)
	{
		SendMessage(hTextures, CB_SETCURSEL, iCount-AngleDistortionOffset, 0 );
		Painter2D::Singleton().SetUnwrapMod(this);
		Painter2D::Singleton().ForceDistortionRedraw();
	}
	else if(GetDistortionType() == eAreaDistortion)
	{
		SendMessage(hTextures, CB_SETCURSEL, iCount-AreaDistortionOffset, 0 );
		Painter2D::Singleton().SetUnwrapMod(this);
		Painter2D::Singleton().ForceDistortionRedraw();
	}

	ReleaseDC(hTextures,hdc); 
	if ( width > 0 ) 
		SendMessage(hTextures, CB_SETDROPPEDWIDTH, width+5, 0);
}

void UnwrapMod::ShowTextureCheckerMtl()
{
	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();

	Mtl *stdMatTextureChecker = GetTextureCheckerMtl();							
	Mtl *checkerMat = GetCheckerMap();

	macroRecorder->Disable();
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		INode *SelfNode = mMeshTopoData.GetNode(ldID);
		if (SelfNode)
		{
			Mtl *currentBaseMtl = SelfNode->GetMtl();

			if ((currentBaseMtl != checkerMat) && (currentBaseMtl != stdMatTextureChecker))
			{
				//backup the node's current material that is neither the regular checker,nor the texture checker. 
				pblock->SetValue(unwrap_originalmtl_list,0,currentBaseMtl,ldID);
			}

			if(currentBaseMtl != stdMatTextureChecker)
			{
				//set the texture checker to the material
				SelfNode->SetMtl(stdMatTextureChecker);
				//turn it on
				ip->ActivateTexture(stdMatTextureChecker->GetSubTexmap(1), stdMatTextureChecker, 1);
			}
		}
	}

	ip->RedrawViews(t);
	macroRecorder->Enable();
}

void UnwrapMod::ShowCheckerMaterial(BOOL show)
{
	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	Mtl *textureCheckerMat = GetTextureCheckerMtl();
	Mtl *checkerMat = GetCheckerMap();	

	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		INode *SelfNode = mMeshTopoData.GetNode(ldID);

		macroRecorder->Disable();
		if (SelfNode)
		{
			if (ldID < pblock->Count(unwrap_originalmtl_list))
			{				
				Mtl *currentBaseMtl = SelfNode->GetMtl();
				if ((show) && (checkerMat))
				{
					if ((currentBaseMtl != checkerMat) && (currentBaseMtl != textureCheckerMat))
					{
						//backup the node's current material that is neither the regular checker,nor the texture checker. 
						pblock->SetValue(unwrap_originalmtl_list,0,currentBaseMtl,ldID);
					}

					//set the checker to the material
					SelfNode->SetMtl(checkerMat);
					//turn it on
					ip->ActivateTexture(checkerMat->GetSubTexmap(1), checkerMat, 1);
				}
				else 
				{
					if (checkerMat)
						ip->DeActivateTexture(checkerMat->GetSubTexmap(1), checkerMat, 1);
					if(textureCheckerMat)
						ip->DeActivateTexture(textureCheckerMat->GetSubTexmap(1), textureCheckerMat, 1);

					checkerWasShowing = FALSE;
					if (currentBaseMtl == checkerMat || currentBaseMtl == textureCheckerMat)
					{
						//restore the node's material if the current material is either the regular checker, or the texture checker.
						Mtl *storedBaseMtl = NULL;
						pblock->GetValue(unwrap_originalmtl_list,0,storedBaseMtl,FOREVER,ldID);
						SelfNode->SetMtl(storedBaseMtl);
						checkerWasShowing = TRUE;
					}
					Mtl *nullMat = NULL;
					pblock->SetValue(unwrap_originalmtl_list,0,nullMat,ldID);
				}
				ip->RedrawViews(t);
			}
		}
		macroRecorder->Enable();
	}
}

void UnwrapMod::ResetMaterialList()
{
	//set the param block to 1
	pblock->SetCount(unwrap_texmaplist, 0);
	pblock->SetCount(unwrap_texmapidlist, 0);
	//get our mat list

	Mtl* textureCheckerMat = GetTextureCheckerMtl();
	Mtl* checkerMat = GetCheckerMap();
	Mtl *baseMtl = NULL;

	pblock->SetCount(unwrap_originalmtl_list, mMeshTopoData.Count());
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		INode *node = mMeshTopoData.GetNode(ldID);

		baseMtl = node->GetMtl();

		if(baseMtl == textureCheckerMat)
		{
			continue;
		}

		if (baseMtl == checkerMat)
		{
			pblock->GetValue(unwrap_originalmtl_list, 0, baseMtl, FOREVER, ldID);
		}
		else
		{
			pblock->SetValue(unwrap_originalmtl_list, 0, baseMtl, ldID);
		}
	}

	//add it to the pblock
	Tab<Texmap*> tmaps;
	Tab<int> matIDs;
	for (int ldID = 0; ldID < mMeshTopoData.Count(); ldID++)
	{
		INode *node = mMeshTopoData.GetNode(ldID);
		baseMtl = node->GetMtl();
		if (baseMtl 
			&& baseMtl != textureCheckerMat &&
			baseMtl != checkerMat)
		{
			ParseMaterials(baseMtl, tmaps, matIDs);
		}
	}

	pblock->SetCount(unwrap_texmaplist, tmaps.Count());
	pblock->SetCount(unwrap_texmapidlist, tmaps.Count());

	if (checkerMat)
	{
		pblock->SetCount(unwrap_texmaplist, 1 + tmaps.Count());
		pblock->SetCount(unwrap_texmapidlist, 1 + tmaps.Count());

		Texmap *checkMap = NULL;
		checkMap = (Texmap *)checkerMat->GetSubTexmap(1);
		if (checkMap)
		{
			pblock->SetValue(unwrap_texmaplist, 0, checkMap, 0);
			int id = -1;
			pblock->SetValue(unwrap_texmapidlist, 0, id, 0);
		}
	}
	if ((baseMtl != NULL))
	{
		for (int i = 0; i < tmaps.Count(); i++)
		{
			//The 0 index is for the regular checker, so using i + 1.
			pblock->SetValue(unwrap_texmaplist, 0, tmaps[i], i + 1);
			pblock->SetValue(unwrap_texmapidlist, 0, matIDs[i], i + 1);
		}
	}
}

void UnwrapMod::ParseMaterials(Mtl *base, Tab<Texmap*> &tmaps, Tab<int> &matIDs)
{
	if (base == NULL) return;
	Tab<Mtl*> materialStack;
	materialStack.Append(1, &base);
	while (materialStack.Count() != 0)
	{
		Mtl* topMaterial = materialStack[0];
		materialStack.Delete(0, 1);
		//append any mtl
		if (topMaterial && (topMaterial->ClassID() != Class_ID(MULTI_CLASS_ID, 0)))
		{
			for (int i = 0; i < topMaterial->NumSubMtls(); i++)
			{
				Mtl* addMat = topMaterial->GetSubMtl(i);
				if (addMat)
					materialStack.Append(1, &addMat, 100);
			}
		}

		IDxMaterial *dxMaterial = (IDxMaterial *)topMaterial->GetInterface(IDXMATERIAL_INTERFACE);
		if (dxMaterial != NULL)
		{
			int numberBitmaps = dxMaterial->GetNumberOfEffectBitmaps();
			for (int i = 0; i < numberBitmaps; i++)
			{
				PBBitmap *pmap = dxMaterial->GetEffectBitmap(i);
				if (pmap)
				{
					//create new bitmap texture
					BitmapTex *bmtex = (BitmapTex *)CreateInstance(TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID, 0));
					//add it to the list			
					TSTR mapName;
					mapName.printf(_T("%s"), pmap->bi.Name());
					bmtex->SetMapName(mapName);
					Texmap *map = (Texmap *)bmtex;
					tmaps.Append(1, &map, 100);
					int id = -1;
					matIDs.Append(1, &id, 100);
				}
			}
		}
		else if (topMaterial->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
		{
			MultiMtl *mtl = (MultiMtl*)topMaterial;
			IParamBlock2 *pb = mtl->GetParamBlock(0);
			if (pb)
			{
				int numMaterials = pb->Count(multi_mtls);
				for (int i = 0; i < numMaterials; i++)
				{
					int id = 0;
					Mtl *mat = NULL;
					pb->GetValue(multi_mtls, 0, mat, FOREVER, i);
					pb->GetValue(multi_ids, 0, id, FOREVER, i);

					if (mat)
					{
						int tex_count = mat->NumSubTexmaps();
						for (int j = 0; j < tex_count; j++)
						{
							Texmap *tmap;
							tmap = mat->GetSubTexmap(j);

							if (tmap != NULL)
							{
								tmaps.Append(1, &tmap, 100);
								matIDs.Append(1, &id, 100);
							}
						}
					}
				}
			}
		}
		else
		{
			int tex_count = topMaterial->NumSubTexmaps();
			for (int i = 0; i < tex_count; i++)
			{
				Texmap *tmap = topMaterial->GetSubTexmap(i);
				if (tmap != NULL)
				{
					tmaps.Append(1, &tmap, 100);
					int id = -1;
					matIDs.Append(1, &id, 100);
				}
			}
		}

	}
}

Texmap*  UnwrapMod::GetActiveMap()
{
	if(CurrentMap == TEXTURECHECKERINDEX)
	{
		Mtl *stdMatTextureChecker = GetTextureCheckerMtl();
		BitmapTex *textureChecker = NULL;
		if(stdMatTextureChecker)
		{
			textureChecker = (BitmapTex *)stdMatTextureChecker->GetSubTexmap(1);
		}		
		return textureChecker;
	}

	if ((CurrentMap >= 0) && (CurrentMap < pblock->Count(unwrap_texmaplist)))
	{
		Texmap *map = NULL;
		pblock->GetValue(unwrap_texmaplist, 0, map, FOREVER, CurrentMap);
		return map;
	}
	else return NULL;
}

Mtl* UnwrapMod::GetCheckerMap()
{
	Mtl *mtl = NULL;
	pblock->GetValue(unwrap_checkmtl, 0, mtl, FOREVER);
	return mtl;
}

Mtl* UnwrapMod::GetOriginalMap()
{
	Mtl *mtl = NULL;
	pblock->GetValue(unwrap_checkmtl, 0, mtl, FOREVER);
	return mtl;
}

void UnwrapMod::AddToMaterialList(Texmap *map, int id)
{
	pblock->Append(unwrap_texmaplist, 1, &map);
	pblock->Append(unwrap_texmapidlist, 1, &id);
}

void UnwrapMod::DeleteFromMaterialList(int index)
{
	if(index >=0)
	{
		pblock->Delete(unwrap_texmaplist, index, 1);
		pblock->Delete(unwrap_texmapidlist, index, 1);
	}	
}

void UnwrapMod::AddMaterial(MtlBase *mtl, BOOL update)
{
	if (mtl)
	{
		if (mtl->ClassID() == Class_ID(0x243e22c6, 0x63f6a014)) //gnormal material
		{
			if (mtl->GetSubTexmap(0) != NULL)
				mtl = mtl->GetSubTexmap(0);
		}

		int ct = pblock->Count(unwrap_texmaplist);
		for (int i = 0; i < ct; i++)
		{
			Texmap *map = NULL;
			pblock->GetValue(unwrap_texmaplist,0,map,FOREVER,i);
			if (map == mtl) 
			{
				return;
			}
		}
		AddToMaterialList((Texmap*) mtl, -1);
	}

	//The pblock->Count(unwrap_texmaplist) = 1(regular checker) + added texture count
	CurrentMap = pblock->Count(unwrap_texmaplist)-1;
	UpdateMapListBox();
}

void UnwrapMod::PickMap()
{	
	BOOL newMat=FALSE, cancel=FALSE;
	MtlBase *mtl = GetCOREInterface()->DoMaterialBrowseDlg(
		hDialogWnd,
		BROWSE_MAPSONLY|BROWSE_INCNONE|BROWSE_INSTANCEONLY,
		newMat,cancel);
	if (cancel) {
		if (newMat) mtl->MaybeAutoDelete();
		return;
	}

	if (mtl != NULL)
	{
		// Users can cancel the file browser dialog when picking bitmap file
		// which would put the unwrap tool into a bad state.
		// So make sure there is an actual bitmap before adding.
		if (mtl->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
		{
			BitmapTex* bmt = static_cast<BitmapTex*>(mtl);
			if (!bmt->GetBitmap(GetCOREInterface()->GetTime()))
			{
				if (newMat) mtl->MaybeAutoDelete();
				return;
			}
		}

		TSTR mstr = GetMacroStr(_T("modifiers[#unwrap_uvw].unwrap4.AddMap"));
		macroRecorder->FunctionCall(mstr, 1, 0,
			mr_reftarg,mtl);

		AddMaterial(mtl);
	}
}

using namespace tbb;

void GetPixels(Bitmap *bmp,UBYTE *pStart,int width,int height,int j,float wRadio,float hRadio,int scan)
{	
	UBYTE *p1 = pStart + (height-j-1)*scan;
	BMM_Color_64 color;
	for (int i=0; i<width; i++) 
	{
		bmp->GetPixels(int(i*wRadio),int(j*hRadio),1,&color);

		*p1++ = (UBYTE)(color.b>>8);
		*p1++ = (UBYTE)(color.g>>8);
		*p1++ = (UBYTE)(color.r>>8);
	}
}


class apply_texture{
	Bitmap *pBmp;
	UBYTE *pStart;	
	int width;
	int height;
	float wRadio;
	float hRadio;
	int scanw;
public:  
	apply_texture (Bitmap *b,UBYTE *ima,int w,int h,float fWR,float fHR): 
		pBmp(b),
		pStart(ima),
		width(w),
		height(h),
		wRadio(fWR),
		hRadio(fHR),
		scanw(ByteWidth(width)) {}
	void operator()(const blocked_range<int> & r) const
	{  	
		for (int j=r.begin(); j!=r.end(); j++)
		{			
			GetPixels(pBmp,pStart,width,height,j,wRadio,hRadio,scanw);
		}  
	}  
};

UBYTE *RenderBitmap(Bitmap *bmp,int w, int h,float wRadio = 1.0f,float hRadio = 1.0f)
{
	const int scanw = ByteWidth(w);
	UBYTE *image = new UBYTE[scanw * h];
	parallel_for(blocked_range<int>(0, h), apply_texture(bmp,image,w,h,wRadio,hRadio), auto_partitioner());
	return image;
}

void CheckTexTiling(StdUVGen* uvGen,float fExpectedTiling)
{
	if(uvGen == nullptr)
	{
		return;
	}

	const TimeValue tValue = GetCOREInterface()->GetTime();
	const float fMinValue = 0.001f;

	float fUTiling = uvGen->GetUScl(tValue);
	if(abs(fExpectedTiling - fUTiling) > fMinValue)
	{
		uvGen->SetUScl(fExpectedTiling,tValue);
	}

	float fVTiling = uvGen->GetVScl(tValue);
	if(abs(fExpectedTiling - fVTiling) > fMinValue)
	{
		uvGen->SetVScl(fExpectedTiling,tValue);
	}
}

void UnwrapMod::SetupImage()
{
	UBYTE* imageBefore = image;
	int iwBefore = iw;
	int ihBefore = ih;
	delete[] image;
	image = nullptr;

	if (GetActiveMap()) {		
		iw = rendW;
		ih = rendH;
		aspect = 1.0f;

		Bitmap *bmp = NULL;
		if (GetActiveMap()->ClassID() == Class_ID(BMTEX_CLASS_ID,0) )
		{
			isBitmap = 1;
			BitmapTex *bmt;
			bmt = (BitmapTex *) GetActiveMap();
			bmp = bmt->GetBitmap(GetCOREInterface()->GetTime());
			if (bmp!= NULL)
			{
				if (useBitmapRes)
				{
					bitmapWidth = bmp->Width();
					bitmapHeight = bmp->Height();
					iw = bitmapWidth;
					ih = bitmapHeight;
					aspect = (float)bitmapWidth/(float)bitmapHeight;
				}
				else	
				{
					bitmapWidth = iw;
					bitmapHeight = ih;

					aspect = (float)iw/(float)ih;
				}
			}
		}
		else
		{
			aspect = (float)iw/(float)ih;
			isBitmap = 0;
		}
		if (iw==0 || ih==0) return;
		GetActiveMap()->Update(GetCOREInterface()->GetTime(), FOREVER);
		GetActiveMap()->LoadMapFiles(GetCOREInterface()->GetTime());
		SetCursor(LoadCursor(NULL,IDC_WAIT));

		if (GetActiveMap()->ClassID() == Class_ID(BMTEX_CLASS_ID,0) )
		{
			if (bmp != NULL)
			{
				if (useBitmapRes && CurrentMap != TEXTURECHECKERINDEX)
				{
					image = RenderBitmap(bmp,iw,ih);
				}
				else
				{
					//Check the tiling value consistence withe the value in the property dialog
					BitmapTex* subBmTex = dynamic_cast<BitmapTex*>(GetActiveMap());
					if(subBmTex && CurrentMap == TEXTURECHECKERINDEX)
					{
						StdUVGen *uvGen = subBmTex->GetUVGen();
						CheckTexTiling(uvGen,fCheckerTiling);
					}
					image = RenderTexMap(GetActiveMap(),iw,ih,GetShowImageAlpha());
				}
			}
		}
		else if ( GetActiveMap()->ClassID() == Class_ID(MULTITILE_CLASS_ID,0) )
		{
			ClearImagesContainer();

			int iNumSubs = GetActiveMap()->NumSubTexmaps();
			for (int i = 0; i < iNumSubs; i++)
			{
				BitmapTex* subBmTex = dynamic_cast<BitmapTex*>(GetActiveMap()->GetSubTexmap(i));
				if(subBmTex)
				{
					StdUVGen *uvGen = subBmTex->GetUVGen();
					if(uvGen)
					{
						const TimeValue tValve = GetCOREInterface()->GetTime();
						Bitmap *subBmp = subBmTex->GetBitmap(tValve);
						if (subBmp) {
							const float wRadio = subBmp->Width() / float(iw);
							const float hRadio = subBmp->Height() / float(ih);
							UBYTE *bitmap = RenderBitmap(subBmp, iw, ih, wRadio, hRadio);
							imagesContainer.emplace_back(uvGen->GetUOffs(tValve), uvGen->GetVOffs(tValve), bitmap);
						}
					}
				}
			}

			Mtl *checkerMat = GetCheckerMap();
			DbgAssert(checkerMat);
			image = RenderTexMap(checkerMat->GetSubTexmap(1),iw,ih,GetShowImageAlpha());
		}
		else 
		{
			Texmap* tex = GetActiveMap();
			if(tex && CurrentMap == 0)
			{
				//For retuglar checker: UVGen *uvGen; // ref #0
				StdUVGen *uvGen = dynamic_cast<StdUVGen*>(tex->GetReference(0));
				//The tiling value is OK for texture checker, but is too small for regular checker.
				//Multiply 10
				CheckTexTiling(uvGen,fCheckerTiling*10);
			}
			image = RenderTexMap(GetActiveMap(),iw,ih,GetShowImageAlpha());
		}
		SetScalePixelUnits( iw, 1 ); // image width is the scale for the U axis
		SetScalePixelUnits( ih, 2 ); // image height is the scale for the V axis
		SetCursor(LoadCursor(NULL,IDC_ARROW));
		InvalidateView();
	}
	if (image)
		mUIManager.Enable(ID_SHOWMAP,TRUE);
	else
		mUIManager.Enable(ID_SHOWMAP,FALSE);

	if( (imageBefore!=image) || (iwBefore!=iw) || (ihBefore!=ih) )
	{
		SetupTypeins();
	}

	tileValid = FALSE;
}

void UnwrapMod::ClearImagesContainer()
{
	for (int i=0;i<imagesContainer.size();++i)
	{
		if(imagesContainer[i].singleImage)
		{
			delete[] imagesContainer[i].singleImage;
			imagesContainer[i].singleImage = nullptr;
		}
	}
	imagesContainer.clear();
}

Mtl* UnwrapMod::CreateTextureCheckerMtl()
{
	theHold.Suspend();
	Mtl *stdMatTextureChecker = NewDefaultStdMat();

	//add material for texture checker	
	BitmapTex *textureCheckerMap = (BitmapTex *) CreateInstance(TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID,0));
	if (textureCheckerMap != nullptr)
	{
		TCHAR imageFile[1024];
		_tcscpy_s(imageFile, _countof(imageFile), GetCOREInterface()->GetDir(APP_MAX_SYS_ROOT_DIR));
		TSTR mapPathName;
		mapPathName.printf(_T("%s\\maps\\uvwunwrap\\UV_Checker.png"),imageFile);
		textureCheckerMap->SetMapName(mapPathName);
		textureCheckerMap->SetName(GetString(IDS_TEXTURE_CHECKER));
	}

	if (stdMatTextureChecker != nullptr && textureCheckerMap != nullptr)
	{
		stdMatTextureChecker->SetName(GetString(IDS_TEXTURE_CHECKER_MATERIAL));
		stdMatTextureChecker->SetSubTexmap(1, textureCheckerMap);
		pblock->SetValue(unwrap_texturecheckermtl,0,stdMatTextureChecker);
	}	
	theHold.Resume();
	return stdMatTextureChecker;
}

Mtl* UnwrapMod::GetTextureCheckerMtl()
{
	Mtl *stdMatTextureChecker = nullptr;
	pblock->GetValue(unwrap_texturecheckermtl,0,stdMatTextureChecker,FOREVER,0);
	return stdMatTextureChecker;
}

void UnwrapMod::UpdateCheckerTiling()
{
	//update the 2d background in the editor
	SetupImage();
	tileValid = FALSE;
	InvalidateView();

	//redraw the 3d viewport
	Interface *ip = GetCOREInterface();
	TimeValue t = ip->GetTime();
	ip->RedrawViews(t);
}