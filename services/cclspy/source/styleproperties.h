//************************************************************************************************
//
// CCL Spy
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
// Filename    : styleproperties.h
// Description :
//
//************************************************************************************************

#ifndef _styleproperties_h
#define _styleproperties_h

#include "objectinfo.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/controlproperties.h"

using namespace CCL;

namespace Spy {

//************************************************************************************************
// ImagePropertyHandler
//************************************************************************************************

class ImagePropertyHandler: public PropertyHandler
{
public:
	// PropertyHandler
	bool draw  (VariantRef value, const DrawInfo& info) override
	{
		UnknownPtr<IImage> image (value);
		if(image)
		{
			Rect src (0, 0, image->getWidth (), image->getHeight ());

			String string;
			string << src.right << " x " << src.bottom;
			if(image->getFrameCount () > 1)
				string << ", " << image->getFrameCount () << " frames";

			Rect r (info.rect);
			info.graphics.drawString (r, string, info.style.font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);

			Rect dst (src);
			dst.fitProportionally (r);
			dst.offset (info.rect.right - dst.right, 0);
			info.graphics.drawImage (image, src, dst);
		}
		return true;
	}

	#if 0
	int getEditCapability (VariantRef value) { return value.asUnknown () ? kCustomLink : kNoEdit; }

	bool edit (VariantRef value, EditContext& context)
	{
		UnknownPtr<IImage> image (value);
		if(image)
		{
			Rect rect (0, 0, image->getWidth (), image->getHeight ());
			Rect maxRect (0, 0, 1024, 1024);
			if(rect.getWidth () > maxRect.getWidth () || rect.getHeight () > maxRect.getHeight ())
				rect.fitProportionally (maxRect);

			ViewBox view (ClassID::ImageView, rect, StyleFlags (0, Styles::kImageViewWindowMovable));
			view.setName (CCLSTR ("Image"));
			UnknownPtr<IObject> (view)->setProperty (kImageViewBackground, Variant (image, true));

			DialogBox ()->runDialog (view, Styles::kWindowBehaviorRestoreCenter | Styles::kWindowBehaviorCenter, Styles::kCloseButton);
		}
		return true;
	}
	#endif
};

//************************************************************************************************
// ColorPropertyHandler
//************************************************************************************************

class ColorPropertyHandler: public PropertyHandler
{
public:
	static Color toColor (VariantRef value)
	{
		// support multiple color representations in variant
		Color c;
		if(IUIValue* uiValue = IUIValue::toValue (value))
			uiValue->toColor (c);
		else if(value.isString ())
			Colors::fromString (c, value);
		else if(value.isInt ())
			c = Color::fromInt (value.asUInt ());
		return c;
	}

	// PropertyHandler
	bool draw  (VariantRef value, const DrawInfo& info) override
	{
		Rect r (info.rect);
		r.setWidth (r.getHeight ());
		r.expand (-1);

		Color c = toColor (value);		
		info.graphics.fillRect (r, SolidBrush (c));

		String string;
		toString (string, value);
		if(c.getAlphaF () == 1 && string.length () == 9 && string.endsWith (CCLSTR ("FF")))
			string.truncate (7); // remove unneccessary trailing alpha FF

		Coord left = r.right + 3;
		r = info.rect;
		r.left = left;
		info.graphics.drawString (r, string, info.style.font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
		return true;
	}

	void toString (String& string, VariantRef value) override
	{
		Color c = toColor (value);		
		Colors::toString (c, string);
	}
};

//************************************************************************************************
// MutableColorPropertyHandler
//************************************************************************************************

class MutableColorPropertyHandler: public ColorPropertyHandler
{
public:
	int getEditCapability (VariantRef value) override { return kColorEdit; }
};

//************************************************************************************************
// FontPropertyHandler
//************************************************************************************************

class FontPropertyHandler: public PropertyHandler
{
public:
	// PropertyHandler
	void toString (String& string, VariantRef value) override
	{
		UnknownPtr<IFont> iFont (value);
		if(iFont)
		{
			Font font;
			iFont->copyTo (font);

			string << "face=\"" << font.getFace () << "\" size=\" " << font.getSize () << "\" spacing=\"" << font.getSpacing () << "\"";

			if(font.getStyle () != 0)
			{
				string << " style=\"";
				#define CHECK_FONTSTYLE(method,name) if(font.method ()) string << name << " ";
				CHECK_FONTSTYLE (isBold, "bold")
				CHECK_FONTSTYLE (isItalic, "italic")
				CHECK_FONTSTYLE (isUnderline, "underline")
				CHECK_FONTSTYLE (isStrikeout, "stikeout")
				#undef CHECK_FONTSTYLE
				string << "\"";
			}

			if(font.getMode () != Font::kDefault)
			{
				string << " smoothing=\"";
				switch(font.getMode ())
				{
				case Font::kNone : string << "none"; break;
				case Font::kAntiAlias : string << "antialias"; break;
				}
				string << "\"";
			}
		}
	}
};

} // namespace Spy

#endif // _styleproperties_h
