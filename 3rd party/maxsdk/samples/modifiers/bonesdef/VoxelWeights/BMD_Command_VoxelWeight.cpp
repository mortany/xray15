

// Copyright (c) 2015 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.


#include <omp.h>
#pragma warning( push )
#pragma warning( disable: 4265 )
#include <thread>
#pragma warning( pop )

#include "geombind.h"
#include "..\BMD_Command.h"
#include "..\BonesDef.h"

#include "systemutilities.h"
#include "macrorec.h"
#include <WindowsMessageFilter.h>
#include <maxapi.h>

/* this is silly OGL setup glew cannot startup with out a context
so you need to create fake OGL window first
*/
#define SIMPLE_OPENGL_CLASS_NAME L"Simple_openGL_class"


/*-----------------------------------------------

Name:	MsgHandlerSimpleOpenGLClass

Params:	windows messages stuff

Result:	Handles messages from windows that use
		simple OpenGL class.

/*---------------------------------------------*/



LRESULT CALLBACK MsgHandlerSimpleOpenGLClass(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	switch(uiMsg)
	{
		case WM_PAINT:									
			BeginPaint(hWnd, &ps);							
			EndPaint(hWnd, &ps);					
			break;
		default:
			return DefWindowProc(hWnd, uiMsg, wParam, lParam); // Default window procedure
	}
	return 0;
}

static bool bClassRegistered = false;

void RegisterSimpleOpenGLClass(HINSTANCE hInstance)
{

	if(bClassRegistered)return;
	bClassRegistered = true;

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style =  CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC)MsgHandlerSimpleOpenGLClass;
	wc.cbClsExtra = 0; wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_MENUBAR+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = SIMPLE_OPENGL_CLASS_NAME;

	RegisterClassEx(&wc);

}


DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}




/*********************************************

This can only be updated from the main thread 
otherwise you can crash beware multithreading

**********************************************/
class MaxProgressImpl : public MaxSDK::GeomBind::IProgress
{
	

public:
	MaxProgressImpl()
		
	{
		mCancelOp = false;
		mMainThread = std::this_thread::get_id();
		mLastpct = -1;
		mHasUI = false;
	}


	inline virtual MaxProgressImpl & start(const char * description, int max, int value )
	{
		WCHAR outStr[1024];
		mbstowcs(outStr,description,1024);
		title.printf(_M("%s"),outStr);
		LPVOID  arg = 0;
		GetCOREInterface()->ReplacePrompt(title);
		mDotCount = 0;
		return setMaxValue( max, value );
	}

	inline virtual MaxProgressImpl & end()
	{
		TSTR done(_M(" "));
		GetCOREInterface()->ReplacePrompt(done);		
		return *this;
	}

	inline virtual MaxProgressImpl & setMaxValue( int, int )
	{
		return *this;
	}

	virtual bool setValue( const int pct)
	{		
		if (mMainThread == std::this_thread::get_id())
		{
		

			if (pct != mLastpct)
			{
				QueryPerformanceCounter(&mT2);

				// compute and print the elapsed time in millisec
				mElapsedTime = (mT2.QuadPart - mT1.QuadPart) * 1000.0 / mFrequency.QuadPart;

				//update the status every half second
				if (mElapsedTime > 500)
				{
					mT1 = mT2;
					TSTR msg;		
					TSTR dotString;
					mDotCount ++;
					if (mDotCount > 300)
						mDotCount = 0;
					for (int j = 0; j < mDotCount; j++)
						dotString.append(_M("."));
					msg.printf(_M("%s%s "),title,dotString);

					GetCOREInterface()->ReplacePrompt(msg);
				}
			}
			mLastpct = pct;

			CheckWindowMessages();	
		}
		
		return false;
	}

	void        startOperation()
	{		
		// get ticks per second
		QueryPerformanceFrequency(&mFrequency);
	}
	virtual bool        cancelOperation()
	{
		return mCancelOp;
	}
	void        endOperation()
	{
	}

	void CheckWindowMessages()
	{
		if ((mCancelOp == false) && mHasUI)
		{
//			_messageFilter.RunNonBlockingMessageLoop();
			if(_messageFilter.Aborted())
			{
				mCancelOp = true;
			}
		}	

	}

