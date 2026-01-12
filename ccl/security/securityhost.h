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
// Filename    : ccl/security/securityhost.h
// Description : Security Scripting
//
//************************************************************************************************

#ifndef _ccl_securityhost_h
#define _ccl_securityhost_h

#include "ccl/base/singleton.h"

#include "ccl/public/plugins/icoderesource.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// SecurityHost
/** Object accessed from script applications via "Host.Security". */
//************************************************************************************************

class SecurityHost: public Object,
					public Singleton<SecurityHost>,
					public ICodeResourceLoaderHook
{
public:
	DECLARE_CLASS_ABSTRACT (SecurityHost, Object)
	DECLARE_METHOD_NAMES (SecurityHost)

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// ICodeResourceLoaderHook
	void CCL_API onLoad (ICodeResource& codeResource) override;
	void CCL_API onUnload (ICodeResource& codeResource) override;

	CLASS_INTERFACE (ICodeResourceLoaderHook, Object)
};

} // namespace Security
} // namespace CCL

#endif // _ccl_securityhost_h
