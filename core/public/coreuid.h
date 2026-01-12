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
// Filename    : core/public/coreuid.h
// Description : 16-Byte GUID
//
//************************************************************************************************

#ifndef _coreuid_h
#define _coreuid_h

#include "core/public/coretypes.h"

namespace Core {

struct UIDBytes;

/**	Unique Identifier Reference.
	\ingroup core  */
typedef const UIDBytes& UIDRef;

//************************************************************************************************
// UID macros
//************************************************************************************************

/** Macro for inline UID definition. */
#define INLINE_UID(data1, data2, data3, a, b, c, d, e, f, g, h) \
{(data1), (data2), (data3), {(a), (b), (c), (d), (e), (f), (g), (h)}}
	
//************************************************************************************************
// UIDBytes
/**	16-Byte Globally Unique Identifier (GUID).
	\ingroup core  */
//************************************************************************************************

struct UIDBytes
{
	uint32 data1;
	uint16 data2;
	uint16 data3;
	uint8 data4[8];

	/** Check if UID is valid, i.e. not equal to kNullUID. */
	bool isValid () const;
	
	/** Compare with other UID. */
	bool equals (UIDRef uid) const;

	/** Initialize with kNullUID. */
	UIDBytes& prepare ();
	
	/** Assign other UID. */
	UIDBytes& assign (UIDRef uid);

	/** Assign other UID. */
	UIDBytes& operator = (UIDRef uid);
	
	bool operator == (UIDRef uid) const;
	bool operator != (UIDRef uid) const;

	/** UID string format. */
	enum Format
	{
		kStandard,			///< standard format (with braces and separators)
		kStandardNoBraces,	///< standard format without braces
		kCompact			///< compact format (no braces, no separators)
	};

	/** Print UID to C-string buffer in given format. */
	void toCString (char* cString, int cStringSize, int format = kStandard) const;
	
	/** Scan UID from C-string in given format. */
	bool fromCString (CStringPtr cString, int format = kStandard);

	/** Get UID format string. */
	static CStringPtr getFormatString (int format);
};

/**	Empty UID (all zeros).
	\ingroup core  */
extern const UIDBytes kNullUID;

//////////////////////////////////////////////////////////////////////////////////////////////////
// UIDBytes inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline UIDBytes& UIDBytes::prepare () 
{ return assign (kNullUID);  }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline UIDBytes& UIDBytes::assign (UIDRef uid)
{ ::memcpy (static_cast<void*> (this), &uid, sizeof(UIDBytes)); return *this; }
	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline UIDBytes& UIDBytes::operator = (UIDRef uid)		
{ return assign (uid); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool UIDBytes::isValid () const
{ return !equals (kNullUID);  }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool UIDBytes::equals (UIDRef uid) const
{ return ::memcmp (this, &uid, sizeof(UIDBytes)) == 0; }
	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool UIDBytes::operator == (UIDRef uid) const	
{ return equals (uid); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool UIDBytes::operator != (UIDRef uid) const	
{ return !equals (uid); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _coreuid_h
