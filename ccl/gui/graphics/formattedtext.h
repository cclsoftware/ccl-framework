//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/graphics/formattedtext.h
// Description : Formatted Text
//
//************************************************************************************************

#ifndef _ccl_formattedtext_h
#define _ccl_formattedtext_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/textformatting.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/gui/graphics/color.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

interface IFormattedTextHandler;

//************************************************************************************************
// TextStyles
/** Common text style identifiers. */
//************************************************************************************************

namespace TextStyles
{
	DEFINE_STRINGID (kBlockQuote, "")
	DEFINE_STRINGID (kCodeBlock, "")
	DEFINE_STRINGID (kCode, "")
	DEFINE_STRINGID (kLink, "")
}

//************************************************************************************************
// FormattedText
/** Stores string with range-based format information. */
//************************************************************************************************

class FormattedText: public Object
{
public:
	FormattedText (StringRef plainText = nullptr);

	class FormatRange;

	StringRef getText () const;

	void addFormatRange (int start, int length, TextNodeType type, VariantRef argument = Variant ());
	const ObjectArray& getFormatRanges () const;
	const FormatRange* findFormatRange (int textPosition) const;

	void applyTo (IFormattedTextHandler& handler) const;

private:
	String plainText;
	ObjectArray formatRanges;
};

//************************************************************************************************
// IFormattedTextHandler
//************************************************************************************************

interface IFormattedTextHandler
{
	virtual void applyFormat (const FormattedText::FormatRange& range) = 0;
};

//************************************************************************************************
// FormattedText::FormatRange
//************************************************************************************************

class FormattedText::FormatRange: public Object
{
public:
	FormatRange (TextNodeType type)
	: type (type),
	  start (0),
	  length (0)
	{}

	PROPERTY_VARIABLE (TextNodeType, type, Type)
	PROPERTY_OBJECT (Variant, argument, Argument)
	PROPERTY_VARIABLE (int, start, Start)
	PROPERTY_VARIABLE (int, length, Length)

	void setArgumentColor (Color color)
	{
		setArgument ((int64)(uint32)color);
	}

	Color getArgumentColor () const
	{
		int64 colorCode = getArgument ().asLargeInt ();
		return Color::fromInt ((uint32)colorCode);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// FormattedText inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringRef FormattedText::getText () const
{ return plainText; }

inline const ObjectArray& FormattedText::getFormatRanges () const
{ return formatRanges; }

} // namespace CCL

#endif // _ccl_formattedtext_h
