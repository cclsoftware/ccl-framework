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
// Filename    : ccl/platform/linux/gui/platformdialog.linux.cpp
// Description : Linux Platform Dialog
//
//************************************************************************************************

#include "ccl/platform/linux/gui/platformdialog.linux.h"

#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Linux;
using namespace PlatformIntegration;

//************************************************************************************************
// LinuxPlatformDialog
//************************************************************************************************

LinuxPlatformDialog::LinuxPlatformDialog ()
: listener (*this),
  nativeDialog (nullptr),
  windowContext {nullptr},
  exportedParent (nullptr),
  exportedParentV1 (nullptr),
  parentWindow (nullptr)
{
	WaylandClient::instance ().registerObject (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxPlatformDialog::~LinuxPlatformDialog ()
{
	WaylandClient::instance ().unregisterObject (*this);
	
	if(exportedParent && WaylandClient::instance ().isInitialized ())
		zxdg_exported_v2_destroy (exportedParent);
	if(exportedParentV1 && WaylandClient::instance ().isInitialized ())
		zxdg_exported_v1_destroy (exportedParentV1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::onCompositorDisconnected ()
{
	exportedParent = nullptr;
	exportedParentV1 = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* LinuxPlatformDialog::setParentWindow (IWindow* parent)
{
	if(parent == nullptr)
		parent = System::GetDesktop ().getDialogParentWindow ();

	if(parent == parentWindow)
		return AsyncOperation::createFailed (false);

	parentWindow = parent;

	if(exportedParent && WaylandClient::instance ().isInitialized ())
		zxdg_exported_v2_destroy (exportedParent);
	if(exportedParentV1 && WaylandClient::instance ().isInitialized ())
		zxdg_exported_v1_destroy (exportedParentV1);

	windowContext.parent = parent;
	WindowContext* parentContext = windowContext.parent ? static_cast<WindowContext*> (windowContext.parent->getSystemWindow ()) : nullptr;
	
	if(parentContext == nullptr || parentContext->waylandSurface == nullptr)
		return AsyncOperation::createFailed (false);
	
	zxdg_exporter_v2* exporter = WaylandClient::instance ().getExporter ();
	if(exporter)
	{
		exportedParent = zxdg_exporter_v2_export_toplevel (exporter, parentContext->waylandSurface);
		zxdg_exported_v2_add_listener (exportedParent, &listener, &listener);
	}
	
	zxdg_exporter_v1* exporterV1 = WaylandClient::instance ().getExporterV1 ();
	if(exporterV1)
	{
		exportedParentV1 = zxdg_exporter_v1_export (exporterV1, parentContext->waylandSurface);
		zxdg_exported_v1_add_listener (exportedParentV1, &listener, &listener);
	}

	if(exportedParent == nullptr && exportedParentV1 == nullptr)
		return AsyncOperation::createFailed (false);
	
	exportOperation = NEW AsyncOperation;
	exportOperation->setState (IAsyncOperation::kStarted);
	return exportOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::onPlatformDialogOpened (NativeWindowHandle* handle)
{
	if(handle)
	{
		windowContext.topLevelWindow = handle->topLevelWindow;
		windowContext.popupWindow = handle->popupWindow;
	}
	systemWindow = NEW ModalSystemWindow (&windowContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::onPlatformDialogClosed ()
{
	systemWindow.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::onParentWindowExported (CStringPtr handle, int version)
{
	if(nativeDialog)
	{
		NativeWindowHandle parentHandle;
		switch(version)
		{
		case 2 :
			parentHandle.exportedHandle = handle;
			break;
		case 1 :
			parentHandle.exportedHandleV1 = handle;
		}
		nativeDialog->setParent (&parentHandle);
	}

	if(exportOperation)
		exportOperation->setStateDeferred (IAsyncOperation::kCompleted);
	exportOperation.release ();
}

//************************************************************************************************
// LinuxPlatformDialog::Listener
//************************************************************************************************

LinuxPlatformDialog::Listener::Listener (LinuxPlatformDialog& dialog)
: dialog (dialog)
{
	zxdg_exported_v1_listener::handle = onV1HandleExported;
	zxdg_exported_v2_listener::handle = onHandleExported;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::Listener::onHandleExported (void* data, zxdg_exported_v2* zxdg_exported_v2, CStringPtr handle)
{
	Listener* This = static_cast<Listener*> (data);
	This->dialog.onParentWindowExported (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxPlatformDialog::Listener::onV1HandleExported (void* data, zxdg_exported_v1* zxdg_exported_v1, CStringPtr handle)
{
	Listener* This = static_cast<Listener*> (data);
	This->dialog.onParentWindowExported (handle, 1);
}
