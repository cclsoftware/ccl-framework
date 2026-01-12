//************************************************************************************************
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
// Filename    : ccl/public/system/alerttypes.cpp
// Description : Alert types
//
//************************************************************************************************

#include "ccl/public/system/alerttypes.h"

#include "ccl/public/system/formatter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (Alert::IReporter, 0xf8f3a8bd, 0x85a2, 0x460d, 0x84, 0xf8, 0x29, 0xb3, 0x77, 0x23, 0x54, 0x79)

//************************************************************************************************
// Alert::Event
//************************************************************************************************

bool Alert::Event::isLowLevel () const
{
	return !moduleName.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Alert::Event::format (int flags) const
{
	String string;
	bool withTime = (flags & kWithTime) != 0;
	if(withTime && time != DateTime ())
		string << Format::PortableDateTime::print (time) << ": ";

	bool withAlertType = (flags & kWithAlertType) != 0;
	if(withAlertType)
	{
		switch(type)
		{
		case kInformation : string << "[Info]"; break;
		case kWarning     : string << "[Warning]"; break;
		case kError       : string << "[Error]"; break;
		}
		string << " ";
	}

	// Do not allow alert type and severity level simultaneously.
	// Make alert type override severity.
	bool withSeverity = (flags & kWithSeverity) != 0;
	if(!withAlertType && withSeverity)
	{
		switch(severity)
		{
		case kSeverityFatal : string << "[Fatal]"; break;
		case kSeverityError : string << "[Error]"; break;
		case kSeverityWarning : string << "[Warning]"; break;
		case kSeverityInfo : string << "[Info]"; break;
		case kSeverityDebug : string << "[Debug]"; break;
		case kSeverityTrace : string << "[Trace]"; break;
		}
		string << " ";
	}

	// Module name may not exist.
	bool withModule = (flags & kWithModule) != 0;
	if(withModule)
	{
		if(!moduleName.isEmpty ())
			string << " (" << moduleName << ") ";

	}

	string << message;
	return string;
}
