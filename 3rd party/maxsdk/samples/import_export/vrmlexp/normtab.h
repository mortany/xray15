/**********************************************************************
 *<
	FILE: normtab.h

	DESCRIPTION:  Normal Hash Table class defs

	CREATED BY: Scott Morrison

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "maxtextfile.h"

#define NUM_NORMS 10000.0f
#define normNorm(w) ((float) ((int) (NUM_NORMS * (w))))

// Truncate normals into the specified range.
inline 
Point3 NormalizeNorm(Point3 norm)
{
    Point3 p;
    p.x = normNorm(norm.x);
    p.y = normNorm(norm.y);
    p.z = normNorm(norm.z);
    return p;
}

// A normal table hash bucket.
class NormalDesc {
public:
    NormalDesc(Point3& norm) {
        n = NormalizeNorm(norm);
        index = -1;
        next = NULL;
    }
    ~NormalDesc() { delete next; }
    Point3 n;           // The normalize normal
    int    index;       // The index in the index face set
    NormalDesc* next;   // Next hash bucket.
};

// Un-comment this line to get data on the normal hash table
// #define DEBUG_NORM_HASH

// Hash table for rendering normals
class NormalTable 
{
public:
    NormalTable();
    ~NormalTable();

    void AddNormal(Point3& norm);
    int GetIndex(Point3& norm);
    NormalDesc* Get(int i) { return tab[i]; }
    void PrintStats(MaxSDK::Util::TextFile::Writer &mStream);
    
private:
    DWORD HashCode(Point3 &norm);
    Tab<NormalDesc*> tab;
};


#define NORM_TABLE_SIZE 1001
