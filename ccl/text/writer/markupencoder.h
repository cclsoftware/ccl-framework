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
// Filename    : ccl/text/writer/markupencoder.h
// Description : Markup Encoder
//
//************************************************************************************************

#ifndef _ccl_markupencoder_h
#define _ccl_markupencoder_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// MarkupEncoder
//************************************************************************************************

class MarkupEncoder: public Unknown
{
public:
	virtual String encode (StringRef text) = 0;

	virtual MutableCString encodeToASCII (StringRef text) = 0;

	virtual String decode (StringRef text) = 0;
};

//************************************************************************************************
// PlainMarkupEncoder
//************************************************************************************************

class PlainMarkupEncoder: public MarkupEncoder
{
public:
	// MarkupEncoder
	String encode (StringRef text) override					{ return text; }
	MutableCString encodeToASCII (StringRef text) override	{ return text; }
	String decode (StringRef text) override					{ return text; }
};

} // namespace CCL

#endif // _ccl_markupencoder_h
