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
// Filename    : ccl/public/gui/framework/ipresentable.h
// Description : Presentable Interface
//
//************************************************************************************************

#ifndef _ccl_ipresentable_h
#define _ccl_ipresentable_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

interface IView;
interface IVisualStyle;

//************************************************************************************************
// IPresentable
/**
	\ingroup gui */
//************************************************************************************************

interface IPresentable: IUnknown
{
	/** Create image representation. */
	virtual IImage* CCL_API createImage (const Point& size, const IVisualStyle& style) = 0;

	/** Create view representation. */
	virtual IView* CCL_API createView (const Rect& size, const IVisualStyle& style) = 0;

	/** Create text representation. */
	virtual String CCL_API createText () = 0;

	DECLARE_IID (IPresentable)
};

DEFINE_IID (IPresentable, 0x583282b5, 0x6949, 0x445e, 0x91, 0x1, 0x3c, 0x44, 0x57, 0x7e, 0xc0, 0x3)

} // namespace CCL

#endif // _ccl_ipresentable_h
