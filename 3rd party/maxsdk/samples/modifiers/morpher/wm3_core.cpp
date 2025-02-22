/*===========================================================================*\
 | 
 |  FILE:	wM3_core.cpp
 |			Weighted Morpher for MAX R3
 |			ModifyObject
 | 
 |  AUTH:   Harry Denholm
 |			Copyright(c) Kinetix 1999
 |			All Rights Reserved.
 |
 |  HIST:	Started 27-8-98
 | 
\*===========================================================================*/

#include "wM3.h"
#include "PerformanceTools.h"
#include <omp.h>

namespace
{
	float epsilon=1E-6f;
}

void MorphR3::Bez3D(Point3 &b, const Point3 *p, const float &u)
{
	float t01[3], t12[3], t02[3], t13[3];
		
	t01[0] = p[0][0] + (p[1][0] - p[0][0])*u;
	t01[1] = p[0][1] + (p[1][1] - p[0][1])*u;
	t01[2] = p[0][2] + (p[1][2] - p[0][2])*u;
	
	t12[0] = p[1][0] + (p[2][0] - p[1][0])*u;
	t12[1] = p[1][1] + (p[2][1] - p[1][1])*u;
	t12[2] = p[1][2] + (p[2][2] - p[1][2])*u;

	t02[0] = t01[0] + (t12[0] - t01[0])*u;
	t02[1] = t01[1] + (t12[1] - t01[1])*u;
	t02[2] = t01[2] + (t12[2] - t01[2])*u;

	t01[0] = p[2][0] + (p[3][0] - p[2][0])*u;
	t01[1] = p[2][1] + (p[3][1] - p[2][1])*u;
	t01[2] = p[2][2] + (p[3][2] - p[2][2])*u;

	t13[0] = t12[0] + (t01[0] - t12[0])*u;
	t13[1] = t12[1] + (t01[1] - t12[1])*u;
	t13[2] = t12[2] + (t01[2] - t12[2])*u;

	b[0] = t02[0] + (t13[0] - t02[0])*u;
	b[1] = t02[1] + (t13[1] - t02[1])*u;
	b[2] = t02[2] + (t13[2] - t02[2])*u;	
}

