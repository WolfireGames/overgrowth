
#ifndef NV_PHOTOSHOP_EXPORTER_H
#define NV_PHOTOSHOP_EXPORTER_H

#include <PIExport.h>     // Export Photoshop header file.
#include <PIUtilities.h>  // SDK Utility library.

// Photoshop crazyness:
//#define gResult 				(*(globals->result))
#define gStuff  				(globals->exportParamBlock)


// This is our structure that we use to pass globals between routines:
struct Globals
{
	short * result;                    // Must always be first in Globals.
	ExportRecord * exportParamBlock;   // Must always be second in Globals.

	Boolean queryForParameters;

	// ...

};



#endif // NV_PHOTOSHOP_EXPORTER_H
