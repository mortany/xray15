//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////

#include "Util.h"
// rti
#include <rti/math/Math.h>
#include <rti/scene/geometry.h>
// std
#include <vector>

namespace Max
{;
namespace RapidRTTranslator
{;

namespace RRTUtil
{;

void CreateRapidPlane(const rti::GeometryHandle &hGeometry, float width, float height)
{
	rti::TEditPtr<rti::Geometry> pGeometry(hGeometry);
		
	width  *= 0.5f;
	height *= 0.5f;
	
	int indexes[] = {0,1,2,1,3,2};
	rti::Vec3f	vertices[] = {
		rti::Vec3f(-width, height,  0.0f),
		rti::Vec3f(width,  height,  0.0f),
		rti::Vec3f(-width, -height, 0.0f),
		rti::Vec3f(width,  -height, 0.0f)};
	rti::Vec3f	normals[]  = {
		rti::Vec3f(0.0f, 0.0f, -1.0f),
		rti::Vec3f(0.0f, 0.0f, -1.0f),
		rti::Vec3f(0.0f, 0.0f, -1.0f),
		rti::Vec3f(0.0f, 0.0f, -1.0f)};
	
	RRTUtil::check_rti_result(pGeometry->init(rti::GEOM_TRIANGLE_MESH));
	RRTUtil::check_rti_result(pGeometry->setAttribute1iv(rti::GEOM_ATTRIB_INDEX_ARRAY,  6, indexes));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_COORD_ARRAY,  4, vertices));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_NORMAL_ARRAY, 4, normals));
}
	
