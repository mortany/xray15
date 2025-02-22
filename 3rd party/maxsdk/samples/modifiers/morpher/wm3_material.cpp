/*===========================================================================*\
 | 
 |  FILE:	wM3_material.cpp
 |			Weighted Morpher for MAX R3
 |			Morph Material plugin component
 | 
 |  AUTH:   Harry Denholm
 |			Copyright(c) Kinetix 1998
 |			All Rights Reserved.
 |
 |  HIST:	Started 21-12-98
 | 
\*===========================================================================*/


#include "wM3.h"
#include "buildver.h"

extern HINSTANCE hInstance;

class M3MatClassDesc:public ClassDesc {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading) {return new M3Mat(loading);}
	// following made a separate resource string, 010809  --prs.
	const TCHAR *	ClassName() {return GetString(IDS_MORPHMTL_OBJECT);}
	SClass_ID		SuperClassID() {return MATERIAL_CLASS_ID;}
	Class_ID 		ClassID() {return M3MatClassID;}
	const TCHAR* 	Category() {return _T("");}
	};

static M3MatClassDesc M3MatCD;

ClassDesc* GetM3MatDesc() {return &M3MatCD;}


M3Mat::~M3Mat()
	{	
//	if(matDlg) delete matDlg; 
	matDlg = NULL;
	
	}


M3Mat::M3Mat(BOOL loading): mReshadeRQ(RR_None)
	{	
	pblockMat = NULL;
	matDlg = NULL;
	morphp = NULL;
	listSel = 0;

	int i;

	mTex.SetCount(101);
	mapOn.SetCount(101);

	for(i=0;i<101;i++)
	{
		mTex[i] = NULL;
	}

	ivalid.SetEmpty();

	for (i=0; i<100; i++) 
		mapOn[i] = 1;

	if (!loading) 
		Reset();
	}

ParamBlockDescID* M3Mat::MakeParamDescr(int channel100Count)
{
	if (channel100Count <= 0) return nullptr;

	ParamBlockDescID *oldDescVer = new ParamBlockDescID[channel100Count*100+1];

	channel100Count--;
	int x;
	for(x=0;x<100;x++) //first 100 float morph blend value
	{
		ParamBlockDescID add;
		add.type=TYPE_FLOAT;
		add.user=NULL;
		add.animatable=TRUE;
		add.id=x;
		oldDescVer[x] = add;
	}

	ParamBlockDescID add;
	add.type=TYPE_INT;
	add.user=NULL;
	add.animatable=FALSE;
	add.id=x;
	oldDescVer[x] = add;	// this is the blend mode

	//now add the remaining float morph blend value
	for(x=101;x<101+channel100Count*100;x++)
	{
		ParamBlockDescID add;
		add.type=TYPE_FLOAT;
		add.user=NULL;
		add.animatable=TRUE;
		add.id=x;
		oldDescVer[x] = add;
	}

	
	return oldDescVer;
}
void M3Mat::Add100Channels()
{
	int originalCount = mTex.Count();
	mTex.SetCount(mTex.Count()+100);
	mapOn.SetCount(mapOn.Count()+100);

	for(int i=originalCount;i<mTex.Count();i++)
	{
		mTex[i] = NULL;
	}

	ivalid.SetEmpty();

	for (int i=originalCount; i<mapOn.Count(); i++) 
		mapOn[i] = 1;

	int hundreCount = originalCount/100;
	
	ParamBlockDescID *oldDescVer = MakeParamDescr(hundreCount);
	ParamBlockDescID *newDescVer = MakeParamDescr(hundreCount+1);



	IParamBlock *newPB = UpdateParameterBlock(	oldDescVer, originalCount, pblockMat,	newDescVer, originalCount+100, 1);
	pblockMat = newPB;

}
/*
int M3Mat::GetChannelID(int refID)
{
	if ( refID > 100)
		refID -= 2;
	return refID;
}
*/
int M3Mat::GetRefID(int matID)
{
	if (matID > 100)
		matID += 2;
	return matID;
}

