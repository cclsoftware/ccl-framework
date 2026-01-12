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
// Filename    : ccl/security/securityhost.cpp
// Description : Security Scripting
//
//************************************************************************************************

#include "ccl/security/securityhost.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/security/classauthorizer.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/plugins/iclassfactory.h"

using namespace CCL;
using namespace Security;

//************************************************************************************************
// SecurityHost
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (SecurityHost, Object)
DEFINE_CLASS_NAMESPACE (SecurityHost, NAMESPACE_CCL)
DEFINE_SINGLETON (SecurityHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SecurityHost)
	DEFINE_METHOD_ARGR ("checkAccess", "resourceSid, itemSid", "bool")
END_METHOD_NAMES (SecurityHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SecurityHost::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "checkAccess")
	{
		String resourceSid (msg[0].asString ());
		String itemSid (msg[1].asString ());

		returnValue = FeatureAuthorizer (resourceSid).isAccessible (itemSid);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SecurityHost::onLoad (ICodeResource& codeResource)
{
	if(codeResource.getType () != CodeResourceType::kScript)
		return;

	// check for class authorization
	if(IAttributeList* metaInfo = codeResource.getMetaInfo ())
	{
		AttributeAccessor accessor (*metaInfo);
		bool authorizationNeeded = accessor.getBool ("Security:ClassAuthorization");
		if(authorizationNeeded)
		{
			String packageID = accessor.getString (Meta::kPackageID);
			ASSERT (!packageID.isEmpty ())

			IClassFactory* factory = codeResource.getClassFactory ();
			UnknownPtr<IObject> factoryObject (factory);
			ASSERT (factory != nullptr && factoryObject.isValid ())

			ClassAuthorizationFilter filter (packageID);
			for(int i = 0; i < factory->getNumClasses (); i++)
			{
				ClassDesc description;
				factory->getClassDescription (description, i);

				if(!filter.matches (description))
				{
					Variant returnValue;
					factoryObject->invokeMethod (returnValue, Message ("remove", UIDString (description.classID)));
					ASSERT (returnValue.asBool ())
					i--;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SecurityHost::onUnload (ICodeResource& codeResource)
{
	// nothing here
}
