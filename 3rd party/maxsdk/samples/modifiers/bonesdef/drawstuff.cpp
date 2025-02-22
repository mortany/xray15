
/**********************************************************************

FILE: DrawStuff.cpp

DESCRIPTION:  Stuff to draw bones and envelopes

CREATED BY: Peter Watje

HISTORY: 8/5/98

*>	Copyright (c) 1998, All Rights Reserved.
**********************************************************************/

#include "mods.h"
#include "shape.h"
#include "spline3d.h"

// This uses the linked-list class templates
#include "linklist.h"
#include "bonesdef.h"

void BonesDefMod::DrawCrossSection(Point3 a, Point3 align, float length,  Matrix3 tm, GraphicsWindow *gw)
{
#define NUM_SEGS	16

	Point3 plist[NUM_SEGS+1];
	Point3 mka,mkb,mkc,mkd;

	align = Normalize(align);
	{
		int ct = 0;
		float angle = TWOPI/float(NUM_SEGS) ;
		Matrix3 rtm = RotAngleAxisMatrix(align, angle);
		Point3 p(0.0f,0.0f,0.0f);
		if (align.x == 1.0f)
		{
			p.z = length;
		}
		else if (align.y == 1.0f)
		{
			p.x = length;
		}
		else if (align.z == 1.0f)
		{
			p.y = length;
		}
		else if (align.x == -1.0f)
		{
			p.z = -length;
		}
		else if (align.y == -1.0f)
		{
			p.x = -length;
		}
		else if (align.z == -1.0f)
		{
			p.y = -length;
		}
		else
		{
			p = Normalize(align^Point3(1.0f,0.0f,0.0f))*length;
		}

		for (int i=0; i<NUM_SEGS; i++) {
			p = p * rtm;
			plist[ct++] = p;
		}

		p = p * rtm;
		plist[ct++] = p;

		for (int i=0; i<NUM_SEGS+1; i++)
		{
			plist[i].x += a.x;
			plist[i].y += a.y;
			plist[i].z += a.z;
		}
	}
	mka = plist[15];
	mkb = plist[3];
	mkc = plist[7];
	mkd = plist[11];

	for (int i = 0; i < NUM_SEGS; i++)
		gw->segment(&plist[i],1);

	crossSectionVerts.Append(1,&mka,100);
	crossSectionVerts.Append(1,&mkb,100);
	crossSectionVerts.Append(1,&mkc,100);
	crossSectionVerts.Append(1,&mkd,100);
}

void BonesDefMod::DrawCrossSectionNoMarkers(Point3 a, Point3 align, float length, GraphicsWindow *gw)
{
#define NNUM_SEGS	8

	Point3 plist[NNUM_SEGS+1];

	align = Normalize(align);
	{
		float angle = TWOPI/float(NNUM_SEGS) ;
		Matrix3 rtm = RotAngleAxisMatrix(align, angle);
		Point3 p(0.0f,0.0f,0.0f);
		if (align.x == 1.0f)
		{
			p.z = length;
		}
		else if (align.y == 1.0f)
		{
			p.x = length;
		}
		else if (align.z == 1.0f)
		{
			p.y = length;
		}
		else if (align.x == -1.0f)
		{
			p.z = -length;
		}
		else if (align.y == -1.0f)
		{
			p.x = -length;
		}
		else if (align.z == -1.0f)
		{
			p.y = -length;
		}
		else
		{
			p = Normalize(align^Point3(1.0f,0.0f,0.0f))*length;
		}

		int ct = 0;
		for (int i=0; i<NNUM_SEGS; i++) {
			p = p * rtm;
			plist[ct++] = p;
		}

		p = p * rtm;
		plist[ct++] = p;

		for (int i=0; i<NNUM_SEGS+1; i++)
		{
			plist[i].x += a.x;
			plist[i].y += a.y;
			plist[i].z += a.z;
		}
	}

	for (int i = 0; i < NNUM_SEGS; i++)
		gw->segment(&plist[i],1);
}

void BonesDefMod::DrawEndCrossSection(Point3 a, Point3 align, float length,  Matrix3 tm, GraphicsWindow *gw)
{
#define NUM_SEGS	16
	Point3 p_edge[4];
	Point3 plist[NUM_SEGS+1];
	GetCrossSectionLocal(a,align, length,  p_edge);

	int ct = 0;
	float angle = TWOPI/float(NUM_SEGS) *.5f;

	align = Normalize(p_edge[1]-a);

	Matrix3 rtm = RotAngleAxisMatrix(align, angle);
	Point3 p(0.0f,0.0f,0.0f);
	p = p_edge[0]-a;

	plist[0] = p;

	for (int i=1; i<(NUM_SEGS+1); i++)
	{
		p = p * rtm;
		plist[i] = p;
	}

	for (int i=0; i<(NUM_SEGS+1); i++)
	{
		plist[i].x += a.x;
		plist[i].y += a.y;
		plist[i].z += a.z;
	}

	gw->polyline((NUM_SEGS+1), plist, NULL, NULL, 0);

	align = Normalize(p_edge[2]-a);

	rtm = RotAngleAxisMatrix(align, angle);

	p = p_edge[1]-a;

	plist[0] = p;

	for ( int i=1; i<(NUM_SEGS+1); i++)
	{
		p = p * rtm;
		plist[i] = p;
	}

	for (int i=0; i<(NUM_SEGS+1); i++)
	{
		plist[i].x += a.x;
		plist[i].y += a.y;
		plist[i].z += a.z;
	}

	gw->polyline((NUM_SEGS+1), plist, NULL, NULL, 0);
}

