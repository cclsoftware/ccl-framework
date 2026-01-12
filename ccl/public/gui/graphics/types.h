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
// Filename    : ccl/public/gui/graphics/types.h
// Description : Graphics types
//
//************************************************************************************************

#ifndef _ccl_graphicstypes_h
#define _ccl_graphicstypes_h

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/pen.h"
#include "ccl/public/gui/graphics/brush.h"
#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/textformat.h"
#include "ccl/public/gui/graphics/transform.h"

namespace CCL {

class ImageMode;
interface IImage;
interface IGraphics;
interface IGraphicsPath;
interface ITextLayout;

//************************************************************************************************
// GraphicsContentHint
/**
	\ingroup gui_graphics */
//************************************************************************************************

DEFINE_ENUM (GraphicsContentHint)
{
	kGraphicsContentEmpty,			///< content is currently empty, nothing will be drawn into destination
	kGraphicsContentOpaque,			///< destination will be filled with opaque content
	kGraphicsContentTranslucent,	///< destination will be filled with translucent content

	kGraphicsContentHintDefault = kGraphicsContentTranslucent
};

} // namespace CCL

#endif // _ccl_graphicstypes_h
