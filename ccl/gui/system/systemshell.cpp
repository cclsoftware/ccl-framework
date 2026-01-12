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
// Filename    : ccl/gui/system/systemshell.h
// Description : System Shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"

#include "ccl/gui/gui.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// BrowserAuthenticationOperation
//************************************************************************************************

class BrowserAuthenticationOperation: public AsyncOperation,
									  private AbstractFileHandler
{
public:
	DECLARE_CLASS_ABSTRACT (BrowserAuthenticationOperation, AsyncOperation)

	BrowserAuthenticationOperation (StringRef callbackScheme);
	~BrowserAuthenticationOperation ();

	// AsyncOperation
	void setState (AsyncOperation::State state) override;
	using AsyncOperation::getState;

	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;

	CLASS_INTERFACE (IFileHandler, AsyncOperation)

protected:
	String callbackScheme;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISystemShell& CCL_API System::CCL_ISOLATED (GetSystemShell) ()
{
	return SystemShell::instance ();
}

//************************************************************************************************
// SystemShell
//************************************************************************************************

SystemShell::SystemShell ()
{
	pendingOperations.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemShell::~SystemShell ()
{
	SOFT_ASSERT (pendingOperations.isEmpty (), "System Shell operations not completed.")
	pendingOperations.removeAll ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SystemShell::addOperation (AsyncOperation* operation)
{
	pendingOperations.add (operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SystemShell::removeOperation (AsyncOperation* operation)
{
	return pendingOperations.remove (operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SystemShell::openUrl (UrlRef _url, int flags)
{
	// check if call should be deferred
	if(flags & System::kDeferOpenURL)
	{
		AutoPtr<Url> url = NEW Url (_url);
		(NEW Message ("openUrl", static_cast<IUrl*> (url), flags &~ System::kDeferOpenURL))->post (this);
		return kResultOk;
	}

	// resolve symbolic paths to real locations
	Url url (_url);
	if(url.getProtocol () == CCLSTR ("local"))
		System::GetSystem ().resolveLocation (url, _url);

	// try to open internally first
	if(IApplication* app = GUI.getApplication ())
		if(app->openFile (url))
			return kResultOk;

	if(flags & System::kDoNotOpenExternally) // do not try to open in external application
		return kResultFailed;

	return openNativeUrl (url, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SystemShell::showFile (UrlRef _url)
{
	// resolve symbolic paths to real locations
	Url url (_url);
	if(url.getProtocol () == CCLSTR ("local"))
		System::GetSystem ().resolveLocation (url, _url);

	return showNativeFile (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SystemShell::addRecentFile (UrlRef url)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SystemShell::setRunAtStartupEnabled (tbool state)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemShell::isRunAtStartupEnabled ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemShell::isRunAtStartupHidden (ArgsRef args)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SystemShell::openApplicationSettings ()
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API SystemShell::startBrowserAuthentication (UrlRef url, StringRef scheme, IWindow* window)
{
	auto* operation = NEW BrowserAuthenticationOperation (scheme);
	addOperation (return_shared (operation)); // keep an internal ref count
	tresult result = openUrl (url);
	operation->setState (result == kResultOk ? IAsyncInfo::kStarted : IAsyncInfo::kFailed);	
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SystemShell::openNativeUrl (UrlRef url, int flags)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SystemShell::showNativeFile (UrlRef url)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemShell::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "openUrl")
	{
		UnknownPtr<IUrl> url (msg[0].asUnknown ());
		int flags = msg[1].asInt ();
		ASSERT (url != nullptr)
		openUrl (*url, flags);
	}
}

//************************************************************************************************
// BrowserAuthenticationOperation
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BrowserAuthenticationOperation, AsyncOperation)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserAuthenticationOperation::BrowserAuthenticationOperation (StringRef callbackScheme)
: callbackScheme (callbackScheme)
{
	System::GetFileTypeRegistry ().registerHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserAuthenticationOperation::~BrowserAuthenticationOperation ()
{
	System::GetFileTypeRegistry ().unregisterHandler (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserAuthenticationOperation::setState (AsyncOperation::State newState)
{
	if(newState != getState ())
	{
		if(newState >= kCompleted)
		{
			SystemShell::instance ().removeOperation (this);
			deferDestruction (this); // this will release the internal ref count
		}

		SuperClass::setState (newState);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserAuthenticationOperation::openFile (UrlRef path)
{
	if(path.getProtocol () == callbackScheme)
	{
		AutoPtr<Url> pathCopy = NEW Url (path);
		setResult (Variant ().takeShared (pathCopy->asUnknown ()));
		setState (kCompleted);
		return true;
	}
	return false;
}
