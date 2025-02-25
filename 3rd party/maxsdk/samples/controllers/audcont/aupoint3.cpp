//***************************************************************************
//* Audio Amplitude Point3 Controller for 3D Studio MAX.
//* 
//* Code & design by Christer Janson
//* Autodesk European Developer Support - Neuchatel Switzerland
//* December 1995
//*

#include "auctrl.h"
#include "aup3base.h"
#include "aup3dlg.h"

class AudioPoint3Control : public AudioP3Control {
public:
	Class_ID ClassID() { return Class_ID(AUDIO_POINT3_CONTROL_CLASS_ID1, AUDIO_POINT3_CONTROL_CLASS_ID2); }  
	SClass_ID SuperClassID() { return CTRL_POINT3_CLASS_ID; } 
	void GetClassName(TSTR& s) {s = AUDIO_POINT3_CONTROL_CNAME;}

	AudioPoint3Control();

	void Copy(Control *from);
	RefTargetHandle Clone(RemapDir& remap);
	void RefDeleted();

	void *CreateTempValue() {return new Point3;}
	void DeleteTempValue(void *val) {delete (Point3*)val;}
	void ApplyValue(void *val, void *delta) {*((Point3*)val) += *((Point3*)delta);}
	void MultiplyValue(void *val, float m) {*((Point3*)val) *= m;}

	void GetValueLocalTime(TimeValue t, void *val, Interval &valid, GetSetMethod method=CTRL_ABSOLUTE);
	void SetValueLocalTime(TimeValue t, void *val, int commit, GetSetMethod method);
};

// Class description
class AudioPoint3ClassDesc:public ClassDesc {
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading) { return new AudioPoint3Control(); }
	const TCHAR *	ClassName() { return AUDIO_POINT3_CONTROL_CNAME; }
	SClass_ID		SuperClassID() { return CTRL_POINT3_CLASS_ID; }
	Class_ID		ClassID() { return Class_ID(AUDIO_POINT3_CONTROL_CLASS_ID1,AUDIO_POINT3_CONTROL_CLASS_ID2); }
	const TCHAR* 	Category() { return _T("");  }
};

static AudioPoint3ClassDesc point3AudioCD;

ClassDesc* GetAudioPoint3Desc()
{
	return &point3AudioCD;
}

AudioPoint3Control::AudioPoint3Control() 
{
	type = AUDIO_POINT3_CONTROL_CLASS_ID1;
	basePoint.x = 0.0f; basePoint.y = 0.0f; basePoint.z = 0.0f;
	targetPoint.x = 1.0f; targetPoint.y = 1.0f; targetPoint.z = 1.0f;
} 

void AudioPoint3Control::Copy(Control* from)
{
	if(GetLocked()==false)
	{
		Point3 fval;

		if (from->ClassID() == ClassID()) {
			basePoint = ((AudioPoint3Control*)from)->basePoint;
			targetPoint = ((AudioPoint3Control*)from)->targetPoint;
			mLocked = ((AudioPoint3Control*)from)->mLocked;
		}
		else {
		  if(from&&GetLockedTrackInterface(from))
			  mLocked = GetLockedTrackInterface(from)->GetLocked();
			from->GetValue(0, &fval, Interval(0,0));
			basePoint = fval;
			targetPoint = fval;
		}
	}
}

RefTargetHandle AudioPoint3Control::Clone(RemapDir& remap)
{
	// make a new controller and give it our param values.
	AudioPoint3Control *cont = new AudioPoint3Control;
	// *cont = *this;
	cont->type = type;
	cont->range = range;
	cont->channel = channel;
	cont->absolute = absolute;
	cont->numsamples = numsamples;
	cont->enableRuntime = enableRuntime;
	cont->SetFile(GetFile());
	cont->quickdraw = quickdraw;
	cont->basePoint = basePoint;
	cont->targetPoint = targetPoint;
	cont->mLocked = mLocked;

	BaseClone(this, cont, remap);
	return cont;
}

// When the last reference to a controller is
// deleted we need to close the realtime recording device and 
// its parameter dialog needs to be closed
void AudioPoint3Control::RefDeleted()
{
	int c=0;
	DependentIterator di(this);
	ReferenceMaker* maker = NULL;
	while ((maker = di.Next()) != NULL) {
		if (maker->SuperClassID()) {
			c++;
			break;
		}
	}  
	if (!c) {
		// Stop the real-time recording is the object is deleted.
		if (rtwave->IsRecording())
			rtwave->StopRecording();

		if (pDlg != NULL)
			DestroyWindow(pDlg->hWnd);
	}
}

// Return a value at a speficif instance.
void AudioPoint3Control::GetValueLocalTime(TimeValue t, void *val, Interval &valid,
	GetSetMethod method)
{
	valid.SetInstant(t); // This controller is always changing.

	// Multiply the target-base with sample and add base
	*((Point3*)val) = basePoint + (targetPoint - basePoint) * SampleAtTime(t - range.Start(), 0, FALSE);
}

void AudioPoint3Control::SetValueLocalTime(TimeValue t, void *val, int commit, GetSetMethod method)
{
}