void* M3Mat::GetInterface(ULONG id)
{
	if( id == IID_IReshading )
		return (IReshading*)( this );
	else
		return Mtl::GetInterface(id);
}

void M3Mat::Reset()
	{

//		TCHAR s[25];

		DeleteReference(101); // is the pblock
		for(int i=0;i<mTex.Count();i++) //0-99  and 103-xxx are the remaining morphs
		{
			int refID = GetRefID(i);
			DeleteReference(refID);
			mTex[i] = NULL;
	//		ReplaceReference(i,NewDefaultStdMat());
	//		_stprintf(s,GetString(IDS_MTL_CNAME),i+1);
	//		mTex[i]->SetName(s);
		}
		mTex.SetCount(101);
		mapOn.SetCount(101);

		ReplaceReference(100,NewDefaultStdMat());  //100 is the base material
		mTex[100]->SetName(GetString(IDS_MTL_BASE));


	ParamBlockDescID *descVer = new ParamBlockDescID[101];

   int x;
	for(x=0;x<100;x++){

		ParamBlockDescID add;

		add.type=TYPE_FLOAT;

		add.user=NULL;

		add.animatable=TRUE;

		add.id=x;

	 descVer[x] = add;

	}

	ParamBlockDescID add;
	add.type=TYPE_INT;
	add.user=NULL;
	add.animatable=FALSE;
	add.id=x;
	descVer[x] = add;	// x == 100 we guess?

	IParamBlock *pblockMat = (IParamBlock*)CreateParameterBlock(descVer,101,1);

	ReplaceReference(101,pblockMat);	
	//ReplaceReference(102,NULL);

	delete [] descVer;

	pblockMat->SetValue(100,0,0);	// set param [100], the mystery param

	}
void M3Mat::SyncChannels(int ct)
{
	while (mTex.Count() < ct)
	{
		Add100Channels();
	}
}
static Color black(0,0,0);

Color M3Mat::GetAmbient(int mtlNum, BOOL backFace) { 
	return mTex[100]?mTex[100]->GetAmbient(mtlNum,backFace):black;
	}		
Color M3Mat::GetDiffuse(int mtlNum, BOOL backFace){ 
	return mTex[100]?mTex[100]->GetDiffuse(mtlNum,backFace):black;
	}				
Color M3Mat::GetSpecular(int mtlNum, BOOL backFace){
	return mTex[100]?mTex[100]->GetSpecular(mtlNum,backFace):black;
	}		
float M3Mat::GetXParency(int mtlNum, BOOL backFace) {
	return mTex[100]?mTex[100]->GetXParency(mtlNum,backFace):0.0f;
	}
float M3Mat::GetShininess(int mtlNum, BOOL backFace) {
	return mTex[100]?mTex[100]->GetXParency(mtlNum,backFace):0.0f;
	}		
float M3Mat::GetShinStr(int mtlNum, BOOL backFace) {
	return mTex[100]?mTex[100]->GetXParency(mtlNum,backFace):0.0f;
	}
float M3Mat::WireSize(int mtlNum, BOOL backFace) {
	return mTex[100]?mTex[100]->WireSize(mtlNum,backFace):0.0f;
	}
		
ParamDlg* M3Mat::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp)
	{
	matDlg = new M3MatDlg(hwMtlEdit, imp, this);
	return matDlg;
	}

