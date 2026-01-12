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
// Filename    : ccl/system/packaging/packagehandler.h
// Description : Package Handler
//
//************************************************************************************************

#ifndef _ccl_packagehandler_h
#define _ccl_packagehandler_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/system/ipackagehandler.h"

namespace CCL {

class PackageProtocolHandler;

//************************************************************************************************
// PackageHandler
//************************************************************************************************

class PackageHandler: public Object,
					  public IPackageHandler,
					  public Singleton<PackageHandler>
{
public:
	PackageHandler ();
	~PackageHandler ();

	UIDRef getPackageClassForMimeType (StringRef mimeType) const;

	// IPackageHandler
	tresult CCL_API setCryptoFactory (Security::Crypto::ICryptoFactory* factory) override;
	tbool CCL_API isPackage (UrlRef path) override;
	IPackageFile* CCL_API createPackage (UrlRef path, UIDRef cid = kNullUID) override;
	IPackageFile* CCL_API openPackage (UrlRef path, int options = 0) override;
	IPackageFile* CCL_API createPackageWithStream (IStream& stream, UIDRef cid) override;
	IPackageFile* CCL_API openPackageWithStream (IStream& stream, UIDRef cid = kNullUID) override;
	tresult CCL_API mountPackageVolume (IPackageFile* package, StringRef packageID, int options) override;
	tresult CCL_API unmountPackageVolume (IPackageFile* package) override;
	IPackageVolume* CCL_API openPackageVolume (StringRef packageID) override;
	tbool CCL_API isMounted (UrlRef path) override;
	tresult CCL_API terminate () override;

	CLASS_INTERFACE (IPackageHandler, Object)

protected:
	PackageProtocolHandler* protocolHandler;
};

//************************************************************************************************
// PackageProtocolHandler
//************************************************************************************************

class PackageProtocolHandler: public MountProtocolHandler
{
public:
	DECLARE_CLASS (PackageProtocolHandler, MountProtocolHandler)

	PackageProtocolHandler ();

	bool addPackage (StringRef name, IPackageFile* package, int options);
	bool removePackage (IPackageFile* package);
	void collectPaths (Container& paths, bool wantHidden);
	IPackageVolume* openVolume (StringRef name);
	bool isMounted (UrlRef path);
	void unmountAll ();

	// IProtocolHandler
	StringRef CCL_API getProtocol () const override;
	IFileSystem* CCL_API getMountPoint (StringRef name) override;

protected:
	class PackageEntry: public MountPoint,
						public IPackageVolume
	{
	public:
		PackageEntry (StringRef name = nullptr, IPackageFile* package = nullptr, int options = 0);
		~PackageEntry ();

		// IPackageVolume
		int CCL_API getOptions () const override { return options; }
		IPackageFile* CCL_API getPackage () override { return package; }
		int64 CCL_API getUseCount () const override;

		CLASS_INTERFACE (IPackageVolume, MountPoint)

	protected:
		int options;
		IPackageFile* package;
	};

	Threading::CriticalSection lock;
	AutoPtr<IFileSystem> rootFileSystem;
};

} // namespace CCL

#endif // _ccl_packagehandler_h
