#pragma once

//this class contains info on the envelope cross sections
class CrossSectionClass
{
public:
	float u;		//this is the U value of the cross section,
					// basically the percent between the start and end point of the envelope

	Control *InnerControl,*OuterControl;  // these are pointer to float controller that control
										  // the inner/outer radius of the cross section
										  // these controllers are also stored in refHandleList
										  // and are just here for convience access.  Deleting
										  // or changing these reference should be done in the refHandleList

	int RefInnerID,RefOuterID;				// these are indices into refHandleList for the inner and outer
											// float controller references
	BOOL innerSelected, outerSelected;		// whether the cross sections are selected
	CrossSectionClass():u(0.0f),InnerControl(NULL),OuterControl(NULL),RefInnerID(0),RefOuterID(0),
						innerSelected(FALSE),outerSelected(FALSE)
	{}
};

//this class contains all our info for the bone
class BoneDataClass
{
public:
	//these are pointer to references for convience sake
	//if you want to delete or change references go through refHandleList
	INode *Node;	//this is the pointer to node for this bone
	Control *EndPoint1Control,*EndPoint2Control; //these are pointer to point3 controllers that are used to
												 //store the end point positions

    Matrix3 tm;				//this is the inverse of the node objects tm at the time the bone is assigned
		// NS Added these, to store the Initial Node TM and the Initial Object TM
	Matrix3 InitObjectTM;	// this is the initial object tm for the bone
	Matrix3 InitNodeTM;		// this is the initial node tm for the bone
    Matrix3 temptm;			// this is the cache tm it is the concatentation of bunch of tms
							// basically multiple this tm times the point gives you the vertex motion for this bone

    Tab<CrossSectionClass> CrossSectionList;	//this is the table of all our cross sections
    BYTE flags;									//flags I dont think this is used
    BYTE FalloffType;							//this the fall off the envelope

	int BoneRefID;							// this is the index into refHandleList for the node

	int RefEndPt1ID,RefEndPt2ID;			// these are indices into refHandleList for the start and end pointer
											// point3 controller references

	BOOL end1Selected, end2Selected;		// whether the end points are selected
	TSTR name;								// the name of the bone for our old bones, not used any more except for older pre Max4 files
	Spline3D referenceSpline;				// if this bone is a spline, this is the copy of the bone when it is assigned
	Matrix3 InitStretchTM;					// this is the initial bone stretch tm, pre 6 this was computed directly in the initial tms
	BOOL bFilterOut;						// whether the bone is filtered out or not in the list box
	BoneDataClass():Node(NULL),EndPoint1Control(NULL),EndPoint2Control(NULL),flags(0),FalloffType(0),
					BoneRefID(0),RefEndPt1ID(0),RefEndPt2ID(0),end1Selected(FALSE),end2Selected(FALSE),bFilterOut(FALSE)
	{}

	// Helper method, sets the current position of a bone node as the initial object tm for the bone.
	// Used for Replace Bone operation
	void InitTM( INode* node, int RefFrame, BOOL hasStretchTM, BOOL isBoneObject )
	{
		if( isBoneObject )
			//get child node
			node = node->GetParentNode();

		Matrix3 otm = node->GetObjectTM(RefFrame);
		Matrix3 stretchTM = node->GetStretchTM(RefFrame);
		Matrix3 ntm = node->GetNodeTM(RefFrame);
		InitStretchTM = stretchTM;
		InitObjectTM = otm;
		tm    = Inverse(otm);
		if (hasStretchTM)
			InitNodeTM = ntm;
		else InitNodeTM = stretchTM * ntm;
	}
};

//this class contains the exclusion data for a bone
//basically we can tell a bone to not influence
//certain vertices
class ExclusionListClass
{
private:
	Tab<int> exList;  //list of vertices that this bone will not influence
public:
	//the number of vertices
	int Count() { return exList.Count();}

	//lets you inspect the vertices that the in the exclusion list
	int Vertex(int id) { if ((id >=0) && (id < exList.Count()))
							return exList[id];
						return 0;
						}

	//Save and load methods
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// returns whether a vertex is in the list or not
	// vertID is which vertex you are looking for
	// where is the location in the array where the vertID is
	BOOL isInList(int vertID, int &where);

	//returns whether a vertex is in the exclusion list
	//vertID the vertex you looking for
	BOOL isExcluded(int vertID);

	//given a tab of verts add them to the exculsion list
	//Tab<int> exList this if vertices you want to add
	void ExcludeVerts(const Tab<int>& exList);

	//given a tab of verts remove them to the exculsion list
	//Tab<int> incList this if vertices you want to remove
	void IncludeVerts(const Tab<int>& incList);

	//replaces our exclusion
	//Tab<int> exclusionList that you want to copy over the existing one
	void SetExclusionVerts(const Tab<int>& exclusionList);
};

//this is our hit data class for the viewport
//since we have multiple type we can hit in the
//viewport this records what gets hit
class BoneHitDataClass:public HitData
{
public:
	BoneHitDataClass(int v, int b, int e, int c, int ch)
		{
        VertexId = v;
        BoneId = b;
        EndPoint = e;
        CrossId = c;
        CrossHandleId = ch;
        }
	~BoneHitDataClass() {}

	int VertexId;		// vertex ID
	int BoneId;			// bone ID
	int EndPoint;		// the start or end point
	int CrossId;		// the cross section ID
	int CrossHandleId;	// which crossection handle
};

