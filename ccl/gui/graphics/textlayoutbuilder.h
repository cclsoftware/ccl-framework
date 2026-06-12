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
// Filename    : ccl/gui/graphics/textlayoutbuilder.h
// Description : Text Layout Builder
//
//************************************************************************************************

#ifndef _ccl_textlayoutbuilder_h
#define _ccl_textlayoutbuilder_h

#include "ccl/gui/graphics/formattedtext.h"

#include "ccl/public/gui/graphics/itextlayout.h"

namespace CCL {

interface IVisualStyle;

//************************************************************************************************
// TextLayoutBuilder
//************************************************************************************************

class TextLayoutBuilder: public Object,
						 public IFormattedTextHandler
{
public:
	TextLayoutBuilder (ITextLayout& textLayout, const IVisualStyle* style = nullptr);
	
	// IFormattedTextHandler
	void applyFormat (const FormattedText::FormatRange& range) override;
	
protected:
	ITextLayout& textLayout;
	const IVisualStyle* style;

	void applyTextStyle (const IVisualStyle* style, StringID styleName, const ITextLayout::Range& range);
};

} // namespace CCL

#endif // _ccl_textlayoutbuilder_h