void BonesDefMod::GetCrossSectionLocal(Point3 a, Point3 align, float length, Point3 *p_edge)
{
#define GNUM_SEGS	4

	Point3 plist[GNUM_SEGS];

	align = Normalize(align);

	{
		int ct = 0;
		float angle = TWOPI/float(GNUM_SEGS) ;
		Matrix3 rtm = RotAngleAxisMatrix(align, angle);
		Point3 p(0.0f,0.0f,0.0f);
		if (align.x == 1.0f)
		{
			p.z = length;
		}
		else if (align.y == 1.0f)
		{
			p.x = length;
		}
		else if (align.z == 1.0f)
		{
			p.y = length;
		}
		else if (align.x == -1.0f)
		{
			p.z = -length;
		}
		else if (align.y == -1.0f)
		{
			p.x = -length;
		}
		else if (align.z == -1.0f)
		{
			p.y = -length;
		}
		else
		{
			p = Normalize(align^Point3(1.0f,0.0f,0.0f))*length;
		}

		for (int i=0; i<GNUM_SEGS; i++) {
			p = p * rtm;
			plist[ct++] = p;
		}

		for (int i=0; i<GNUM_SEGS; i++)
		{
			plist[i].x += a.x;
			plist[i].y += a.y;
			plist[i].z += a.z;
			p_edge[i] = plist[i];
		}
	}
}

void BonesDefMod::GetCrossSection(Point3 a, Point3 align, float length,  Matrix3 tm,  Point3 *p_edge, int pointCount)
{
	Point3* plist = new Point3[pointCount];

	align = Normalize(align);
	{
		int ct = 0;
		float angle = TWOPI/float(pointCount) ;
		Matrix3 rtm = RotAngleAxisMatrix(align, angle);
		Point3 p(0.0f,0.0f,0.0f);
		if (align.x == 1.0f)
		{
			p.z = length;
		}
		else if (align.y == 1.0f)
		{
			p.x = length;
		}
		else if (align.z == 1.0f)
		{
			p.y = length;
		}
		else if (align.x == -1.0f)
		{
			p.z = -length;
		}
		else if (align.y == -1.0f)
		{
			p.x = -length;
		}
		else if (align.z == -1.0f)
		{
			p.y = -length;
		}
		else
		{
			p = Normalize(align^Point3(1.0f,0.0f,0.0f))*length;
		}

		for (int i=0; i<pointCount; ++i)
		{
			p = p * rtm;
			plist[ct++] = p;
		}

		for (int i=0; i<pointCount; ++i)
		{
			plist[i].x += a.x;
			plist[i].y += a.y;
			plist[i].z += a.z;
			p_edge[i] = plist[i];
		}
	}

	if (plist)
	{
		delete[] plist;
	}
}

void BonesDefMod::DrawEnvelope(Tab<Point3> a, Tab<float> length, int count, Matrix3 tm, GraphicsWindow *gw)
{
#define NUM_SEGS	16
	Point3 plist[NUM_SEGS+1];
	Point3 p_env[2];
	Point3 pa_prev,pb_prev,pc_prev,pd_prev;

	Point3 align = Normalize(a[1] - a[0]);

	for (int j = 0; j < count; j++)
	{
		if (j == 0)
		{
			//draw top arcs
			DrawEndCrossSection(a[j], align, length[j], tm, gw);
		}
		else if (j == (count -1))
		{
			//draw bottom arcs
			Point3 align2 = Normalize(a[0] - a[1]);

			DrawEndCrossSection(a[j], align2, length[j], tm, gw);
		}

		Point3 p[4];

		GetCrossSection(a[j], align, length[j],  tm,  p, 4);
		if (j == 0)
		{
			pa_prev =p[0];
			pb_prev =p[1];
			pc_prev =p[2];
			pd_prev =p[3];
		}
		else
		{
			p_env[0] = pa_prev;
			p_env[1] = p[0];
			gw->polyline(2, p_env, NULL, NULL, 0);
			pa_prev = p[0];

			p_env[0] = pb_prev;
			p_env[1] = p[1];
			gw->polyline(2, p_env, NULL, NULL, 0);
			pb_prev = p[1];

			p_env[0] = pc_prev;
			p_env[1] = p[2];
			gw->polyline(2, p_env, NULL, NULL, 0);
			pc_prev = p[2];

			p_env[0] = pd_prev;
			p_env[1] = p[3];
			gw->polyline(2, p_env, NULL, NULL, 0);
			pd_prev = p[3];
		}
	}
}

