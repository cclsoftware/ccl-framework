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
// Filename    : ccl/public/security/iauthorizationpolicy.h
// Description : Authorization Policy Interface
//
//************************************************************************************************

#ifndef _ccl_iauthorizationpolicy_h
#define _ccl_iauthorizationpolicy_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
interface IUnknownIterator;

namespace Security {
namespace Authorization {

//************************************************************************************************
// Authorization::IPolicyItem
//************************************************************************************************

interface IPolicyItem: IUnknown
{
	/** Policy item type. */
	DEFINE_ENUM (ItemType)
	{
		kItem,
		kContainer,
		kResource,
		kClient,
		kAccessAllowed,
		kAccessDenied,
		kAssociatedData,
		kData,
		kCondition
	};

	/** Get item type. */
	virtual ItemType CCL_API getItemType () const = 0;

	/** Get item security identifier. */
	virtual StringRef CCL_API getItemSID () const = 0;

	/** Create iterator for subitems. */
	virtual IUnknownIterator* CCL_API newItemIterator () const = 0;

	/** Find subitem with given security identifier and type. */
	virtual IPolicyItem* CCL_API findItem (StringRef sid, ItemType type) const = 0;

	DECLARE_IID (IPolicyItem)
};

DEFINE_IID (IPolicyItem, 0x1547206b, 0xfd8f, 0x41e2, 0x96, 0x68, 0x66, 0x16, 0xfe, 0xa0, 0xb8, 0xf3)

} // namespace Authorization
} // namespace Security
} // namespace CCL

#endif // _ccl_iauthorizationpolicy_h