void CreateRapidSphere(const rti::GeometryHandle &hGeometry, float radius)
{
	rti::TEditPtr<rti::Geometry> pGeometry(hGeometry);
		
	static const int s_NumPrimitives = 1024;
	
	int ny = (int)sqrtf(float(s_NumPrimitives) * 0.25f);
	int nx = (s_NumPrimitives / 2) / ny;
	
	int nNumPrims = 2 * (nx-1) * (ny-1);
	int nNumVerts = nx * ny;
	
	std::vector<int>		vIndices;
	std::vector<rti::Vec3f> vCoords;
	std::vector<rti::Vec3f> vNormals;
		
	vIndices.reserve(nNumPrims * 3);
	vCoords.reserve(nNumVerts);
	vNormals.reserve(nNumVerts);
		
	std::vector<float> vSinX;
	std::vector<float> vCosX;
	std::vector<float> vSinY;
	std::vector<float> vCosY;
	
	vSinX.resize(nx);
	vCosX.resize(nx);
	vSinY.resize(ny);
	vCosY.resize(ny);
	for (int x = 0; x < nx; ++x) {
		const float fAngle = rti::RTI_TWO_PI * (float(x) / (nx-1));
		vSinX[x] = sinf(fAngle);
		vCosX[x] = cosf(fAngle);
	}
	vSinX[nx-1] = vSinX[0];
	vCosX[nx-1] = vCosX[0];
	
	for (int y = 0; y < ny; ++y) {
		const float fAngle = rti::RTI_PI * (float(y) / (ny-1));
		vSinY[y] = sinf(fAngle);
		vCosY[y] = cosf(fAngle);
	}
	
	for (int y = 0; y < ny; ++y) {
		for (int x = 0; x < nx; ++x) {
			rti::Vec3f fvVert(radius *vCosX[x] * vSinY[y],
						radius * vCosY[y],
						radius * vSinX[x] * vSinY[y]);
	
			vCoords.push_back(fvVert);
			vNormals.push_back(fvVert / radius);
		}
	}
	
		for (int y = 0; y < ny-1; ++y) {
		for (int x = 0; x < nx - 1; ++x){
			int v0 = x + y * nx;
			int v1 = (x+1) + y * nx;
			int v2 = x + (y+1) * nx;
			int v3 = (x+1) + (y+1) * nx;
	
			vIndices.push_back(v0);
			vIndices.push_back(v1);
			vIndices.push_back(v2);
			vIndices.push_back(v1);
			vIndices.push_back(v3);
			vIndices.push_back(v2);
		}
		}
	
	RRTUtil::check_rti_result(pGeometry->init(rti::GEOM_TRIANGLE_MESH));
	RRTUtil::check_rti_result(pGeometry->setAttribute1iv(rti::GEOM_ATTRIB_INDEX_ARRAY, nNumPrims * 3, &vIndices[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_COORD_ARRAY, nNumVerts, &vCoords[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_NORMAL_ARRAY, nNumVerts, &vNormals[0]));
}
	
void CreateRapidDisc(const rti::GeometryHandle &hGeometry, float diameter)
{
	rti::TEditPtr<rti::Geometry> pGeometry(hGeometry);
	
	static const int s_NumPrimitives = 1024;
	
	int ny = (int)powf(float(s_NumPrimitives), 0.25f);
	int nx = (s_NumPrimitives / 2) / ny;
	
	int nNumPrims = 2 * (nx-1) * (ny-1);
	int nNumVerts = nx * ny;
	
	std::vector<int>   vIndices;
	std::vector<rti::Vec3f> vCoords;
	std::vector<rti::Vec3f> vNormals;
	std::vector<rti::Vec2f> vUVs;
	std::vector<rti::Vec3f> vTangentsU;
	std::vector<rti::Vec3f> vTangentsV;
	
	vIndices.reserve(nNumPrims * 3);
	vCoords.reserve(nNumVerts);
	vNormals.reserve(nNumVerts);
	vUVs.reserve(nNumVerts);
	vTangentsU.reserve(nNumVerts);
	vTangentsV.reserve(nNumVerts);
	
	std::vector<float> vSin;
	std::vector<float> vCos;
	  
	vSin.resize(nx);
	vCos.resize(nx);
	
	for (int x = 0; x < nx; ++x) {
	const float fAngle = rti::RTI_TWO_PI * (float(x) / (nx-1));
	vSin[x] = sinf(fAngle);
	vCos[x] = cosf(fAngle);
	}
	vSin[nx-1] = vSin[0];
	vCos[nx-1] = vCos[0];
	
	for (int y = 0; y < ny; ++y) {
		float fRad = float(y) / (ny-1);
		for (int x = 0; x < nx; ++x) {
			rti::Vec3f fvVert(diameter * 0.5f * fRad * vCos[x], diameter * 0.5f * fRad * vSin[x], 0.0f);
	
			vCoords.push_back(fvVert);
			vNormals.push_back(rti::Vec3f(0.0f, 0.0f, -1.0f));
		}
	}
	
	for (int y = 0; y < ny-1; ++y) {
		for (int x = 0; x < nx-1; ++x) {
			int v0 = x + y * nx;
			int v1 = (x+1) + y * nx;
			int v2 = x + (y+1) * nx;
			int v3 = (x+1) + (y+1) * nx;
	
			vIndices.push_back(v0);
			vIndices.push_back(v1);
			vIndices.push_back(v2);
			vIndices.push_back(v1);
			vIndices.push_back(v3);
			vIndices.push_back(v2);
		}
	}
	
	RRTUtil::check_rti_result(pGeometry->init(rti::GEOM_TRIANGLE_MESH));
	RRTUtil::check_rti_result(pGeometry->setAttribute1iv(rti::GEOM_ATTRIB_INDEX_ARRAY, nNumPrims * 3, &vIndices[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_COORD_ARRAY, nNumVerts, &vCoords[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_NORMAL_ARRAY, nNumVerts, &vNormals[0]));
}
	
//Non-capped horiontal cylinder
void CreateRapidCylinder(const rti::GeometryHandle &hGeometry, float diameter, float height)
{
	rti::TEditPtr<rti::Geometry> pGeometry(hGeometry);
	
	static const int s_NumPrimitives = 1024;
	
	int nSurfaceSegs  = s_NumPrimitives / 4;
	int nSurfacePrims = nSurfaceSegs * 2;
	int nCapPrims     = nSurfaceSegs;
	int nSurfaceVerts = (nSurfaceSegs + 1) * 2;
	int nCapVerts     = nCapPrims + 2;
	
	int nNumPrims = nSurfacePrims + nCapPrims * 2;
	int nNumVerts = nSurfaceVerts + nCapVerts * 2;
	
	std::vector<int>   vIndices;
	std::vector<rti::Vec3f> vCoords;
	std::vector<rti::Vec3f> vNormals;
	std::vector<rti::Vec2f> vUVs;
	std::vector<rti::Vec3f> vTangentsU;
	std::vector<rti::Vec3f> vTangentsV;
	
	vIndices.reserve(nNumPrims * 3);
	vCoords.reserve(nNumVerts);
	vNormals.reserve(nNumVerts);
	  
	std::vector<float> vSin;
	std::vector<float> vCos;
	
	vSin.resize(nSurfaceSegs + 1);
	vCos.resize(nSurfaceSegs + 1);
	for (int x = 0; x < nSurfaceSegs; ++x) {
	const float fAngle = rti::RTI_TWO_PI * (float(x) / nSurfaceSegs);
	vSin[x] = sinf(fAngle);
	vCos[x] = cosf(fAngle);
	}
	vSin[nSurfaceSegs] = vSin[0];
	vCos[nSurfaceSegs] = vCos[0];
	
	// main surface
	for (int x = 0; x < nSurfaceSegs + 1; ++x) {
	rti::Vec3f fvVert0(0.5f * diameter * vCos[x], -0.5f * height, -0.5f * diameter * vSin[x]);
	
	vCoords.push_back(fvVert0);
	vNormals.push_back(rti::Vec3f(2.0f * fvVert0[0]/diameter, 0.0f, 2.0f * fvVert0[2]/diameter));
	    
	rti::Vec3f fvVert1(0.5f * diameter * vCos[x], 0.5f * height, -0.5f * diameter * vSin[x]);
	
	vCoords.push_back(fvVert1);
	vNormals.push_back(rti::Vec3f(2.0f * fvVert1[0]/diameter, 0.0f, 2.0f * fvVert1[2]/diameter));
	}
	
	for (int x = 0; x < nSurfaceSegs; ++x) {
	int v0 = x * 2;
	int v1 = v0 + 1;
	int v2 = v0 + 2;
	int v3 = v0 + 3;
	
	vIndices.push_back(v0);
	vIndices.push_back(v3);
	vIndices.push_back(v1);
	vIndices.push_back(v3);
	vIndices.push_back(v0);
	vIndices.push_back(v2);  
	}
	
	// top cap
	{
	int nOffset = int(vCoords.size());
	vCoords.push_back(rti::Vec3f(0.0f, 0.5f * height, 0.0f));
	vNormals.push_back(rti::Vec3f(0.0f, 1.0f, 0.0f));
	
	for (int x = 0; x < nSurfaceSegs + 1; ++x) {
	    rti::Vec3f fvVert1(0.5f * vCos[x] * diameter, 0.5f * height, -0.5f * vSin[x] * diameter);
	
	    vCoords.push_back(fvVert1);
	    vNormals.push_back(rti::Vec3f(0.0f, 1.0f, 0.0f));
	}
	
	for (int x = 0; x < nSurfaceSegs; ++x) {
	    int v0 = nOffset;
	    int v1 = nOffset + x + 1;
	    int v2 = nOffset + x + 2;
	
	    vIndices.push_back(v0);
	    vIndices.push_back(v1);
	    vIndices.push_back(v2);
	}
	}
	  
	// bottom cap
	{
	int nOffset = int(vCoords.size());
	vCoords.push_back(rti::Vec3f(0.0f, -0.5f * height, 0.0f));
	vNormals.push_back(rti::Vec3f(0.0f, -1.0f, 0.0f));
	    
	for (int x = 0; x < nSurfaceSegs + 1; ++x) {
	    rti::Vec3f fvVert1(0.5f * vCos[x] * diameter, -0.5f * height, -0.5f * vSin[x] * diameter);
	
	    vCoords.push_back(fvVert1);
	    vNormals.push_back(rti::Vec3f(0.0f, -1.0f, 0.0f));
	}
	
	for (int x = 0; x < nSurfaceSegs; ++x) {
	    int v0 = nOffset;
	    int v1 = nOffset + x + 1;
	    int v2 = nOffset + x + 2;
	
	    vIndices.push_back(v0);
	    vIndices.push_back(v1);
	    vIndices.push_back(v2);
	}
	}
	
	RRTUtil::check_rti_result(pGeometry->init(rti::GEOM_TRIANGLE_MESH));
	RRTUtil::check_rti_result(pGeometry->setAttribute1iv(rti::GEOM_ATTRIB_INDEX_ARRAY, nNumPrims * 3, &vIndices[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_COORD_ARRAY, nNumVerts, &vCoords[0]));
	RRTUtil::check_rti_result(pGeometry->setAttribute3fv(rti::GEOM_ATTRIB_NORMAL_ARRAY, nNumVerts, &vNormals[0]));
}
	
size_t GenerateHashID(const TCHAR* s)
{
	M_STD_STRING str(s);
	
	//SDBM Hash    function
	size_t hash = 0;
	for(std::size_t i = 0; i < str.length(); i++)
	{
	    hash = str[i] + (hash << 6) + (hash << 16) - hash;
	}
	
	return hash;
}

float glossToRough(float gloss)
{
	if (gloss == 1.0f)
		return 0.0f;
	gloss = gloss < 0.0f ? 0.0f : (gloss > 1.0f ? 1.0f : gloss);
	gloss = 1.0f - gloss;
	if(gloss < 0.5f) {
		return 1.11078f * gloss * gloss - 0.225494f * gloss + 0.0505339f;
	} else if(gloss < 0.9f) {
		return 24.1745f * gloss * gloss * gloss - 45.6128f * gloss * gloss + 28.605f * gloss - 5.72202f;
	} else {
		return 0.7f + 3.0f * (gloss - 0.9f);
	}
}


int Time_HMS::get_total_seconds() const
{
    return seconds + (minutes * 60) + (hours * 3600);
}

Time_HMS Time_HMS::from_seconds(const unsigned int input_seconds)
 {
    Time_HMS hms;

    hms.seconds = input_seconds;
    hms.hours = hms.seconds / 3600;
    hms.seconds = hms.seconds % 3600;
    hms.minutes = hms.seconds / 60;
    hms.seconds = hms.seconds % 60;

    return hms;
}

};

}}