	TSTR title;
	bool mCancelOp;
	std::thread::id mMainThread;
	int mLastpct;	
	int mDotCount;

	LARGE_INTEGER mFrequency;        // ticks per second
	LARGE_INTEGER mT1, mT2;           // ticks
	double mElapsedTime;
	bool mHasUI;
	MaxSDK::WindowsMessageFilter _messageFilter;

};



BMD_Command_VoxelWeight::BMD_Command_VoxelWeight()
{
	mObjTm.IdentityMatrix();
	mFalloff = 0.0f;
	mMaxInfluence = -1;
	mVoxelSize = 256;
	mType = 0;
	mUseWinding = false;
}

BMD_Command_VoxelWeight::~BMD_Command_VoxelWeight()
{

}



//we are only interested in the translation so only that is filled out, the rest is identity
void BMD_Command_VoxelWeight::ToOpenGLMatrix(Matrix3& mat, double* m)
{
	Point3 p;
	Point3 trans = mat.GetRow(3);

	for(int i = 0; i < 16; i++)
	{
		m[i] = 0;
	}
	
	m[3] = trans.x;
	m[7] = trans.y;
	m[11] = trans.z;

	m[0] = 1.0;
	m[5] = 1.0;
	m[10] = 1.0;
	m[15] = 1.0;

	
}


