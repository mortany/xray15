
#include "painterInterface.h"
#include "IpainterInterface.h"

#include "3dsmaxport.h"

void PainterInterface::OpenTablet()
{
	TabletManager* theTabletManager = dynamic_cast<TabletManager*>(GetCOREInterface(TabletManagerInterfaceID));
	if (theTabletManager)
	{
		theTabletManager->RegisterTabletCallback(this);
	}
}

void PainterInterface::CloseTablet()
{
	TabletManager* theTabletManager = dynamic_cast<TabletManager*>(GetCOREInterface(TabletManagerInterfaceID));
	if (theTabletManager)
	{
		theTabletManager->UnregisterTabletCallback(this);
	}
}

void PainterInterface::Event()
{
	TabletManager* theTabletManager = dynamic_cast<TabletManager*>(GetCOREInterface(TabletManagerInterfaceID));
	if (theTabletManager)
	{
		bool isTabletInUse = theTabletManager->GetPenDown();
		fpressure = isTabletInUse ? theTabletManager->GetPressure() : 0.0f;
	}
}