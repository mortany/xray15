/**********************************************************************
*<
FILE: clipboard.cpp

DESCRIPTION: 

CREATED BY: Larry Minton

HISTORY: Created 15 April 2007

*>	Copyright (c) 2007, All Rights Reserved.
**********************************************************************/

#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include "MXSAgni.h"
#include <gamma.h>
#include <IColorCorrectionMgr.h>

#include <maxscript/maxwrapper/bitmaps.h>

#ifdef ScripterExport
#undef ScripterExport
#endif
#define ScripterExport __declspec( dllexport )

#include <maxscript\macros\define_instantiation_functions.h>

// ============================================================================

#include <maxscript\macros\define_external_functions.h>
#	include "namedefs.h"

#include <maxscript\macros\define_instantiation_functions.h>
#	include "clipboard_wraps.h"

// -----------------------------------------------------------------------------------------

Value*
setclipboardText_cf(Value** arg_list, int count)
{
	// setclipboardText <string>
	check_arg_count(setclipboardText, 1, count);
	TSTR workString = arg_list[0]->to_string();
	Replace_LF_with_CRLF(workString);
	int byteLen = (workString.Length()+1)*sizeof(TCHAR);
	HGLOBAL hGlobalMemory = GlobalAlloc (GHND, byteLen);
	if (!hGlobalMemory) 
		return_value (Integer::intern(-1));
	TCHAR* pGlobalMemory = (TCHAR*)GlobalLock (hGlobalMemory);
	memcpy(pGlobalMemory,workString.data(),byteLen);
	GlobalUnlock (hGlobalMemory);
	BOOL res = OpenClipboard (MAXScript_interface->GetMAXHWnd());
	if (!res) 
	{
		GlobalFree (hGlobalMemory);
		return_value (Integer::intern(-2));
	}
	res = EmptyClipboard();
	if (!res) 
	{
		GlobalFree (hGlobalMemory);
		CloseClipboard();
		return_value (Integer::intern(-3));
	}

#ifdef _UNICODE
	UINT clipboardFormat = CF_UNICODETEXT;
#else
	UINT clipboardFormat = CF_TEXT;
#endif
	HANDLE hres = SetClipboardData(clipboardFormat,hGlobalMemory);

	if (!hres) 
	{
		GlobalFree (hGlobalMemory);
		CloseClipboard();
		return_value (Integer::intern(-4));
	}
	CloseClipboard();
	return &true_value;
}

Value*
getclipboardText_cf(Value** arg_list, int count)
{
	// getclipboardText()
	check_arg_count(getclipboardText, 0, count);
#ifdef _UNICODE
	UINT clipboardFormat = CF_UNICODETEXT;
#else
	UINT clipboardFormat = CF_TEXT;
#endif
	BOOL res = IsClipboardFormatAvailable(clipboardFormat);
	if (!res) return &undefined;
	res = OpenClipboard (MAXScript_interface->GetMAXHWnd());
	if (!res) return &undefined;
	HANDLE hClipMem = GetClipboardData (clipboardFormat);
	if (!hClipMem) 
	{
		CloseClipboard();
		return &undefined;
	}
	MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
	one_typed_value_local_tls(String* result);
	TCHAR* pClipMem = (TCHAR*)GlobalLock (hClipMem);
	TSTR workString = pClipMem;
	GlobalUnlock (hClipMem);
	CloseClipboard();
	Replace_CRLF_with_LF(workString);
	vl.result = new String(workString);
	return_value_tls(vl.result);
}

//
// ----- This code was stolen from the dll/Abrowser project
//

