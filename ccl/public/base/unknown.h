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
// Filename    : ccl/public/base/unknown.h
// Description : Unknown class
//
//************************************************************************************************

#ifndef _ccl_unknown_h
#define _ccl_unknown_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/base/primitives.h"
#include "ccl/public/base/debug.h"
#include "ccl/public/base/uid.h"

namespace CCL {

//************************************************************************************************
// Unknown
/** Reference-counted base class for implementing interfaces. \ingroup ccl_base */
//************************************************************************************************

class Unknown: public IUnknown
{
public:
	Unknown ()
	: retainCount (1)
	#if CCL_DEBUG_INTERNAL
	, debugFlags (0)
	#endif
	{}

	Unknown (const Unknown&)
	: retainCount (1)
	#if CCL_DEBUG_INTERNAL
	, debugFlags (0)
	#endif
	{}

	virtual ~Unknown () 
	{
		ASSERT (retainCount <= 1) // will be 1 for stack objects, 0 for heap objects

		#if DEBUG
		retainCount = 0; // for release to assert on destroyed stack objects
		#endif
	}

	Unknown& operator = (const Unknown&) 
	{ return *this; }

	unsigned int getRetainCount () const;

	// IUnknown
	DECLARE_UNKNOWN

private:
	unsigned int retainCount;

#if CCL_DEBUG_INTERNAL
protected:
	/// If this flag is set, anytime a retain is done one the object, the
	/// debugger is called. This allows to monitor the retain calls on a specfic class,
	/// if it is set to true in the constructor of a derived class.
	static constexpr int kDebugFlagRetain = 1<<0;
	int debugFlags;
#endif
};

} // namespace CCL

#endif // _ccl_unknown_h
