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
// Filename    : ccl/public/system/cclsafety.cpp
// Description : Safety Helpers
//
//************************************************************************************************

#include "ccl/public/system/cclsafety.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ccl_safety_begin (CStringRef actionId, const Vector<String>& args)
{
	System::GetSafetyManager ().beginAction (actionId, args, args.count ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ccl_safety_end ()
{
	System::GetSafetyManager ().endAction ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CCL::ccl_safety_check (const IClassDescription& classDescription)
{
	return !System::GetSafetyManager ().getCombinedFilter ().matches (const_cast<IClassDescription*> (&classDescription));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CCL::ccl_safety_check (UIDRef cid)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
	if(description)
		return ccl_safety_check (*description);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CCL::ccl_safety_catch ()
{
	return System::GetSafetyManager ().handleException ();
}

//************************************************************************************************
// SafetyGuard
/** \ingroup ccl_system */
//************************************************************************************************

SafetyGuard::SafetyGuard (CStringRef actionId, const Vector<String>& args)
{
	ccl_safety_begin (actionId, args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SafetyGuard::~SafetyGuard ()
{
	ccl_safety_end ();
}
