//***************************************************************************
//* Audio Amplitude Controller for 3D Studio MAX.
//* 
//* Code & design by Christer Janson
//* Autodesk European Developer Support - Neuchatel Switzerland
//* December 1995
//*

#include "io.h"
#include "auctrl.h"
#include "AssetManagement\IAssetAccessor.h"
#include "assetmanagement\AssetType.h"
#include "IPathConfigMgr.h"

using namespace MaxSDK::AssetManagement;

#define MAX_CONTROLLERS	5
ClassDesc *classDescArray[MAX_CONTROLLERS];
int classDescCount = 0;

void initClassDescArray(void)
{
   if( !classDescCount )
   {
   	classDescArray[classDescCount++] = GetAudioFloatDesc();
   	classDescArray[classDescCount++] = GetAudioPoint3Desc();
   	classDescArray[classDescCount++] = GetAudioPositionDesc();
   	classDescArray[classDescCount++] = GetAudioRotationDesc();
   	classDescArray[classDescCount++] = GetAudioScaleDesc();
   }
}

HINSTANCE hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	switch(fdwReason) {
      case DLL_PROCESS_ATTACH:
         MaxSDK::Util::UseLanguagePackLocale();		 
         hInstance = hinstDLL;
         DisableThreadLibraryCalls(hInstance);
			break;
	}

	return(TRUE);
}

__declspec( dllexport ) const TCHAR *LibDescription() 
{
	return GetString(IDS_LIBDESCRIPTION);
}

__declspec( dllexport ) int LibNumberClasses()
{
   initClassDescArray();

	return classDescCount;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
   initClassDescArray();

	if( i < classDescCount )
		return classDescArray[i];
	else
		return NULL;
}

__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if(hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}

AudioBaseControl::AudioBaseControl()
{
	wave = new WaveForm;
	rtwave = new RunTimeWave;
}

int AudioBaseControl::GetTrackVSpace(int lineHeight)
{
	int height = 1;

	if (!rtwave->IsRecording()) {
		height = 3;
	}

	return height;
}

AudioBaseControl::~AudioBaseControl()
{
	delete wave;
	delete rtwave;
}

class RangeRestore : public RestoreObj {
	public:
		AudioBaseControl *cont;
		Interval ur, rr;
		RangeRestore(AudioBaseControl *c) 
		{
			cont = c;
			ur   = cont->range;
		}   		
		void Restore(int isUndo) 
		{
			rr = cont->range;
			cont->range = ur;
			cont->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}
		void Redo()
		{
			cont->range = rr;
			cont->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}		
		void EndHold() 
		{ 
			cont->ClearAFlag(A_HELD);
		}
};

void AudioBaseControl::Hold()
{
	if (theHold.Holding() && !TestAFlag(A_HELD)) {
		SetAFlag(A_HELD);
		theHold.Put(new RangeRestore(this));
	}	
}

void AudioBaseControl::MapKeys( TimeMap *map, DWORD flags ) 
{
	if(GetLocked()==false)
	{
		Hold();
		range.Set(map->map(range.Start()), map->map(range.End())); 
		NotifyDependents(FOREVER,PART_ALL,REFMSG_CHANGE);
	}
}

int AudioBaseControl::PaintTrack(ParamDimensionBase *dim,HDC hdc,Rect& rcTrack,Rect& rcPaint,float zoom,int scroll,DWORD flags)
{
	HPEN penIn = CreatePen(PS_SOLID,0,RGB(0,0,200));
	HPEN Redpen = CreatePen(PS_SOLID,0,RGB(200,0,0));
	HPEN oldPen;

	int xl = rcPaint.left;
	int xr = rcPaint.right;
	int bot = rcPaint.bottom;
	int top = rcPaint.top;
	TimeValue tp, t;
	float val;
	float newmax;

	tp = ScreenToTime(xl-1,zoom,scroll) ;

	// Kludge to paint the waveform if min == max
	newmax = max;

	if (min == max) {
		newmax += 1.0f;
	}

	if (!rtwave->IsRecording()) {

		oldPen = (HPEN)SelectObject(hdc,penIn);

		for (; xl<=xr; xl++) {
			t   = ScreenToTime(xl,zoom,scroll) - range.Start();
			val	= SampleAtTime(t, quickdraw, TRUE);
			MoveToEx(hdc, xl, bot, NULL);
			LineTo(hdc, xl, (int)(bot + (val-min)/(newmax - min) * (top-bot)));
		}

		SelectObject(hdc,oldPen);
	}
	else {
		oldPen = (HPEN)SelectObject(hdc,Redpen);

		MoveToEx(hdc, xl, bot + (top - bot)/2, NULL);
		LineTo(hdc, xr, bot + (top - bot)/2);

		SelectObject(hdc,oldPen);
	}

	DeleteObject(penIn);
	DeleteObject(Redpen);
	
	return TRACK_DONE;
}

