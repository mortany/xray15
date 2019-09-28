#pragma once

#include "mesh.h"
#include "mnmesh.h"


/*

Simple 1 channel mesh/mnmesh file io blob

INT if this is a polymesh format otherwise assume tri faces onlu
INT number of faces
INT number of vertice

ARRAY POINT3
if POLYMESH
{
	ARRAY INT list of poly face degrees
	ARRAY INT list of BOOL where if an entry is 1 the face is dead
}
INT number of face indices
ARRAY INT list of all the face indices


*/
class MapIO
{
public:
	MapIO(Mesh* msh);
	MapIO(MNMesh* msh);

	~MapIO();

	static bool IsPoly(TSTR filename);

	bool Write(TSTR filename);
	bool Read(TSTR filename);
protected:
	Mesh* mMesh;
	MNMesh* mMNMesh;

};
