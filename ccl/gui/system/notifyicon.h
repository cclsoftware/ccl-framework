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
// Filename    : ccl/gui/system/notifyicon.h
// Description : Notification Icon
//
//************************************************************************************************

#ifndef _ccl_notifyicon_h
#define _ccl_notifyicon_h

#include "ccl/gui/popup/menu.h"
#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/public/gui/framework/inotifyicon.h"

namespace CCL {

//************************************************************************************************
// NotifyIcon
//************************************************************************************************

class NotifyIcon: public Object,
				  public INotifyIcon
{
public:
	DECLARE_CLASS_ABSTRACT (NotifyIcon, Object)

	NotifyIcon ();

	// INotifyIcon
	tresult CCL_API setImage (IImage* image) override;
	tresult CCL_API setTitle (StringRef title) override;
	tresult CCL_API setHandler (IUnknown* handler) override;
	tresult CCL_API setVisible (tbool state) override;
	tresult CCL_API setAutoShow (tbool state) override;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	CLASS_INTERFACE2 (INotifyIcon, IReporter, Object)

protected:
	String title;
	SharedPtr<Image> image;
	SharedPtr<IUnknown> handler;
	bool visible;
	bool autoShow;

	PopupMenu* createContextMenu (); ///< create context menu via handler

	// platform-specific methods:
	virtual void updateVisible (bool state) = 0;
	virtual void updateTitle () = 0;
	virtual void updateImage () = 0;
	virtual void showInfo (const Alert::Event& e) = 0;
};

} // namespace CCL

#endif // _ccl_notifyicon_h
