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
// Filename    : ccl/public/text/iattributehandler.h
// Description : Attribute Handler Interface
//
//************************************************************************************************

#ifndef _ccl_iattributehandler_h
#define _ccl_iattributehandler_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IAttributeHandler
/**	\ingroup ccl_text */
//************************************************************************************************

interface IAttributeHandler: IUnknown
{
	/** Stringification options. */
	enum Options
	{
		kSuppressWhitespace = 1<<0,		///< suppress whitespace (tab, new line)
		kDoublePrecisionEnabled = 1<<1	///< enable double-precision floating point numbers
	};

	/** Start of object definition. */
	virtual tbool CCL_API startObject (StringRef id) = 0;

	/** End of object definition. */
	virtual tbool CCL_API endObject (StringRef id) = 0;

	/** Start of array definition. */
	virtual tbool CCL_API startArray (StringRef id) = 0;

	/** End of array definition. */
	virtual tbool CCL_API endArray (StringRef id) = 0;

	/** Value definition (string, bool, numeric, null). */
	virtual tbool CCL_API setValue (StringRef id, VariantRef value) = 0;

	/** Value definition with UTF-8 identifier. */
	virtual tbool CCL_API setValue (CStringPtr id, VariantRef value) = 0;

	DECLARE_IID (IAttributeHandler)
};

DEFINE_IID (IAttributeHandler, 0x742fea4b, 0xa666, 0x47cf, 0xa6, 0x93, 0x1c, 0xaa, 0x59, 0x5a, 0x8d, 0xbc)

} // namespace CCL

#endif // _ccl_iattributehandler_h
