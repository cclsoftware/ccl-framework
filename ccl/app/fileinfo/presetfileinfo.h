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
// Filename    : ccl/app/fileinfo/presetfileinfo.h
// Description : Preset File Info Component
//
//************************************************************************************************

#ifndef _presetfileinfo_h
#define _presetfileinfo_h

#include "ccl/app/fileinfo/fileinfocomponent.h"

namespace CCL {

//************************************************************************************************
// PresetFileInfo
//************************************************************************************************

class PresetFileInfo: public StandardFileInfo
{
public:
	PresetFileInfo ();

	static void registerInfo ();
	static bool canHandleFile (UrlRef path);

	// FileInfoComponent
	tbool CCL_API setFile (UrlRef path) override;
};

} // namespace CCL

#endif // _presetfileinfo_h
