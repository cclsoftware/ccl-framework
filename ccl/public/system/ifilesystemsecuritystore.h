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
// Filename    : ccl/public/system/ifilesystemsecuritystore.h
// Description : File System Security Store
//
//************************************************************************************************

#ifndef _ccl_ifilesystemsecuritystore_h
#define _ccl_ifilesystemsecuritystore_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IFileSystemSecurityStore
/**
	\ingroup ccl_system 

	IFileSystemSecurityStore is an interface for persisting and resolving OS-level file and folder
	access grants like security-scoped bookmarks on Apple platforms and future equivalents.
	It centralizes storage of these grants so that URLs remain usable across process sessions
	and app updates.

    Resolving a URL into an authorized handle when access is needed is done transparently
	by the framework, starting and ending scoped access on platforms that require it.
*/
//************************************************************************************************

interface IFileSystemSecurityStore: IUnknown
{
	/** Set specific security data for a url. */
	virtual tbool CCL_API setSecurityData (UrlRef url, VariantRef data) = 0;
	
	/** Get specific security data for a url. */
	virtual tbool CCL_API getSecurityData (Variant& data, UrlRef url) = 0;
	
	/** Save security data. */
	virtual void CCL_API saveSecurityData () = 0;
	
	/** Load security data. */
	virtual void CCL_API loadSecurityData () = 0;
	
	DECLARE_IID (IFileSystemSecurityStore)
};

DEFINE_IID (IFileSystemSecurityStore, 0x5f971bfa, 0x4b13, 0x4d3d, 0x87, 0x1a, 0xfc, 0xf9, 0xf2, 0x84, 0x65, 0xce)

} // namespace CCL

#endif // _ccl_ifilesystemsecuritystore_h