void M3Mat::Shade(ShadeContext& sc) {
	int i; 

	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;

	pblockMat->GetValue(100,t,i,FOREVER);

	Mtl *sm1 = mTex[100];
	float total(0.0f);
	ShadeOutput sFinal( sc.out.nElements ); // get nElements correctly
	Tab<ShadeOutput>	sDatabase;
	sDatabase.SetCount(mTex.Count());
//	for(  i = 0; i < 100; ++i )
//		sDatabase[i] = sFinal;	

	Tab<float> u;
	u.SetCount(mTex.Count());

	// handle no base mat
	if(!sm1) 
	{
		sc.ResetOutput();
		sc.out.c = black;
		sc.out.t = black;
		return;
	}

	if(i==0||(i==1&&inRender))
	{
		for( i=0;i<mTex.Count();i++)
		{
			pblockMat->GetValue(i,t,u[i],valid);
			u[i] /= 100.0f;

			if(mTex[i]!=NULL&&u[i]!=0&&mapOn[i] && (i != 100))
			{
				Mtl *comb = mTex[i];
				comb->Shade(sc);
				sDatabase[i] = sc.out;
				sc.ResetOutput();
				total += u[i];
			}
		}

		sc.ResetOutput();
		sm1->Shade(sc);
		sFinal.c = black;
		sFinal.t = black;
		sFinal.ior = 0.0f;

		// LAM - 6/1/04 - 520249 - handle render elements
		for( int j = 0; j < sFinal.nElements; ++j )
			sFinal.elementVals[j] = black;

		// LAM - 6/1/04 - normalize u once
		if(total>1.0f)
			for( i=0;i<mTex.Count();i++)
				u[i] /= total;

		for( i=0;i<mTex.Count();i++)
		{
			if(mTex[i]!=NULL&&u[i]!=0&&mapOn[i] && (i != 100))
			{
				sc.out.flags |= sDatabase[i].flags;
			
				sFinal.c += u[i] * sDatabase[i].c;
				sFinal.t += u[i] * sDatabase[i].t;
				sFinal.ior += u[i] * sDatabase[i].ior;

				// LAM - 6/1/04 - 520249 - handle render elements here and in next code block
				// render elements 
				for( int j = 0; j < sFinal.nElements; ++j )
					sFinal.elementVals[j] += u[i] * sDatabase[i].elementVals[j];
			}
		}
		if(total) {
			sc.out.MixIn(sFinal, 1.0f-total);
		}
		
	}
	else {
		sm1->Shade(sc);
	}
}


void M3Mat::PreShade(ShadeContext& sc, IReshadeFragment* pFrag)
{
	int i; 
	IReshading* pReshading;

	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;

	// get the base material value into i
	pblockMat->GetValue(100, t, i, valid );

	Mtl *sm1 = mTex[100];

	// handle no base mat
	if(sm1 == NULL) 
	{
		return;
	}

	if(i==0||(i==1&&inRender))
	{
		for( i=0;i<mTex.Count();i++)
		{
			float u;
			pblockMat->GetValue(i,t,u,valid);

			if(mTex[i]!=NULL && u!=0 && mapOn[i] && (i != 100))
			{
				Mtl *comb = mTex[i];
				pReshading = (IReshading*)(comb->GetInterface(IID_IReshading));
				if( pReshading ) 
					pReshading->PreShade(sc, pFrag);
			}
		}

		pReshading = (IReshading*)(sm1->GetInterface(IID_IReshading));
		if( pReshading ) 
			pReshading->PreShade(sc, pFrag);
	}
	else {
		// i == 1 && not inRender
		pReshading = (IReshading*)(sm1->GetInterface(IID_IReshading));
		if( pReshading ) 
			pReshading->PreShade(sc, pFrag);
	}
}

