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
// Filename    : ccl/public/base/iconverter.h
// Description : Converter Interface
//
//************************************************************************************************

#ifndef _ccl_iconverter_h
#define _ccl_iconverter_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class FileType;
interface IStream;
interface IUnknownList;
interface IProgressNotify;

//************************************************************************************************
// IConvertFilter
/**	Filter to convert between object types. 
	\ingroup ccl_base */
//************************************************************************************************

interface IConvertFilter: IUnknown
{
	/** Check if object can be converted to given type. */
	virtual tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const = 0;

	/** Convert object to given type. In case of success, the new object is owned by the caller. */
	virtual IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const = 0;

	DECLARE_IID (IConvertFilter)
};

DEFINE_IID (IConvertFilter, 0xb8f8d127, 0xf618, 0x49cd, 0xb3, 0xb7, 0x2e, 0x1f, 0xec, 0x4a, 0x7a, 0xb8)

//************************************************************************************************
// IImportFilter
/**	Filter to import objects from data. 
	\ingroup ccl_base */
//************************************************************************************************

interface IImportFilter: IUnknown
{
	/** Type identifier. */
	typedef CStringRef TypeID;

	/** Check if object of given type can be created from data stream. */
	virtual tbool CCL_API canImport (IStream& stream, TypeID type, UIDRef cid = kNullUID) const = 0;
	
	/** Create object from data stream. In case of success, the new object is owned by the caller. */
	virtual IUnknown* CCL_API import (IStream& stream, TypeID type, UIDRef cid = kNullUID) const = 0;

	DECLARE_IID (IImportFilter)
};

DEFINE_IID (IImportFilter, 0xeff0d18e, 0xd6e6, 0x45ba, 0xa3, 0x9d, 0x54, 0x63, 0x53, 0x85, 0x61, 0x9d)

//************************************************************************************************
// IExportFilter
/** Filter to export objects to files. 
	\ingroup ccl_base */
//************************************************************************************************

interface IExportFilter: IUnknown
{
	/** Get identity for alternative filter comparison (optional). */
	virtual StringID CCL_API getIdentity () const = 0;

	/** Get type of file promises created by this filter (optional). */
	virtual tbool CCL_API getFileType (FileType& fileType) const = 0;

	/** Create list of IFilePromise instances for given object. */
	virtual tbool CCL_API makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const = 0;

	DECLARE_IID (IExportFilter)
};

DEFINE_IID (IExportFilter, 0xf74f20c, 0xfafc, 0x4e50, 0xb7, 0x14, 0x61, 0x52, 0x75, 0xc1, 0xe, 0xbd)

//************************************************************************************************
// IObjectConverter
/** Interface for global converter singleton, allowing to register filters. 
	\ingroup ccl_base */
//************************************************************************************************

interface IObjectConverter: IConvertFilter,
							IImportFilter,
							IExportFilter
{
	/** Register conversion filter. The filter will be shared! */
	virtual void CCL_API registerFilter (IConvertFilter* filter) = 0;

	/** Unregister conversion filter. */
	virtual void CCL_API unregisterFilter (IConvertFilter* filter) = 0;

	/** Register import filter. The filter will be shared! */
	virtual void CCL_API registerImporter (IImportFilter* filter) = 0;

	/** Unregister import filter. */
	virtual void CCL_API unregisterImporter (IImportFilter* filter) = 0;

	/** Register export filter. The filter will be shared! */
	virtual void CCL_API registerExporter (IExportFilter* filter) = 0;

	/** Unregister exporter filter. */
	virtual void CCL_API unregisterExporter (IExportFilter* filter) = 0;

	DECLARE_IID (IObjectConverter)
};

DEFINE_IID (IObjectConverter, 0x49eecf57, 0xc485, 0x4581, 0xba, 0x10, 0xb, 0x9f, 0xfa, 0x65, 0xe8, 0x45)

} // namespace CCL

#endif // _ccl_iconverter_h