Tab<int> BMD_Command_VoxelWeight::ProcessHeatMapMesh(BoneModData *bmd, MaxSDK::GeomBind::IMesh *mesh, Matrix3 objTm, bool flipMesh)
{
	BitArray vSelection(bmd->selected);
	if (vSelection.NumberSet()==0)
		vSelection.SetAll();
	//convert all mesh to tri meshes and keep a remapping back to the original
	Tab<int> bindMeshIndexToOriginalVerts;
	Tab<int> bindMeshIndexToOriginalFaces;
	Mesh triMesh;
	Tab<Point3> verts;
	Tab<int> faces;
	Tab<int> originalFaces;
	int currentIndex = 0;
	if (bmd->isMesh && bmd->mesh)
	{
		//copy our vertices over
		for (int j = 0; j < bmd->mesh->numVerts; j++)
		{
			Point3 v = bmd->mesh->verts[j];
			verts.Append(1,&v,5000);
		}
		//copy our faces over
		for (int j = 0; j < bmd->mesh->numFaces; j++)
		{
			int a = bmd->mesh->faces[j].v[0];
			int b = bmd->mesh->faces[j].v[1];
			int c = bmd->mesh->faces[j].v[2];

			if (flipMesh)
			{
				originalFaces.Append(1,&c,5000);
				originalFaces.Append(1,&b,5000);
				originalFaces.Append(1,&a,5000);
			}
			else
			{
				originalFaces.Append(1,&a,5000);
				originalFaces.Append(1,&b,5000);
				originalFaces.Append(1,&c,5000);
			}

			if (vSelection[a] || vSelection[b] || vSelection[c])
			{
				bindMeshIndexToOriginalFaces.Append(1,&j,5000);
				if (flipMesh)
				{
					faces.Append(1,&c,5000);
					faces.Append(1,&b,5000);
					faces.Append(1,&a,5000);
				}
				else
				{
					faces.Append(1,&a,5000);
					faces.Append(1,&b,5000);
					faces.Append(1,&c,5000);
				}
			}
		}
	}
	else if (bmd->mnMesh)
	{
		//copy our vertices over
		for (int j = 0; j < bmd->mnMesh->numv; j++)
		{
			Point3 v = bmd->mnMesh->v[j].p;
			verts.Append(1,&v,5000);
		}
		//copy our faces over
		Tab<int> faceDiag;
		for (int j = 0; j < bmd->mnMesh->numf; j++)
		{
			if (bmd->mnMesh->f[j].GetFlag(MN_DEAD)) 
				continue;
			int faceCount = bmd->mnMesh->f[j].deg -2;
			bmd->mnMesh->f[j].GetTriangles(faceDiag);
			int* faceDiagPtr = faceDiag.Addr(0);
			for (int k = 0; k < faceCount; k++)
			{
				int a = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];
				int b = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];
				int c = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];

				if (flipMesh)
				{
					originalFaces.Append(1,&c,5000);
					originalFaces.Append(1,&b,5000);
					originalFaces.Append(1,&a,5000);
				}
				else
				{
					originalFaces.Append(1,&a,5000);
					originalFaces.Append(1,&b,5000);
					originalFaces.Append(1,&c,5000);
				}

				if (vSelection[a] || vSelection[b] || vSelection[c])
				{
					int ofaceIndex = originalFaces.Count()/3-1;
					bindMeshIndexToOriginalFaces.Append(1,&ofaceIndex,5000);
					if (flipMesh)
					{
						faces.Append(1,&c,5000);
						faces.Append(1,&b,5000);
						faces.Append(1,&a,5000);
					}
					else
					{
						faces.Append(1,&a,5000);
						faces.Append(1,&b,5000);
						faces.Append(1,&c,5000);
					}
				}					
			}

		}
	}
	else if (bmd->patch)
	{
		Mesh msh;
		int holdSteps = bmd->patch->GetMeshSteps();
		bmd->patch->SetMeshSteps(2);
		bmd->patch->ComputeMesh(msh,0);
		bmd->patch->SetMeshSteps(holdSteps);
		//copy our vertices over
		for (int j = 0; j < msh.numVerts; j++)
		{
			Point3 v = msh.verts[j];
			verts.Append(1,&v,5000);
		}
		//copy our faces over
		for (int j = 0; j < msh.numFaces; j++)
		{
			int a = msh.faces[j].v[0];
			int b = msh.faces[j].v[1];
			int c = msh.faces[j].v[2];

			if (flipMesh)
			{
				originalFaces.Append(1,&c,5000);
				originalFaces.Append(1,&b,5000);
				originalFaces.Append(1,&a,5000);
			}
			else
			{
				originalFaces.Append(1,&a,5000);
				originalFaces.Append(1,&b,5000);
				originalFaces.Append(1,&c,5000);
			}

			if (vSelection[a] || vSelection[b] || vSelection[c])
			{
				bindMeshIndexToOriginalFaces.Append(1,&j,5000);
				if (flipMesh)
				{
					faces.Append(1,&c,5000);
					faces.Append(1,&b,5000);
					faces.Append(1,&a,5000);
				}
				else
				{
					faces.Append(1,&a,5000);
					faces.Append(1,&b,5000);
					faces.Append(1,&c,5000);
				}

			}
		}
	}

	//convert to a tri mesh
	triMesh.setNumVerts(verts.Count());
	triMesh.setNumFaces(faces.Count()/3);
	for (int i = 0; i < verts.Count(); i++)
		triMesh.verts[i] = verts[i];
	int index = 0;
	for (int i = 0; i < faces.Count()/3; i++)
	{
		triMesh.faces[i].v[0] = faces[index++];
		triMesh.faces[i].v[1] = faces[index++];
		triMesh.faces[i].v[2] = faces[index++];
	}

	triMesh.DeleteIsoVerts();
	//convert the mesh to mnmesh to clean up mesh
	MNMesh mnMesh(triMesh);

	//convert the mnmesh to the bind mesh format and also find our vertex remapping indices
	//copy our vertices over
	for (int j = 0; j < mnMesh.numv; j++)
	{
		Point3 v = mnMesh.v[j].p *objTm;
		mesh->appendVertex(v.x,v.y,v.z);

	}

	//copy faces over and keep track of remapped vert indices
	Tab<int> faceDiag;
	bindMeshIndexToOriginalVerts.SetCount(mnMesh.numv);
	
	for (int j = 0; j < mnMesh.numv; j++)
	{
		bindMeshIndexToOriginalVerts[j] = -1;
	}

	for (int j = 0; j < mnMesh.numf; j++)
	{
		if (mnMesh.f[j].GetFlag(MN_DEAD)) 
			continue;
		int faceCount = mnMesh.f[j].deg -2;
		DbgAssert(faceCount == 1);
		
		int a = mnMesh.f[j].vtx[0];
		int b = mnMesh.f[j].vtx[1];
		int c = mnMesh.f[j].vtx[2];
		int originalFace = bindMeshIndexToOriginalFaces[j];
		bindMeshIndexToOriginalVerts[a] = originalFaces[originalFace*3];
		bindMeshIndexToOriginalVerts[b] = originalFaces[originalFace*3+1];
		bindMeshIndexToOriginalVerts[c] = originalFaces[originalFace*3+2];
		mesh->appendTriangle(a,b,c);

	}


	return bindMeshIndexToOriginalVerts;
}



