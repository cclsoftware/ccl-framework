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
// Filename    : ccl/base/storage/isettings.h
// Description : User Settings Interfaces
//
//************************************************************************************************

#ifndef _ccl_isettings_h
#define _ccl_isettings_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class Settings;

//************************************************************************************************
// ISettingsSaver
/** External dependency saved/restored with settings. */
//************************************************************************************************

interface ISettingsSaver: IUnknown
{
	virtual void restore (Settings&) = 0;

	virtual void flush (Settings&) = 0;
	
	DECLARE_IID (ISettingsSaver)
};

} // namespace CCL

#endif // _ccl_isettings_h