void MorphR3::ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *node) 
{
	Update_channelValues();
	// This will see if the local cached object is valid and update it if not
	// It will now also call a full channel rebuild to make sure their deltas are
	// accurate to the new cached object
	if(!MC_Local.AreWeCached())
	{
		UI_MAKEBUSY

		MC_Local.MakeCache(os->obj);

		for(int i=0;i<chanBank.size();i++)
		{
			if(chanBank[i].mActive)
			{
				chanBank[i].rebuildChannel();
			}
		}

		UI_MAKEFREE
	}

	Interval valid=FOREVER;


	// AUTOLOAD
	int itmp; 
	pblock->GetValue(PB_CL_AUTOLOAD, 0, itmp, valid);

	if(itmp==1)
	{
			for(int k=0;k<chanBank.size();k++)
			{
				if(chanBank[k].mConnection) {
					chanBank[k].buildFromNode(chanBank[k].mConnection,FALSE,t,FALSE,TRUE);
					for(int i=0; i<chanBank[k].mNumProgressiveTargs; i++) { chanBank[k].mTargetCache[i].Init(chanBank[k].mTargetCache[i].mTargetINode); }
				}
			}
	}



	// Get count from host
	int hmCount = os->obj->NumPoints();

	int i, pointnum;
	pointnum = 0;
	// to hold percentage
	float fChannelPercent;
	
	// These are our morph deltas / point
	// They get built by cycling through the points and generating
	// the difference data, summing it into these tables and then
	// appling the changes at the end.
	// This will leave us with the total differences per point on the 
	// local mesh. We can then rip through and apply them quickly
	float *difX = new float[hmCount];
	float *difY = new float[hmCount];
	float *difZ = new float[hmCount];
	double *wgts = new double[hmCount];

	// this is the indicator of what points on the host
	// to update after all deltas have been summed
	bool* pPointsMod = new bool[hmCount];
	ZeroMemory(pPointsMod,sizeof(bool) * hmCount);

	int glUsesel;
	pblock->GetValue( PB_OV_USESEL, t, glUsesel, valid);

	BOOL glUseLimit; float glMAX,glMIN;
	pblock->GetValue( PB_OV_USELIMITS, t, glUseLimit, valid);
	pblock->GetValue( PB_OV_SPINMAX, t, glMAX, valid);
	pblock->GetValue( PB_OV_SPINMIN, t, glMIN, valid);


	// --------------------------------------------------- MORPHY BITS
	// cycle through channels, searching for ones to use
	for(i=0;i<chanBank.size();i++)
	{
		morphChannel& currentMorChannel = chanBank[i];
		if( currentMorChannel.mActive )
		{
			// temp fix for diff. pt counts
			if(currentMorChannel.mNumPoints!=hmCount) 
			{
				currentMorChannel.mInvalid = TRUE;
				continue;
			};

			// This channel is considered okay to use
			currentMorChannel.mInvalid = FALSE;
	
			// Is this channel flagged as inactive?
			if(currentMorChannel.mActiveOverride==FALSE) continue;

			// get morph percentage for this channel
			currentMorChannel.cblock->GetValue(0,t,fChannelPercent,valid);

			// Clamp the channel values to the limits
			if(currentMorChannel.mUseLimit||glUseLimit)
			{
				int Pmax; int Pmin;
				if(glUseLimit)  { Pmax = (int)glMAX; Pmin = (int)glMIN; }
				else
				{
					Pmax = (int)currentMorChannel.mSpinmax;
					Pmin = (int)currentMorChannel.mSpinmin;
				}
				if(fChannelPercent>Pmax) fChannelPercent = (float)Pmax;
				if(fChannelPercent<Pmin) fChannelPercent = (float)Pmin;
			}

			// cycle through all morphable points, build delta arrays
//			MaxSDK::PerformanceTools::ThreadTools threadData;
//			int numThreads = threadData.GetNumberOfThreads(MaxSDK::PerformanceTools::ThreadTools::kDeformationThreading, currentMorChannel.mNumPoints);
//			omp_set_num_threads(numThreads);
//			#pragma omp parallel for schedule(dynamic, 128)
			for(pointnum=0; pointnum < currentMorChannel.mNumPoints; pointnum++)
			{
				// some worker variables
				float deltX,deltY,deltZ;
				BOOL pointSelected = FALSE;
				if (pointnum < currentMorChannel.mSel.GetSize())
					pointSelected = currentMorChannel.mSel[pointnum];
				if ( (!currentMorChannel.mUseSel && glUsesel==0)  || pointSelected)
				{
					// get the points to morph between
					//vert = os->obj->GetPoint(mIndex);
					//weight = os->obj->GetWeight(mIndex);
					Point3 vert = MC_Local.oPoints[pointnum];
					double weight = MC_Local.oWeights[pointnum];

					// Get softselection, if applicable
					double decay = 1.0f;
					if(os->obj->GetSubselState()!=0) decay = os->obj->PointSelection(pointnum);
					
					// Add the previous point data into the delta table
					// if its not already been done
					if(!pPointsMod[pointnum])
					{
						difX[pointnum]=vert.x;
						difY[pointnum]=vert.y;
						difZ[pointnum]=vert.z;
						wgts[pointnum]=weight;
					}
		
					// calculate the differences
					// decay by the weighted vertex amount, to support soft selection
					double deltW=(currentMorChannel.mWeights[pointnum]-weight)/100.0f*(double)fChannelPercent;
					wgts[pointnum]+=deltW;

					if(!currentMorChannel.mNumProgressiveTargs)
					{
						deltX=(float)(((currentMorChannel.mDeltas[pointnum].x)*fChannelPercent)*decay);
						deltY=(float)(((currentMorChannel.mDeltas[pointnum].y)*fChannelPercent)*decay);
						deltZ=(float)(((currentMorChannel.mDeltas[pointnum].z)*fChannelPercent)*decay);
					}
					else  
					{	//DO PROGRESSIVE MORPHING

						// worker variables for progressive modification
						float fProgression, length, totaltargs;
						int segment;
						Point3 endpoint[4];
						Point3 splinepoint[4];
						Point3 temppoint[2];
						Point3 progession;

						totaltargs = (float)(currentMorChannel.mNumProgressiveTargs +1);

						fProgression = fChannelPercent;
						if (fProgression<0) fProgression=0;
						if(fProgression>100) fProgression=100;
						segment=1; 
						while(segment<=totaltargs && fProgression >= currentMorChannel.GetTargetPercent(segment-2)) segment++;
						//figure out which segment we are in
						//on the target is the next segment
						//first point (object) MC_Local.oPoints
						//second point (first target) is currentMorChannel.mPoints
						//each additional target is targetcache starting from 0
						if(segment==1) {
							endpoint[0]= MC_Local.oPoints[pointnum]; 
							endpoint[1]= currentMorChannel.mPoints[pointnum];
							endpoint[2]= currentMorChannel.mTargetCache[0].GetPoint(pointnum);
						}
						else if(segment==totaltargs) {
							int targnum= (int)(totaltargs-1);
							for(int j=2; j>=0; j--) {
								targnum--;
								if(targnum==-2) temppoint[0]=MC_Local.oPoints[pointnum];
								else if(targnum==-1) temppoint[0]=currentMorChannel.mPoints[pointnum];
								else temppoint[0]=currentMorChannel.mTargetCache[targnum].GetPoint(pointnum);
								endpoint[j]= temppoint[0]; 
							}
						}
						else {
							int targnum= segment;
							for( int j=3; j>=0; j--) {
								targnum--;
								if(targnum==-2) temppoint[0]=MC_Local.oPoints[pointnum];
								else if(targnum==-1) temppoint[0]=currentMorChannel.mPoints[pointnum];
								else temppoint[0]=currentMorChannel.mTargetCache[targnum].GetPoint(pointnum);
								endpoint[j]= temppoint[0]; 
							}
						}

						//set the middle knot vectors
						if(segment==1) {
							splinepoint[0] = endpoint[0];
							splinepoint[3] = endpoint[1];
							temppoint[1] = endpoint[2] - endpoint[0];
							temppoint[0] = endpoint[1] - endpoint[0];
							length = FLength(temppoint[1]); length *= length;
							if(fabs(length) < epsilon ) {
								splinepoint[1] = endpoint[0];
								splinepoint[2] = endpoint[1];
							}
							else { 
								splinepoint[2] = endpoint[1] - 
									(DotProd(temppoint[0], temppoint[1]) * currentMorChannel.mCurvature / length) * temppoint[1];
								splinepoint[1] = endpoint[0] +  
									currentMorChannel.mCurvature * (splinepoint[2]-endpoint[0]);
							}

						}
						else if (segment==totaltargs) {
							splinepoint[0] = endpoint[1];
							splinepoint[3] = endpoint[2];
							temppoint[1] = endpoint[2] - endpoint[0];
							temppoint[0] = endpoint[1] - endpoint[2];
							length = FLength(temppoint[1]); length *= length;
							if(fabs(length) < epsilon) {
								splinepoint[1] = endpoint[0];
								splinepoint[2] = endpoint[1];
							}
							else {
								Point3 p = (DotProd(temppoint[1], temppoint[0]) * currentMorChannel.mCurvature / length) * temppoint[1];
								splinepoint[1] = endpoint[1] - 
									(DotProd(temppoint[1], temppoint[0]) * currentMorChannel.mCurvature / length) * temppoint[1];
								splinepoint[2] = endpoint[2] +  
									currentMorChannel.mCurvature * (splinepoint[1]-endpoint[2]);
							}
						}
						else {
							temppoint[1] = endpoint[2] - endpoint[0];
							temppoint[0] = endpoint[1] - endpoint[0];
							length = FLength(temppoint[1]); length *= length;
							splinepoint[0] = endpoint[1];
							splinepoint[3] = endpoint[2];
							if(fabs(length) < epsilon) { splinepoint[1] = endpoint[0]; }
							else {
								splinepoint[1] = endpoint[1] + 
								(DotProd(temppoint[0], temppoint[1]) * currentMorChannel.mCurvature / length) * temppoint[1];
							}
							temppoint[1] = endpoint[3] - endpoint[1];
							temppoint[0] = endpoint[2] - endpoint[1];
							length = FLength(temppoint[1]); length *= length;
							if(fabs(length) < epsilon) { splinepoint[2] = endpoint[1]; }
							else {
								splinepoint[2] = endpoint[2] - 
									(DotProd(temppoint[0], temppoint[1]) * currentMorChannel.mCurvature / length) * temppoint[1];
							}
						}

						// this is the normalizing equation
						float targetpercent1, targetpercent2, u;
						targetpercent1 = currentMorChannel.GetTargetPercent(segment-3);
						targetpercent2 = currentMorChannel.GetTargetPercent(segment-2);
						
						float top = fProgression-targetpercent1;
						float bottom = targetpercent2-targetpercent1;
						u = top/bottom;

						////this is just the bezier calculation
						Bez3D(progession, splinepoint, u);
						deltX= (float)((progession[0]-vert.x)*decay);
						deltY= (float)((progession[1]-vert.y)*decay);
						deltZ= (float)((progession[2]-vert.z)*decay);

					}
					
					difX[pointnum]+=deltX;
					difY[pointnum]+=deltY;
					difZ[pointnum]+=deltZ;
					// We've modded this point
					pPointsMod[pointnum] = true;

				} // msel check

			} // nmPts cycle
		}
	}
	// TH 10/12/15: MAXX-26476 -- Multithreading is crashing; turning it "off" for the time being by setting to 1 thread
	// "Real issue" is that ShapeObject::InvalidateGeomCache is not thread-safe and cached mesh freeing is causing a crash. Created Jira defect MAXX-26886 to revisit & fix for Kirin
//	MaxSDK::PerformanceTools::ThreadTools threadData;
//	int numThreads = 1; //threadData.GetNumberOfThreads(MaxSDK::PerformanceTools::ThreadTools::kDeformationThreading,hmCount);
//	omp_set_num_threads(numThreads);
	//#pragma omp parallel for schedule(static, hmCount / numThreads)
	for(int k=0;k<hmCount;k++)
	{
		if(pPointsMod[k]&&(MC_Local.sel[k]||os->obj->GetSubselState()==0))
		{
			Point3 fVert(difX[k], difY[k], difZ[k]);
			os->obj->SetPoint(k,fVert);
			os->obj->SetWeight(k,wgts[k]);
		}
	}

	// Captain Hack Returns...
	// Support for saving of modifications to a channel
	// Most of this is just duped from buildFromNode (delta/point/calc)
	if (recordModifications)
	{
		int tChan = recordTarget;
		morphChannel& morChannel = chanBank[tChan];
		int tPc = hmCount;

		// Prepare the channel
		morChannel.AllocBuffers(tPc, tPc);
		morChannel.mNumPoints = 0;

		int id = 0;
		Point3 DeltP;
		double wtmp;
		Point3 tVert;

		for(int x=0;x<tPc;x++)
		{

			if(pPointsMod[x])
			{
				tVert.x = difX[x];
				tVert.y = difY[x];
				tVert.z = difZ[x];

				wtmp = wgts[x];
			}
			else
			{
				tVert = os->obj->GetPoint(x);
				wtmp = os->obj->GetWeight(x);
			}
			// calculate the delta cache
			DeltP.x=(tVert.x-MC_Local.oPoints[x].x)/100.0f;
			DeltP.y=(tVert.y-MC_Local.oPoints[x].y)/100.0f;
			DeltP.z=(tVert.z-MC_Local.oPoints[x].z)/100.0f;
			morChannel.mDeltas[x] = DeltP;

			morChannel.mWeights[x] = wtmp;

			morChannel.mPoints[x] = tVert;
			morChannel.mNumPoints++;
		}

		recordModifications = FALSE;
		recordTarget = 0;
		morChannel.mInvalid = FALSE;

	}
	// End of record

	// clean up
	if(difX) delete [] difX;
	if(difY) delete [] difY;
	if(difZ) delete [] difZ;
	if(wgts) delete [] wgts;

	if(itmp==1) valid = Interval(t,t);

	// Update all the caches etc
	os->obj->UpdateValidity(GEOM_CHAN_NUM,valid);
	os->obj->PointsWereChanged();

	delete[] pPointsMod;
}

