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
// Filename    : ccl/platform/linux/gui/dialog.linux.cpp
// Description : Platform-specific Dialog implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/asyncoperation.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	if(DragSession* dragSession = DragSession::getActiveSession ())
		dragSession->setCanceled (true);

	makeNativePopupWindow (parent);
	
	attached (this);
	show ();
	
	loopTerminated = false;
	GUI.runModalLoop (this, loopTerminated);
	
	//if(parent)
	//	parent->activate ();
	
	Desktop.removeWindow (this);
	
	return AsyncOperation::createCompleted (dialogResult);
}

//************************************************************************************************
// LinuxDialog
//************************************************************************************************

LinuxDialog::LinuxDialog (const Rect& size, StyleRef style, StringRef title)
: LinuxWindow (size, style, title),
  loopTerminated (false)
#if WAYLAND_USE_XDG_DIALOG
  ,xdgDialog (nullptr)
#endif
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxDialog::close ()
{
	if(onClose ())
	{
		hide ();
		setInCloseEvent ();
		setInDestroyEvent (true);
		
		removed (nullptr);
		onDestroy ();
		setInCloseEvent (false);
		
		loopTerminated = true;
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDialog::showWindow (bool state)
{
	if(!style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
		style.setCustomStyle (Styles::kWindowCombinedStyleDialog, true);
	
	LinuxWindow::showWindow (state);

	if(state)
	{
		if(!style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		{
			center ();

			#if WAYLAND_USE_XDG_DIALOG
			if(windowContext.topLevelWindow != nullptr)
			{
				xdg_wm_dialog_v1* dialogManager = WaylandClient::instance ().getDialogManager ();
				if(dialogManager != nullptr)
				{
					xdgDialog = xdg_wm_dialog_v1_get_xdg_dialog (dialogManager, windowContext.topLevelWindow);
					if(xdgDialog)
						xdg_dialog_v1_set_modal (xdgDialog);
				}
			}
			#endif
		}
	}
	else
	{
		#if WAYLAND_USE_XDG_DIALOG
		if(xdgDialog && WaylandClient::instance ().isInitialized ())
			xdg_dialog_v1_destroy (xdgDialog);
		xdgDialog = nullptr;
		#endif	
	}
}
	
//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

void PopupSelectorWindow::onActivate (bool state)
{
	SuperClass::onActivate (state);
}