void BMD_Command_VoxelWeight::Execute(BonesDefMod * mod, BoneModData *bmd)
{
	//if nothing is selected use all otherwise use the selection
	BitArray vSelection(bmd->selected);
	if (vSelection.NumberSet()==0)
		vSelection.SetAll();

	bool iret = Solve(mod,bmd,vSelection);

	//heat maps can fail if there are multiple elements in this case break up by elements and solve each one
	//independently
	if ( (iret == false) && (mType == 1) )
	{
		macroRecorder->FunctionCall(GetString(IDS_HEATMAP_REPROCESS),0,0);
		macroRecorder->EmitScript();

		GetCOREInterface()->ReplacePrompt(GetString(IDS_HEATMAP_REPROCESS));
//		if (MessageBox(GetCOREInterface()->GetMAXHWnd(),GetString(IDS_HEATMAP_ERROR_MSG),GetString(IDS_HEATMAP_ERROR_MSG),MB_OKCANCEL) == IDOK	)
		{

			BitArray originalSelection(bmd->selected);
			if (originalSelection.NumberSet()==0)
				originalSelection.SetAll();
			BitArray vertsToProcess(originalSelection);
			bool done = false;
			while (!done)
			{	
				int seedIndex = -1;
				for (int i =0; i < vertsToProcess.GetSize();i++)
				{
					if (vertsToProcess[i])
					{
						seedIndex = i;
						break;
					}
				}
				if (seedIndex == -1)
				{
					done = true;
				}
				else
				{
					bmd->selected.ClearAll();
					bmd->selected.Set(seedIndex,TRUE);
					mod->SelectElement(bmd, FALSE);
					BitArray vSelection(bmd->selected);
					vSelection &= vertsToProcess;
					vertsToProcess ^= vSelection;
					iret = Solve(mod,bmd,vSelection);
					if (iret == false)
					{					
						macroRecorder->FunctionCall(_T("-- Failed heatmap solve on these vertices"), 0,0 );
						macroRecorder->EmitScript();
						macroRecorder->FunctionCall(_T("skinOps.SelectVertices"), 2,0, mr_reftarg, mod, mr_bitarray, vSelection);
						macroRecorder->EmitScript();
					}
					else
					{
						macroRecorder->FunctionCall(_T("-- Succeeded heatmap solve on these vertices"), 0,0 );
						macroRecorder->EmitScript();
						macroRecorder->FunctionCall(_T("skinOps.SelectVertices"), 2,0, mr_reftarg, mod, mr_bitarray, vSelection);
						macroRecorder->EmitScript();
					}

				}			
			}
		}
	}

	if (mType == 0)
	{
		if (mod->mVoxelDlg)
			SendMessage(mod->mVoxelDlg,WM_SOLVE_DONE,0,0);
	}
	else 
	{
		if (mod->mHeatMapDlg)
			SendMessage(mod->mHeatMapDlg,WM_SOLVE_DONE,0,0);
	}
}

