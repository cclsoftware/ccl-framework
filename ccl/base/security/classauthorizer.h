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
// Filename    : ccl/base/security/classauthorizer.h
// Description : Class Authorizer
//
//************************************************************************************************

#ifndef _ccl_classauthorizer_h
#define _ccl_classauthorizer_h

#include "ccl/base/security/featureauthorizer.h"

#include "ccl/public/plugins/classfactory.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// ClassAuthorizationFilter
//************************************************************************************************

class ClassAuthorizationFilter: public FeatureAuthorizer,
								public ClassFilter
{
public:
	ClassAuthorizationFilter (StringRef resourceSid, StringRef clientSid = nullptr);

	PROPERTY_BOOL (fullNameCheckEnabled, FullNameCheckEnabled)

	// ClassFilter
	bool matches (const ClassDesc& description) const override;

protected:
	String getClassName (const ClassDesc& description) const;
};

//************************************************************************************************
// ClassAuthorizer
//************************************************************************************************

class ClassAuthorizer: public ClassAuthorizationFilter
{
public:
	ClassAuthorizer (ClassFactory& factory, StringRef resourceSid, StringRef clientSid = nullptr);

	ClassFactory& getFactory () { return factory; }

	INLINE bool registerClass (const ClassDesc& description, 
							   ClassFactory::UnknownCreateFunc createFunc, 
							   void* userData = nullptr,
							   IAttributeList* attributes = nullptr);

protected:
	ClassFactory& factory;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ClassAuthorizer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool ClassAuthorizer::registerClass (const ClassDesc& description, ClassFactory::UnknownCreateFunc createFunc, void* userData, IAttributeList* attributes)
{
	if(checkAccess (getClassName (description)))
		return factory.registerClass (description, createFunc, userData, attributes);
	else
	{
		#if DEBUG
		Debugger::printf ("### Access to Class \"%s\" denied by Authorization Policy! ###\n", MutableCString (description.name).str ());
		#endif
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Security
} // namespace CCL

#endif // _ccl_classauthorizer_h