int BonesDefMod::Display(
	TimeValue t, INode* inode, ViewExp *vpt,
	int flagst, ModContext *mc)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	//this is a hack to make sure we are only drawing selected objects
	//if skin is instances, this will prevent skins that are not selected from being redrawn

	if (inode)
	{
		if (ip)
		{
			int nodeCount = ip->GetSelNodeCount();
			BOOL found = FALSE;
			for (int nct =0; nct < nodeCount; nct++)
			{
				if (inode == ip->GetSelNode(nct))
				{
					found = TRUE;
					nct = nodeCount;
				}
			}
			if (!found) return 0;
		}
	}

	BoneModData *bmd = (BoneModData *) mc->localData;

	if (!bmd) return 0;

	//this makes sure our caches are in sync
	if (bmd->tmCacheToObjectSpace.Count() != BoneData.Count())
		return 0;

	//this is used to check to see if we are in world space from having a wsm on top
	//if so I need to transfer my local space points into world space other wise they will not line up
	Interval iv;
	Matrix3 atm = inode->GetObjTMAfterWSM(t,&iv);
	Matrix3 ctm = inode->GetObjectTM(t,&iv);
	BOOL isWorldSpace = FALSE;
	if ((atm.IsIdentity()) && (ip->GetShowEndResult ()))
		isWorldSpace = TRUE;

	GraphicsWindow *gw = vpt->getGW();
	Point3 pt[4];
	Matrix3 tm = CompMatrix(t,inode,mc);
	int savedLimits;

	//mirror
	if (mirrorData.Enabled())
	{
		mirrorData.DisplayMirrorData(gw);
		return 0;
	}

	DWORD shadeMethod = gw->getRndMode();

	gw->setRndLimits((savedLimits = gw->getRndLimits()) & ~GW_ILLUM);
	gw->setTransform(tm);

	//get selected bone
	int fsel = mParameterListSelection;
	int tsel = ConvertListIDToBoneIndex(fsel);

	Point3 selGizmoColor = GetUIColor(COLOR_SEL_GIZMOS);
	Point3 gizmoColor = GetUIColor(COLOR_GIZMOS);

	//put in mirror check here
	int mirroredBone = -1;
	if (pPainterInterface)
	{
		if (pPainterInterface->GetMirrorEnable() && pPainterInterface->InPaintMode())
		{
			mirroredBone = mirrorIndex;
		}
	}

	if ((tsel>=0) && (ip && ip->GetSubObjectLevel() == 1) )
	{
		//draw 	gizmos
		if (pblock_gizmos->Count(skin_gizmos_list) > 0)
		{
			ReferenceTarget *ref;

			if (displayAllGizmos)
			{
				for (int currentGiz = 0; currentGiz < pblock_gizmos->Count(skin_gizmos_list); currentGiz++)
				{
					if (currentSelectedGizmo == currentGiz)
					{
						Point3 gizColor = selGizmoColor;
						float r = gizColor.x;
						float g = gizColor.y;
						float b = gizColor.z;

						gw->setColor(LINE_COLOR, r,g,b);
					}
					else
					{
						Point3 gizColor = gizmoColor;
						float r = gizColor.x;
						float g = gizColor.y;
						float b = gizColor.z;

						gw->setColor(LINE_COLOR, r,g,b);
					}
					ref = pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,currentGiz);
					GizmoClass *gizmo = (GizmoClass *)ref;
					if (gizmo)
						gizmo->Display(t,gw, Inverse(tm));
				}
			}
			else
			{
				if ((currentSelectedGizmo >= 0) && (currentSelectedGizmo<pblock_gizmos->Count(skin_gizmos_list)))
				{
					ref = pblock_gizmos->GetReferenceTarget(skin_gizmos_list,0,currentSelectedGizmo);
					GizmoClass *gizmo = (GizmoClass *)ref;
					if (gizmo)
					{
						Point3 gizColor = selGizmoColor;
						float r = gizColor.x;
						float g = gizColor.y;
						float b = gizColor.z;

						gw->setColor(LINE_COLOR, r,g,b);

						gizmo->Display(t,gw, Inverse(tm));
						if (gizmo->IsEditing())
						{
							gw->setRndLimits(savedLimits);
							return 0;
						}
					}
				}
			}
		}

		ObjectState os;

		os = inode->EvalWorldState(t);
		//loop through points checking for selection and then marking
		float r,g,b;

		BOOL isPatch = FALSE;
		int knots = 0;
		PatchMesh *pmesh = NULL;

		Interval iv;
		Matrix3 atm = inode->GetObjTMAfterWSM(t,&iv);
		Matrix3 btm = inode->GetObjTMBeforeWSM(t,&iv);

		if (os.obj->IsSubClassOf(patchObjectClassID))
		{
			PatchObject *pobj = (PatchObject*)os.obj;
			pmesh = &(pobj->patch);
			isPatch = TRUE;
			knots = pmesh->numVerts;
		}
		//just grabs our color weights
		Point3 selSoft = GetUIColor(COLOR_SUBSELECTION_SOFT);
		Point3 selMedium = GetUIColor(COLOR_SUBSELECTION_MEDIUM);
		Point3 selHard = GetUIColor(COLOR_SUBSELECTION_HARD);
		Point3 subSelColor = GetUIColor(COLOR_SUBSELECTION);

		//bug fix 276830 Jan 29 2001
		//this is the case when you have a topology changing modifier above skin

		BOOL difTopology = FALSE;
		if (os.obj->NumPoints() != bmd->VertexData.Count())
			difTopology = TRUE;

		BitArray visibleVerts;
		visibleVerts.SetSize(bmd->VertexData.Count());
		visibleVerts.SetAll();

		BOOL showHiddenVerts;
		pblock_display->GetValue(skin_display_showhiddenvertices,t,showHiddenVerts,FOREVER);
		if (!showHiddenVerts)
		{
			for (int i = 0; i < bmd->VertexData.Count(); i++)
			{
				if (bmd->VertexData[i]->IsHidden())
					visibleVerts.Set(i,FALSE);
			}
		}

		if ((GW_WIREFRAME&shadeMethod) || (!shadeVC))
		{
			gw->startMarkers();
			for (int i=0;i<bmd->VertexData.Count();i++)
			{
				for (int j=0;j<bmd->VertexData[i]->WeightCount();j++)
				{
					int rigidBoneID = 0;
					if (rigidVerts)
						rigidBoneID = bmd->VertexData[i]->GetMostAffectedBone();
					if (( (bmd->VertexData[i]->GetBoneIndex(j) == tsel) || (bmd->VertexData[i]->GetBoneIndex(j) == mirroredBone))
						&& (bmd->VertexData[i]->GetWeight(j) != 0.0f)
						&& (DrawVertices ==1)
						)

					{
						if (!((rigidVerts) && (tsel != rigidBoneID)))
						{
							Point3 pt;
							//bug fix 276830 Jan 29 2001
							if (difTopology)
								pt = bmd->VertexData[i]->LocalPos;
							else pt = os.obj->GetPoint(i);

							if (isWorldSpace)
								pt = pt *Inverse(ctm);

							float infl;

							infl = RetrieveNormalizedWeight(bmd,i,j);
							if (rigidVerts) infl = 1.0f;
							Point3 selColor(0.0f,0.0f,0.0f);

							if (infl > 0.0f)
							{
								if ( (infl<0.33f) && (infl > 0.0f))
								{
									selColor = selSoft + ( (selMedium-selSoft) * (infl/0.33f));
								}
								else if (infl <.66f)
								{
									selColor = selMedium + ( (selHard-selMedium) * ((infl-0.1f)/0.66f));
								}
								else if (infl < 0.99f)
								{
									selColor = selHard + ( (subSelColor-selHard) * ((infl-0.66f)/0.33f));
								}

								else
								{
									selColor = subSelColor;
								}

								r = selColor.x;
								g = selColor.y;
								b = selColor.z;

								gw->setColor(LINE_COLOR, r,g,b);
								if (visibleVerts[i])
								{
									if (isPatch)
									{
										if (i< knots)
										{
											gw->marker(&pt,DOT_MRKR);
										}
										else
										{
											if (!bmd->autoInteriorVerts[i])
												gw->marker(&pt,DOT_MRKR);
										}
									}
									else gw->marker(&pt,DOT_MRKR);
								}
								j = bmd->VertexData[i]->WeightCount()+1;
							}
						}
					}
					else if (drawAllVertices)
					{
						Point3 pt;
						//bug fix 276830 Jan 29 2001
						if (difTopology)
							pt = bmd->VertexData[i]->LocalPos;
						else pt = os.obj->GetPoint(i);

						if (isWorldSpace)
							pt = pt *Inverse(ctm);
						if (visibleVerts[i])
						{
							gw->setColor(LINE_COLOR, 1.0f,1.0f,1.0f);
							gw->marker(&pt,POINT_MRKR);
						}
					}
				}
			}
			gw->endMarkers();
		}
		//DRAW vertex selection

		gw->startMarkers();

		int vct = bmd->VertexData.Count();
		for (int i=0;i< vct; i++)
		{
			if (bmd->selected[i] == TRUE)
			{
				gw->setColor(LINE_COLOR, 1.0f,1.0f,1.0f);
				Point3 pt;
				//bug fix 276830 Jan 29 2001
				if (difTopology)
					pt = bmd->VertexData[i]->LocalPos;
				else pt = os.obj->GetPoint(i);

				if (isWorldSpace)
					pt = pt *Inverse(ctm);
				if (isPatch)
				{
					if (!bmd->autoInteriorVerts[i])
						gw->marker(&pt,HOLLOW_BOX_MRKR);
				}
				else gw->marker(&pt,HOLLOW_BOX_MRKR);
			}

			if (visibleVerts[i])
			{
				if (tsel < bmd->exclusionList.Count() )
				{
					if (bmd->exclusionList[tsel])
					{
						if (bmd->isExcluded(tsel,i))
						{
							Point3 pt;
							//bug fix 276830 Jan 29 2001
							if (difTopology)
								pt = bmd->VertexData[i]->LocalPos;
							else pt = os.obj->GetPoint(i);

							if (isWorldSpace)
								pt = pt *Inverse(ctm);

							gw->setColor(LINE_COLOR, 0.2f,0.2f,0.2f);
							gw->marker(&pt,SM_DIAMOND_MRKR);
						}
					}
				}
			}
		}
		gw->endMarkers();

		if (isPatch)
		{
			for (int i=0;i<bmd->VertexData.Count();i++)
			{
				if (!visibleVerts[i]) continue;

				//draws all weighed vertices for the selected bones
				for (int j=0;j<bmd->VertexData[i]->WeightCount();j++)
				{
					int rigidBoneID = 0;
					if (rigidVerts)
						rigidBoneID = bmd->VertexData[i]->GetMostAffectedBone();
					if ( ( (bmd->VertexData[i]->GetBoneIndex(j) == tsel) || (bmd->VertexData[i]->GetBoneIndex(j) == mirroredBone))
						&& (bmd->VertexData[i]->GetWeight(j) != 0.0f) &&
						(DrawVertices ==1) )
					{
						if (!((rigidVerts) && (tsel != rigidBoneID)))
						{
							Point3 pt;
							//bug fix 276830 Jan 29 2001
							if (difTopology)
								pt = bmd->VertexData[i]->LocalPos;
							else pt = os.obj->GetPoint(i);

							if (isWorldSpace)
								pt = pt *Inverse(ctm);

							float infl;

							infl = RetrieveNormalizedWeight(bmd,i,j);
							if (rigidVerts) infl = 1.0f;

							if (infl > 0.0f)
							{
								if (isPatch)
								{
									if (i< knots)
									{
										if (bmd->selected[i] == FALSE)
										{
											//it is a knot draw the handle
											PatchVert pv = pmesh->getVert(i);
											Point3 lp[3];
											lp[0] = pt;
											gw->setColor(LINE_COLOR, 0.8f,0.8f,0.8f);

											for (int vec_count = 0; vec_count < pv.vectors.Count(); vec_count++)
											{
												int	idv = pv.vectors[vec_count];
												if (isWorldSpace)
													lp[1] = pmesh->getVec(idv).p * Inverse(ctm);
												else lp[1] = pmesh->getVec(idv).p;
												gw->polyline(2, lp, NULL, NULL, 0);
											}
										}
									}
								}

								j = bmd->VertexData[i]->WeightCount()+1;
							}
						}
					}
				}

				//draws out selected vertices
				if (bmd->selected[i] == TRUE)
				{
					Point3 pt;
					//bug fix 276830 Jan 29 2001
					if (difTopology)
						pt = bmd->VertexData[i]->LocalPos;
					else pt = os.obj->GetPoint(i);

					if (isWorldSpace)
						pt = pt *Inverse(ctm);

					gw->setColor(LINE_COLOR, 1.0f,1.0f,1.0f);

					if ((i< knots) && (isPatch))
					{
						//it is a knot draw the handle
						PatchVert pv = pmesh->getVert(i);
						Point3 lp[3];
						lp[0] = pt;
						gw->setColor(LINE_COLOR, 0.8f,0.8f,0.8f);

						for (int vec_count = 0; vec_count < pv.vectors.Count(); vec_count++)
						{
							int idv = pv.vectors[vec_count];
							if (isWorldSpace)
								lp[1] = pmesh->getVec(idv).p * Inverse(ctm);
							else lp[1] = pmesh->getVec(idv).p;
							gw->polyline(2, lp, NULL, NULL, 0);
						}
					}
					else if (isPatch)
					{
						Point3 lp[3];
						lp[0] = pt;
						int vecIndex = i - knots;
						PatchVec pv = pmesh->getVec(vecIndex);
						int vertIndex = pv.vert;
						if ((vertIndex != -1) && (!bmd->selected[vertIndex]) && (pv.flags != PVEC_INTERIOR))
						{
							if (isWorldSpace)
								lp[1] = pmesh->getVert(vertIndex).p * Inverse(ctm);
							else lp[1] = pmesh->getVert(vertIndex).p;
							gw->setColor(LINE_COLOR, 0.8f,0.8f,0.8f);
							gw->polyline(2, lp, NULL, NULL, 0);
						}
					}
				}
				else if (drawAllVertices)
				{
					Point3 pt;

					if (difTopology)
						pt = bmd->VertexData[i]->LocalPos;
					else pt = os.obj->GetPoint(i);

					if (isWorldSpace)
						pt = pt *Inverse(ctm);

					if ((i< knots) && (isPatch))
					{
						//it is a knot draw the handle
						PatchVert pv = pmesh->getVert(i);
						Point3 lp[3];
						lp[0] = pt;
						gw->setColor(LINE_COLOR, 0.8f,0.8f,0.8f);

						for (int vec_count = 0; vec_count < pv.vectors.Count(); vec_count++)
						{
							int idv = pv.vectors[vec_count];
							if (isWorldSpace)
								lp[1] = pmesh->getVec(idv).p * Inverse(ctm);
							else lp[1] = pmesh->getVec(idv).p;
							gw->polyline(2, lp, NULL, NULL, 0);
						}
					}
				}
			}
		}

		//draw selected bone
		BOOL envOnTop;
		BOOL crossOnTop;
		pblock_display->GetValue(skin_display_envelopesalwaysontop,0,envOnTop,FOREVER);
		pblock_display->GetValue(skin_display_crosssectionsalwaysontop,0,crossOnTop,FOREVER);

		BOOL showEnvelopes;
		pblock_display->GetValue(skin_display_shownoenvelopes,0,showEnvelopes,FOREVER);
		showEnvelopes = !showEnvelopes;

		//draw the end points
		if (envOnTop)
			gw->setRndLimits(savedLimits & ~GW_ILLUM & ~GW_Z_BUFFER);
		else gw->setRndLimits(savedLimits & ~GW_ILLUM & GW_Z_BUFFER);

		gw->startMarkers();
		for (int i =0;i<BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node != NULL)
			{
				if (i== tsel)
				{
					r = 1.0f;
					g = 1.0f;
					b = 0.0f;
				}
				else
				{
					r = 0.3f;
					g = 0.3f;
					b = 0.3f;
				}

				Point3 pta, ptb;
				Point3 pta_tm,ptb_tm;
				Point3 plist[2];

				GetEndPointsLocal(bmd, t,pta, ptb, i);

				gw->setColor(LINE_COLOR, r,g,b);

				if (boneData.flags & BONE_SPLINE_FLAG)
				{
					ShapeObject *pathOb = NULL;

					ObjectState os = boneData.Node->EvalWorldState(t);
					pathOb = (ShapeObject*)os.obj;

					float su = 0.0f;
					float eu = 0.1f;
					float inc = 0.1f;
					Point3 sp_line[10];

					if (pathOb->NumberOfCurves(t) != 0)
					{
						Point3 l1 = pathOb->InterpPiece3D(t, 0,0 ,0.0f ) * bmd->tmCacheToObjectSpace[i];
						Point3 l2 = pathOb->InterpPiece3D(t, 0,0 ,1.0f ) * bmd->tmCacheToObjectSpace[i];

						pta = l1;
						ptb = l2;

						plist[0] = pta;
						plist[1] = ptb;
					}
				}
				else
				{
					Point3 invA,invB;

					invA = (ptb-pta) *.1f;
					invB = (pta-ptb) *.1f;

					plist[0] = pta + invA;
					plist[1] = ptb + invB;
				}

				if ((boneData.end1Selected) && (i== tsel))
				{
					gw->setColor(LINE_COLOR, 1.0f,0.0f,1.0f);
					gw->marker(&plist[0],DOT_MRKR);
				}
				else
				{
					gw->setColor(LINE_COLOR, .3f,.3f,0.3f);
					gw->marker(&plist[0],DOT_MRKR);
				}

				if ((boneData.end2Selected) && (i== tsel))
				{
					gw->setColor(LINE_COLOR, 1.0f,0.0f,1.0f);
					gw->marker(&plist[1],DOT_MRKR);
				}
				else
				{
					gw->setColor(LINE_COLOR, .3f,.3f,0.3f);
					gw->marker(&plist[1],DOT_MRKR);
				}

				if (i == ModeBoneIndex)
				{
					Worldl1 = plist[0] * tm;
					Worldl2 = plist[1] * tm;
				}
			}
		}
		gw->endMarkers();

		//draw the bone envelopes && crosssections
		gw->startSegments();

		for (int i =0;i<BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node != NULL)
			{
				if (i== tsel)
				{
					r = 1.0f;
					g = 1.0f;
					b = 0.0f;
				}
				else
				{
					r = 0.3f;
					g = 0.3f;
					b = 0.3f;
				}

				Point3 pta, ptb;
				Point3 pta_tm,ptb_tm;
				Point3 plist[2];

				GetEndPointsLocal(bmd, t,pta, ptb, i);

				gw->setColor(LINE_COLOR, r,g,b);

				ObjectState os;
				ShapeObject *pathOb = NULL;

				if (boneData.flags & BONE_SPLINE_FLAG)
				{
					ObjectState os = boneData.Node->EvalWorldState(t);
					pathOb = (ShapeObject*)os.obj;

					float su = 0.0f;
					float eu = 0.1f;
					float inc = 0.1f;
					Point3 sp_line[10];

					Point3 l1,l2;

					if (pathOb->NumberOfCurves(t) != 0)
					{
						l1 = pathOb->InterpPiece3D(t, 0,0 ,0.0f ) * bmd->tmCacheToObjectSpace[i];
						l2 = pathOb->InterpPiece3D(t, 0,0 ,1.0f ) * bmd->tmCacheToObjectSpace[i];

						pta = l1;
						ptb = l2;

						plist[0] = pta;
						plist[1] = ptb;

						for (int cid = 0; cid < pathOb->NumberOfCurves(t); cid++)
						{
							for (int sid = 0; sid < pathOb->NumberOfPieces(t,cid); sid++)
							{
								float su = 0.0f;
								inc = 1.0f/4.0f;
								for (int spid = 0; spid < 5; spid++)
								{									
									sp_line[spid] = pathOb->InterpPiece3D(t, cid,sid ,su ) * bmd->tmCacheToObjectSpace[i];  //optimize reduce the count here
									su += inc;
									if (spid > 0)
										gw->segment(&sp_line[spid-1],1);
								}
							}
						}
					}
				}
				else
				{
					Point3 invA,invB;

					invA = (ptb-pta) *.1f;
					invB = (pta-ptb) *.1f;

					plist[0] = pta + invA;
					plist[1] = ptb + invB;

					gw->segment(plist,1);
				}
			}
		}
		gw->endSegments();

		if (crossOnTop)
			gw->setRndLimits(savedLimits & ~GW_ILLUM & ~GW_Z_BUFFER);
		else gw->setRndLimits(savedLimits & ~GW_ILLUM & GW_Z_BUFFER);

		crossSectionVerts.SetCount(0);

		gw->startSegments();
		for (int i =0;i<BoneData.Count();i++)
		{
			BoneDataClass &boneData = BoneData[i];
			if (boneData.Node != NULL)
			{
				if (showEnvelopes)
				{
					Point3 pta, ptb;
					Point3 plist[2];

					GetEndPointsLocal(bmd, t,pta, ptb, i);

					ShapeObject *pathOb = NULL;
					if (boneData.flags & BONE_SPLINE_FLAG)
					{
						ObjectState os = boneData.Node->EvalWorldState(t);
						pathOb = (ShapeObject*)os.obj;
					}

					//Draw Cross Sections
					Tab<Point3> CList;
					Tab<float> InnerList, OuterList;

					for (int ccount = 0; ccount < boneData.CrossSectionList.Count();ccount++)
					{
						Point3 m;
						float inner;
						float outer;
						Interval v;

						BoneDataClass *bd = &boneData;

						boneData.CrossSectionList[ccount].InnerControl->GetValue(currentTime,&inner,v);
						boneData.CrossSectionList[ccount].OuterControl->GetValue(currentTime,&outer,v);

						GetCrossSectionRanges(inner, outer, i, ccount);

						if (tsel == i)
						{
							gw->setColor(LINE_COLOR, 1.0f,0.0f,0.0f);
							if ( (ModeBoneEnvelopeIndex == ccount) && (ModeBoneEnvelopeSubType<4))
								gw->setColor(LINE_COLOR, 1.0f,0.0f,1.0f);
						}

						Point3 nvec;
						Matrix3 rtm;
						if ((DrawEnvelopes ==1) || (tsel == i) || (boneData.flags & BONE_DRAW_ENVELOPE_FLAG) || (i==mirroredBone) )
						{
							Point3 vec;

							InnerList.Append(1,&inner,1);
							OuterList.Append(1,&outer,1);

							if ((pathOb) && (pathOb->NumberOfCurves(t) != 0) &&(boneData.flags & BONE_SPLINE_FLAG))
							{
								vec = pathOb->InterpCurve3D(t, 0,boneData.CrossSectionList[ccount].u) * bmd->tmCacheToObjectSpace[i];

								CList.Append(1,&(vec),1);
								nvec = VectorTransform(bmd->tmCacheToObjectSpace[i],pathOb->TangentCurve3D(t, 0,boneData.CrossSectionList[ccount].u));
								DrawCrossSection(vec, nvec, inner, boneData.temptm, gw);  // optimize these can be moved out of the loop
							}
							else
							{
								nvec = (ptb-pta);
								vec = nvec * boneData.CrossSectionList[ccount].u;
								CList.Append(1,&(pta+vec),1);
								DrawCrossSection(pta+vec, nvec, inner, boneData.temptm, gw); // optimize these can be moved out of the loop
							}
						}
						if (tsel == i)
						{
							gw->setColor(LINE_COLOR, 0.5f,0.0f,0.0f);
							if ( (ModeBoneEnvelopeIndex == ccount) && (ModeBoneEnvelopeSubType>=4))
								gw->setColor(LINE_COLOR, 1.0f,0.0f,1.0f);
						}

						if ((DrawEnvelopes ==1) || (tsel == i) || (boneData.flags & BONE_DRAW_ENVELOPE_FLAG))
						{
							Point3 vec;
							if ((pathOb) && (pathOb->NumberOfCurves(t) != 0) && (boneData.flags & BONE_SPLINE_FLAG))
							{
								vec = pathOb->InterpCurve3D(t, 0,boneData.CrossSectionList[ccount].u) * bmd->tmCacheToObjectSpace[i];
								nvec = VectorTransform(bmd->tmCacheToObjectSpace[i],pathOb->TangentCurve3D(t, 0,boneData.CrossSectionList[ccount].u));  // optimize these can be moved out of the loop
								DrawCrossSection(vec, nvec, outer, boneData.temptm, gw);
							}
							else
							{
								nvec = (ptb-pta);
								vec = nvec * boneData.CrossSectionList[ccount].u;
								DrawCrossSection(pta+vec, nvec, outer, boneData.temptm, gw);
							}
						}
					}

					if ((DrawEnvelopes ==1) || (tsel == i) || (boneData.flags & BONE_DRAW_ENVELOPE_FLAG) || (i==mirroredBone))
					{
						if (!(boneData.flags & BONE_SPLINE_FLAG))
						{
							if (i==mirroredBone)
								gw->setColor(LINE_COLOR, 0.2f,0.0f,0.0f);
							else gw->setColor(LINE_COLOR, 1.0f,0.0f,0.0f);
							DrawEnvelope(CList, InnerList, CList.Count(), boneData.temptm,  gw);
							if (i==mirroredBone)
								gw->setColor(LINE_COLOR, 0.2f,0.0f,0.0f);
							else gw->setColor(LINE_COLOR, 0.5f,0.0f,0.0f);
							DrawEnvelope(CList, OuterList, CList.Count(), boneData.temptm,  gw);
						}
					}
				}
			}
		}
		gw->endSegments();

		gw->startMarkers();
		gw->setColor(LINE_COLOR, .3f,.3f,0.3f);
		for (int i =0;i<crossSectionVerts.Count();i++)
		{
			gw->marker(&crossSectionVerts[i],DOT_MRKR);
		}
		gw->endMarkers();

		//WEIGHTTABLE
		//DRAW IN WEIGHT TABLE VERTS
		BOOL showWT = FALSE;
		Point3  wt_markerColor;
		int wt_markerType;
		this->pblock_weighttable->GetValue(skin_wt_showmarker,0,showWT,FOREVER);
		if ( (showWT) && (weightTableWindow.hWnd) )
		{
			this->pblock_weighttable->GetValue(skin_wt_markertype,0,wt_markerType,FOREVER);
			this->pblock_weighttable->GetValue(skin_wt_markercolor,0,wt_markerColor,FOREVER);

			gw->setColor(LINE_COLOR, wt_markerColor.x,wt_markerColor.y,wt_markerColor.z);
			gw->startMarkers();
			for (int i = 0; i < weightTableWindow.vertexPtrList.Count(); i++)
			{
				if ( (bmd == weightTableWindow.vertexPtrList[i].bmd) &&
					(weightTableWindow.vertexPtrList[i].IsSelected() ) )
				{
					int index = weightTableWindow.vertexPtrList[i].index;
					Point3 pt;
					if (isWorldSpace)
						pt = os.obj->GetPoint(index) *Inverse(ctm);
					else pt = os.obj->GetPoint(index);
					if (wt_markerType == 0)
						gw->marker(&pt,POINT_MRKR);
					else if (wt_markerType == 1)
						gw->marker(&pt,HOLLOW_BOX_MRKR);
					else if (wt_markerType == 2)
						gw->marker(&pt,PLUS_SIGN_MRKR);
					else if (wt_markerType == 3)
						gw->marker(&pt,ASTERISK_MRKR);
					else if (wt_markerType == 4)
						gw->marker(&pt,X_MRKR);
					else if (wt_markerType == 5)
						gw->marker(&pt,BIG_BOX_MRKR);
					else if (wt_markerType == 6)
						gw->marker(&pt,CIRCLE_MRKR);
					else if (wt_markerType == 7)
						gw->marker(&pt,TRIANGLE_MRKR);
					else if (wt_markerType == 8)
						gw->marker(&pt,DIAMOND_MRKR);
					else if (wt_markerType == 9)
						gw->marker(&pt,SM_HOLLOW_BOX_MRKR);
					else if (wt_markerType == 10)
						gw->marker(&pt,SM_CIRCLE_MRKR);
					else if (wt_markerType == 11)
						gw->marker(&pt,SM_TRIANGLE_MRKR);
					else if (wt_markerType == 12)
						gw->marker(&pt,SM_DIAMOND_MRKR);
					else if (wt_markerType == 13)
						gw->marker(&pt,DOT_MRKR);
					else if (wt_markerType == 14)
						gw->marker(&pt,SM_DOT_MRKR);
				}
			}
			gw->endMarkers();
		}
	}

	gw->setRndLimits(savedLimits);

	return 0;
}

