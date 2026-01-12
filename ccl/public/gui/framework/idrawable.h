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
// Filename    : ccl/public/gui/framework/idrawable.h
// Description : Drawable Interface
//
//************************************************************************************************

#ifndef _ccl_idrawable_h
#define _ccl_idrawable_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

//************************************************************************************************
// IDrawable
/** 
	\ingroup gui */
//************************************************************************************************

interface IDrawable: IUnknown
{
	struct DrawArgs
	{
		IGraphics& graphics;
		RectRef size;
		const UpdateRgn& updateRgn;

		DrawArgs (IGraphics& graphics,
				  RectRef size,
				  const UpdateRgn& updateRgn)
		: graphics (graphics),
		  size (size),
		  updateRgn (updateRgn)
		{}
	};

	virtual void CCL_API draw (const DrawArgs& args) = 0;

	virtual float CCL_API getOpacity () const = 0;

	virtual void CCL_API takeOpacity () = 0; ///< take opacity from brush color or image mode, if possible

	DECLARE_IID (IDrawable)
};

DEFINE_IID (IDrawable, 0xe999120b, 0xca98, 0x4f48, 0xab, 0x2f, 0x76, 0xa9, 0xcc, 0x64, 0x4, 0xba)

//************************************************************************************************
// IImageDrawable
/** 
	\ingroup gui */
//************************************************************************************************

interface IImageDrawable: IDrawable
{
	virtual IImage* CCL_API getImage () const = 0;

	DECLARE_IID (IImageDrawable)
};

DEFINE_IID (IImageDrawable, 0xcd8bad78, 0xbe69, 0x4515, 0x83, 0x9, 0xaf, 0x34, 0x70, 0xa, 0x8a, 0xf0)

//************************************************************************************************
// AbstractDrawable
/** 
	\ingroup gui */
//************************************************************************************************

class AbstractDrawable: public IDrawable
{
public:
	// IDrawable
	float CCL_API getOpacity () const override { return 1.f; }
	void CCL_API takeOpacity () override {}
};

//************************************************************************************************
// SolidDrawable
/** 
	\ingroup gui */
//************************************************************************************************

class SolidDrawable: public Unknown,
					 public IDrawable
{
public:
	SolidDrawable (const SolidBrush& brush, float opacity = 1.f);

	PROPERTY_OBJECT (SolidBrush, brush, Brush)
	
	void setOpacity (float opacity);	///< set opacity

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override;
	float CCL_API getOpacity () const override;
	void CCL_API takeOpacity () override;		///< take opacity from brush color

	CLASS_INTERFACE (IDrawable, Unknown)

protected:
	float opacity;
};

//************************************************************************************************
// BorderDrawable
//************************************************************************************************

class BorderDrawable: public Unknown,
					  public AbstractDrawable
{
public:
	BorderDrawable (ColorRef fillColor, ColorRef borderColor, Coord cornerRadius = 0);

	PROPERTY_OBJECT (SolidBrush, fillBrush, FillBrush)
	PROPERTY_OBJECT (Pen, borderPen, BorderPen)
	PROPERTY_VARIABLE (Coord, cornerRadius, CornerRadius)

	Coord getSafetyMargin () const;

	// IDrawable
	void CCL_API draw (const IDrawable::DrawArgs& args) override;

	CLASS_INTERFACE (IDrawable, Unknown)
};

//************************************************************************************************
// ImageDrawable
/** 
	\ingroup gui */
//************************************************************************************************

class ImageDrawable: public Unknown,
					 public IImageDrawable
{
public:
	ImageDrawable (IImage* image, float alpha = 1.f);

	// IImageDrawable
	void CCL_API draw (const DrawArgs& args) override;
	float CCL_API getOpacity () const override;
	void CCL_API takeOpacity () override;	///< take opacity from image mode
	IImage* CCL_API getImage () const override;

	CLASS_INTERFACE2 (IDrawable, IImageDrawable, Unknown)

protected:
	AutoPtr<IImage> image;
	ImageMode imageMode;
	float opacity;
};

} // namespace CCL

#endif // _ccl_idrawable_h
