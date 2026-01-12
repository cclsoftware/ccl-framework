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
// Filename    : ccl/public/storage/ipersistattributes.h
// Description : Attribute Persistence Interface
//
//************************************************************************************************

#ifndef _ccl_ipersistattributes_h
#define _ccl_ipersistattributes_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// IPersistAttributes
/**	Interface to load and save object using simple attributes.
	\ingroup base_io */
//************************************************************************************************

interface IPersistAttributes: IUnknown
{
	/** Save member values. */
	virtual tresult CCL_API storeValues (IAttributeList& values) const = 0;

	/** Restore member values. */
	virtual tresult CCL_API restoreValues (const IAttributeList& values) = 0;

	DECLARE_IID (IPersistAttributes)
};

DEFINE_IID (IPersistAttributes, 0x479a1fc0, 0x9f10, 0x4fa0, 0x94, 0xb0, 0x6b, 0x8d, 0x15, 0xd4, 0x87, 0x40)

} // namespace CCL

#endif // _ccl_ipersistattributes_h