void M3Mat::PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex, IllumParams* ip)
{
	int i; 
	IReshading* pReshading;

	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;

	pblockMat->GetValue(100,t,i,FOREVER);

	Mtl *sm1 = mTex[100];
	float total(0.0f);
	Tab<ShadeOutput>	sDatabase;
	sDatabase.SetCount(mTex.Count());
	Tab<float> u;
	u.SetCount(mTex.Count());
	ShadeOutput sFinal; 

	// handle no base mat
	if(!sm1) 
	{
		sc.ResetOutput();
		sc.out.c = black;
		sc.out.t = black;
		return;
	}

	if(i==0 || (i==1 && inRender) )
	{
		for( i=0; i<mTex.Count(); i++)
		{
			pblockMat->GetValue(i,t,u[i],valid);
			u[i] /= 100.0f;

			if( mTex[i]!=NULL && u[i]!=0 && mapOn[i] && (i != 100) )
			{
				Mtl *comb = mTex[i];
				pReshading = (IReshading*)(comb->GetInterface(IID_IReshading));
				if( pReshading ) 
					pReshading->PostShade(sc, pFrag, nextTexIndex, ip );
				sDatabase[i] = sc.out;
				sc.ResetOutput();
				total += u[i];
			}
		}

		sc.ResetOutput();
		pReshading = (IReshading*)(sm1->GetInterface(IID_IReshading));
		if( pReshading ) 
			pReshading->PostShade(sc, pFrag, nextTexIndex, ip );

		sFinal.c = black;
		sFinal.t = black;
		sFinal.ior = 0.0f;

		// LAM - 6/1/04 - 520249 - handle render elements
		for( int j = 0; j < sFinal.nElements; ++j )
			sFinal.elementVals[j] = black;

		// LAM - 6/1/04 - normalize u once
		if(total>1.0f)
			for( i=0;i<mTex.Count();i++)
				u[i] /= total;

		for( i=0;i<mTex.Count();i++)
		{
			if(mTex[i]!=NULL && u[i]!=0 && mapOn[i]  && (i != 100))
			{
				sc.out.flags |= sDatabase[i].flags;
			
				sFinal.c += u[i] * sDatabase[i].c;
				sFinal.t += u[i] * sDatabase[i].t;
				sFinal.ior += u[i] * sDatabase[i].ior;

				// render elements
				for( int j = 0; j < sFinal.nElements; ++j )
					sFinal.elementVals[j] += u[i] * sDatabase[i].elementVals[j];
			}
		}
		if(total) {
			sc.out.MixIn(sFinal, 1.0f-total);
		}
		
	}
	else {
		pReshading = (IReshading*)(sm1->GetInterface(IID_IReshading));
		if( pReshading ) 
			pReshading->PostShade(sc, pFrag, nextTexIndex, ip );
	}
}


void M3Mat::Update(TimeValue t, Interval& valid)
	{	
	if (!ivalid.InInterval(t))
	{
		ivalid.SetInfinite();
		for(int i=0;i<mTex.Count();i++)
		{
			if (mTex[i]) mTex[i]->Update(t,ivalid);
		}
	}
	valid &= ivalid;
	}

Interval M3Mat::Validity(TimeValue t)
	{
	float f;
	int i;

	if ( !pblockMat ) 
		return FOREVER;

	if (!ivalid.InInterval(t))
	{
		ivalid.SetInfinite();
		for(i=0;i<mTex.Count();i++)
		{
			if (mTex[i]) ivalid &= mTex[i]->Validity(t);
		}
		for(i=0;i<mTex.Count();i++)
		{
			pblockMat->GetValue(i,t,f,ivalid);
		}
	}

	return ivalid;
	}




/*===========================================================================*\
 | Subanims and References setup
\*===========================================================================*/

Animatable* M3Mat::SubAnim(int i)
	{
	if(i<mTex.Count()) return mTex[i];
	return NULL;
	}

TSTR M3Mat::SubAnimName(int i)
	{
	 return GetSubMtlSlotName(i);
	}

/* Ref ID Structure
 0 -100 map channels
 101  base material
 102 
 */

RefTargetHandle M3Mat::GetReference(int i)
	{
	if(i<101) return mTex[i];    
	if(i==101) return pblockMat;
	if(i==102) return morphp;

	if (i > 102)  //these area all the remaining extra morphs
	{
		i = i - 2;
		if (i < mTex.Count())
			return mTex[i];    
	}
	return NULL;
	}

