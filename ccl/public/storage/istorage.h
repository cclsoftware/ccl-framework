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
// Filename    : ccl/public/storage/istorage.h
// Description : Storage Interfaces
//
//************************************************************************************************

#ifndef _ccl_istorage_h
#define _ccl_istorage_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class FileType;
interface IStream;
interface IFileSystem;
interface IProgressNotify;

//************************************************************************************************
// IStorable
/** Interface for saving/loading an object's state.
	\ingroup base_io  */
//************************************************************************************************

interface IStorable: IUnknown
{
	/** Get file type describing the data format. */
	virtual tbool CCL_API getFormat (FileType& format) const = 0;

	/** Save state to stream. */
	virtual tbool CCL_API save (IStream& stream) const = 0;

	/** Load state from stream. */
	virtual tbool CCL_API load (IStream& stream) = 0;

	DECLARE_IID (IStorable)
};

DEFINE_IID (IStorable, 0xb131b242, 0xbff4, 0x446f, 0x96, 0xe4, 0xec, 0x19, 0xdf, 0xad, 0x11, 0xe6)

//************************************************************************************************
// IStorageHandler
/** Handler for saving/loading multiple objects to/from persistent storage.
	\ingroup base_io  */
//************************************************************************************************

interface IStorageHandler: IUnknown
{
	/** Save content to persistent storage. */
	virtual tbool CCL_API saveContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress = nullptr) const = 0;

	/** Load content from persistent storage. */
	virtual tbool CCL_API loadContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress = nullptr) = 0;

	DECLARE_IID (IStorageHandler)
};

DEFINE_IID (IStorageHandler, 0xa8c81701, 0x8e28, 0x47b8, 0xa0, 0xab, 0x34, 0x96, 0xa6, 0xf5, 0xff, 0xe2)

//************************************************************************************************
// IStorageRegistry
/** Registry of storage handlers.
	\ingroup base_io  */
//************************************************************************************************

interface IStorageRegistry: CCL::IUnknown
{
	/** Register handler for load/save operations. */
	virtual void CCL_API registerHandler (IStorageHandler* handler) = 0;

	/** Unregister handler from load/save operations. */
	virtual void CCL_API unregisterHandler (IStorageHandler* handler) = 0;

	DECLARE_IID (IStorageRegistry)
};

DEFINE_IID (IStorageRegistry, 0x3e7c9bfe, 0x3c40, 0x49f5, 0x99, 0xd3, 0xe8, 0x80, 0x61, 0xd9, 0x8a, 0xad)

} // namespace CCL

#endif // _ccl_istorage_h
