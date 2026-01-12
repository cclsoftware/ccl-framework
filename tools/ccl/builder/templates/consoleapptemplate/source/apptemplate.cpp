//************************************************************************************************
//
// Application Template
// (Application Copyright)
//
// Filename    : apptemplate.cpp
// Description : Application Template
//
//************************************************************************************************

#include "appversion.h"

#include "ccl/extras/tools/argumentparser.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args)
{
	System::IConsole& console = System::GetConsole ();
	console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

	ArgumentParser argParser;
	
	bool succeeded = argParser.parse (args) == kResultOk;
	
	return succeeded ? 0 : -1;
}
