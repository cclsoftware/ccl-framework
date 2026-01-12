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
// Filename    : ccl/public/gui/graphics/textformat.h
// Description : Text Format
//
//************************************************************************************************

#ifndef _ccl_textformat_h
#define _ccl_textformat_h

#include "ccl/public/gui/graphics/alignment.h"

#include "ccl/public/base/cclmacros.h"

#include "ccl/meta/generated/cpp/graphics-constants-generated.h"

namespace CCL {

class TextFormat;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text format reference
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Text format reference type. */
typedef const TextFormat& TextFormatRef;

//************************************************************************************************
// PlainTextFormat
/** The text format class below is binary equivalent to this C structure. 
	\ingroup gui_graphics */
//************************************************************************************************

struct PlainTextFormat
{
	Alignment alignment = Alignment::kLeftCenter;
	int flags = 0;
};

//************************************************************************************************
// TextFormat
/** Text format definition. 
	\ingroup gui_graphics */
//************************************************************************************************

class TextFormat: protected PlainTextFormat
{
public:
	/** Format flags. */
	enum Flags
	{
		kWordBreak = kTextFormatOptionsWordBreak
	};

	TextFormat (AlignmentRef _alignment = Alignment::kLeftCenter, int _flags = 0)
	{
		alignment = _alignment;
		flags = _flags;
	}

	TextFormat (TextFormatRef format)
	{
		alignment = format.getAlignment ();
		flags = format.getFlags ();
	}

	PROPERTY_BY_REFERENCE (Alignment, alignment, Alignment)
	PROPERTY_BY_VALUE (int, flags, Flags)
	PROPERTY_FLAG (flags, kWordBreak, isWordBreak)
};

} // namespace CCL

#endif // _ccl_textformat_h
