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
// Filename    : ccl/platform/linux/gui/fileselector.linux.cpp
// Description : Linux File Selector Implementation
//
//************************************************************************************************

#include "ccl/platform/linux/gui/platformdialog.linux.h"

#include "ccl/platform/shared/host/platformfileselectorbase.h"

namespace CCL {

//************************************************************************************************
// LinuxFileSelector
//************************************************************************************************

class LinuxFileSelector: public PlatformIntegration::PlatformFileSelectorBase,
						 public LinuxPlatformDialog
{
public:
	DECLARE_CLASS (LinuxFileSelector, PlatformFileSelectorBase)
	
	LinuxFileSelector ();

	// PlatformFileSelectorBase
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	void opened (void* nativeWindowHandle) override;
	void closed (int result) override;
};

//************************************************************************************************
// LinuxFolderSelector
//************************************************************************************************

class LinuxFolderSelector: public PlatformIntegration::PlatformFolderSelectorBase,
						   public LinuxPlatformDialog
{
public:
	DECLARE_CLASS (LinuxFolderSelector, PlatformFolderSelectorBase)

	LinuxFolderSelector ();
	
	// PlatformFolderSelectorBase
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
	void opened (void* nativeWindowHandle) override;
	void closed (int result) override;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (LinuxFileSelector, PlatformFileSelectorBase, "FileSelector")
DEFINE_CLASS_UID (LinuxFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFileSelector::LinuxFileSelector ()
{
	nativeDialog = platformSelector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* LinuxFileSelector::runPlatformSelectorAsync (int type, StringRef _title, int filterIndex, IWindow* window)
{
	String title (_title);

	AutoPtr<AsyncSequence> sequence = NEW AsyncSequence;
	sequence->setCancelOnError (false);
	sequence->add ([=] () { return setParentWindow (window); });
	sequence->add ([=] () { return SuperClass::runPlatformSelectorAsync (type, title, filterIndex, window); });
	return return_shared<IAsyncOperation> (sequence->start ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileSelector::opened (void* nativeWindowHandle)
{
	onPlatformDialogOpened (static_cast<PlatformIntegration::NativeWindowHandle*> (nativeWindowHandle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFileSelector::closed (int result)
{
	SuperClass::closed (result);
	onPlatformDialogClosed ();
}

//************************************************************************************************
// PlatformFolderSelectorBase
//************************************************************************************************

DEFINE_CLASS (LinuxFolderSelector, PlatformFolderSelectorBase)
DEFINE_CLASS_UID (LinuxFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxFolderSelector::LinuxFolderSelector ()
{
	nativeDialog = platformSelector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* LinuxFolderSelector::runPlatformSelectorAsync (StringRef _title, IWindow* window)
{
	String title (_title);

	AutoPtr<AsyncSequence> sequence = NEW AsyncSequence;
	sequence->setCancelOnError (false);
	sequence->add ([=] () { return setParentWindow (window); });
	sequence->add ([=] () { return SuperClass::runPlatformSelectorAsync (title, window); });
	return return_shared<IAsyncOperation> (sequence->start ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFolderSelector::opened (void* nativeWindowHandle)
{
	onPlatformDialogOpened (static_cast<PlatformIntegration::NativeWindowHandle*> (nativeWindowHandle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxFolderSelector::closed (int result)
{
	SuperClass::closed (result);
	onPlatformDialogClosed ();
}