BOOL ConvertToRGB(BITMAPINFO **ptr)
{
	BOOL res = false;
	BITMAPINFO *bOld = *ptr;
	int depth = bOld->bmiHeader.biBitCount;
	int w	  = bOld->bmiHeader.biWidth;
	int h	  = bOld->bmiHeader.biHeight;
	int i;

	if(bOld->bmiHeader.biCompression != BI_RGB)
		return false;

	if(depth>=32)
		return true;

	int clr = bOld->bmiHeader.biClrUsed;
	if(!clr)
		clr = 2<<(depth-1);								

	RGBQUAD *rgb= (RGBQUAD*) bOld->bmiColors;

	BYTE *buf = new BYTE[sizeof(BITMAPINFOHEADER) + w*h*sizeof(RGBQUAD)];
	if(!buf)
		return false;


	BITMAPINFO *bInfo = (BITMAPINFO*)buf;
	bInfo->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bInfo->bmiHeader.biWidth= w;
	bInfo->bmiHeader.biHeight=h;
	bInfo->bmiHeader.biPlanes=1; 
	bInfo->bmiHeader.biBitCount=32;
	bInfo->bmiHeader.biCompression=BI_RGB;
	bInfo->bmiHeader.biSizeImage=0;
	bInfo->bmiHeader.biClrUsed=0;
	bInfo->bmiHeader.biXPelsPerMeter=27306;
	bInfo->bmiHeader.biYPelsPerMeter=27306;
	bInfo->bmiHeader.biClrImportant=0;

	BYTE *data = (BYTE*) ((BYTE*)rgb + clr*sizeof(RGBQUAD));

	switch(depth)
	{
	case 24:
		{
			for(int j=0;j<h;j++)
			{
				int padW = ((3*w+ 3) & ~3);
				BYTE *line = (BYTE*)rgb + j*padW;
				for(i=0;i<w;i++)
				{
					bInfo->bmiColors[j*w+i].rgbBlue		= line[3*i];
					bInfo->bmiColors[j*w+i].rgbGreen	= line[3*i+1];
					bInfo->bmiColors[j*w+i].rgbRed		= line[3*i+2];
					bInfo->bmiColors[j*w+i].rgbReserved = 0;
				}
			}
		}
		break;


	case 8:
		{
			for(int j=0;j<h;j++)
			{
				int padW = ((w+ 3) & ~3);
				BYTE *line = data + j*padW;
				for(i=0;i<w;i++)
				{
					bInfo->bmiColors[j*w+i].rgbBlue	 = rgb[line[i]].rgbBlue;	//rgb[line[i]].rgbRed;
					bInfo->bmiColors[j*w+i].rgbGreen = rgb[line[i]].rgbGreen;
					bInfo->bmiColors[j*w+i].rgbRed   = rgb[line[i]].rgbRed;		//rgb[line[i]].rgbBlue;
					bInfo->bmiColors[j*w+i].rgbReserved = rgb[line[i]].rgbReserved;
				}
			}
		}
		break;

	case 4:
		{
			for(int j=0;j<h;j++)
			{
				int padW = ((w/2)+ (w%2?1:0)+ 3) & ~3;
				BYTE *line = data + j*padW;
				for(i=0;i<w/2;i++)
				{
					BYTE b = line[i];					
					bInfo->bmiColors[j*w+2*i].rgbBlue		= rgb[b&0x0f].rgbBlue;
					bInfo->bmiColors[j*w+2*i].rgbGreen		= rgb[b&0x0f].rgbGreen;
					bInfo->bmiColors[j*w+2*i].rgbRed		= rgb[b&0x0f].rgbRed;
					bInfo->bmiColors[j*w+2*i].rgbReserved	= rgb[b&0x0f].rgbReserved;

					bInfo->bmiColors[j*w+2*i+1].rgbBlue		= rgb[(b>>4)&0x0f].rgbBlue;
					bInfo->bmiColors[j*w+2*i+1].rgbGreen	= rgb[(b>>4)&0x0f].rgbGreen;
					bInfo->bmiColors[j*w+2*i+1].rgbRed		= rgb[(b>>4)&0x0f].rgbRed;
					bInfo->bmiColors[j*w+2*i+1].rgbReserved	= rgb[(b>>4)&0x0f].rgbReserved;
				}
			}
		}
		break;

	case 1:
		{
			for(int j=0;j<h;j++)
			{
				int padW = ((w/8)+ (w%8?1:0) + 3) & ~3;
				BYTE *line = data + j*padW;
				for(i=0;i<w/8;i++)
				{
					BYTE b = line[i];					
					for(int k=0;k<8;k++)
					{
						bInfo->bmiColors[j*w+8*i+k].rgbBlue		= rgb[(b&(0x80>>k))?1:0].rgbBlue;
						bInfo->bmiColors[j*w+8*i+k].rgbGreen	= rgb[(b&(0x80>>k))?1:0].rgbGreen;
						bInfo->bmiColors[j*w+8*i+k].rgbRed		= rgb[(b&(0x80>>k))?1:0].rgbRed;
						bInfo->bmiColors[j*w+8*i+k].rgbReserved	= rgb[(b&(0x80>>k))?1:0].rgbReserved;
					}
				}
			}
		}
		break;

	default: goto Done;
	}

	res = true;

Done:
	if(!res)
	{
		delete bInfo;
	}	
	else
	{
		delete *ptr;
		*ptr = bInfo;
	}
	return res;
}