float AudioBaseControl::SampleAtTime(TimeValue t, int ignore_oversampling, int painting)
{
	double sampleTime;
	long sampleNo;
	int sample = 0;
	float normSample;
	WaveForm::Channel ch;
	int i;
	float divider;
    float minval, maxval;

    minval = min;
    maxval = max;
    if (painting && (min == max)) {
        if (minval == maxval) {
            maxval += 1.0f;
        }
    }
	
	if (rtwave->IsRecording())
		return (float)(abs(rtwave->GetSample(numsamples)))/128.0f * (maxval - minval) + minval;

	if (wave->IsEmpty()) {
		// Is a file assigned?
		if (GetFile().GetId()==kInvalidId) {
			return minval;
		}
		else {
			// Initialize the WaveForm and load the audio stream
			BOOL loaded = wave->InitOpen(GetFile());
			if(!loaded) {
				return minval;
			}
		}
	}

	sampleTime = t/4.8; // "Instant" in milliseconds
	sampleNo = (long)(sampleTime * wave->GetSamplesPerSec() / 1000);
	switch (channel) {
		case 0:
			ch = WaveForm::kLeftChannel;
			break;
		case 1:
			ch = WaveForm::kRightChannel;
			break;
		default:
			ch = WaveForm::kMixChannels;
			break;
	}

	if (ignore_oversampling) {
		sample = abs(wave->GetSample(sampleNo, ch));
	}
	else {
		// Smooth out the wave through oversampling
		for (i=0; i < numsamples; i++)
			sample += abs(wave->GetSample(sampleNo + i, ch));

        if (i != 0)
			sample = sample / i;
	}

	if (absolute)
		divider = (float)wave->GetMaxValue();
	else 
		divider = wave->GetBitsPerSample() == 8 ? 128.0f : 32768.0f;

	if (((float)sample)/divider < threshold)
		sample = 0;

	normSample = (float)(sample)/divider * (maxval - minval) + minval;

	return normSample;
}


BOOL AudioBaseControl::FixupFilename(TSTR &name, const TCHAR *dir)
{
	TSTR pathName, fileName, newPathFile;

	// Check if file exist
	// Return ok if it does
	if (_taccess(name, 0) == 0)
		return TRUE;

	// If file does not exist, look for it in the specified directory
	SplitPathFile(name, &pathName, &fileName);
	if (dir[_tcslen(dir)-1] == _T('\\') ||  dir[_tcslen(dir)-1] == _T(':') || dir[_tcslen(dir)-1] == _T('/')) 
		newPathFile = TSTR(dir) + fileName;
	else 
		newPathFile = TSTR(dir) + _T("\\") + fileName;

	if (_taccess(newPathFile, 0) == 0) {
		name = newPathFile;
		return TRUE;
	}

	return FALSE;
}

class AudioBaseAccessor : public IAssetAccessor	{
public:

	AudioBaseAccessor(AudioBaseControl* aAudioController) : mAudioController(aAudioController) {}

	virtual AssetType GetAssetType() const	{ return kSoundAsset; }

	// path accessor functions
	virtual MaxSDK::AssetManagement::AssetUser GetAsset()  const;
	virtual bool SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAsset);

protected:
	AudioBaseControl* mAudioController;
};

 MaxSDK::AssetManagement::AssetUser AudioBaseAccessor::GetAsset() const	{
	return mAudioController->GetFile();
}

bool AudioBaseAccessor::SetAsset(const MaxSDK::AssetManagement::AssetUser& aNewAsset)	{
	IAssetManager* assetMgr = IAssetManager::GetInstance();
	if(assetMgr){
		mAudioController->SetFile(aNewAsset);
		return true;
	}
	return false;
}

void AudioBaseControl::EnumAuxFiles(AssetEnumCallback& nameEnum, DWORD flags)
{
	if ((flags&FILE_ENUM_CHECK_AWORK1)&&TestAFlag(A_WORK1)) return; // LAM - 4/21/03
	StdControl::EnumAuxFiles( nameEnum, flags ); // LAM - 4/21/03

	// No external file, do realtime recording.
	if (rtwave->IsRecording())
		return;

	if (GetFile().GetId()==kInvalidId)
		return;

	if(flags & FILE_ENUM_ACCESSOR_INTERFACE)	{
		IEnumAuxAssetsCallback* callback = static_cast<IEnumAuxAssetsCallback*>(&nameEnum);
		callback->DeclareAsset(AudioBaseAccessor(this));
	}
	else	{
		IPathConfigMgr::GetPathConfigMgr()->
			RecordInputAsset(GetFile(), nameEnum, flags);
	}
	
	return;
}

