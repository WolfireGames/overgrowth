
#include "PhotoshopExporter.h"

#include <Plugin.h>

SPBasicSuite * sSPBasic = NULL;



void InitGlobals (Ptr globalPtr)
{	
	Globals * globals = (Globals *)globalPtr;
	
	// Set default values.
	globals->queryForParameters = true;
}




DLLExport MACPASCAL void PluginMain (const short selector,
						             void *exportParamBlock,
						             long *data,
						             short *result)
{
	if (selector == exportSelectorAbout)
	{
		sSPBasic = ((AboutRecord*)exportParamBlock)->sSPBasic;
		//DoAbout((AboutRecordPtr)exportParamBlock);
	}
	else
	{
		sSPBasic = ((ExportRecordPtr)exportParamBlock)->sSPBasic;

		// Allocate and initialize globals.
		Ptr globalPtr = AllocateGlobals ((uint32)result, (uint32)exportParamBlock, ((ExportRecordPtr)exportParamBlock)->handleProcs, sizeof(Globals), data, InitGlobals);
		
		if (globalPtr == NULL)
		{ 
		  *result = memFullErr;
		  return;
		}
		
		// Get our "globals" variable assigned as a Global Pointer struct with the
		// data we've returned:
		Globals * globals = (Globals *)globalPtr;


		//-----------------------------------------------------------------------
		//	(3) Dispatch selector.
		//-----------------------------------------------------------------------

		switch (selector)
		{
			case exportSelectorPrepare:
			//	DoPrepare(globals);
				break;
			case exportSelectorStart:
			//	DoStart(globals);
				break;
			case exportSelectorContinue:
			//	DoContinue(globals);
				break;
			case exportSelectorFinish:
			//	DoFinish(globals);
				break;
		}
							
		// unlock handle pointing to parameter block and data so it can move
		// if memory gets shuffled:
		if ((Handle)*data != NULL)
		{
			PIUnlockHandle((Handle)*data);
		}
	}
}