BMM_Color_64 local_c;
Bitmap* CreateBitmapFromBInfo(void **ptr, const int cx, const int cy, bool captureAlpha)
{
	Bitmap		*map=NULL;
	Bitmap		*mapTmp=NULL;
	BitmapInfo  bi,biTmp;
	float		w,h;
	BITMAPINFO  *bInfo;

	if(!ConvertToRGB((BITMAPINFO**)ptr))
		goto Done;

	bInfo = (BITMAPINFO*) *ptr;

	biTmp.SetName(_T(""));
	biTmp.SetType(BMM_TRUE_32);

	if(!bInfo)
		goto Done;

	w = (float)bInfo->bmiHeader.biWidth;
	h = (float)bInfo->bmiHeader.biHeight;
	bool hasAlpha = captureAlpha && bInfo->bmiHeader.biBitCount >= 32;

	biTmp.SetWidth((int)w);
	biTmp.SetHeight((int)h);
	if (hasAlpha)
		biTmp.SetFlags(MAP_HAS_ALPHA);

	mapTmp = CreateBitmapFromBitmapInfo(biTmp);
	if(!mapTmp) goto Done;
	mapTmp->FromDib(bInfo);

	bi.SetName(_T(""));
	bi.SetType(BMM_TRUE_64);
	if (hasAlpha)
		bi.SetFlags(MAP_HAS_ALPHA);
	if(w>h)
	{
		bi.SetWidth(cx);
		bi.SetHeight((int)(cx*h/w));
	}
	else
	{
		bi.SetHeight(cy);
		bi.SetWidth((int)(cy*w/h));
	}
	map = CreateBitmapFromBitmapInfo(bi);
	if(!map) goto Done;

	map->CopyImage(mapTmp,COPY_IMAGE_RESIZE_HI_QUALITY,local_c,&bi);

Done:
	if(mapTmp) mapTmp->DeleteThis();
	return map;
}

/* 
The code that saves a bitmap always assumes the bitmap to be saved comes from the renderer,
and hence, is linear. Since we are grabbing off the viewport (with an embedded gamma) we
need to linearize the bitmap first 
*/
void DeGammaCorrectBitmap(Bitmap* map)
{
	if (map == nullptr)
		return;
	IColorCorrectionMgr* idispGamMgr = (IColorCorrectionMgr*) GetCOREInterface(COLORCORRECTIONMGR_INTERFACE);
	if(gammaMgr.IsEnabled() && idispGamMgr)
	{
		float gamma = idispGamMgr->GetGamma();
		UWORD  *gammatab = new UWORD[RCOLN];

		// Build gamma correction table
		if (gammatab)
		{
			BuildGammaTab(gammatab, 1.0f/gamma, true);
			map->Storage()->bi.SetGamma(gamma);
			int width = map->Width();
			int height = map->Height();

			BMM_Color_64 *pixelrow = (BMM_Color_64 *)LocalAlloc(LPTR,width*sizeof(BMM_Color_64));

			if (pixelrow) 
			{
				for (int iy = 0; iy < height; iy++) {
					map->GetPixels(0, iy, width, pixelrow);
					for (int ix = 0; ix < width; ix++) {

						pixelrow[ix].r = gammatab[UWORD(pixelrow[ix].r) >> RCSH16];
						pixelrow[ix].g = gammatab[UWORD(pixelrow[ix].g) >> RCSH16];
						pixelrow[ix].b = gammatab[UWORD(pixelrow[ix].b) >> RCSH16];
					}
					map->PutPixels(0, iy, width, pixelrow);
				}
				LocalFree(pixelrow);
			}
			delete [] gammatab;

			// We still want this to be SAVED with a gamma. What gamma MaxScript will save
			// this with (by default) is defined by the gamma of the BitmapInfo bi's gamma
			// And we intentionally want it to look like it was displayed - hence we use the
			// display gamma!!
			map->Storage()->bi.SetGamma(gamma);
		}
	}
}

