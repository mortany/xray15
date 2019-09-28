#include "MapIO.h"
#include <stdio.h>
#include <containers\Array.h>

MapIO::MapIO(Mesh* msh) : mMesh(msh), mMNMesh(nullptr)
{
}
MapIO::MapIO(MNMesh* msh) : mMesh(nullptr), mMNMesh(msh)
{
}

MapIO::~MapIO()
{
	mMesh = nullptr;
	mMNMesh = nullptr;
}

/*
Simple mesh/mnmesh format

BOOL if PolyMesh
int geom count
Point3 Array Geom
int face count
int array degree count 
int array face indices poly only
BitArray deadFaces poly only

*/


bool MapIO::Write(TSTR filename)
{
	BOOL isPolyMesh = TRUE;
		
	int faceCount = 0;
	int indexCount = 0;
	int geomVertCount = 0;
	int geomEdgeCount = 0;
//	int UVWVertCount = 0;

	MaxSDK::Array<Point3> geomVerts;
	MaxSDK::Array<MNEdge> geomEdges;
	MaxSDK::Array<int> geomFaceDegree;
	MaxSDK::Array<int> geomFaceIndices;

/*	MaxSDK::Array<Point3> uvwVerts;
	MaxSDK::Array<int> uvwFaceIndices;
*/
	MaxSDK::Array<int> deadFaces;

	if (mMesh)
	{
		isPolyMesh = FALSE;
		faceCount = mMesh->numFaces;

		if (faceCount == 0) return false;

		geomVertCount = mMesh->numVerts;
		geomVerts.setLengthUsed(geomVertCount);
		for (int i = 0; i < geomVertCount; i++)
		{
			geomVerts[i] = mMesh->verts[i];
		}

		int index = 0;
		geomFaceIndices.setLengthUsed(faceCount*3);
		for (int i = 0; i < faceCount; i++)
		{
			int degree = 3;
			for (int j = 0; j < degree; j++)
			{
				geomFaceIndices[index++] = mMesh->faces[i].v[j];
			}			
		}

		/*
		if (!mMesh->mapSupport(mMapChannel))
			return false;

		UVWVertCount = mMesh->getNumMapVerts(mMapChannel);
		uvwVerts.setLengthUsed(UVWVertCount);
		for (int i = 0; i < UVWVertCount; i++)
		{
			uvwVerts[i] = mMesh->mapVerts(mMapChannel)[i];
		}

		index = 0;
		uvwFaceIndices.setLengthUsed(faceCount * 3);
		for (int i = 0; i < faceCount; i++)
		{
			int degree = 3;
			for (int j = 0; j < degree; j++)
			{
				uvwFaceIndices[index++] = mMesh->mapFaces(mMapChannel)[i].t[j];
			}
		}
		*/

	}
	else if (mMNMesh)
	{
		isPolyMesh = TRUE;
		faceCount = mMNMesh->numf;

		if (faceCount == 0) return false;

		geomVertCount = mMNMesh->numv;
		geomVerts.setLengthUsed(geomVertCount);
		for (int i = 0; i < geomVertCount; i++)
		{
			geomVerts[i] = mMNMesh->v[i].p;
		}

		geomFaceDegree.setLengthUsed(faceCount);
		deadFaces.setLengthUsed(faceCount);
		
		
		geomFaceIndices.reserve(faceCount * 4);
		for (int i = 0; i < faceCount; i++)
		{
			int degree = mMNMesh->f[i].deg;
			if (mMNMesh->f[i].GetFlag(MN_DEAD))
			{
				deadFaces[i] = TRUE;
				degree = 0;
				geomFaceDegree[i] = degree;
				continue;
			}
			deadFaces[i] = FALSE;
			geomFaceDegree[i] = degree;
			for (int j = 0; j < degree; j++)
			{
				int faceIndex = mMNMesh->f[i].vtx[j];
				geomFaceIndices.append(faceIndex);
			}
		}

		geomEdgeCount = mMNMesh->nume;

		geomEdges.setLengthUsed(geomEdgeCount);
		for (int i = 0; i < geomEdgeCount; i++)
		{
			geomEdges[i] = mMNMesh->e[i];
		}

		/*
		int index = 0;
		if (mMNMesh->M(mMapChannel) == nullptr)
			return false;

		MNMapFace *tvFace = mMNMesh->M(mMapChannel)->f;
		Point3 *tvVert = mMNMesh->M(mMapChannel)->v;
		if (tvVert == nullptr) return false;
		if (tvFace == nullptr) return false;

		UVWVertCount = mMNMesh->M(mMapChannel)->numv;
		uvwVerts.setLengthUsed(UVWVertCount);
		for (int i = 0; i < UVWVertCount; i++)
		{
			uvwVerts[i] = tvVert[i];
		}

		index = 0;
		uvwFaceIndices.setLengthUsed(geomFaceIndices.lengthUsed());
		for (int i = 0; i < faceCount; i++)
		{
			if (deadFaces[i]) continue;

			int degree = geomFaceDegree[i];
			for (int j = 0; j < degree; j++)
			{
				uvwFaceIndices[index++] = tvFace[i].tv[j];
			}
		}
		*/
	}

	FILE *stream;
	fopen_s(&stream, filename.ToCStr(), "wb");

	if (stream == nullptr) {
		return false;
	}

	fwrite(&isPolyMesh, sizeof(int), 1, stream);
	fwrite(&faceCount, sizeof(int), 1, stream);
	fwrite(&geomVertCount, sizeof(int), 1, stream);
//	fwrite(&UVWVertCount, sizeof(int), 1, stream);

	fwrite(geomVerts.asArrayPtr(), sizeof(Point3), geomVerts.lengthUsed(), stream);

	if (isPolyMesh)
	{
		fwrite(geomFaceDegree.asArrayPtr(), sizeof(int), geomFaceDegree.lengthUsed(), stream);
		fwrite(deadFaces.asArrayPtr(), sizeof(int), deadFaces.lengthUsed(), stream);
		fwrite(&geomEdgeCount, sizeof(int), 1, stream);
		fwrite(geomEdges.asArrayPtr(), sizeof(MNEdge), geomEdgeCount, stream);
	}

	indexCount = (int)geomFaceIndices.lengthUsed();
	fwrite(&indexCount, sizeof(int), 1, stream);

	fwrite(geomFaceIndices.asArrayPtr(), sizeof(int), geomFaceIndices.lengthUsed(), stream);

	/*
	fwrite(uvwVerts.asArrayPtr(), sizeof(Point3), uvwVerts.lengthUsed(), stream);
	fwrite(uvwFaceIndices.asArrayPtr(), sizeof(int), uvwFaceIndices.lengthUsed(), stream);
	*/
	fclose(stream);

	return true;

}

