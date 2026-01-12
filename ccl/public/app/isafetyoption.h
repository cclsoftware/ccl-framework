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
// Filename    : ccl/public/app/isafetyoption.h
// Description : Safety option description and provider interface
//
//************************************************************************************************

#ifndef _ccl_isafetyoption_h
#define _ccl_isafetyoption_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// SafetyOptionDescription
/**	\ingroup app_inter */
//************************************************************************************************

struct SafetyOptionDescription
{
	MutableCString id;
	String title;
	String explanationText;
	int displayPriority;

	SafetyOptionDescription (CStringRef id = nullptr, StringRef title = nullptr, StringRef explanation = nullptr)
	: id (id),
	  title (title),
	  explanationText (explanation),
	  displayPriority (100)
	{}
};

//************************************************************************************************
// ISafetyOptionProvider
/**	\ingroup app_inter */
//************************************************************************************************

interface ISafetyOptionProvider: IUnknown
{
	virtual tbool CCL_API checkContext (IUnknown* context) = 0;
	
	virtual int CCL_API getOptionCount () const = 0;
	
	virtual tbool CCL_API getOptionDescription (SafetyOptionDescription& description, int index) const = 0;
	
	DECLARE_IID (ISafetyOptionProvider)
};

DEFINE_IID (ISafetyOptionProvider, 0x23d159d4, 0xb153, 0x4a46, 0x85, 0x55, 0x71, 0x49, 0x3e, 0x90, 0xfd, 0x2f)

} // namespace CCL

#endif // _ccl_isafetyoption_h
