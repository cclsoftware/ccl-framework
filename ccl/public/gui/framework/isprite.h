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
// Filename    : ccl/public/gui/framework/isprite.h
// Description : Sprite Interface
//
//************************************************************************************************

#ifndef _ccl_isprite_h
#define _ccl_isprite_h

#include "ccl/public/gui/framework/idrawable.h"

namespace CCL {

interface IView;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in Sprite classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (FloatingSprite, 0x7da79f66, 0x4676, 0x460e, 0xb1, 0x66, 0x2c, 0xde, 0xf8, 0x74, 0xbd, 0xf4)
	DEFINE_CID (SublayerSprite, 0x0876288A, 0xBEB8, 0xF243, 0x94, 0xDF, 0x63, 0xD7, 0xC7, 0xCB, 0x68, 0xD1)
}

//************************************************************************************************
// ISprite
/** Sprite interface. 
	\ingroup gui */
//************************************************************************************************

interface ISprite: IUnknown
{
	/** Sprite options. */
	enum Options
	{
		kKeepOnTop = 1<<0	///< do not clip to IView client area (floating sprite)
	};

	/** Initialize sprite. */
	virtual tresult CCL_API construct (IView* view, RectRef size = Rect (), IDrawable* drawable = nullptr, int options = 0) = 0;

	/** Get current size. */
	virtual RectRef CCL_API getSize () const = 0;

	/** Get associated IDrawable. */
	virtual IDrawable* CCL_API getDrawable () const = 0;

	/** Check if sprite is currently visible. */
	virtual tbool CCL_API isVisible () const = 0;

	/** Show sprite. */
	virtual void CCL_API show () = 0;

	/** Hide sprite. */
	virtual void CCL_API hide () = 0;

	/** Move (and resize) sprite. */
	virtual void CCL_API move (RectRef size) = 0;
	
	/** Move sprite. */
	virtual void CCL_API moveTo (PointRef position) = 0;

	/** Inform sprite that is has been scrolled on screen. */
	virtual void CCL_API scrolled (PointRef delta) = 0;

	/** Inform sprite that the drawable has changed. */
	virtual void CCL_API refresh () = 0;

	/** Let drawable take opacity from its content, implemented only in certain sprite classes. */
	virtual void CCL_API takeOpacity (IDrawable* drawable) = 0;

	DECLARE_IID (ISprite)
};

DEFINE_IID (ISprite, 0x7abefc8, 0x48ad, 0x401d, 0x88, 0x9c, 0xc2, 0xef, 0x1a, 0x11, 0x8b, 0xe7)

} // namespace CCL

#endif // _ccl_isprite_h