void M3Mat::SetReference(int i, RefTargetHandle rtarg)
{
	if(i<101) mTex[i] = (Mtl*)rtarg;
	if(i==101) pblockMat = (IParamBlock*)rtarg;
	if(i==102) {
		morphp = (MorphR3*)rtarg;
	}
	if (i > 102)  //these area all the remaining extra morphs
	{
		i = i - 2;
		if (i < mTex.Count())
			mTex[i] = (Mtl*)rtarg;    
	}

	
}

void M3Mat::SetSubMtl(int i, Mtl *m) {
	if(m && morphp ) {
		if( m->ClassID() == M3MatClassID ) {
			M3Mat *m3m = static_cast<M3Mat *> (m);
			MorphR3 *mp = m3m->morphp;
			if(mp){
				//FIX
				for(int j=101; j<=200; j++){
					for(int k=101; k<=200; k++){
						if( morphp->GetReference(j) && morphp->GetReference(j) == mp->GetReference(k) ) {
							if(morphp->hMaxWnd) 
							{
								TSTR cyclic;
								cyclic = GetString(IDS_CYCLIC_MATERIAL);
								MessageBox(morphp->hMaxWnd,cyclic,GetString(IDS_CLASS_NAME),MB_OK);
							}
							return;
						}
					}
				}
			}
		}
	}	
	int refID = GetRefID(i);
	ReplaceReference(refID,m);
	if (matDlg) matDlg->UpdateSubMtlNames();
}

/*===========================================================================*\
 | Duplicate myself
\*===========================================================================*/

RefTargetHandle M3Mat::Clone(RemapDir &remap)
	{
	M3Mat *mtl = new M3Mat(FALSE);
	*((MtlBase*)mtl) = *((MtlBase*)this);

	mtl->mapOn.SetCount(mapOn.Count());
	mtl->mTex.SetCount(mTex.Count());

	int i;

	for(i=0;i<mTex.Count();i++)
	{
		int refID = GetRefID(i);
		if (mTex[i]) mtl->ReplaceReference(refID,remap.CloneRef(mTex[i]));
	}

	for (i=0; i<mapOn.Count(); i++)
		mtl->mapOn[i] = mapOn[i];

	//mtl->morphp = morphp;
	mtl->ReplaceReference(101,remap.CloneRef(pblockMat));
	mtl->ReplaceReference(102,remap.CloneRef(morphp));
	MorphR3 *mp = (MorphR3 *) mtl->GetReference(102);
	if(mp)	mp->morphmaterial = mtl;

	mtl->obName = obName;

	BaseClone(this, mtl, remap);

	return (RefTargetHandle)mtl;
	}



/*===========================================================================*\
 | NotifyRefChanged
\*===========================================================================*/

RefResult M3Mat::NotifyRefChanged(
		const Interval& changeInt, 
		RefTargetHandle hTarget, 
		PartID& partID, 
		RefMessage message, 
		BOOL propagate)
	{
	switch (message) {
		case REFMSG_CHANGE:
			if (matDlg && matDlg->theMtl==this) {
				matDlg->Invalidate();
			}
			if( hTarget == pblockMat ){
				mReshadeRQ = RR_NeedPreshade;
				NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			}
			else if (hTarget != NULL) {
				switch (hTarget->SuperClassID()) {
					case MATERIAL_CLASS_ID: {
						IReshading* r = static_cast<IReshading*>(hTarget->GetInterface(IID_IReshading));
						mReshadeRQ = (r == NULL)? RR_None : r->GetReshadeRequirements();
					} break;
				}
			}
		break;

		case REFMSG_SUBANIM_STRUCTURE_CHANGED:
			mReshadeRQ = RR_NeedPreshade;
			NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		break;
				
		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			gpd->dim = stdPercentDim;
			return REF_HALT; 
			}

		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			TCHAR s[50];
			_stprintf(s,GetString(IDS_MTL_CNAME),gpn->index);
			gpn->name = s;
			return REF_HALT; 
			}
		}
	return REF_SUCCEED;
	}



