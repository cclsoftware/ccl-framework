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

#ifndef _ccl_systemshell_h
#define _ccl_systemshell_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/isystemshell.h"

namespace CCL {

class AsyncOperation;

//************************************************************************************************
// SystemShell
//************************************************************************************************

class SystemShell: public Object,
				   public ISystemShell,
				   public ExternalSingleton<SystemShell>
{
public:
	SystemShell ();
	~SystemShell ();

	void addOperation (AsyncOperation* operation);
	bool removeOperation (AsyncOperation* operation);

	// ISystemShell
	tresult CCL_API openUrl (UrlRef url, int flags = 0) override;
	tresult CCL_API showFile (UrlRef url) override;
	tresult CCL_API addRecentFile (UrlRef url) override;
	tresult CCL_API setRunAtStartupEnabled (tbool state) override;
	tbool CCL_API isRunAtStartupEnabled () override;
	tbool CCL_API isRunAtStartupHidden (ArgsRef args) override;
	tresult CCL_API openApplicationSettings () override;
	IAsyncOperation* CCL_API startBrowserAuthentication (UrlRef url, StringRef scheme,
														 IWindow* window = nullptr) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (ISystemShell, Object)

protected:
	ObjectArray pendingOperations;

	// platform-specific:
	virtual tresult openNativeUrl (UrlRef url, int flags);
	virtual tresult showNativeFile (UrlRef url);
};

} // namespace CCL

#endif // _ccl_systemshell_h