void BonesDefMod::SetHiddenVerts(TimeValue t, BoneModData *bmd, Object *obj)
{
	if (bmd->VertexData.Count() == 0) return;

	if (obj->IsSubClassOf(triObjectClassID))
	{
		TriObject *tobj = (TriObject*)obj;
		Mesh *msh = &tobj->GetMesh();

		int hiddenSize = msh->vertHide.GetSize();
		for (int i = 0; i < bmd->VertexData.Count(); i++)
		{
			if (i < hiddenSize)
			{
				if (msh->vertHide[i])
					bmd->VertexData[i]->Hide(TRUE);
				else bmd->VertexData[i]->Hide(FALSE);
			}
		}
	}

	else if (obj->IsSubClassOf(polyObjectClassID))
	{
		PolyObject *tobj = (PolyObject*)obj;

		MNMesh &mesh = tobj->GetMesh();

		for (int i = 0; i < mesh.numv; i++)
		{
			if (mesh.v[i].GetFlag(MN_HIDDEN))
				bmd->VertexData[i]->Hide(TRUE);
			else bmd->VertexData[i]->Hide(FALSE);
		}
	}
	else if (obj->IsSubClassOf(patchObjectClassID))
	{
		PatchObject *pobj = (PatchObject*)obj;
		PatchMesh &msh = pobj->patch;

		for (int i = 0; i < msh.numVerts; i++)
		{
			if (msh.verts[i].IsHidden())
				bmd->VertexData[i]->Hide(TRUE);
			else bmd->VertexData[i]->Hide(FALSE);
			int ct= msh.verts[i].vectors.Count();
			for (int j = 0; j < ct; j++)
			{
				int index = msh.verts[i].vectors[j]+msh.numVerts;
				if (msh.verts[i].IsHidden())
					bmd->VertexData[index]->Hide(TRUE);
				else bmd->VertexData[index]->Hide(FALSE);
			}
		}
	}
}
