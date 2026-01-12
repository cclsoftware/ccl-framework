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
// Filename    : ccl/system/filesystemsecuritystore.h
// Description : File system security store base class
//
//************************************************************************************************

#ifndef _ccl_filesystemsecuritystore_h
#define _ccl_filesystemsecuritystore_h

#include "ccl/base/singleton.h"

#include "ccl/public/system/ifilesystemsecuritystore.h"

namespace CCL {

//************************************************************************************************
// FileSystemSecurityStore
//************************************************************************************************

class FileSystemSecurityStore: public Object,
							   public IFileSystemSecurityStore,
							   public ExternalSingleton<FileSystemSecurityStore>
{
public:
	// IFileSystemSecurityStore
	tbool CCL_API setSecurityData (UrlRef url, VariantRef data) override { return true; };
	tbool CCL_API getSecurityData (Variant& data, UrlRef url) override { return true; };
	void CCL_API saveSecurityData () override {};
	void CCL_API loadSecurityData () override {};
	
	CLASS_INTERFACE (IFileSystemSecurityStore, Object)
};

} // namespace CCL

#endif // _ccl_filesystemsecuritystore_h
