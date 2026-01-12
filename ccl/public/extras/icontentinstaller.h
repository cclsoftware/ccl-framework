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
// Filename    : ccl/public/extras/icontentinstaller.h
// Description : Content Installer Interfaces
//
//************************************************************************************************

#ifndef _ccl_icontentinstaller_h
#define _ccl_icontentinstaller_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
interface IFileDescriptor;

//************************************************************************************************
// IFileInstallHandler
/** Extension to IFileHandler with installation-specific calls. */
//************************************************************************************************

interface IFileInstallHandler: IUnknown
{
	enum InstallationOrder
	{
		kInstallOrderFirst = 0,
		kInstallOrderSecond,
		kInstallOrderThird,
		kInstallOrderLast = 100
	};

	/** Get preferred order of installation, used to sort multiple handlers. */
	virtual int CCL_API getInstallationOrder () const = 0;

	/** Check if given file can be handled by this instance. */
	virtual tbool CCL_API canHandle (IFileDescriptor& descriptor) = 0;

	/** Begin/end installation of multiple files. */
	virtual void CCL_API beginInstallation (tbool state) = 0;

	/** Perform installation of given file. Path can be changed by handler. */
	virtual tbool CCL_API performInstallation (IFileDescriptor& descriptor, IUrl& path) = 0;

	/** Check if application restart is required after installation. */
	virtual tbool CCL_API isRestartRequired () const = 0;
		
	/** Get location of file on local system if already present. */
	virtual tbool CCL_API getFileLocation (IUrl& path, IFileDescriptor& descriptor) = 0;

	DECLARE_IID (IFileInstallHandler)
};

DEFINE_IID (IFileInstallHandler, 0xcfd4937b, 0x3fd0, 0x4ca5, 0xb3, 0xe0, 0x1d, 0x12, 0x31, 0xcf, 0x5e, 0x16)

//************************************************************************************************
// AbstractFileInstallHandler
//************************************************************************************************

class AbstractFileInstallHandler: public IFileInstallHandler
{
public:
	AbstractFileInstallHandler (int installationOrder)
	: installationOrder (installationOrder)
	{}

	int CCL_API getInstallationOrder () const override
	{
		return installationOrder; 
	}

	tbool CCL_API canHandle (IFileDescriptor& descriptor) override
	{
		return false;
	}

	void CCL_API beginInstallation (tbool state) override
	{}

	tbool CCL_API performInstallation (IFileDescriptor& descriptor, IUrl& path) override
	{
		return false; 
	}

	tbool CCL_API isRestartRequired () const override
	{
		return false;
	}
		
	tbool CCL_API getFileLocation (IUrl& path, IFileDescriptor& descriptor) override
	{
		return false; 
	}

protected:
	int installationOrder;
};

} // namespace CCL

#endif // _ccl_icontentinstaller_h
