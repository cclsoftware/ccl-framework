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
// Filename    : ccl/base/security/classauthorizer.cpp
// Description : Class Authorizer
//
//************************************************************************************************

#include "ccl/base/security/classauthorizer.h"

using namespace CCL;
using namespace Security;

//************************************************************************************************
// ClassAuthorizationFilter
//************************************************************************************************

ClassAuthorizationFilter::ClassAuthorizationFilter (StringRef resourceSid, StringRef clientSid)
: FeatureAuthorizer (resourceSid, clientSid),
  fullNameCheckEnabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ClassAuthorizationFilter::getClassName (const ClassDesc& description) const
{
	String className;
	if(fullNameCheckEnabled)
	{
		className << description.category;
		if(!description.subCategory.isEmpty ())
			className << ":" << description.subCategory;
		className << ":" << description.name;
	}
	else
		className = description.name;
	return className;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassAuthorizationFilter::matches (const ClassDesc& description) const
{	
	if(checkAccess (getClassName (description)))
		return true;
	else
	{
		#if DEBUG
		Debugger::printf ("### Access to Class \"%s\" denied by Authorization Policy! ###\n", MutableCString (description.name).str ());
		#endif
		return false;
	}
}

//************************************************************************************************
// ClassAuthorizer
//************************************************************************************************

ClassAuthorizer::ClassAuthorizer (ClassFactory& factory, StringRef resourceSid, StringRef clientSid)
: ClassAuthorizationFilter (resourceSid, clientSid),
  factory (factory)
{}

