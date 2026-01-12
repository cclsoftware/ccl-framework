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
// Filename    : ccl/app/utilities/appdiagnostic.cpp
// Description : Application Diagnostic
//
//************************************************************************************************

#include "ccl/app/utilities/appdiagnostic.h"
#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Diagnostics")
	XSTRING (SmallDuration, "<1 ms")
	XSTRING (SmallSize, "<1 KB")
END_XSTRINGS

//************************************************************************************************
// DiagnosticPresentation
//************************************************************************************************

const IClassDescription* DiagnosticPresentation::toClass (const IDiagnosticResult* result)
{
	UID cid;
	if(result->getContext ().startsWith (DiagnosticID::kClassIDPrefix))
		if(cid.fromCString (result->getContext ().subString (DiagnosticID::kClassIDPrefix.length ())))
			return System::GetPlugInManager ().getClassDescription (cid);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileType DiagnosticPresentation::toFileType (const IDiagnosticResult* result)
{
	FileType fileType;
	if(result->getContext ().startsWith (DiagnosticID::kFileTypePrefix))
		fileType.setExtension (String (result->getContext ().subString (DiagnosticID::kFileTypePrefix.length ())));
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DiagnosticPresentation::getLabel (const IDiagnosticResult* result)
{
	String label;
	if(!result->getLabel ().isEmpty ())
		label = result->getLabel ();
	else if(auto* description = toClass (result))
		label = description->getName ();
	return label;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* DiagnosticPresentation::createIcon (const IDiagnosticResult* result)
{
	if(auto* description = toClass (result))
		return return_shared (PlugInClass (*description).getIcon ());
	else
	{
		FileType fileType = toFileType (result);
		if(fileType.isValid ())
			return FileIcons::instance ().createIcon (fileType);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DiagnosticPresentation::printDuration (double value)
{
	if(value < 0.001)
		return XSTR (SmallDuration);
	else
		return Format::Duration::print (value, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DiagnosticPresentation::printSize (double value)
{
	if(value < 1024.)
		return XSTR (SmallSize);
	else
		return Format::ByteSize::print (value);
}