void MorphR3::DisplayMemoryUsage(void )
{
	if(!hwAdvanced) return;
	float tmSize = sizeof(*this);
	for(int i=0;i<chanBank.size();i++) tmSize += chanBank[i].getMemSize();
	TCHAR s[20];
	_stprintf(s,_T("%i KB"), (int)tmSize/1000);
	SetWindowText(GetDlgItem(hwAdvanced,IDC_MEMSIZE),s);	
}


BOOL IMorphClass::AddProgessiveMorph(MorphR3 *mp,int morphIndex, INode *node)
{
	Interval valid; 
	
	if (NULL == mp || mp->ip == NULL || NULL == node) return FALSE;
	
	ObjectState os = node->GetObjectRef()->Eval(mp->ip->GetTime());

	if( os.obj->IsDeformable() == FALSE ) return FALSE;

	// Check for same-num-of-verts-count
	if( os.obj->NumPoints()!= mp->MC_Local.Count) return FALSE;

	node->BeginDependencyTest();
	mp->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest()) {		
		return FALSE;
	}

	// check to make sure that the max number of progressive targets will not be exceeded
	//
	morphChannel &bank = mp->chanBank[morphIndex];
	if (NULL == bank.mConnection || bank.mNumProgressiveTargs >= MAX_PROGRESSIVE_TARGETS) 
	{
		return FALSE;
	}


	if( mp->CheckMaterialDependency() ) return FALSE;
	// Make the node reference, and then ask the channel to load itself
		
	if (theHold.Holding())
		theHold.Put(new Restore_FullChannel(mp, morphIndex));

	int  refIDOffset = mp->GetRefIDOffset(morphIndex);
	int refnum = ((morphIndex%100)*MAX_PROGRESSIVE_TARGETS) + 201 + bank.mNumProgressiveTargs;
	mp->ReplaceReference(refnum + refIDOffset,node);
	bank.InitTargetCache(bank.mNumProgressiveTargs,node);
	bank.mNumProgressiveTargs++;
	assert(bank.mNumProgressiveTargs<=MAX_PROGRESSIVE_TARGETS);
	bank.ReNormalize();
	mp->Update_channelParams();
	return TRUE;
}

