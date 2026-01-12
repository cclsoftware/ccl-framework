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
// Filename    : ccl/public/gui/graphics/imarkuppainter.h
// Description : Markup Painter Interface
//
//************************************************************************************************

#ifndef _ccl_imarkuppainter_h
#define _ccl_imarkuppainter_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (MarkupPainter, 0x9253c60e, 0xfd30, 0x4706, 0x91, 0x03, 0x1d, 0xce, 0x48, 0xfd, 0xd7, 0x49);
}

//************************************************************************************************
// IMarkupPainter
/** Markup painter provides methods to draw and measure strings containing markup.
	\ingroup gui_graphics */
//************************************************************************************************

interface IMarkupPainter: IUnknown
{
	/** Draw Unicode string containing markup with given brush. */
	virtual tresult CCL_API drawMarkupString (IGraphics& graphics, RectRef rect, StringRef text, FontRef font, BrushRef brush, 
											  AlignmentRef alignment = Alignment ()) = 0;
	
	/** Draw Unicode string containing markup with given brush (floating coordinates). */
	virtual tresult CCL_API drawMarkupString (IGraphics& graphics, RectFRef rect, StringRef text, FontRef font, BrushRef brush,
											  AlignmentRef alignment = Alignment ()) = 0;

	/** Measure Unicode string containing markup with given font and ITextLayout::MeasureFlags. */
	virtual tresult CCL_API measureMarkupString (Rect& size, StringRef text, FontRef font, int flags = 0) = 0;
	
	/** Measure Unicode string containing markup with given font and ITextLayout::MeasureFlags (floating coordinates). */
	virtual tresult CCL_API measureMarkupString (RectF& size, StringRef text, FontRef font, int flags = 0) = 0;

	DECLARE_IID (IMarkupPainter)
};

DEFINE_IID (IMarkupPainter, 0x9de6ff33, 0xab31, 0x47b1, 0x87, 0x93, 0x4b, 0xa0, 0x40, 0x32, 0x25, 0xf8)

} // namespace CCL

#endif // _ccl_imarkuppainter_h
