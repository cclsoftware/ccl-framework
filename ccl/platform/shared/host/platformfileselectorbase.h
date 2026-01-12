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
// Filename    : ccl/platform/shared/host/platformfileselectorbase.h
// Description : Platform File Selector
//
//************************************************************************************************

#ifndef _ccl_platformfileselectorbase_h
#define _ccl_platformfileselectorbase_h

#include "ccl/gui/dialogs/fileselector.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/shared/interfaces/platformfileselector.h"
#include "ccl/platform/shared/host/iplatformintegrationloader.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// PlatformFileSelectorBase
//************************************************************************************************

class PlatformFileSelectorBase: public NativeFileSelector,
								public PlatformIntegration::IPlatformFileSelectorObserver
{
public:
	DECLARE_CLASS_ABSTRACT (PlatformFileSelectorBase, NativeFileSelector)

	PlatformFileSelectorBase ();
	
	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	
	// IPlatformFileSelectorObserver
	void addResult (CStringPtr path) override;
	void opened (void* nativeWindowHandle) override;
	void closed (int result) override;
	
protected:
	PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformFileSelector> platformSelector;
	SharedPtr<AsyncOperation> operation;
	tbool terminated;
};

//************************************************************************************************
// PlatformFolderSelectorBase
//************************************************************************************************

class PlatformFolderSelectorBase: public NativeFolderSelector,
								  public PlatformIntegration::IPlatformFileSelectorObserver
{
public:
	DECLARE_CLASS_ABSTRACT (PlatformFolderSelectorBase, NativeFolderSelector)

	PlatformFolderSelectorBase ();
	
	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
	
	// IPlatformFileSelectorObserver
	void addResult (CStringPtr path) override;
	void opened (void* nativeWindowHandle) override;
	void closed (int result) override;
	
protected:
	PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformFileSelector> platformSelector;
	SharedPtr<AsyncOperation> operation;
	tbool terminated;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformfileselectorbase_h
