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
// Filename    : ccl/public/system/cclsafety.h
// Description : Safety Helpers
//
//************************************************************************************************

#ifndef _cclsafety_h
#define _cclsafety_h

#include "ccl/public/base/uid.h"
#include "ccl/public/system/isafetymanager.h"

namespace CCL {

interface IClassDescription;

//************************************************************************************************
// SafetyGuard
/** \ingroup ccl_system */
//************************************************************************************************

class SafetyGuard
{
public:
	SafetyGuard (CStringRef actionId, const Vector<String>& args = {});
	~SafetyGuard ();
};

//************************************************************************************************
// Helper functions
//************************************************************************************************

/** Begin a safety section, allowing to detect crashes inside the section. 
	\ingroup ccl_system */
void ccl_safety_begin (CStringRef actionId, const Vector<String>& args = {});

/** End a safety section.
	\ingroup ccl_system */
void ccl_safety_end ();

/** Check if a given class is blocked by the safety manager.
	\ingroup ccl_system */
bool ccl_safety_check (const IClassDescription& classDescription);

/** Check if a class with given class ID is blocked by the safety manager.
	\ingroup ccl_system */
bool ccl_safety_check (UIDRef cid);

/** Handle an exception. Call in a catch block.
	@return true if the exception was handled, false otherwise.
	\ingroup ccl_system */
bool ccl_safety_catch ();

} // namespace CCL

#endif // _cclsafety_h