/*===========================================================================*\
 | Displacement support
\*===========================================================================*/

float M3Mat::EvalDisplacement(ShadeContext& sc) {

	int i; 

	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;

	pblockMat->GetValue(100,t,i,FOREVER);

	Mtl *sm1 = mTex[100];

	int counter = 0;
	float final = 0.0f;

	// handle no base mat
	if(sm1==NULL) 
	{
		return 0.0f;
	}

	if(i==0||(i==1&&inRender))
	{

		Tab<float> u;
		u.SetCount(mTex.Count());

		for( i=0;i<mTex.Count();i++)
		{
			pblockMat->GetValue(i,t,u[i],valid);

			if(mTex[i]!=NULL&&u[i]!=0&&mapOn[i]  && (i != 100))
			{
				Mtl *comb = mTex[i];
				float mI = u[i]/100.0f;
				final += (comb->EvalDisplacement(sc)*mI);
				counter++;
			}
		}

		float tF = final;
		if(counter>0) tF /= (float)counter;

		return final;

	}
	else 
	{
		return sm1->EvalDisplacement(sc);
	}

}

Interval M3Mat::DisplacementValidity(TimeValue t) 
{
	
	Interval iv; iv.SetInfinite();

	Mtl *sm1 = mTex[100];
	if(sm1) iv &= sm1->DisplacementValidity(t);

	for( int i=0;i<mTex.Count();i++)
	{
		if(mTex[i]!=NULL&&mapOn[i])
		{
			Mtl *comb = mTex[i];
			iv &= comb->DisplacementValidity(t);
		}
	}

	return iv;	
} 