bool BMD_Command_VoxelWeight::Solve(BonesDefMod * mod, BoneModData *bmd, const BitArray& vSelection)
{
    if (false == MaxSDK::Graphics::IsGpuPresent()) return false;
	if (mod->BoneData.Count() < 2) return true;
	
	Interface::SuspendSceneRedrawGuard drawGruard;
	
	//build our transform mapping since we can have null elements in our list and dont want those to go to the voxel engine
	MaxProgressImpl maxProgesss;

	MaxSDK::GeomBind::ITransformHierarchy* skel = MaxSDK::GeomBind::ITransformHierarchy::create();
	//get our hierarchy and transform
	//need to check root names and also what happens to nodes not part of a hierarchy
	int boneIndex = 0;
	bool boneRemapNeeded;
	Tab<int> boneRemap;  //maps from a voxel bone to a skin bone
//	boneRemap.SetCount(mod->BoneData.Count());
	int voxelBoneIndex = 0;

	//just a quick index look up to see if a bone has multiple representation so we can quickly find those
	BitArray boneHasMultipleReps;
	boneHasMultipleReps.SetSize(mod->BoneData.Count());
	boneHasMultipleReps.ClearAll();

	//we need to flag so nodes so need to clear some temp user flags
	INode* root = GetCOREInterface()->GetRootNode();
	TimeValue t = GetCOREInterface()->GetTime();


	//loop through the scene graph and clear our temp flag
	//so we can find nubs and end bones that are not part of skin
	//voxel weighting does best joint to joint so add the end 
	//nub if there is one or use the bounding box to create one
	Tab<INode*> nodeStack;
	bool done = false;
	nodeStack.Append(1,&root,5000);
	while (!done)
	{
		int count = nodeStack.Count();
		INode* currentNode = nodeStack[count-1];
		nodeStack.Delete(count-1,1);
		int numChildren = currentNode->NumberOfChildren();
		for (int i = 0; i < numChildren; i++)
		{
			INode* child = currentNode->GetChildNode(i);
			nodeStack.Append(1,&child,5000);
		}
		
		if (currentNode != nullptr)
			currentNode->ClearAFlag(A_PLUGIN1);

		if (nodeStack.Count() == 0) done = true;
	}
	

	//mark all our bones so we can find nub bones
	for (int i = 0; i < mod->BoneData.Count(); i++)
	{
		INode* node = mod->BoneData[i].Node;
		if ( node != nullptr)
			node->SetAFlag(A_PLUGIN1);
	}
	
	bool autoNub = mAutoNubs;


	for (int i = 0; i < mod->BoneData.Count(); i++)
	{
		INode* node = mod->BoneData[i].Node;
		//check for null nodes since this can happen
		if ( node != nullptr)
		{
			//get our names and transforms
			INode* parentNode = node->GetParentNode();
			Matrix3 objTm = mBoneObjTm[boneIndex++];
			TSTR nodeNameW;
			nodeNameW.printf(_M("%s%d"),node->GetName(),node->GetHandle());
			TSTR parentNameW;
			parentNameW.printf(_M("%s%d"),parentNode->GetName(),parentNode->GetHandle());
			//spline bones are treated differently, they are sampled down into little bones
			if (mod->BoneData[i].flags & BONE_SPLINE_FLAG)
			{
				//need to tell the system that spline weights have changed and they need to be updated
				bmd->reevaluate = TRUE;
				boneRemapNeeded = true;
				int segments = mod->BoneData[i].referenceSpline.Segments();
				// break every segment into 4 samples in world space
				Tab<Point3> samples;
				for (int k = 0; k < segments; k++)
				{
					int segCount = 4;
					int segDivsor = segCount;
					if (k == segments-1)
						segCount++;
					for (int m = 0; m < segCount; m++)
					{
						float u = (float)m/(float)segDivsor;
						Point3 p = mod->BoneData[i].referenceSpline.InterpBezier3D(k,u,SPLINE_INTERP_SIMPLE);
						p = p * objTm;
						samples.Append(1,&p,1000);
					}					
				}

				double worldTm[16];

				//just a new stupid name to make unique
				parentNameW.printf(_M("xxxPKWxxx_%s"),nodeNameW);

				//loop through each sample and add it as a bone
				for (int k = 0; k < samples.Count(); k++)
				{
					Point3 p = samples[k];
					Matrix3 sampleTm(objTm);
					sampleTm.SetRow(3,p);

					//convert to an OGL friendly matrix ( note only the translation data is used so everything else is identity.
					ToOpenGLMatrix(sampleTm,worldTm);
					
					//convert our names to chars from multibyte
					TSTR newNodeNameW;
					newNodeNameW.printf(_M("xxxPKWxxx_%s_%d"),nodeNameW,k);

					char nodeName[512];
					wcstombs(nodeName, newNodeNameW.data(),512);
					char parentName[512];
					wcstombs(parentName, parentNameW.data(),512);
					//add to our hierarchy
					skel->registerTransform(nodeName,worldTm,parentName);
					
					//update the bone index remapper so we can match new bones to old bones
					boneHasMultipleReps.Set(i,TRUE);
					boneRemap.Append(1,&i,1000);
					
					parentNameW = newNodeNameW;
				}
			}
			//it is just a regular bone
			else
			{
				//convert to friendly tm
				double worldTm[16];
				ToOpenGLMatrix(objTm,worldTm);

				//convert down to chars
				char nodeName[512];
				wcstombs(nodeName, nodeNameW.data(),512);
				char parentName[512];
				wcstombs(parentName, parentNameW.data(),512);
				//add our hierarchy
				skel->registerTransform(nodeName,worldTm,parentName);	
				
				//update the bone index remapper
				boneRemap.Append(1,&i,1000);

				//if we are doing auto nub handle nubs that are not skinned and end bones specially.  This is done since the voxelizer
				//likes parent child links and this just adds that
				if (autoNub)
				{
					//nub is just one bone at the end
				/*	if (node->NumChildren() == 1)
					{
						INode* childNode = node->GetChildNode(0);
						//check to see if we have a nub which is just a single end bone
						//and not already part of the bones system
						if (childNode && (childNode->TestAFlag(A_PLUGIN1) == false))
						{
							//add it to the the hierarchy 
							Matrix3 childObjTm = childNode->GetObjectTM(t);
							TSTR childNameW;
							childNameW.printf(_M("xxxPKWNubxxx_%s"),nodeNameW);
							char childName[512];
							wcstombs(childName, childNameW.data(),512);

							ToOpenGLMatrix(childObjTm,worldTm);
							skel->registerTransform(childName,worldTm,nodeName);	

							boneHasMultipleReps.Set(i,TRUE);
							boneRemap.Append(1,&i,1000);

						}
					}
					//or no end bone add a nub by using the bounding box and bone direction
					else */
					if ( (node->NumChildren() == 0) || ((node->NumChildren() == 1) && (node->GetChildNode(0)->TestAFlag(A_PLUGIN1) == false))) 
					{
						//should we do this for all nodes and just use the largest axis?
						if (node->GetBoneNodeOnOff())
						{							
							int axis = node->GetBoneAxis();
							Object *obj = node->GetObjectRef();
							//get our bounds and use that to create a new tm
							Box3 localBounds;
							localBounds.Init();
							obj->GetDeformBBox(t,localBounds);
							float offsetLength = localBounds.Max()[axis];
							Point3 offsetVector = Normalize(objTm.GetRow(axis)) * offsetLength;
							Point3 trans = objTm.GetRow(3);
							trans += offsetVector;
							Matrix3 extendedObjTm(objTm);
							extendedObjTm.SetRow(3,trans);

							//add that to our hierarchy
							TSTR childNameW;
							childNameW.printf(_M("xxxPKWBoxxxx_%s"),nodeNameW);
							char childName[512];
							wcstombs(childName, childNameW.data(),512);

							ToOpenGLMatrix(extendedObjTm,worldTm);
							skel->registerTransform(childName,worldTm,nodeName);	

							boneHasMultipleReps.Set(i,TRUE);
							boneRemap.Append(1,&i,1000);
						}
					}
				}
			}
		}

	}



	//stupid OGL artifact, the voxelizer uses OGL so we must have a dummy window to host the context
	RegisterSimpleOpenGLClass(hInstance);		
	HWND hWndFake = CreateWindow(SIMPLE_OPENGL_CLASS_NAME, L"FAKE", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_CLIPCHILDREN,
		0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, 
		NULL, hInstance, NULL); 

	HDC hDC = GetDC(hWndFake); 

	// First, choose false pixel format

	PIXELFORMATDESCRIPTOR pfd; 
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); 
	pfd.nSize= sizeof(PIXELFORMATDESCRIPTOR); 
	pfd.nVersion   = 1; 
	pfd.dwFlags    = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; 
	pfd.iPixelType = PFD_TYPE_RGBA; 
	pfd.cColorBits = 32; 
	pfd.cDepthBits = 32; 
	pfd.iLayerType = PFD_MAIN_PLANE; 

	int iPixelFormat = ChoosePixelFormat(hDC, &pfd); 
	if (iPixelFormat == 0)
		return true; 

	if(!SetPixelFormat(hDC, iPixelFormat, &pfd))return true; 

	// Create the false, old style context (OpenGL 2.1 and before)

	HGLRC hRCFake = wglCreateContext(hDC); 
	wglMakeCurrent(hDC, hRCFake); 

	MaxSDK::GeomBind::IContext* context = MaxSDK::GeomBind::IContext::create();
	context->makeCurrent();
	Matrix3 objTm = mObjTm;

	//need to check to see if we need to flip the winding from scaling
	bool flipMesh = false;
	if (mObjTm.Parity())
		flipMesh = true;

	//get the mesh
	MaxSDK::GeomBind::IMesh *mesh = MaxSDK::GeomBind::IMesh::create();
	

	Tab<int> vertRemapper;
	bool remapVerts = false;

	if ( (mType == 1) || (bmd->selected.NumberSet()) )
	{
		vertRemapper = ProcessHeatMapMesh(bmd,mesh,objTm,flipMesh);
		remapVerts = true;
	}
	else if (bmd->isMesh && bmd->mesh)
	{
		//copy our vertices over
		for (int j = 0; j < bmd->mesh->numVerts; j++)
		{
			Point3 v = bmd->mesh->verts[j]*objTm;
			mesh->appendVertex(v.x,v.y,v.z);
		}
		//copy our faces over
		for (int j = 0; j < bmd->mesh->numFaces; j++)
		{
			int a = bmd->mesh->faces[j].v[0];
			int b = bmd->mesh->faces[j].v[1];
			int c = bmd->mesh->faces[j].v[2];

			if (vSelection[a] || vSelection[b] || vSelection[c])
			{
				if (flipMesh)
				{
					mesh->appendTriangle(c,b,a);
				}
				else
				{
					mesh->appendTriangle(a,b,c);
				}
			}
		}
	}
	else if (bmd->mnMesh)
	{
		//copy our vertices over
		for (int j = 0; j < bmd->mnMesh->numv; j++)
		{
			Point3 v = bmd->mnMesh->v[j].p*objTm;
			mesh->appendVertex(v.x,v.y,v.z);
		}
		//copy our faces over
		Tab<int> faceDiag;
		for (int j = 0; j < bmd->mnMesh->numf; j++)
		{
			if (bmd->mnMesh->f[j].GetFlag(MN_DEAD)) continue;
			int faceCount = bmd->mnMesh->f[j].deg -2;
			bmd->mnMesh->f[j].GetTriangles(faceDiag);
			int* faceDiagPtr = faceDiag.Addr(0);
			for (int k = 0; k < faceCount; k++)
			{
				int a = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];
				int b = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];
				int c = bmd->mnMesh->f[j].vtx[*faceDiagPtr++];
				if (vSelection[a] || vSelection[b] || vSelection[c])
				{
					if (flipMesh)					
						mesh->appendTriangle(c,b,a);
					else
						mesh->appendTriangle(a,b,c);
				}					
			}
			
		}
	}
	else if (bmd->patch)
	{
		Mesh msh;
		int holdSteps = bmd->patch->GetMeshSteps();
		bmd->patch->SetMeshSteps(2);
		bmd->patch->ComputeMesh(msh,0);
		bmd->patch->SetMeshSteps(holdSteps);
		//copy our vertices over
		for (int j = 0; j < msh.numVerts; j++)
		{
			Point3 v = msh.verts[j]*objTm;
			mesh->appendVertex(v.x,v.y,v.z);
		}
		//copy our faces over
		for (int j = 0; j < msh.numFaces; j++)
		{
			int a = msh.faces[j].v[0];
			int b = msh.faces[j].v[1];
			int c = msh.faces[j].v[2];

			if (vSelection[a] || vSelection[b] || vSelection[c])
			{
				if (flipMesh)
					mesh->appendTriangle(c,b,a);
				else
					mesh->appendTriangle(a,b,c);
			}
		}
	}

	maxProgesss.startOperation();
	//compute the weights
	const MaxSDK::GeomBind::ISparseVertexWeights* weights = nullptr;

	if (mMaxInfluence == 0)
		mMaxInfluence = 1;
	if (mMaxInfluence < -1)
		mMaxInfluence = -1;

	if (mFalloff < 0.0f)
		mFalloff = 0.0f;
	if (mFalloff > 1.0f)
		mFalloff = 1.0f;
	mVoxelSize = mVoxelSize/2;
	if (mVoxelSize < 16)
		mVoxelSize = 16;
	if (mVoxelSize > 2096)
		mVoxelSize = 2096;
	bool clearPrompt = true;
	bool iret = true;	
	if ((this->mType == 0) && (mod->mVoxelDlg != nullptr))
		maxProgesss.mHasUI = true;
	else if ((this->mType == 1) && (mod->mHeatMapDlg != nullptr))
		maxProgesss.mHasUI = true;
	try
	{
		if (this->mType == 0)
		{
			weights = context->bindSparseGV(mesh,skel,&maxProgesss,mFalloff,mMaxInfluence,mVoxelSize,mUseWinding);
		}
		else
		{
			weights = context->bindHeatmap(mesh,skel,&maxProgesss,mFalloff,mMaxInfluence);
		}

	}
	catch (...)
	{
		TSTR msg(GetString(IDS_BADMESH));
		GetCOREInterface()->ReplacePrompt(msg);
		clearPrompt = false;
		iret = false;
	}
	if (clearPrompt)
	{
		if (((weights == nullptr) || (weights->count()==0)) && (maxProgesss.cancelOperation()== false))
		{
			TSTR msg(GetString(IDS_BADMESH));
			GetCOREInterface()->ReplacePrompt(msg);
			iret = false;
		}
		else
		{
			TSTR msg(_M(" "));
			GetCOREInterface()->ReplacePrompt(msg);
		}
	}
	
	maxProgesss.endOperation();


	//now fill in the weights
	if (weights != nullptr)
	{
		Tab<VertexInfluenceListClass> infl;
		int ct = weights->count();
		for (int j = 0; j < ct; j++)
		{
			int vertIndex = j;
			if (remapVerts)
				vertIndex = vertRemapper[j];

			if (vSelection[vertIndex] == false) continue;

			int count = weights->influenceCount(j);
			infl.SetCount(count);


			for (int k = 0; k < count; k++)
			{

				float w = weights->influence(j,k).weight;
				int ob = weights->influence(j,k).index;
				int b = boneRemap[ob];

				infl[k].Influences = w;					
				infl[k].normalizedInfluences = w;
				infl[k].Bones = b;
				infl[k].SubCurveIds -= -1;
				infl[k].SubSegIds = -1;
				infl[k].u = -1.0f;
			}
			if (j < bmd->VertexData.Count())
			{
				for (int k = 0; k < infl.Count(); k++)
				{
					//see if we need to collapse multiple identical bones
					if (boneHasMultipleReps[infl[k].Bones])
					{
						int checkBone = infl[k].Bones;
						for (int m = k+1; m < infl.Count(); m++)
						{
							if (infl[m].Bones == checkBone)
							{
								infl[k].Influences += infl[m].Influences;
								infl.Delete(m,1);
								m--;
							}
						}
					}
				}
				bmd->VertexData[vertIndex]->Modified(TRUE);
				bmd->VertexData[vertIndex]->PasteWeights(infl);
			}
		}
	}


	//clean up all our stuff
	MaxSDK::GeomBind::IMesh::dispose(mesh);
	MaxSDK::GeomBind::IContext::dispose(context);

	MaxSDK::GeomBind::ITransformHierarchy::dispose(skel);
	wglMakeCurrent(NULL, NULL); 
	wglDeleteContext(hRCFake); 
	ReleaseDC(hWndFake,hDC);
	DestroyWindow(hWndFake); 	
	return iret;
}