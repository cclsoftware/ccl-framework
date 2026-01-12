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
// Filename    : ccl/app/applicationspecifics.h
// Description : Application Specifics
//
//************************************************************************************************

#ifndef _ccl_applicationspecifics_h
#define _ccl_applicationspecifics_h

#include "ccl/app/component.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/framework/inotifyicon.h"
#include "ccl/public/gui/framework/iwin32specifics.h"

namespace CCL {

//************************************************************************************************
// ApplicationSpecifics
//************************************************************************************************

class ApplicationSpecifics: public Component
{
public:
	DECLARE_CLASS (ApplicationSpecifics, Component)

	ApplicationSpecifics ();
	~ApplicationSpecifics ();

	DECLARE_STRINGID_MEMBER (kAppNotifyIcon)

	static ApplicationSpecifics* createInstance ();

	/** Enable/disable notification icon. */
	void enableNotifyIcon (bool state, bool autoShow);
	INotifyIcon* getNotifyIcon () const;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;

protected:
	INotifyIcon* notifyIcon;
};

//************************************************************************************************
// Win32ApplicationSpecifics
//************************************************************************************************

class Win32ApplicationSpecifics: public ApplicationSpecifics
{
public:
	DECLARE_CLASS (Win32ApplicationSpecifics, ApplicationSpecifics)

	Win32ApplicationSpecifics ();
	~Win32ApplicationSpecifics ();

	/** Enable/disable task bar progress indicator. */
	void enableTaskBarProgressIndicator (IWindow* window, bool state);

	// ApplicationSpecifics
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;

protected:
	Win32::ITaskBar* taskBar;
	SharedPtr<IProgressNotify> taskBarProgressIndicator;

	Win32::ITaskBar& getTaskBar ();
};

//************************************************************************************************
// MacOSApplicationSpecifics
//************************************************************************************************

class MacOSApplicationSpecifics: public ApplicationSpecifics
{
public:
	DECLARE_CLASS (MacOSApplicationSpecifics, ApplicationSpecifics)
	
	MacOSApplicationSpecifics ();
	~MacOSApplicationSpecifics ();
};

//************************************************************************************************
// IosApplicationSpecifics
//************************************************************************************************

class IosApplicationSpecifics: public ApplicationSpecifics
{
public:
	DECLARE_CLASS (IosApplicationSpecifics, ApplicationSpecifics)
	
	IosApplicationSpecifics ();
	~IosApplicationSpecifics ();
};

//************************************************************************************************
// LinuxApplicationSpecifics
//************************************************************************************************

class LinuxApplicationSpecifics: public ApplicationSpecifics
{
public:
	DECLARE_CLASS (LinuxApplicationSpecifics, ApplicationSpecifics)
};

} // namespace CCL

#endif // _ccl_applicationspecifics_h
