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
// Filename    : ccl/base/security/featureauthorizer.h
// Description : Feature Authorizer
//
//************************************************************************************************

#ifndef _ccl_featureauthorizer_h
#define _ccl_featureauthorizer_h

#include "ccl/public/base/debug.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/iunknownlist.h"
#include "ccl/public/security/iauthorizationpolicy.h"

#if defined(__GNUC__) && !defined(__clang__)
// Not inlineable with GCC
#define FEATUREAUTHORIZER_INLINE inline
#else
#define FEATUREAUTHORIZER_INLINE INLINE
#endif

namespace CCL {
interface IEncryptionKeyProvider;

namespace Security {

//************************************************************************************************
// PolicyAccessor
//************************************************************************************************

namespace PolicyAccessor
{
	/**
		Helper to get data from authorization policy.
			<AuthAssociatedData sid="{dataSid}>
				<AuthData sid="{dataPrefix}{outData}"/> 
			</AuthAssociatedData>
	*/
	bool getDataFromPolicy (String& outData, StringRef dataSid, StringRef dataPrefix);

	/** Helper to get encryption key from policy. */
	bool getEncryptionKeyFromPolicy (String& key, StringRef dataSid);
	IEncryptionKeyProvider* getEncryptionKeyProvider ();
}

//************************************************************************************************
// FeatureAuthorizer
//************************************************************************************************

class FeatureAuthorizer
{
public:
	enum ModeFlags
	{
		kStrictClient = 1<<0, //< don't allow wildcards (*) when matching client IDs
		kStrictItem = 1<<1 //< don't allow wildcards (*) when matching item IDs
	};

	static String getFullAppId ();
	static String makeFullAppId (StringRef appId);

	FeatureAuthorizer (StringRef resourceSid, StringRef clientSid = nullptr, int mode = 0);

	FEATUREAUTHORIZER_INLINE bool isAccessible (StringRef itemSid) const;
	INLINE bool isDefaultAccessible () const;

protected:
	static const String kAny;
	static const String kDefaultItemSid;

	String clientSid;
	Authorization::IPolicyItem* resource;
	int mode;

	INLINE bool checkAccess (StringRef itemSid) const;
	INLINE bool isItemAccessible (Authorization::IPolicyItem* client, StringRef itemSid) const;
};

//************************************************************************************************
// MultiAuthorizer
//************************************************************************************************

class MultiAuthorizer: public Vector<FeatureAuthorizer*>
{
public:
	INLINE bool isAccessible (StringRef itemSid) const
	{
		VectorForEach (*this, FeatureAuthorizer*, authorizer)
			if(authorizer->isAccessible (itemSid))
				return true;
		EndFor
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// FeatureAuthorizer inline
//////////////////////////////////////////////////////////////////////////////////////////////////

FEATUREAUTHORIZER_INLINE bool FeatureAuthorizer::isAccessible (StringRef itemSid) const
{
	if(checkAccess (itemSid))
		return true;
	else
	{
		CCL_PRINTF ("### Access to Feature \"%s\" denied by Authorization Policy! ###\n", MutableCString (itemSid).str ());
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool FeatureAuthorizer::isDefaultAccessible () const
{
	return isAccessible (kDefaultItemSid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool FeatureAuthorizer::isItemAccessible (Authorization::IPolicyItem* client, StringRef itemSid) const
{
	Authorization::IPolicyItem* fallbackItem = nullptr;
	IterForEachUnknown (client->newItemIterator (), unk)
		UnknownPtr<Authorization::IPolicyItem> item (unk);
		ASSERT (item)
		String sid (item->getItemSID ());

		if(sid == itemSid)
		{
			if(item->getItemType () == Authorization::IPolicyItem::kAccessDenied)
				return false;
			if(item->getItemType () == Authorization::IPolicyItem::kAccessAllowed)
				return true;
		}
		else if(sid == kAny && !get_flag<int> (mode, kStrictItem))
			fallbackItem = item;
	EndFor

	if(fallbackItem)
	{
		if(fallbackItem->getItemType () == Authorization::IPolicyItem::kAccessDenied)
			return false;
		if(fallbackItem->getItemType () == Authorization::IPolicyItem::kAccessAllowed)
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INLINE bool FeatureAuthorizer::checkAccess (StringRef itemSid) const
{
	if(resource == nullptr)
		return false;

	if(Authorization::IPolicyItem* client = resource->findItem (clientSid, Authorization::IPolicyItem::kClient))
	{
		bool allowAny = !get_flag<int> (mode, kStrictClient);
		if(allowAny || client->getItemSID () == clientSid)
			return isItemAccessible (client, itemSid);
	}

	// check for conditions
	IterForEachUnknown (resource->newItemIterator (), unk)
		UnknownPtr<Authorization::IPolicyItem> item (unk);
		ASSERT (item)
		if(item->getItemType () == Authorization::IPolicyItem::kCondition)
		{
			ForEachStringToken (item->getItemSID (), CCLSTR (","), resourceSid) // could be multiple alternatives
				FeatureAuthorizer conditionAuthorizer (resourceSid, clientSid, mode);
				if(conditionAuthorizer.isAccessible (resourceSid) && isItemAccessible (item, itemSid)) // (double use of resource name)
					return true;
			EndFor
		}
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Security
} // namespace CCL

#undef FEATUREAUTHORIZER_INLINE

#endif // _ccl_featureauthorizer_h