void AudioBaseControl::Extrapolate(Interval range,TimeValue t,void *val,Interval &valid,int etype)
{
	if(type == AUDIO_FLOAT_CONTROL_CLASS_ID1) {
		float fval0, fval1, fval2, res = 0.0f;
		switch (etype) {
		case ORT_LINEAR:			
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&fval0,valid);
				GetValueLocalTime(range.Start()+1,&fval1,valid);
				res = LinearExtrapolate(range.Start(),t,fval0,fval1,fval0);				
			} 
			else {
				GetValueLocalTime(range.End()-1,&fval0,valid);
				GetValueLocalTime(range.End(),&fval1,valid);
				res = LinearExtrapolate(range.End(),t,fval0,fval1,fval1);
			}
			break;

		case ORT_IDENTITY:
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&fval0,valid);
				res = IdentityExtrapolate(range.Start(),t,fval0);
			} 
			else {
				GetValueLocalTime(range.End(),&fval0,valid);
				res = IdentityExtrapolate(range.End(),t,fval0);
			}
			break;

		case ORT_RELATIVE_REPEAT:
			GetValueLocalTime(range.Start(),&fval0,valid);
			GetValueLocalTime(range.End(),&fval1,valid);
			GetValueLocalTime(CycleTime(range,t),&fval2,valid);
			res = RepeatExtrapolate(range,t,fval0,fval1,fval2);			
			break;
		}
		valid.Set(t,t);
		*((float*)val) = res;
	}
	else if(type == AUDIO_SCALE_CONTROL_CLASS_ID1) {
		ScaleValue val0, val1, val2, res;
		switch (etype) {
		case ORT_LINEAR:			
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&val0,valid);
				GetValueLocalTime(range.Start()+1,&val1,valid);
				res = LinearExtrapolate(range.Start(),t,val0,val1,val0);				
			} 
			else {
				GetValueLocalTime(range.End()-1,&val0,valid);
				GetValueLocalTime(range.End(),&val1,valid);
				res = LinearExtrapolate(range.End(),t,val0,val1,val1);
			}
			break;

		case ORT_IDENTITY:
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&val0,valid);
				res = IdentityExtrapolate(range.Start(),t,val0);
			} 
			else {
				GetValueLocalTime(range.End(),&val0,valid);
				res = IdentityExtrapolate(range.End(),t,val0);
			}
			break;

		case ORT_RELATIVE_REPEAT:
			GetValueLocalTime(range.Start(),&val0,valid);
			GetValueLocalTime(range.End(),&val1,valid);
			GetValueLocalTime(CycleTime(range,t),&val2,valid);
			res = RepeatExtrapolate(range,t,val0,val1,val2);			
			break;
		}
		valid.Set(t,t);
		*((ScaleValue *)val) = res;
	}
	else {
		Point3 val0, val1, val2, res;
		switch (etype) {
		case ORT_LINEAR:			
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&val0,valid);
				GetValueLocalTime(range.Start()+1,&val1,valid);
				res = LinearExtrapolate(range.Start(),t,val0,val1,val0);				
			} 
			else {
				GetValueLocalTime(range.End()-1,&val0,valid);
				GetValueLocalTime(range.End(),&val1,valid);
				res = LinearExtrapolate(range.End(),t,val0,val1,val1);
			}
			break;

		case ORT_IDENTITY:
			if (t<range.Start()) {
				GetValueLocalTime(range.Start(),&val0,valid);
				res = IdentityExtrapolate(range.Start(),t,val0);
			} 
			else {
				GetValueLocalTime(range.End(),&val0,valid);
				res = IdentityExtrapolate(range.End(),t,val0);
			}
			break;

		case ORT_RELATIVE_REPEAT:
			GetValueLocalTime(range.Start(),&val0,valid);
			GetValueLocalTime(range.End(),&val1,valid);
			GetValueLocalTime(CycleTime(range,t),&val2,valid);
			res = RepeatExtrapolate(range,t,val0,val1,val2);			
			break;
		}
		valid.Set(t,t);
		*((Point3 *)val) = res;
	}
}

const AssetUser& AudioBaseControl::GetFile()
{
	return szFilenameAsset;
}

void AudioBaseControl::SetFile(const AssetUser& file)
{
	szFilenameAsset = file;
}
