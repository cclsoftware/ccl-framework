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
// Filename    : ccl/public/system/ipackagehandler.h
// Description : Package Handler Interface
//
//************************************************************************************************

#ifndef _ccl_ipackagehandler_h
#define _ccl_ipackagehandler_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IPackageFile;
interface IStream;

namespace Security {
namespace Crypto {
interface ICryptoFactory; }}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Package Handler Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to package handler. */
	DEFINE_STRINGID (kPackageHandler, "CCL.PackageHandler")

		/** A package location changed. */
		DEFINE_STRINGID (kPackageLocationChanged, "PackageLocationChanged")

		/** List of mounted packages has changed. */
		DEFINE_STRINGID (kPackagesChanged, "PackagesChanged")

		/** A package was mounted. arg[0]: IPackageFile */
		DEFINE_STRINGID (kPackageMounted, "PackageMounted")

		/** A package was unmounted. arg[0]: IPackageFile */
		DEFINE_STRINGID (kPackageUnmounted, "PackageUnmounted")

		/** Rescan packages. */
		DEFINE_STRINGID (kRescanPackages, "RescanPackages")
}

//************************************************************************************************
// IPackageVolume
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPackageVolume: IUnknown
{
	/** Package volume options. */
	enum Options
	{
		kHidden = 1<<0	///< mounted as hidden package volume
	};

	/** Get volume options. */
	virtual int CCL_API getOptions () const = 0;

	/** Get package instance. */
	virtual IPackageFile* CCL_API getPackage () = 0;

	/** Get number of currently open files. */
	virtual int64 CCL_API getUseCount () const = 0;

	DECLARE_IID (IPackageVolume)
};

DEFINE_IID (IPackageVolume, 0x8560551f, 0xbaa2, 0x4ef7, 0xab, 0xc4, 0x6a, 0xc6, 0x82, 0x2, 0xf4, 0x6f)

//************************************************************************************************
// IPackageHandler
/**	\ingroup ccl_system */
//************************************************************************************************

interface IPackageHandler: IUnknown
{
	/** Package handler options. */
	enum OpenPackageOptions
	{
		kNestedPackageSupported = 1 << 0	///< support package in package with compression
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Factory Methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set external factory for cryptographical algorithms. */
	virtual tresult CCL_API setCryptoFactory (Security::Crypto::ICryptoFactory* factory) = 0;

	/** Check if given location points to a package. */
	virtual tbool CCL_API isPackage (UrlRef path) = 0;

	/** Create new package file object, type is defined by path or class identifier. */
	virtual IPackageFile* CCL_API createPackage (UrlRef path, UIDRef cid = kNullUID) = 0;

	/** Open existing package file, type is detected automatically. */
	virtual IPackageFile* CCL_API openPackage (UrlRef path, int options = 0) = 0;

	/** Create package with pre-existing stream, the stream will be shared. */
	virtual IPackageFile* CCL_API createPackageWithStream (IStream& stream, UIDRef cid) = 0;

	/** Open package from pre-existing stream, the stream will be shared. */
	virtual IPackageFile* CCL_API openPackageWithStream (IStream& stream, UIDRef cid = kNullUID) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Package Volumes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Mount package into file system (can be accessed via "package://{package-id}/..."). */
	virtual tresult CCL_API mountPackageVolume (IPackageFile* package, StringRef packageID, int options = 0) = 0;

	/** Unmount package from file system. */
	virtual tresult CCL_API unmountPackageVolume (IPackageFile* package) = 0;

	/** Get package volume interface by identifier, must be released by caller! */
	virtual IPackageVolume* CCL_API openPackageVolume (StringRef packageID) = 0;

	/** Check if package at given location is already mounted. */
	virtual tbool CCL_API isMounted (UrlRef path) = 0;

	/** Unmount all packages from file system. */
	virtual tresult CCL_API terminate () = 0;

	DECLARE_IID (IPackageHandler)
};

DEFINE_IID (IPackageHandler, 0xa6016937, 0x56e, 0x4d50, 0xa4, 0x1d, 0xd7, 0xf4, 0x25, 0x43, 0x7d, 0xff)

} // namespace CCL

#endif // _ccl_ipackagehandler_h
