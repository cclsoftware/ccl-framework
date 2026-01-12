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
// Filename    : ccl/public/base/uid.h
// Description : UID class
//
//************************************************************************************************

#ifndef _ccl_uid_h
#define _ccl_uid_h

#include "ccl/public/base/uiddef.h"

namespace CCL {

class MutableCString;

//************************************************************************************************
// UIDBuffer
/** 16 byte buffer for handling UIDs in an endian-safe way. */
//************************************************************************************************

typedef uint8 UIDBuffer[16];

//************************************************************************************************
// UID
/** Unique Identifier class with constructor. \ingroup ccl_base */
//************************************************************************************************

struct UID: UIDBytes
{
	UID (int unused = 0);

	UID (UIDRef uid);

	UID (uint32 data1, uint16 data2, uint16 data3, 
		 uint8 a, uint8 b, uint8 c, uint8 d, 
		 uint8 e, uint8 f, uint8 g, uint8 h);

	using UIDBytes::operator =;

	bool generate ();
	uint32 hash () const;
	
	using UIDBytes::toCString;
	void toCString (MutableCString& cString, int format = kStandard) const;

	void toString (String& string, int format = kStandard) const;
	bool fromString (StringRef string, int format = kStandard);	

	void toBuffer (UIDBuffer& buffer) const;
	bool fromBuffer (const UIDBuffer& buffer);
};

} // namespace CCL

#endif // _ccl_uid_h
