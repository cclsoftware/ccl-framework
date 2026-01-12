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
// Filename    : core/text/coreattributehandler.h
// Description : Attribute Handler Interface
//
//************************************************************************************************

#ifndef _coreattributehandler_h
#define _coreattributehandler_h

#include "core/public/coretypes.h"

namespace Core {

//************************************************************************************************
// AttributeHandler
//************************************************************************************************

struct AttributeHandler
{
	enum Flags
	{
		kInplace		= 1<<0, ///< flag for inplace parser / shared attribute identifier
		kInplaceValue	= 1<<1  ///< flag for inplace parser / shared attribute value (string)
	};

	virtual void startObject (CStringPtr id, int flags = 0) = 0;
	virtual void endObject (CStringPtr id, int flags = 0) = 0;

	virtual void startArray (CStringPtr id, int flags = 0) = 0;
	virtual void endArray (CStringPtr id, int flags = 0) = 0;

	virtual void setValue (CStringPtr id, int64 value, int flags = 0) = 0;
	virtual void setValue (CStringPtr id, double value, int flags = 0) = 0;
	virtual void setValue (CStringPtr id, bool value, int flags = 0) = 0;
	virtual void setValue (CStringPtr id, CStringPtr value, int flags = 0) = 0;
	virtual void setNullValue (CStringPtr id, int flags = 0) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////

	void setValue (CStringPtr id, int value, int flags = 0)		{ setValue (id, (int64)value, flags); }
	void setValue (CStringPtr id, float value, int flags = 0)	{ setValue (id, (double)value, flags); }
};

} // namespace Core

#endif // _coreattributehandler_h