BOOL IMorphClass::DeleteProgessiveMorph(MorphR3 *mp,int morphIndex, int progressiveMorphIndex)
{

	morphChannel &bank = mp->chanBank[morphIndex];
	

	if(!&bank || !bank.mNumProgressiveTargs) return FALSE;

	int targetnum = progressiveMorphIndex;

	if (theHold.Holding())
		theHold.Put(new Restore_FullChannel(mp, morphIndex) );

	if(targetnum==0)
	{
		if(bank.mNumProgressiveTargs)
		{
			bank = bank.mTargetCache[0];
			for(int i=0; i<bank.mNumProgressiveTargs-1; i++) {
				bank.mTargetCache[i] = bank.mTargetCache[i+1];
			}
			bank.mTargetCache[bank.mNumProgressiveTargs-1].Clear();
		}
	}
	else if(targetnum>0 && bank.mNumProgressiveTargs && targetnum<=bank.mNumProgressiveTargs)
	{
		for(int i=targetnum; i<bank.mNumProgressiveTargs; i++) {
				bank.mTargetCache[i-1] = bank.mTargetCache[i];
		}
		bank.mTargetCache[bank.mNumProgressiveTargs-1].Clear();
	}

	//reset the references
	bank.ResetRefs(mp, progressiveMorphIndex);

	bank.mNumProgressiveTargs--;

	bank.ReNormalize();
	bank.iTargetListSelection--;
	if (bank.iTargetListSelection < 0) {
		bank.iTargetListSelection = 0;
	}

	bank.rebuildChannel();
	mp->Update_channelFULL();

	if (theHold.Holding())
	{
		theHold.Put( new Restore_Display( mp ) );
	}

	return TRUE;	
}

