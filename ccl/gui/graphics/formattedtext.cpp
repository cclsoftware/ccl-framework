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
// Filename    : ccl/gui/graphics/formattedtext.cpp
// Description : Formatted Text
//
//************************************************************************************************

#include "ccl/gui/graphics/formattedtext.h"

using namespace CCL;

//************************************************************************************************
// TextStyles
//************************************************************************************************

DEFINE_STRINGID_ (TextStyles::kBlockQuote, "BlockQuote")
DEFINE_STRINGID_ (TextStyles::kCodeBlock, "CodeBlock")
DEFINE_STRINGID_ (TextStyles::kCode, "Code")
DEFINE_STRINGID_ (TextStyles::kLink, "Link")

//************************************************************************************************
// FormattedText
//************************************************************************************************

FormattedText::FormattedText (StringRef plainText)
: plainText (plainText)
{
	formatRanges.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormattedText::addFormatRange (int start, int length, TextNodeType type, VariantRef argument)
{
	auto* range = NEW FormatRange (type);
	range->setStart (start);
	range->setLength (length);
	range->setArgument (argument);
	formatRanges.add (range);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FormattedText::FormatRange* FormattedText::findFormatRange (int textPosition) const
{
	for(auto* range : iterate_as<FormattedText::FormatRange> (formatRanges))
		if(textPosition >= range->getStart () && textPosition < range->getStart () + range->getLength ())
			return range;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormattedText::applyTo (IFormattedTextHandler& handler) const
{
	for(auto* range : iterate_as<FormattedText::FormatRange> (formatRanges))
		handler.applyFormat (*range);
}