Value*
setclipboardBitmap_cf(Value** arg_list, int count)
{
	// setclipboardBitmap <bitmap>
	check_arg_count(setclipboardBitmap, 1, count);

	MAXBitMap* mbm = (MAXBitMap*)arg_list[0];
	type_check(mbm, MAXBitMap, _T("setclipboardBitmap"));
	int depth = (mbm->bm->HasAlpha()) ? 32 : 24;
	PBITMAPINFO bmi = mbm->bm->ToDib(depth);
	HDC hdc = GetDC(GetDesktopWindow());
	HBITMAP new_map = CreateDIBitmap(hdc, &bmi->bmiHeader, CBM_INIT, bmi->bmiColors, bmi, DIB_RGB_COLORS); 
	ReleaseDC(GetDesktopWindow(), hdc);	
	LocalFree(bmi);
	if (!new_map)
		return_value (Integer::intern(-1));
	BOOL res = OpenClipboard (MAXScript_interface->GetMAXHWnd());
	if (!res) 
	{
		DeleteObject(new_map);
		return_value (Integer::intern(-2));
	}
	res = EmptyClipboard();
	if (!res) 
	{
		DeleteObject(new_map);
		CloseClipboard();
		return_value (Integer::intern(-3));
	}
	HANDLE hres = SetClipboardData (CF_BITMAP, new_map);
	if (!hres) 
	{
		DeleteObject(new_map);
		CloseClipboard();
		return_value (Integer::intern(-4));
	}
	CloseClipboard();
	return &true_value;
}

Value*
getclipboardBitmap_cf(Value** arg_list, int count)
{
	// getclipboardBitmap()
	check_arg_count(getclipboardBitmap, 0, count);
	BOOL res = IsClipboardFormatAvailable(CF_BITMAP);
	if (!res) 
		return &undefined;
	res = OpenClipboard (MAXScript_interface->GetMAXHWnd());
	if (!res) 
		return &undefined;
	HBITMAP hBitmap = (HBITMAP)GetClipboardData (CF_BITMAP); // clipboard controls the handle
	if (!hBitmap) 
	{
		CloseClipboard();
		return &undefined;
	}

	BITMAP bm;
	GetObject(hBitmap, sizeof(BITMAP), &bm);
	LPBITMAPINFO pbmi = (LPBITMAPINFO)calloc(1, sizeof(BITMAPINFOHEADER) + ((bm.bmBitsPixel/8) * bm.bmWidth * bm.bmHeight));

	if (pbmi)
	{
		HDC dc = GetDC(NULL);
		pbmi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		pbmi->bmiHeader.biWidth       = bm.bmWidth;
		pbmi->bmiHeader.biHeight      = bm.bmHeight;
		pbmi->bmiHeader.biPlanes      = bm.bmPlanes;
		pbmi->bmiHeader.biBitCount    = bm.bmBitsPixel;
		pbmi->bmiHeader.biCompression = BI_RGB;
		pbmi->bmiHeader.biSizeImage   = ((bm.bmBitsPixel/8) * bm.bmWidth * bm.bmHeight);
		res = (GetDIBits(dc,hBitmap,0,bm.bmHeight,&pbmi->bmiColors, pbmi, DIB_RGB_COLORS) != 0);
		ReleaseDC(NULL, dc);
	}
	CloseClipboard();
	Bitmap* bitmap = res ? TheManager->Create(pbmi, true) : nullptr;
	free(pbmi);
	if (bitmap)
	{
		MAXScript_TLS* _tls = (MAXScript_TLS*)TlsGetValue(thread_locals_index);
		return_value_tls (new MAXBitMap(bitmap->GetBitmapInfo(), bitmap));
	}
	return &undefined;
}

