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
// Filename    : ccl/platform/win/interfaces/iwinrtplatform.h
// Description : Windows Runtime (WinRT) Integration
//
//************************************************************************************************

#ifndef _ccl_iwinrtplatform_h
#define _ccl_iwinrtplatform_h

#include <roapi.h>

#include "ccl/public/base/platform.h"
#include "ccl/public/cclexports.h"

namespace CCL {
namespace WinRT {

//************************************************************************************************
// IWinRTPlatform
//************************************************************************************************

struct IWinRTPlatform
{
	virtual HRESULT CCL_API initialize () = 0;

	virtual void CCL_API uninitialize () = 0;
	
	virtual HRESULT CCL_API getActivationFactory (UStringPtr activatableClassId, REFIID iid, void** factory) = 0;

	virtual HSTRING CCL_API createString (UStringPtr string) = 0;

	virtual void CCL_API deleteString (HSTRING hString) = 0;

	virtual UStringPtr CCL_API getStringBuffer (HSTRING hString, uint32& length) = 0;
};

} // namespace WinRT

namespace System
{
	/** Get WinRT platform singleton. */
	CCL_EXPORT WinRT::IWinRTPlatform& CCL_API CCL_ISOLATED (GetWinRTPlatform) ();
	inline WinRT::IWinRTPlatform& GetWinRTPlatform () { return CCL_ISOLATED (GetWinRTPlatform) (); }
}

} // namespace CCL

#endif // _ccl_iwinrtplatform_h