bool MapIO::IsPoly(TSTR filename)
{
	FILE *stream;
	fopen_s(&stream, filename.ToCStr(), "rb");

	if (stream == nullptr) return false;


	BOOL isPolyMesh = TRUE;

	fread(&isPolyMesh, sizeof(int), 1, stream);
	fclose(stream);

	if (isPolyMesh)
		return true;
	return false;
}

bool MapIO::Read(TSTR filename)
{
	FILE *stream;
	fopen_s(&stream, filename.ToCStr(), "rb");

	if (stream == nullptr) return false;


	BOOL isPolyMesh = TRUE;

	int faceCount = 0;
	int geomVertCount = 0;
	int geomEdgeCount = 0;
//	int UVWVertCount = 0;

	MaxSDK::Array<Point3> geomVerts;
	MaxSDK::Array<int> geomFaceDegree;
	MaxSDK::Array<int> geomFaceIndices;
	MaxSDK::Array<MNEdge> geomEdges;

//	MaxSDK::Array<Point3> uvwVerts;
//	MaxSDK::Array<int> uvwFaceIndices;
	MaxSDK::Array<int> deadFaces;


	
	fread(&isPolyMesh, sizeof(int), 1, stream);
	fread(&faceCount, sizeof(int), 1, stream);
	fread(&geomVertCount, sizeof(int), 1, stream);
//	fread(&UVWVertCount, sizeof(int), 1, stream);

	geomVerts.setLengthUsed(geomVertCount);
	fread(geomVerts.asArrayPtr(), sizeof(Point3), geomVerts.lengthUsed(), stream);

	if (isPolyMesh)
	{
		geomFaceDegree.setLengthUsed(faceCount);
		deadFaces.setLengthUsed(faceCount);
		fread(geomFaceDegree.asArrayPtr(), sizeof(int), geomFaceDegree.lengthUsed(), stream);
		fread(deadFaces.asArrayPtr(), sizeof(int), deadFaces.lengthUsed(), stream);
		fread(&geomEdgeCount, sizeof(int), 1, stream);
		geomEdges.setLengthUsed(geomEdgeCount);
		fread(geomEdges.asArrayPtr(), sizeof(MNEdge), geomEdgeCount, stream);
	}

	int indexCount = 0;
	fread(&indexCount, sizeof(int), 1, stream);

	geomFaceIndices.setLengthUsed(indexCount);
	fread(geomFaceIndices.asArrayPtr(), sizeof(int), geomFaceIndices.lengthUsed(), stream);
/*
	uvwVerts.setLengthUsed(UVWVertCount);
	uvwFaceIndices.setLengthUsed(indexCount);
	fread(uvwVerts.asArrayPtr(), sizeof(Point3), uvwVerts.lengthUsed(), stream);
	fread(uvwFaceIndices.asArrayPtr(), sizeof(int), uvwFaceIndices.lengthUsed(), stream);
*/
	fclose(stream);

	if (mMesh)
	{
		mMesh->setNumFaces(faceCount);

		mMesh->setNumVerts(geomVertCount);
		for (int i = 0; i < geomVertCount; i++)
			mMesh->verts[i] = geomVerts[i];

		int index = 0;
		for (int i = 0; i < faceCount; i++)
		{
			int degree = 3;
			for (int j = 0; j < degree; j++)
			{
				if (index >= geomFaceIndices.lengthUsed()) return false;
				mMesh->faces[i].v[j] = geomFaceIndices[index++];
			}
		}

		/*
		mMesh->setNumMaps(mMapChannel + 1, 1);
		mMesh->setMapSupport(mMapChannel, 1);
		MeshMap& meshMap = mMesh->Map(mMapChannel);
		meshMap.setNumFaces(mMesh->numFaces);
		meshMap.setNumVerts(UVWVertCount);
		int index = 0;
		for (int i = 0; i < mMesh->numFaces; i++)
		{
			int degree = 3;
			for (int j = 0; j < degree; j++)
			{
				if (index >= uvwFaceIndices.length())
					return false;
				meshMap.tf[i].t[j] = uvwFaceIndices[index++];
			}
		}
		for (int i = 0; i < meshMap.getNumVerts(); i++)
		{
			meshMap.tv[i] = uvwVerts[i];
		}
		*/

	}
	else if(mMNMesh)
	{
		mMNMesh->setNumFaces(faceCount);

		mMNMesh->setNumVerts(geomVertCount);
		for (int i = 0; i < geomVertCount; i++)
		{
			mMNMesh->v[i].p = geomVerts[i];
		}

		int index = 0;
		for (int i = 0; i < faceCount; i++)
		{
			mMNMesh->f[i].SetFlag(MN_DEAD, (deadFaces[i] == TRUE));
			if (mMNMesh->f[i].GetFlag(MN_DEAD)) continue;

			int degree = geomFaceDegree[i];
			mMNMesh->f[i].SetDeg(degree);
			
			for (int j = 0; j < degree; j++)
			{
				if (index >= geomFaceIndices.lengthUsed()) {
					return false;
				}
				mMNMesh->f[i].vtx[j] = geomFaceIndices[index++];
			}
		}

		mMNMesh->setNumEdges(geomEdgeCount);
		for (int i = 0; i < geomEdgeCount; i++)
		{
			mMNMesh->e[i] = geomEdges[i];
		}

		/*
		mMNMesh->SetMapNum(mMapChannel + 1);
		mMNMesh->InitMap(mMapChannel);
		MNMap* meshMap = mMNMesh->M(mMapChannel);
		meshMap->setNumFaces(mMNMesh->numf);
		meshMap->setNumVerts(UVWVertCount);
		int index = 0;
		for (int i = 0; i < mMNMesh->numf; i++)
		{
			if (deadFaces[i]) continue;
			int degree = geomFaceDegree[i];
			meshMap->f[i].SetSize(degree);
			for (int j = 0; j < degree; j++)
			{
				if (index >= uvwFaceIndices.length())
					return false;
				meshMap->f[i].tv[j] = uvwFaceIndices[index++];
			}
		}
		for (int i = 0; i <meshMap->numv; i++)
		{
			meshMap->v[i] = uvwVerts[i];
		}
		*/
	}

	return true;

}