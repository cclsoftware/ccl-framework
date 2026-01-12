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
// Filename    : ccl/platform/win/system/winrtplatform.cpp
// Description : Windows Runtime (WinRT) Integration
//
//************************************************************************************************

#include "ccl/platform/win/interfaces/iwinrtplatform.h"

#include <wrl\wrappers\corewrappers.h>

#pragma comment (lib, "runtimeobject.lib")

namespace CCL {
namespace WinRT {

//************************************************************************************************
// WinRTPlatformImplementation
//************************************************************************************************

class WinRTPlatformImplementation: public IWinRTPlatform
{
public:
	// IWinRTPlatform
	HRESULT CCL_API initialize () override
	{
		// Must be initialized single-threaded, otherwise some classic COM components don't work!!!
		return Windows::Foundation::Initialize (RO_INIT_SINGLETHREADED);
	}

	void CCL_API uninitialize () override
	{
		__try
		{
			Windows::Foundation::Uninitialize ();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ASSERT (0)			
		}
	}

	HRESULT CCL_API getActivationFactory (UStringPtr activatableClassId, REFIID iid, void** factory) override
	{
		return ::RoGetActivationFactory (Microsoft::WRL::Wrappers::HStringReference (activatableClassId).Get (), iid, factory);
	}

	HSTRING CCL_API createString (UStringPtr string) override
	{
		HSTRING hString = nullptr;
		::WindowsCreateString (string, string ? (uint32)::wcslen (string) : 0, &hString);
		return hString;
	}

	void CCL_API deleteString (HSTRING hString) override
	{
		::WindowsDeleteString (hString);
	}

	UStringPtr CCL_API getStringBuffer (HSTRING hString, uint32& length) override
	{
		return ::WindowsGetStringRawBuffer (hString, &length);
	}
};

} // namespace WinRT
} // namespace CCL

using namespace CCL;
using namespace WinRT;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Exported API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IWinRTPlatform& CCL_API System::CCL_ISOLATED (GetWinRTPlatform) ()
{
	static WinRTPlatformImplementation thePlatform;
	return thePlatform;
}
