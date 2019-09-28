// UnwrapRuntime.cpp : Defines the entry point for the console application.
//

#include <iostream>

using namespace std;

#include "stdafx.h"
#include "../uvwunwrap/unwrap.h"
#include "../uvwunwrap/MapIOModifier/MapIO.h"
#include <fcntl.h>
#include <locale.h>
#include <io.h> 
#include <string>
#include <codecvt>

#include <polyobj.h>
#include <triobj.h>
#include <mesh.h>


float uval[3] = { 1.0f,0.0f,1.0f };
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
const static int MESSAGE_BUFFER_SIZE = BUFSIZ;

// Prints a message in UTF16 so that DA can see it
void print(string message)
{
	// Flush the current content of stdout so that we can change the encoding of the document to UTF16
	fflush(stdout);
	int lastMode = _setmode(_fileno(stdout), _O_U16TEXT);
	// Convert the message to a wide char string and print it
	std::wstring wide = converter.from_bytes(message + "\n");
	wprintf(wide.c_str(), MESSAGE_BUFFER_SIZE);
	// Flush again and reset to the previous mode in case other functions do not use wide chars and UTF16
	fflush(stdout);
	_setmode(_fileno(stdout), lastMode);
}

void BuildMesh(Mesh& mesh)
{
	Point3 p;
	int na = 0;
	int nb = 0;
	int nc = 0;
	int nd = 0;
	int jx = 0;
	int nf = 0;
	int nv = 0;
	float delta = 0.0f;
	float delta2 = 0.0f;
	float a = 0.0f;
	float alt = 0.0f;
	float secrad = 0.0f;
	float secang = 0.0f;
	float b = 0.0f;
	float c = 0.0f;
	int segs = 0;
	int smooth = 0;
	float radius = 0.0f;
	float hemi = 0.0f;
	BOOL noHemi = FALSE;
	int squash = 0;
	int recenter = 0;
	BOOL genUVs = TRUE;
	float startAng = 0.0f;
	float pie1 = 0.0f;
	float pie2 = 0.0f;
	int doPie = 0;

	// Start the validity interval at forever and widdle it down.
	radius = 10.0f;
	segs = 10;
	smooth = true;

	float totalPie(0.0f);
	if (doPie) doPie = 1;
	else doPie = 0;
	if (doPie)
	{
		pie2 += startAng; pie1 += startAng;
		while (pie1 < pie2) pie1 += TWOPI;
		while (pie1 > pie2 + TWOPI) pie1 -= TWOPI;
		if (pie1 == pie2) totalPie = TWOPI;
		else totalPie = pie1 - pie2;
	}

	if (hemi<0.00001f) noHemi = TRUE;
	if (hemi >= 1.0f) hemi = 0.9999f;
	hemi = (1.0f - hemi) * PI;
	float basedelta = 2.0f*PI / (float)segs;
	delta2 = (doPie ? totalPie / (float)segs : basedelta);
	if (!noHemi && squash) {
		delta = 2.0f*hemi / float(segs - 2);
	}
	else {
		delta = basedelta;
	}

	int rows;
	if (noHemi || squash) {
		rows = (segs / 2 - 1);
	}
	else {
		rows = int(hemi / delta) + 1;
	}
	int realsegs = (doPie ? segs + 2 : segs);
	int nverts = rows * realsegs + 2;
	int nfaces = rows * realsegs * 2;
	if (doPie)
	{
		startAng = pie2; segs += 1;
		if (!noHemi) { nfaces -= 2; nverts -= 1; }
	}

	
	mesh.setNumVerts(nverts);
	mesh.setNumFaces(nfaces);
	mesh.setSmoothFlags(smooth != 0);
	int lastvert = nverts - 1;



	// Top vertex 
	mesh.setVert(nv, 0.0f, 0.0f, radius);
	nv++;

	// Middle vertices 
	alt = delta;
	for (int ix = 1; ix <= rows; ix++) {
		if (!noHemi && ix == rows) alt = hemi;
		a = (float)cos(alt)*radius;
		secrad = (float)sin(alt)*radius;
		secang = startAng; //0.0f
		for (int jx = 0; jx<segs; ++jx) {
			b = (float)cos(secang)*secrad;
			c = (float)sin(secang)*secrad;
			mesh.setVert(nv++, b, c, a);
			secang += delta2;
		}
		if (doPie && (noHemi || (ix<rows))) mesh.setVert(nv++, 0.0f, 0.0f, a);
		alt += delta;
	}

	/* Bottom vertex */
	if (noHemi) {
		mesh.setVert(nv++, 0.0f, 0.0f, -radius);
	}
	else {
		a = (float)cos(hemi)*radius;
		mesh.setVert(nv++, 0.0f, 0.0f, a);
	}

	BOOL issliceface;
	// Now make faces 
	if (doPie) segs++;

	BOOL usePhysUVs = FALSE;
	BitArray startSliceFaces;
	BitArray endSliceFaces;

	if (usePhysUVs) {
		startSliceFaces.SetSize(mesh.numFaces);
		endSliceFaces.SetSize(mesh.numFaces);
	}
	// Make top conic cap
	for (int ix = 1; ix <= segs; ++ix) {
		issliceface = (doPie && (ix >= segs - 1));
		nc = (ix == segs) ? 1 : ix + 1;
		mesh.faces[nf].setEdgeVisFlags(1, 1, 1);
		if ((issliceface) && (ix == segs - 1))
		{
			mesh.faces[nf].setSmGroup(smooth ? 4 : 0);
			mesh.faces[nf].setMatID(2);
			if (usePhysUVs)
				startSliceFaces.Set(nf);
		}
		else if ((issliceface) && (ix == segs))
		{
			mesh.faces[nf].setSmGroup(smooth ? 8 : 0);
			mesh.faces[nf].setMatID(3);
			if (usePhysUVs)
				endSliceFaces.Set(nf);
		}
		else
		{
			mesh.faces[nf].setSmGroup(smooth ? 1 : 0);
			mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
										//			mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		mesh.faces[nf].setVerts(0, ix, nc);
		nf++;
	}

	/* Make midsection */
	int lastrow = rows - 1, lastseg = segs - 1, almostlast = lastseg - 1;
	BOOL weirdpt = doPie && !noHemi, weirdmid = weirdpt && (rows == 2);
	for (int ix = 1; ix<rows; ++ix) {
		jx = (ix - 1)*segs + 1;
		for (int kx = 0; kx<segs; ++kx) {
			issliceface = (doPie && (kx >= almostlast));

			na = jx + kx;
			nb = na + segs;
			nb = (weirdmid && (kx == lastseg) ? lastvert : na + segs);
			if ((weirdmid) && (kx == almostlast)) nc = lastvert; else
				nc = (kx == lastseg) ? jx + segs : nb + 1;
			nd = (kx == lastseg) ? jx : na + 1;

			mesh.faces[nf].setEdgeVisFlags(1, 1, 0);


			if ((issliceface) && ((kx == almostlast - 2) || (kx == almostlast)))
			{
				mesh.faces[nf].setSmGroup(smooth ? 4 : 0);
				mesh.faces[nf].setMatID(2);
				if (usePhysUVs)
					startSliceFaces.Set(nf);
			}
			else if ((issliceface) && ((kx == almostlast - 1) || (kx == almostlast + 1)))
			{
				mesh.faces[nf].setSmGroup(smooth ? 8 : 0);
				mesh.faces[nf].setMatID(3);
				if (usePhysUVs)
					endSliceFaces.Set(nf);
			}
			else
			{
				mesh.faces[nf].setSmGroup(smooth ? 1 : 0);
				mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
											//				mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[nf].setVerts(na, nb, nc);
			nf++;

			mesh.faces[nf].setEdgeVisFlags(0, 1, 1);

			if ((issliceface) && ((kx == almostlast - 2) || (kx == almostlast)))
			{
				mesh.faces[nf].setSmGroup(smooth ? 4 : 0);
				mesh.faces[nf].setMatID(2);
				if (usePhysUVs)
					startSliceFaces.Set(nf);
			}
			else if ((issliceface) && ((kx == almostlast - 1) || (kx == almostlast + 1)))
			{
				mesh.faces[nf].setSmGroup(smooth ? 8 : 0);
				mesh.faces[nf].setMatID(3);
				if (usePhysUVs)
					endSliceFaces.Set(nf);
			}
			else
			{
				mesh.faces[nf].setSmGroup(smooth ? 1 : 0);
				mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
											//				mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
			}

			mesh.faces[nf].setVerts(na, nc, nd);
			nf++;
		}
	}

	// Make bottom conic cap
	na = mesh.getNumVerts() - 1;
	int botsegs = (weirdpt ? segs - 2 : segs);
	jx = (rows - 1)*segs + 1; lastseg = botsegs - 1;
	int fstart = nf;
	for (int ix = 0; ix<botsegs; ++ix) {
		issliceface = (doPie && (ix >= botsegs - 2));
		nc = ix + jx;
		nb = (!weirdpt && (ix == lastseg) ? jx : nc + 1);
		mesh.faces[nf].setEdgeVisFlags(1, 1, 1);

		if ((issliceface) && (noHemi) && (ix == botsegs - 2))
		{
			mesh.faces[nf].setSmGroup(smooth ? 4 : 0);
			mesh.faces[nf].setMatID(2);
		}
		else if ((issliceface) && (noHemi) && (ix == botsegs - 1))
		{
			mesh.faces[nf].setSmGroup(smooth ? 8 : 0);
			mesh.faces[nf].setMatID(3);
		}
		else if ((!issliceface) && (noHemi))
		{
			mesh.faces[nf].setSmGroup(smooth ? 1 : 0);
			mesh.faces[nf].setMatID(1); // mjm - 5.5.99 - rollback change - should be fixed in later release
										//			mesh.faces[nf].setMatID(0); // mjm - 3.2.99 - was set to 1
		}
		else if (!noHemi)
		{
			mesh.faces[nf].setSmGroup(smooth ? 2 : 0);
			mesh.faces[nf].setMatID(0); // mjm - 5.5.99 - rollback change - should be fixed in later release
										//			mesh.faces[nf].setMatID(1); // mjm - 3.2.99 - was set to 0
		}
		//		else
		//		{	mesh.faces[nf].setSmGroup(0);
		//			mesh.faces[nf].setMatID(noHemi?1:0); // mjm - 5.5.99 - rollback change - should be fixed in later release
		//			mesh.faces[nf].setMatID(noHemi?0:1); // mjm - 3.2.99 - was commented out but set to 1:0
		//		}

		mesh.faces[nf].setVerts(na, nb, nc);

		nf++;
	}

	int fend = nf;
	// Put the flat part of the hemisphere at z=0
	if (recenter) {
		float shift = (float)cos(hemi) * radius;
		for (int ix = 0; ix<mesh.getNumVerts(); ix++) {
			mesh.verts[ix].z -= shift;
		}
	}

	if (genUVs) {
		int tvsegs = segs;
		int tvpts = (doPie ? segs + 1 : segs);
		int ntverts = (rows + 2)*(tvpts + 1);
		//		if (doPie) {ntverts-=6; if (weirdpt) ntverts-3;}
		mesh.setNumTVerts(ntverts);
		mesh.setNumTVFaces(nfaces);
		nv = 0;
		delta = basedelta;  // make the texture squash too
		alt = 0.0f; // = delta;
		float uScale = usePhysUVs ? ((float) 2.0f * PI * radius) : 1.0f;
		float vScale = usePhysUVs ? ((float)PI * radius) : 1.0f;
		int dsegs = (doPie ? 3 : 0), midsegs = tvpts - dsegs, m1 = midsegs + 1, t1 = tvpts + 1;
		for (int ix = 0; ix < rows + 2; ix++) {
			//	if (!noHemi && ix==rows) alt = hemi;		
			secang = 0.0f; //angle;
			float yang = 1.0f - alt / PI;
			for (int jx = 0; jx <= midsegs; ++jx) {
				mesh.setTVert(nv++, uScale*(secang / TWOPI), vScale*yang, 0.0f);
				secang += delta2;
			}
			for (int jx = 0; jx<dsegs; jx++) mesh.setTVert(nv++, uScale*uval[jx], vScale*yang, 0.0f);
			alt += delta;
		}

		nf = 0; dsegs = (doPie ? 2 : 0), midsegs = segs - dsegs;
		// Make top conic cap
		for (int ix = 0; ix<midsegs; ++ix) {
			mesh.tvFace[nf++].setTVerts(ix, ix + t1, ix + t1 + 1);
		} int ix = midsegs + 1; int topv = ix + 1;
		for (int jx = 0; jx<dsegs; jx++)
		{
			mesh.tvFace[nf++].setTVerts(topv, ix + t1, ix + t1 + 1); ix++;
		}
		int cpt;
		/* Make midsection */
		for (ix = 1; ix<rows; ++ix) {
			cpt = ix*t1;
			for (int kx = 0; kx<tvsegs; ++kx) {
				if (kx == midsegs) cpt++;
				na = cpt + kx;
				nb = na + t1;
				nc = nb + 1;
				nd = na + 1;
				assert(nc<ntverts);
				assert(nd<ntverts);
				mesh.tvFace[nf++].setTVerts(na, nb, nc);
				mesh.tvFace[nf++].setTVerts(na, nc, nd);
			}
		}
		// Make bottom conic cap
		if (noHemi || !usePhysUVs) {
			int lastv = rows*t1, jx = lastv + t1;
			if (weirdpt) dsegs = 0;
			int j1;
			for (int j1 = lastv; j1<lastv + midsegs; j1++) {
				mesh.tvFace[nf++].setTVerts(jx, j1 + 1, j1); jx++;
			}
			j1 = lastv + midsegs + 1; topv = j1 + t1 + 1;
			for (int ix = 0; ix<dsegs; ix++) {
				mesh.tvFace[nf++].setTVerts(topv, j1 + 1, j1); j1++;
			}
			assert(nf == nfaces);
		}
		else {
			Matrix3 m = TransMatrix(Point3(0.0f, 0.0f, -a));
			m.PreScale(Point3(1.0f, -1.0f, 1.0f));
		}
	}
	else {
		mesh.setNumTVerts(0);
		mesh.setNumTVFaces(0);
	}

	mesh.InvalidateTopologyCache();
	
}


#define _CRT_SECURE_NO_WARNINGS

int main(int argc,      // Number of strings in array argv  
	char *argv[],   // Array of command-line argument strings  
	char *envp[])
{
	if (argc == 3)
	{
		wchar_t wc[2408];
		size_t ccount;

		char* cfilename = argv[1];
		mbstowcs_s(&ccount,wc, cfilename, 2408);
		TSTR fileName;
		fileName.printf(_T("%s"), wc);

		cfilename = argv[2];
		mbstowcs_s(&ccount, wc, cfilename, 2408);
		TSTR fileNameOut;
		fileNameOut.printf(_T("%s"), wc);


		UnwrapMod* unwrapMod = new UnwrapMod();
		ObjectState objectState;

		bool isMesh = !MapIO::IsPoly(fileName);

		if (isMesh)
		{
			TriObject* triObj = new TriObject();
			Mesh& msh = triObj->GetMesh();
			MapIO mapIO(&msh);
			bool iret = mapIO.Read(fileName);
			if (iret == false) return 0;
//			BuildMesh(msh);
			objectState.obj = triObj;
		}
		else
		{
			PolyObject* polyObj = new PolyObject();
			MNMesh& msh = polyObj->GetMesh();
			MapIO mapIO(&msh);
			bool iret = mapIO.Read(fileName);
			if (iret == false) return 0;
//			Mesh sphereMesh;
//			BuildMesh(sphereMesh);
//			msh.SetFromTri(sphereMesh);
			objectState.obj = polyObj;
		}

		ModContext mc;

		unwrapMod->ModifyObject(0, mc, &objectState, nullptr);
		MeshTopoData* meshTopoData = (MeshTopoData*)mc.localData;
		if (meshTopoData != nullptr)
		{
			print("Starting UV Count " + std::to_string(meshTopoData->GetNumberTVVerts()));
			unwrapMod->fnFlattenMapNoParams();
			print("Ending UV Count " + std::to_string(meshTopoData->GetNumberTVVerts()));
		}
		else {
			print("No data...");
			return 3;
		}


		wstring directoryOut = fileNameOut;
		directoryOut = directoryOut.substr(0, directoryOut.find_last_of(L"\\/"));
		if (CreateDirectory(directoryOut.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
		{
			//copy the UVW data into a mesh or mnmesh and write that back
			if (isMesh)
			{
				Mesh msh;
				msh.setNumVerts(meshTopoData->GetNumberTVVerts());
				for (int i = 0; i < msh.numVerts; i++)
				{
					msh.verts[i] = meshTopoData->GetTVVert(i);
				}
				msh.setNumFaces(meshTopoData->GetNumberFaces());
				for (int i = 0; i < msh.numFaces; i++)
				{
					int degree = 3;
					for (int j = 0; j < degree; j++)
					{
						msh.faces[i].v[j] = meshTopoData->GetFaceTVVert(i, j);
					}
				}
				MapIO mapIO(&msh);
				mapIO.Write(fileNameOut);
			}
			else
			{
				MNMesh msh;
				msh.setNumVerts(meshTopoData->GetNumberTVVerts());
				for (int i = 0; i < msh.numv; i++)
				{
					msh.v[i].p = meshTopoData->GetTVVert(i);
				}
				msh.setNumFaces(meshTopoData->GetNumberFaces());
				for (int i = 0; i < msh.numf; i++)
				{
					if (meshTopoData->GetFaceDead(i))
					{
						msh.f[i].SetFlag(MN_DEAD);
					}
					else
					{
						int degree = meshTopoData->GetFaceDegree(i);
						msh.f[i].SetDeg(degree);
						for (int j = 0; j < degree; j++)
						{
							msh.f[i].vtx[j] = meshTopoData->GetFaceTVVert(i, j);
						}
					}
				}
				MapIO mapIO(&msh);
				mapIO.Write(fileNameOut);
			}

			delete unwrapMod;
			delete objectState.obj;
			delete meshTopoData;
			objectState.obj = nullptr;
			return 0;
		}
		else
		{
			print("Failed to create the output directory: " + GetLastError());
			return 1;
		}
	} 
	else
	{
		print("Wrong number of arguments");
		return 2;
	}
}