/*===========================================================================*\
 | Loading and Saving of material data
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define MAPOFF_CHUNK 0x1000
#define MAPOFF_COUNT_CHUNK 0x1010

IOResult M3Mat::Save(ISave *isave) { 
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();

	ULONG nb = 0;
	int ct = mapOn.Count();
	isave->BeginChunk(MAPOFF_COUNT_CHUNK);	
	isave->Write(&ct, sizeof(int), &nb);
	isave->EndChunk();

	for (int i=0; i<mapOn.Count(); i++) {
		if (mapOn[i]==0) {
			isave->BeginChunk(MAPOFF_CHUNK+i);
			isave->EndChunk();
			}
		}

	return IO_OK;
	}	
	  

IOResult M3Mat::Load(ILoad *iload) { 
	int id;
	IOResult res;
	int texCount = 100;
	ULONG nb = 0;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
			case MAPOFF_COUNT_CHUNK:
				res = iload->Read(&texCount,sizeof(int), &nb);					
				mapOn.SetCount(texCount);
				break;
			}
		
		for(int i=0;i<texCount;i++)
		{
			if(id==MAPOFF_CHUNK+i) mapOn[id-MAPOFF_CHUNK] = 0;
		}

		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	return IO_OK;
	}

//ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
// The output is constant if all submats are constant
//
bool M3Mat::IsOutputConst
( 
	ShadeContext& sc, // describes context of evaluation
	int stdID				// must be ID_AM, ect
)
{
	// does nothing without a base mat
	Mtl *sm1 = mTex[100];
	if ( sm1 == NULL ) 
		return true;

	if ( !sm1->IsOutputConst( sc, stdID ) )
		return false;

	int i; 
	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;
	pblockMat->GetValue( 100, t, i, FOREVER );

	if ( i==0 || ( i==1 && inRender ) )
	{
		for( i = 0; i < mTex.Count(); i++ )
		{
			Tab<float> u;
			u.SetCount(mTex.Count());
			pblockMat->GetValue( i, t, u[i], valid );
			if( mTex[i] != NULL && u[i] != 0 && mapOn[i]  && (i != 100))
			{
				Mtl *comb = mTex[i];
				if ( !comb->IsOutputConst( sc, stdID ) )
					return false;
			}
		}
	}
	return true;
}

//ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
// Evaluates the material on a single texmap channel. 
// 
bool M3Mat::EvalColorStdChannel
( 
	ShadeContext& sc, // describes context of evaluation
	int stdID,				// must be ID_AM, ect
	Color& outClr			// output var
)
{
	int i; 

	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;
	Mtl *sm1 = mTex[100];

	pblockMat->GetValue( 100, t, i, FOREVER );

	// handle no base mat
	if ( sm1 == NULL ){
		outClr.Black();
		return true;
	}

	if( i==0 || ( i==1 && inRender ) ){

		float total(0.0f);
		Tab<float> u;
		u.SetCount(mTex.Count());

		Tab<Color>	cDatabase;
		cDatabase.SetCount(mTex.Count());

		for( i=0; i<mTex.Count(); i++ ){
			pblockMat->GetValue( i, t, u[i], valid );
			u[i] /= 100.0f;

			if( mTex[i] != NULL && u[i] != 0 && mapOn[i]  && (i != 100))
			{
				Mtl *comb = mTex[i];
				if ( !comb->EvalColorStdChannel( sc, stdID, outClr ) )
					return false;
				cDatabase[i] = outClr;
				outClr.Black();
				total += u[i];
			}
		}


		Color cFinal = black;
		outClr.Black();

		if ( !sm1->EvalColorStdChannel( sc, stdID, outClr ) )
			return false;

		for( i=0; i<mTex.Count(); i++ ){
			if( mTex[i] != NULL && u[i] != 0 && mapOn[i]  && (i != 100)){
				if(total > 1.0f)
					cFinal += u[i]/total * cDatabase[i];
				else
					cFinal += u[i] * cDatabase[i];
			}
		}

		if(total){
			// copies the behaviour of ShadeOutput::MixIn()
			float f = 1.0f - total;
			if(f <= 0.0f)
				outClr = cFinal;
			else if(f < 1.0f)
				outClr = (1.0f - f) * cFinal + f * outClr;
		}
	}
	else 
		return sm1->EvalColorStdChannel( sc, stdID, outClr );
	
	return true;
}

//ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ
// Evaluates the material on a single texmap channel. 
// 
bool M3Mat::EvalMonoStdChannel
( 
	ShadeContext& sc, // describes context of evaluation
	int stdID,				// must be ID_AM, ect
	float& outVal			// output var
)
{

	// handle no base mat
	Mtl *sm1 = mTex[100];
	if ( sm1 == NULL ) 
		return false;

	int i; 
	TimeValue t = sc.CurTime();
	Interval valid = FOREVER;
	pblockMat->GetValue( 100, t, i, FOREVER );

	if ( i==0 || ( i==1 && inRender ) )
	{

		Tab<float>	cDatabase;
		cDatabase.SetCount(mTex.Count());
		Tab<float> u;
		u.SetCount(mTex.Count());

		for( i=0; i<mTex.Count(); i++ )
		{
			pblockMat->GetValue( i, t, u[i], valid );

			if( mTex[i] != NULL && u[i] != 0 && mapOn[i]  && (i != 100))
			{
				Mtl *comb = mTex[i];
				if ( !comb->EvalMonoStdChannel( sc, stdID, outVal ) )
					return false;
				cDatabase[i] = outVal;
				outVal = 0.0f;
			}
		}

		outVal = 0.0f;
		if ( !sm1->EvalMonoStdChannel( sc, stdID, outVal ) )
			return false;

		for( i=0; i<mTex.Count(); i++ )
		{
			if( mTex[i] != NULL && u[i] != 0 && mapOn[i] )
			{
				float mI = u[i]/100.0f;

				// the old 'mix' fn that doesn't work for >2 mtls
				//sc.out.MixIn(sDatabase[i],1.0f-mI);

				float s = mI;
				outVal = outVal + s*cDatabase[i]; 
			}
		}

	}
	else 
		return sm1->EvalMonoStdChannel( sc, stdID, outVal );
	
	return true;
}
