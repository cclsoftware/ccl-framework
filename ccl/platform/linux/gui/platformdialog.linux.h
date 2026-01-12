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
// Filename    : ccl/platform/linux/gui/platformdialog.linux.h
// Description : Linux Platform Dialog
//
//************************************************************************************************

#ifndef _ccl_platformdialog_linux_h
#define _ccl_platformdialog_linux_h

#include "ccl/gui/windows/systemwindow.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/linux/gui/nativewindowcontext.h"
#include "ccl/platform/shared/interfaces/platformdialog.h"

namespace CCL {

//************************************************************************************************
// LinuxPlatformDialog
//************************************************************************************************

class LinuxPlatformDialog: public Linux::WaylandObject
{
public:
	LinuxPlatformDialog ();
	~LinuxPlatformDialog ();
	
	// WaylandObject
	void onCompositorDisconnected () override;
	
protected:
	struct Listener: zxdg_exported_v2_listener,
					 zxdg_exported_v1_listener
	{
		Listener (LinuxPlatformDialog& dialog);
		
		static void onHandleExported (void* data, zxdg_exported_v2* zxdg_exported_v2, CStringPtr handle);
		static void onV1HandleExported (void* data, zxdg_exported_v1* zxdg_exported_v1, CStringPtr handle);
		
	protected:
		LinuxPlatformDialog& dialog;
	};
	Listener listener;

	SharedPtr<AsyncOperation> exportOperation;
	
	IWindow* parentWindow;
	Linux::NativeWindowContext windowContext;
	PlatformIntegration::IPlatformDialog* nativeDialog;
	zxdg_exported_v1* exportedParentV1;
	zxdg_exported_v2* exportedParent;
	AutoPtr<ModalSystemWindow> systemWindow;
	
	IAsyncOperation* setParentWindow (IWindow* parent = nullptr);
	void onPlatformDialogOpened (PlatformIntegration::NativeWindowHandle* handle);
	void onPlatformDialogClosed ();
	
	void onParentWindowExported (CStringPtr, int version = 2);
};

} // namespace CCL
 
#endif // _ccl_platformdialog_linux_h