void IMorphClass::SwapMorphs(MorphR3 *mp,const int from, const int to, BOOL swap)
{

	if (theHold.Holding())
	{
		theHold.Put( new Restore_Display( mp ) );
		theHold.Put(new Restore_FullChannel(mp, from, FALSE));
		theHold.Put(new Restore_FullChannel(mp, to, FALSE));
	}
	
	if (swap)
		mp->ChannelOp(from,to,OP_SWAP);
	else mp->ChannelOp(from,to,OP_MOVE);

	if (theHold.Holding())
	{
		theHold.Put( new Restore_Display( mp ) );
	}

}


void IMorphClass::SwapPTargets(MorphR3 *mp,const int morphIndex, const int from, const int to, const bool isundo)
{
	/////////////////////////////////
	int currentChan = morphIndex;
	morphChannel &cBank =  mp->chanBank[currentChan];
	if(from<0 || to<0 || from>cBank.NumProgressiveTargets() || to>cBank.NumProgressiveTargets()) return;


	int refIDOffsetFrom = mp->GetRefIDOffset(to);
	int refIDOffsetTo = mp->GetRefIDOffset(from);

	if(from!=0 && to!=0) 
	{
		TargetCache toCache(cBank.mTargetCache[to-1]);
		
		float wa,wb;
		wa = cBank.mTargetCache[to-1].mTargetPercent;
		wb = cBank.mTargetCache[from-1].mTargetPercent;

		cBank.mTargetCache[to-1] = cBank.mTargetCache[from-1]; 
		cBank.mTargetCache[from-1] = toCache; 

		cBank.mTargetCache[to-1].mTargetPercent = wb;
		cBank.mTargetCache[from-1].mTargetPercent = wa;

		mp->ReplaceReference(mp->GetRefNumber(currentChan, from)+refIDOffsetFrom, cBank.mTargetCache[from-1].RefNode() );
		mp->ReplaceReference(mp->GetRefNumber(currentChan, to)+refIDOffsetTo, cBank.mTargetCache[to-1].RefNode());
	}
	else //switch channel and first targetcache
	{

		float wa,wb;
		wa = cBank.mTargetCache[0].mTargetPercent;
		wb = cBank.mTargetPercent;

		TargetCache tempCache(cBank.mTargetCache[0]);
		cBank.mTargetCache[0] = cBank;
		cBank = tempCache;

		cBank.mTargetCache[0].mTargetPercent = wb;
		cBank.mTargetPercent = wa;

		int refIDOffset = mp->GetRefIDOffset(currentChan);
		mp->ReplaceReference(101+currentChan%100+refIDOffset, cBank.mConnection );
		mp->ReplaceReference(mp->GetRefNumber(currentChan, 0)+refIDOffset, cBank.mTargetCache[0].RefNode() );		
	}


}


void IMorphClass::SetTension(MorphR3 *mp, int morphIndex, float tension )
{
	if (theHold.Holding())
		theHold.Put( new Restore_FullChannel(mp, morphIndex ));

	mp->chanBank[morphIndex].mCurvature = tension;

	if (theHold.Holding())
		theHold.Put( new Restore_Display( mp ) );
	
 	mp->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	mp->ip->RedrawViews(mp->ip->GetTime(),REDRAW_INTERACTIVE,NULL);
	
}

void IMorphClass::HoldMarkers(MorphR3 *mp) 
{
	if (theHold.Holding()) theHold.Put(new Restore_Marker(mp)); 
};

void IMorphClass::HoldChannel(MorphR3 *mp, int channel) 
{
	if (theHold.Holding()) 
		theHold.Put(new Restore_FullChannel(mp, channel) );
}