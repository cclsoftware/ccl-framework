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
// Filename    : ccl/app/utilities/appdiagnostic.h
// Description : Application Diagnostic
//
//************************************************************************************************

#ifndef _ccl_appdiagnostic_h
#define _ccl_appdiagnostic_h

#include "ccl/public/system/idiagnosticstore.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {

interface IClassDescription;
interface IImage;

//************************************************************************************************
// DiagnosticPresentation
//************************************************************************************************

namespace DiagnosticPresentation
{
	const IClassDescription* toClass (const IDiagnosticResult* result);
	FileType toFileType (const IDiagnosticResult* result);

	String getLabel (const IDiagnosticResult* result);
	IImage* createIcon (const IDiagnosticResult* result);

	String printDuration (double value);
	String printSize (double value);
};

} // namespace CCL

#endif // _ccl_appdiagnostic_h
