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
// Filename    : ccl/public/base/imessage.h
// Description : Message interface
//
//************************************************************************************************

#ifndef _ccl_imessage_h
#define _ccl_imessage_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// Common messages
//************************************************************************************************

/** Object state changed, no further information. */
DEFINE_STRINGID (kChanged, "changed")

/** A property of the object has changed (IObject::getProperty). */
DEFINE_STRINGID (kPropertyChanged, "propertyChanged")

/** Object is about to be destroyed. */
DEFINE_STRINGID (kDestroyed, "destroyed")

//************************************************************************************************
// IMessage
/**
	\ingroup ccl_base */
//************************************************************************************************

interface IMessage: IUnknown
{
	/** Get message identifier. */
	virtual StringID CCL_API getID () const = 0;

	/** Get number of arguments. */
	virtual int CCL_API getArgCount () const = 0; 
	
	/** Get argument at index. */
	virtual VariantRef CCL_API getArg (int index) const = 0; 

	DECLARE_IID (IMessage)

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool operator == (CStringPtr id) const;
	bool operator != (CStringPtr id) const;
	bool operator == (StringID id) const;
	bool operator != (StringID id) const;
	const Variant& operator [] (int index) const;
};

DEFINE_IID (IMessage, 0xd943e242, 0xf8c7, 0x4ed8, 0x80, 0x5c, 0x4c, 0x47, 0x85, 0x46, 0xb1, 0x14)

} // namespace CCL

#endif // _ccl_imessage_h
