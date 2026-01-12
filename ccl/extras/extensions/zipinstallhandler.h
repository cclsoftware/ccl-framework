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
// Filename    : ccl/extras/extensions/zipinstallhandler.h
// Description : ZIP Installation Handler
//
//************************************************************************************************

#ifndef _ccl_zipinstallhandler_h
#define _ccl_zipinstallhandler_h

#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/public/extras/icontentinstaller.h"
#include "ccl/public/system/ifileutilities.h" // IFileHandler

namespace CCL {

struct DragEvent;
interface IDragHandler;
interface IView;

namespace Install {

//************************************************************************************************
// ZipInstallHandler
//************************************************************************************************

class ZipInstallHandler: public Object,
						 public AbstractFileHandler,
						 public AbstractFileInstallHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ZipInstallHandler, Object)

	ZipInstallHandler (int installationOrder);

	static IDragHandler* createDragHandler (const DragEvent& event, IView* view);

	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;

	// IFileInstallHandler
	tbool CCL_API canHandle (IFileDescriptor& descriptor) override;
	tbool CCL_API performInstallation (IFileDescriptor& descriptor, IUrl& path) override;

	CLASS_INTERFACE2 (IFileHandler, IFileInstallHandler, Object)

protected:
	void toDefaultPath (IUrl& dstPath, UrlRef srcPath) const;
	virtual bool canHandlePackage (StringRef packageId) const = 0;
	virtual bool extractFile (IUrl& dstPath, ExtensionDescription& description);
};

} // namespace Install
} // namespace CCL

#endif // _ccl_zipinstallhandler_h
