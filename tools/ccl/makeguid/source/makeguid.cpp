//************************************************************************************************
//
// CCL GUID Generator
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : makeguid.cpp
// Description : CCL GUID Generator
//
//************************************************************************************************

#include "appversion.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/base/uid.h"
#include "ccl/public/base/debug.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/public/system/iconsole.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

static CStringPtr uidFormat = "UID (0x%08lx, 0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x)\n";

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main (ArgsRef args) 
{
	System::IConsole& console = System::GetConsole ();
	console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);
	console.writeLine ("");
	
	System::GetAlertService ().setTitle (APP_NAME);

	UID uid;
	uid.generate ();
	
	MutableCString string;
	string.appendFormat (uidFormat, uid.data1, uid.data2, uid.data3, 
		uid.data4[0], uid.data4[1], uid.data4[2], uid.data4[3], 
		uid.data4[4], uid.data4[5], uid.data4[6], uid.data4[7]);

	console.writeLine (string);

	System::GetClipboard ().setText (String (string));

	return 0;
}

