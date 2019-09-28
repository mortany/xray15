[!output TEMPLATESTRING_COPYRIGHT]

#include "[!output PROJECT_NAME].h"

#define [!output CLASS_NAME]_CLASS_ID	Class_ID([!output CLASSID1], [!output CLASSID2])


class [!output CLASS_NAME] : public [!output SUPER_CLASS_NAME]
{
public:
	//Constructor/Destructor
	[!output CLASS_NAME]();
	virtual ~[!output CLASS_NAME]();

	// GUP Methods
	virtual DWORD     Start();
	virtual void      Stop();
	virtual DWORD_PTR Control(DWORD parameter);
	virtual void      DeleteThis();

	// Loading/Saving
	virtual IOResult Save(ISave* isave);
	virtual IOResult Load(ILoad* iload);
};


[!output TEMPLATESTRING_CLASSDESC]

[!if PARAM_MAPS != 0]
[!output TEMPLATESTRING_PARAMBLOCKDESC]
[!endif] 

[!output CLASS_NAME]::[!output CLASS_NAME]()
{

}

[!output CLASS_NAME]::~[!output CLASS_NAME]()
{

}

void [!output CLASS_NAME]::DeleteThis()
{
	delete this;
}

// Activate and Stay Resident
DWORD [!output CLASS_NAME]::Start()
{
	#pragma message(TODO("Do plugin initialization here"))
	#pragma message(TODO("Return if you want remain loaded or not"))
	return GUPRESULT_KEEP;
}

void [!output CLASS_NAME]::Stop()
{
	#pragma message(TODO("Do plugin un-initialization here"))
}

DWORD_PTR [!output CLASS_NAME]::Control( DWORD /*parameter*/ )
{
	return 0;
}

IOResult [!output CLASS_NAME]::Save(ISave* /*isave*/)
{
	return IO_OK;
}

IOResult [!output CLASS_NAME]::Load(ILoad* /*iload*/)
{
	return IO_OK;
}

