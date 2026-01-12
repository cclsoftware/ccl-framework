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
// Filename    : ccl/gui/system/notifyicon.cpp
// Description : Notification Icon
//
//************************************************************************************************

#include "ccl/gui/system/notifyicon.h"

#include "ccl/gui/popup/contextmenu.h"

using namespace CCL;

//************************************************************************************************
// NotifyIcon
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NotifyIcon, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NotifyIcon::NotifyIcon ()
: visible (false),
  autoShow (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotifyIcon::setImage (IImage* _image)
{
	image = unknown_cast<Image> (_image);
	if(visible)
		updateImage ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotifyIcon::setTitle (StringRef _title)
{
	title = _title;
	if(visible)
		updateTitle ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotifyIcon::setHandler (IUnknown* handler)
{
	this->handler = handler;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotifyIcon::setVisible (tbool _state)
{
	bool state = _state != 0;
	if(state != visible)
	{
		updateVisible (state);
		visible = state;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NotifyIcon::setAutoShow (tbool state)
{
	autoShow = state != 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotifyIcon::reportEvent (const Alert::Event& e)
{
	if(!visible)
	{
		if(autoShow)
			setVisible (true);
		else
			return;
	}

	showInfo (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NotifyIcon::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupMenu* NotifyIcon::createContextMenu ()
{
	UnknownPtr<IContextMenuHandler> handler (this->handler);
	if(!handler)
		return nullptr;

	AutoPtr<ContextPopupMenu> contextMenu = NEW ContextPopupMenu (MenuPresentation::kNative);
	contextMenu->setContextID (kContextID);
	contextMenu->setFocusItem (this->asUnknown ());
	handler->appendContextMenu (*contextMenu);

	// we need to init the menu manually here, because there is no PopupSelector involved
	contextMenu->getMenu ()->updateKeys ();
	contextMenu->getMenu ()->init ();

	return return_shared (ccl_cast<PopupMenu> (contextMenu->getMenu ()));
}
