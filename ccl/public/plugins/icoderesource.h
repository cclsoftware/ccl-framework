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
// Filename    : ccl/public/plugins/icoderesource.h
// Description : Code Resource Interface
//
//************************************************************************************************

#ifndef _ccl_icoderesource_h
#define _ccl_icoderesource_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IClassFactory;
interface IAttributeList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Code Resource Types
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CodeResourceType
{
	DEFINE_STRINGID (kNative, "Native")
	DEFINE_STRINGID (kScript, "Script")
	DEFINE_STRINGID (kCore,   "Core")
}

#define PLUG_CATEGORY_CODERESOURCELOADER CCLSTR ("CodeResourceLoader")

//************************************************************************************************
// ICodeResource
/** \ingroup base_plug */
//************************************************************************************************

interface ICodeResource: IUnknown
{
	/** Get type of code resource. */
	virtual StringID CCL_API getType () const = 0;

	/** Get class factory object. */
	virtual IClassFactory* CCL_API getClassFactory () = 0;
	
	/** Get meta information. */
	virtual IAttributeList* CCL_API getMetaInfo () = 0;

	/** Get path to code resource on disk. */
	virtual tbool CCL_API getPath (IUrl& path) const = 0;

	// TODO???
	// virtual tbool CCL_API canUnload () = 0;

	DECLARE_IID (ICodeResource)
};

DEFINE_IID (ICodeResource, 0xabee9e32, 0x7e50, 0x4bc4, 0x84, 0x20, 0x4d, 0x4, 0x41, 0x7, 0xe7, 0xcc)

//************************************************************************************************
// ICodeResourceLoader
/** \ingroup base_plug */
//************************************************************************************************

interface ICodeResourceLoader: IUnknown
{
	/** Get type of code resources. */
	virtual StringID CCL_API getType () const = 0;

	/** Check if given resource is compatible. */
	virtual tbool CCL_API isCodeResource (UrlRef path) = 0;

	/** Load code resource into memory. */
	virtual tresult CCL_API loadCodeResource (ICodeResource*& codeResource, UrlRef path) = 0;
	
	/** Check if given path is inside a known location. */
	virtual tbool CCL_API isKnownLocation (UrlRef path) = 0;

	DECLARE_IID (ICodeResourceLoader)
};

DEFINE_IID (ICodeResourceLoader, 0x68205645, 0xab69, 0x46b5, 0x80, 0xb1, 0x4b, 0x8d, 0x10, 0xfa, 0xb0, 0xe7)

//************************************************************************************************
// ICodeResourceLoaderHook
/** \ingroup base_plug */
//************************************************************************************************

interface ICodeResourceLoaderHook: IUnknown
{
	/** Called when code resource is loaded. */
	virtual void CCL_API onLoad (ICodeResource& codeResource) = 0;
	
	/** Called when code resource is unloaded. */
	virtual void CCL_API onUnload (ICodeResource& codeResource) = 0;

	DECLARE_IID (ICodeResourceLoaderHook)
};

DEFINE_IID (ICodeResourceLoaderHook, 0xd4c11b62, 0xfcdc, 0x4bd1, 0xba, 0x5a, 0x92, 0xdd, 0x57, 0xa3, 0x3f, 0x7c)

} // namespace CCL

#endif // _ccl_icoderesource_h
