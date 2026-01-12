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
// Filename    : ccl/gui/graphics/graphicshelper.cpp
// Description : Graphics Helper
//
//************************************************************************************************

#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/gui/graphics/graphicspath.h"
#include "ccl/gui/graphics/colorgradient.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/imagepart.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapebuilder.h"

#include "ccl/base/collections/container.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Graphics Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT Internal::IGraphicsHelper& CCL_API System::CCL_ISOLATED (GetGraphicsHelper) ()
{
	return GraphicsHelper::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Default Colors
//////////////////////////////////////////////////////////////////////////////////////////////////

struct ColorDesc
{
	const char* name;
	const Color color;
};

static const ColorDesc defaultColors[] =
{
    {"aliceblue",            Color (240,248,255) },
    {"antiquewhite",         Color (250,235,215) },
    {"aqua",                 Color (  0,255,255) },
    {"aquamarine",           Color (127,255,212) },
    {"azure",                Color (240,255,255) },
    {"beige",                Color (245,245,220) },
    {"bisque",               Color (255,228,196) },
    {"black",                Color (  0,  0,  0) },
    {"blanchedalmond",       Color (255,235,205) },
    {"blue",                 Color (  0,  0,255) },
    {"blueviolet",           Color (138, 43,226) },
    {"brown",                Color (165, 42, 42) },
    {"burlywood",            Color (222,184,135) },
    {"cadetblue",            Color ( 95,158,160) },
    {"chartreuse",           Color (127,255,  0) },
    {"chocolate",            Color (210,105, 30) },
    {"coral",                Color (255,127, 80) },
    {"cornflowerblue",       Color (100,149,237) },
    {"cornsilk",             Color (255,248,220) },
    {"crimson",              Color (220, 20, 60) },
    {"cyan",                 Color (  0,255,255) },
    {"darkblue",             Color (  0,  0,139) },
    {"darkcyan",             Color (  0,139,139) },
    {"darkgoldenrod",        Color (184,132, 11) },
    {"darkgray",             Color (169,169,168) },
    {"darkgreen",            Color (  0,100,  0) },
    {"darkgrey",             Color (169,169,169) },
    {"darkkhaki",            Color (189,183,107) },
    {"darkmagenta",          Color (139,  0,139) },
    {"darkolivegreen",       Color ( 85,107, 47) },
    {"darkorange",           Color (255,140,  0) },
    {"darkorchid",           Color (153, 50,204) },
    {"darkred",              Color (139,  0,  0) },
    {"darksalmon",           Color (233,150,122) },
    {"darkseagreen",         Color (143,188,143) },
    {"darkslateblue",        Color ( 72, 61,139) },
    {"darkslategray",        Color ( 47, 79, 79) },
    {"darkslategrey",        Color ( 47, 79, 79) },
    {"darkturquoise",        Color (  0,206,209) },
    {"darkviolet",           Color (148,  0,211) },
    {"deeppink",             Color (255, 20,147) },
    {"deepskyblue",          Color (  0,191,255) },
    {"dimgray",              Color (105,105,105) },
    {"dimgrey",              Color (105,105,105) },
    {"dodgerblue",           Color ( 30,144,255) },
    {"firebrick",            Color (178, 34, 34) },
    {"floralwhite",          Color (255,255,240) },
    {"forestgreen",          Color ( 34,139, 34) },
    {"fuchsia",              Color (255,  0,255) },
    {"gainsboro",            Color (220,220,220) },
    {"ghostwhite",           Color (248,248,255) },
    {"gold",                 Color (215,215,  0) },
    {"goldenrod",            Color (218,165, 32) },
    {"gray",                 Color (128,128,128) },
    {"grey",                 Color (128,128,128) },
    {"green",                Color (  0,128,  0) },
    {"greenyellow",          Color (173,255, 47) },
    {"honeydew",             Color (240,255,240) },
    {"hotpink",              Color (255,105,180) },
    {"indianred",            Color (205, 92, 92) },
    {"indigo",               Color ( 75,  0,130) },
    {"ivory",                Color (255,255,240) },
    {"khaki",                Color (240,230,140) },
    {"lavender",             Color (230,230,250) },
    {"lavenderblush",        Color (255,240,245) },
    {"lawngreen",            Color (124,252,  0) },
    {"lemonchiffon",         Color (255,250,205) },
    {"lightblue",            Color (173,216,230) },
    {"lightcoral",           Color (240,128,128) },
    {"lightcyan",            Color (224,255,255) },
    {"lightgoldenrodyellow", Color (250,250,210) },
    {"lightgray",            Color (211,211,211) },
    {"lightgreen",           Color (144,238,144) },
    {"lightgrey",            Color (211,211,211) },
    {"lightpink",            Color (255,182,193) },
    {"lightsalmon",          Color (255,160,122) },
    {"lightseagreen",        Color ( 32,178,170) },
    {"lightskyblue",         Color (135,206,250) },
    {"lightslategray",       Color (119,136,153) },
    {"lightslategrey",       Color (119,136,153) },
    {"lightsteelblue",       Color (176,196,222) },
    {"lightyellow",          Color (255,255,224) },
    {"lime",                 Color (  0,255,  0) },
    {"limegreen",            Color ( 50,205, 50) },
    {"linen",                Color (250,240,230) },
    {"magenta",              Color (255,  0,255) },
    {"maroon",               Color (128,  0,  0) },
    {"mediumaquamarine",     Color (102,205,170) },
    {"mediumblue",           Color (  0,  0,205) },
    {"mediumorchid",         Color (186, 85,211) },
    {"mediumpurple",         Color (147,112,219) },
    {"mediumseagreen",       Color ( 60,179,113) },
    {"mediumslateblue",      Color (123,104,238) },
    {"mediumspringgreen",    Color (  0,250,154) },
    {"mediumturquoise",      Color ( 72,209,204) },
    {"mediumvioletred",      Color (199, 21,133) },
    {"mediumnightblue",      Color ( 25, 25,112) },
    {"mintcream",            Color (245,255,250) },
    {"mintyrose",            Color (255,228,225) },
    {"moccasin",             Color (255,228,181) },
    {"navajowhite",          Color (255,222,173) },
    {"navy",                 Color (  0,  0,128) },
    {"oldlace",              Color (253,245,230) },
    {"olive",                Color (128,128,  0) },
    {"olivedrab",            Color (107,142, 35) },
    {"orange",               Color (255,165,  0) },
    {"orangered",            Color (255, 69,  0) },
    {"orchid",               Color (218,112,214) },
    {"palegoldenrod",        Color (238,232,170) },
    {"palegreen",            Color (152,251,152) },
    {"paleturquoise",        Color (175,238,238) },
    {"palevioletred",        Color (219,112,147) },
    {"papayawhip",           Color (255,239,213) },
    {"peachpuff",            Color (255,218,185) },
    {"peru",                 Color (205,133, 63) },
    {"pink",                 Color (255,192,203) },
    {"plum",                 Color (221,160,203) },
    {"powderblue",           Color (176,224,230) },
    {"purple",               Color (128,  0,128) },
    {"red",                  Color (255,  0,  0) },
    {"rosybrown",            Color (188,143,143) },
    {"royalblue",            Color ( 65,105,225) },
    {"saddlebrown",          Color (139, 69, 19) },
    {"salmon",               Color (250,128,114) },
    {"sandybrown",           Color (244,164, 96) },
    {"seagreen",             Color ( 46,139, 87) },
    {"seashell",             Color (255,245,238) },
    {"sienna",               Color (160, 82, 45) },
    {"silver",               Color (192,192,192) },
    {"skyblue",              Color (135,206,235) },
    {"slateblue",            Color (106, 90,205) },
    {"slategray",            Color (112,128,144) },
    {"slategrey",            Color (112,128,114) },
    {"snow",                 Color (255,255,250) },
    {"springgreen",          Color (  0,255,127) },
    {"steelblue",            Color ( 70,130,180) },
    {"tan",                  Color (210,180,140) },
    {"teal",                 Color (  0,128,128) },
    {"thistle",              Color (216,191,216) },
    {"tomato",               Color (255, 99, 71) },
    {"turquoise",            Color ( 64,224,208) },
    {"violet",               Color (238,130,238) },
    {"wheat",                Color (245,222,179) },
    {"white",                Color (255,255,255) },
    {"whitesmoke",           Color (245,245,245) },
    {"yellow",               Color (255,255,  0) },
    {"yellowgreen",          Color (154,205, 50) }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsHelper::DefaultColorEnum::DefaultColorEnum ()
: EnumTypeInfo ("DefaultColors")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API GraphicsHelper::DefaultColorEnum::getEnumeratorCount () const
{
	return ARRAY_COUNT (defaultColors);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::DefaultColorEnum::getEnumerator (MutableCString& name, Variant& value, int index) const
{
	ASSERT (index >= 0 && index < ARRAY_COUNT (defaultColors))
	name = defaultColors[index].name;
	String string;
	Colors::toString (defaultColors[index].color, string);
	value = string;
	value.share ();
	return true;
}

//************************************************************************************************
// GraphicsHelper
//************************************************************************************************

GraphicsHelper& GraphicsHelper::instance ()
{
	static GraphicsHelper theHelper;
	return theHelper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT (GraphicsHelper, Object)
DEFINE_CLASS_NAMESPACE (GraphicsHelper, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

EnumTypeInfo& GraphicsHelper::getDefaultColors ()
{
	return defaultColorEnum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::Color_fromCString (Color& This, CStringPtr _cString)
{
	CString cString (_cString);
	if(cString.isEmpty ())
		return false;

	if(cString[0] == '#')
	{
		int r = 0, g = 0, b = 0, a = -1;
		if(cString.length () >= 7)
			::sscanf (cString + 1, "%2X%2X%2X%2X", &r, &g, &b, &a);
		else if(cString.length () >= 4)
		{
			::sscanf (cString + 1, "%1X%1X%1X%1X", &r, &g, &b, &a);

			// short form (one digit per channel): must repeat each written digit
			r += r << 4;
			g += g << 4;
			b += b << 4;

			if(a >= 0)
				a += a << 4;
		}
		This.red = (uint8)r; This.green = (uint8)g; This.blue = (uint8)b;
		This.alpha = a >= 0 ? (uint8)a : 0xFF;
		return true;
	}
	else if(cString.startsWith ("rgb"))
	{
		int offset = 4;
		if(cString.at (3) == 'a')
			offset++;
		
		if(cString.contains ("%"))
		{
			float r = 0, g = 0, b = 0, a = -1;
			::sscanf (cString + offset, "%f%%,%f%%,%f%%,%f%%", &r, &g, &b, &a);
			
			This.setRedF (r / 100.f);
			This.setGreenF (g / 100.f);
			This.setBlueF (b / 100.f);
			This.setAlphaF (a >= 0.f ? a / 100.f : 1.f);
		}
		else
		{
			int r = 0, g = 0, b = 0, a = -1;
			::sscanf (cString + offset, "%d,%d,%d,%d", &r, &g, &b, &a);
			This.red = (uint8)r; This.green = (uint8)g; This.blue = (uint8)b;
			This.alpha = a >= 0 ? (uint8)a : 0xFF;
		}
		return true;
	}
	else if(cString.startsWith ("hs")) // hs(v|l)[a](360,100%,100%[,(50%|0.5)]) || hs(v|l)[a](360,100,100[,(50|0.5)])
	{
		int offset = 4;
		if(cString.at (3) == 'a')
			offset++;
		
		float h = 0, s = 0, vl = 0;
		float a = -1;
		bool alphaInPercent = false;
		
		if(cString.contains ("%"))
		{
			if(cString.contains ("."))
			{
				::sscanf (cString + offset, "%f,%f%%,%f%%,%f", &h, &s, &vl, &a);
			}
			else
			{
				::sscanf (cString + offset, "%f,%f%%,%f%%,%f%%", &h, &s, &vl, &a);
				alphaInPercent = true;
			}
		}
		else
		{
			::sscanf (cString + offset, "%f,%f,%f,%f", &h, &s, &vl, &a);

			if(!cString.contains ("."))
				alphaInPercent = true;
		}
		
		if(a == -1)
			a = 1;
		else if(alphaInPercent)
			a /= 100;
		
		// expected format ranges...
		ASSERT ((h >= 0 && h <= 360))
		ASSERT ((s >= 0 && s <= 100))
		ASSERT ((vl >= 0 && vl <= 100))
		ASSERT ((a >= 0 && a <= 1))
		
		if(cString.contains ("l"))
		{
			ColorHSL hsla (h, s/100.f, vl/100.f, a);
			hsla.toColor (This);
		}
		else
		{
			ColorHSV hsv (h, s/100.f, vl/100.f);
			hsv.toColor (This);
			This.setAlphaF (a);
		}
		return true;
	}
	else
	{
		for(int i = 0; i < ARRAY_COUNT (defaultColors); i++)
			if(cString == defaultColors[i].name)
			{
				This = defaultColors[i].color;
				return true;
			}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::Color_toCString (const Color& This, char* cString, int cStringSize, int flags)
{
	int r = This.red, g = This.green, b = This.blue, a = This.alpha;
	if(flags & kColorWithAlpha)
		snprintf (cString, cStringSize, "#%02X%02X%02X%02X", r, g, b, a);
	else
		snprintf (cString, cStringSize, "#%02X%02X%02X", r, g, b);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Font
//////////////////////////////////////////////////////////////////////////////////////////////////

const Font& CCL_API GraphicsHelper::Font_getDefaultFont ()
{
	#if CCL_PLATFORM_WINDOWS
	static const Font defaultFont ("MS Shell Dlg", 12, Font::kNormal);
	#elif CCL_PLATFORM_IOS
	static const Font defaultFont ("Helvetica Neue", 12, Font::kNormal);
	#elif CCL_PLATFORM_ANDROID
	static const Font defaultFont ("Roboto", 12, Font::kNormal);
	#elif CCL_PLATFORM_LINUX
	static const Font defaultFont ("sans", 12, Font::kNormal);
	#else // CCL_PLATFORM_MAC
	static const Font defaultFont ("Helvetica", 12, Font::kNormal);
	#endif
	return defaultFont;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_measureString (Rect& size, StringRef text, const Font& font, int flags)
{
	AutoPtr<ITextLayout> layout = NativeGraphicsEngine::instance ().createTextLayout ();
	layout->construct (text, kMaxCoord, kMaxCoord, font, ITextLayout::kSingleLine, TextFormat (Alignment::kLeftTop));
	layout->getBounds (size, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_measureString (RectF& size, StringRef text, const Font& font, int flags)
{
	AutoPtr<ITextLayout> layout = NativeGraphicsEngine::instance ().createTextLayout ();
	layout->construct (text, (CoordF)kMaxCoord, (CoordF)kMaxCoord, font, ITextLayout::kSingleLine, TextFormat (Alignment::kLeftTop));
	layout->getBounds (size, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_measureStringImage (RectF& size, StringRef text, const Font& font, tbool shiftToBaseline)
{
	AutoPtr<ITextLayout> layout = NativeGraphicsEngine::instance ().createTextLayout ();
	layout->construct (text, (CoordF)kMaxCoord, (CoordF)kMaxCoord, font, ITextLayout::kSingleLine, TextFormat (Alignment::kLeftTop));
	tresult result = layout->getImageBounds (size);
	if(result != kResultOk)
		layout->getBounds (size);
	if(shiftToBaseline)
	{
		PointF offset;
		layout->getBaselineOffset (offset);
		size.offset (-offset.x, -offset.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_measureText (Rect& size, Coord lineWidth, StringRef text, const Font& font, TextFormatRef format)
{
	AutoPtr<ITextLayout> layout = NativeGraphicsEngine::instance ().createTextLayout ();
	layout->construct (text, lineWidth, kMaxCoord, font, ITextLayout::kMultiLine, format);
	layout->getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_measureText (RectF& size, CoordF lineWidth, StringRef text, const Font& font, TextFormatRef format)
{
	AutoPtr<ITextLayout> layout = NativeGraphicsEngine::instance ().createTextLayout ();
	layout->construct (text, lineWidth, (CoordF)kMaxCoord, font, ITextLayout::kMultiLine, format);
	layout->getBounds (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GraphicsHelper::Font_collapseString (String& string, CoordF maxWidth, const Font& font, int trimMode, tbool exact)
{
	static const String doubleSpace = CCLSTR ("  ");
	static const String singleSpace = CCLSTR (" ");
	static const String empty = CCLSTR ("");
	static const String dots = CCLSTR ("..");

	auto getStringWidth = [exact] (StringRef string, const Font& font)
	{
		if(exact)
		{
			RectF rect;
			Font::measureStringImage (rect, string, font, true);
			return rect.getWidth ();
		}

		return Font::getStringWidthF (string, font);
	};

	// Don't waste time on very long strings
	static const int kVeryLongString = 1024;
	if(string.length () > kVeryLongString)
	{
		// Estimate the right size assuming that character widths are equally distributed across the string
		CoordF width = getStringWidth (string, font);
		float ratio = maxWidth / width;
		if(ratio >= 1.f)
			return;

		string.truncate (int (float (string.length ()) * ratio));
	}

	if(trimMode == Font::kTrimModeNumeric)
	{
		// Try to remove redundant double spaces
		while(string.contains (doubleSpace))
		{
			CoordF width = getStringWidth (string, font);
			if(width <= maxWidth)
				return;
			string.replace (doubleSpace, singleSpace);
		}
			
		// Try to remove redundant single spaces
		while(string.contains (singleSpace))
		{
			CoordF width = getStringWidth (string, font);
			if(width <= maxWidth)
				return;
			string.replace (singleSpace, empty);
		}
	}

	int originalLength = string.length ();
	if(originalLength > 7)
	{
		CoordF width = getStringWidth (string, font);
		int length = originalLength;
		if(width > maxWidth)
		{
			String temp;
			switch(trimMode) 
			{
				case Font::kTrimModeKeepEnd:
				case Font::kTrimModeNumeric:
				{
					String tail (string.subString (length - 4));
					length -= 4;
					while(length > 2)
					{
						temp = string.subString (0, length);
						if(maxWidth > 50)
							temp.append (dots);
						temp.append (tail);
						
						width = getStringWidth (temp, font);
						if(width <= maxWidth)
							break;
						length--;
					}
					string = temp;
					break;
				}
				case Font::kTrimModeRight:
				{
					length -= 2;
					while(length > 2)
					{
						temp = string.subString (0, length);
						temp.append (dots);
						width = getStringWidth (temp, font);
						if(width <= maxWidth)
							break;
						length--;
					}
					string = temp;
					break;
				}
				case Font::kTrimModeLeft:
				{
					length -= 2;
					while(length > 2)
					{
						temp = dots;
						temp.append (string.subString (originalLength - length, length));
						width = getStringWidth (temp, font);
						if(width <= maxWidth)
							break;
						length--;
					}
					string = temp;
					break;
				}
				case Font::kTrimModeMiddle:
				{
					length -= 2;
					while(length > 2)
					{
						int halfLength = length / 2;
						temp = string.subString (0, halfLength + (length % 2));
						temp.append (dots);
						temp.append (string.subString (originalLength - halfLength, halfLength));
						width = getStringWidth (temp, font);
						if(width <= maxWidth)
							break;
						length--;
					}
					string = temp;
					break;
				}
				default:
					break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* CCL_API GraphicsHelper::Font_collectFonts (int flags)
{
	return NativeGraphicsEngine::instance ().collectFonts (flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Factory
//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API GraphicsHelper::Factory_getNumImageFormats ()
{
	int count = 0;
	ForEach (Image::getHandlerList (), ImageHandler, handler)
		count += handler->getNumFileTypes ();
	EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API GraphicsHelper::Factory_getImageFormat (int index)
{
	int i = 0;
	ForEach (Image::getHandlerList (), ImageHandler, handler)
		int count = handler->getNumFileTypes ();
		if(index >= i && index < i + count)
			return handler->getFileType (index - i);
		i += count;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_loadImageFile (UrlRef path)
{
	return Image::loadImage (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::Factory_saveImageFile (UrlRef path, IImage* image, const IAttributeList* encoderOptions)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	ASSERT (stream != nullptr)
	const FileType& format = path.getFileType ();
	return stream ? Factory_saveImageStream (*stream, image, format, encoderOptions) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_loadImageStream (IStream& stream, const FileType& format)
{
	return Image::loadImage (stream, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::Factory_saveImageStream (IStream& stream, IImage* _image, const FileType& format,
													   const IAttributeList* encoderOptions)
{
	Image* image = unknown_cast<Image> (_image);
	ASSERT (image != nullptr)
	return image ? Image::saveImage (stream, image, format, encoderOptions) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createBitmap (int width, int height, IBitmap::PixelFormat format, float scaleFactor)
{
	return NEW Bitmap (width, height, format, scaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphics* CCL_API GraphicsHelper::Factory_createBitmapGraphics (IImage* bitmap)
{
	Bitmap* bmp = unknown_cast<Bitmap> (bitmap);
	return bmp ? NEW BitmapGraphicsDevice (bmp) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapFilter* CCL_API GraphicsHelper::Factory_createBitmapFilter (StringID which)
{
	return BitmapFilterFactory::createFilter (which);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPath* CCL_API GraphicsHelper::Factory_createPath (IGraphicsPath::TypeHint type)
{
	return NEW GraphicsPath (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API GraphicsHelper::Factory_createGradient (IGradient::TypeHint type)
{
	ColorGradient* gradient = nullptr;
	switch(type)
	{
	case IGradient::kLinearGradient : gradient = NEW LinearColorGradient; break;
	case IGradient::kRadialGradient : gradient = NEW RadialColorGradient; break;
	}
	return gradient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createShapeImage ()
{
	return NEW ShapeImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphics* CCL_API GraphicsHelper::Factory_createShapeBuilder (IImage* _shapeImage)
{
	ShapeImage* shapeImage = unknown_cast<ShapeImage> (_shapeImage);
	return shapeImage ? NEW ShapeBuilder (shapeImage) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* CCL_API GraphicsHelper::Factory_createTextLayout ()
{
	return NativeGraphicsEngine::instance ().createTextLayout ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API GraphicsHelper::Factory_createGraphicsLayer (UIDRef cid)
{
	return NativeGraphicsEngine::instance ().createGraphicsLayer (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUIValue* CCL_API GraphicsHelper::Factory_createValue ()
{
	return NEW UIValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createFilmstrip (IImage* _sourceImage, StringID _frames)
{
	Image* sourceImage = unknown_cast<Image> (_sourceImage);
	String frames (_frames);
	Filmstrip* filmstrip = NEW Filmstrip (sourceImage);
	filmstrip->parseFrameNames (frames);
	return filmstrip;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createImagePart (IImage* _sourceImage, RectRef partRect)
{
	Image* sourceImage = unknown_cast<Image> (_sourceImage);
	return NEW ImagePart (sourceImage, partRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createMultiImage (IImage* images[], CString frameNames[], int count)
{
	MultiImage* multiImage = NEW MultiImage;
	for(int i = 0; i < count; i++)
		if(Image* image = unknown_cast<Image> (images[i]))
		{		
			CString name = frameNames ? frameNames[i] : nullptr;
			multiImage->addFrame (image, name);
		}
	return multiImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API GraphicsHelper::Factory_createMultiResolutionBitmap (IImage* bitmaps[], float scaleFactors[], int count)
{
	if(count == 2 && scaleFactors[0] == 1.f && scaleFactors[1] == 2.f)
	{
		Bitmap* bitmap1x = unknown_cast<Bitmap> (bitmaps[0]);
		Bitmap* bitmap2x = unknown_cast<Bitmap> (bitmaps[1]);
		
		if(bitmap1x && bitmap2x)
		{
			// adjust scale factor
			NativeBitmap* nativeBitmap = bitmap2x->getNativeBitmap ();
			nativeBitmap->setContentScaleFactor (scaleFactors[1]);
			
			return NEW MultiResolutionBitmap (bitmap1x->getNativeBitmap (), bitmap2x->getNativeBitmap ());
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (GraphicsHelper)
	DEFINE_METHOD_ARGR ("loadImage", "path", "Image")
	DEFINE_METHOD_ARGS ("saveImage", "path, image, encoderOptions=null")
	DEFINE_METHOD_ARGR ("createBitmap", "width, height", "Image")
	DEFINE_METHOD_ARGR ("createFilmstrip", "image, frames", "Image")
	DEFINE_METHOD_ARGR ("createImagePart", "image, left, top, width, height", "Image")
	DEFINE_METHOD_ARGS ("copyBitmap", "dstBitmap, srcBitmap, offsetX, offsetY")
	DEFINE_METHOD_ARGR ("createBitmapFilter", "name", "BitmapFilter")
	DEFINE_METHOD_ARGR ("processBitmap", "bitmap, filter, inplace=false", "Image")
END_METHOD_NAMES (GraphicsHelper)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsHelper::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "loadImage")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path)
		AutoPtr<IImage> image = path ? Factory_loadImageFile (*path) : nullptr;
		returnValue.takeShared (image);
		return true;
	}
	else if(msg == "saveImage")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		UnknownPtr<IImage> image (msg[1].asUnknown ());
		UnknownPtr<IAttributeList> encoderOptions (msg.getArgCount () > 2 ? msg[2].asUnknown () : nullptr);
		ASSERT (path && image)
		returnValue = path && image && Factory_saveImageFile (*path, image, encoderOptions);
		return true;
	}
	else if(msg == "createBitmap")
	{
		int width = msg[0].asInt ();
		int height = msg[1].asInt ();
		AutoPtr<IImage> bitmap = Factory_createBitmap (width, height, IBitmap::kRGBAlpha);
		returnValue.takeShared (bitmap);
		return true;
	}
	else if(msg == "createFilmstrip")
	{
		UnknownPtr<IImage> sourceImage (msg[0].asUnknown ());
		MutableCString frames (msg[1].asString ());
		AutoPtr<IImage> filmstrip = Factory_createFilmstrip (sourceImage, frames);
		returnValue.takeShared (filmstrip);
		return true;
	}
	else if(msg == "createImagePart")
	{
		UnknownPtr<IImage> sourceImage (msg[0].asUnknown ());
		Rect partRect (msg[1].asInt (), msg[2].asInt (), Point (msg[3].asInt (), msg[4].asInt ()));
		AutoPtr<IImage> imagePart = Factory_createImagePart (sourceImage, partRect);
		returnValue.takeShared (imagePart);
		return true;
	}
	else if(msg == "copyBitmap")
	{
		UnknownPtr<IImage> dstBitmap (msg[0].asUnknown ());
		UnknownPtr<IImage> srcBitmap (msg[1].asUnknown ());
		Point offset;
		if(msg.getArgCount () >= 4)		
			offset (msg[2].asInt (), msg[3].asInt ());

		AutoPtr<IGraphics> bitmapDevice = Factory_createBitmapGraphics (dstBitmap);
		ASSERT (bitmapDevice != nullptr)
		if(bitmapDevice)
			bitmapDevice->drawImage (srcBitmap, offset);
		return true;
	}
	else if(msg == "createBitmapFilter")
	{
		MutableCString name (msg[0].asString ());
		returnValue.takeShared (AutoPtr<IBitmapFilter> (Factory_createBitmapFilter (name)));
		return true;
	}
	else if(msg == "processBitmap")
	{
		UnknownPtr<IImage> srcBitmap (msg[0].asUnknown ());
		UnknownPtr<IBitmapFilter> filter (msg[1].asUnknown ());
		bool inplace = msg.getArgCount () >= 3 ? msg[2].asBool () : false;
		if(srcBitmap && filter)
		{
			BitmapProcessor processor;
			int options = inplace ? IBitmapProcessor::kInplace : 0;
			processor.setup (srcBitmap, Colors::kBlack, options);
			processor.process (*filter);
			returnValue.takeShared (processor.getOutput ());
		}
		return true;
	}
	else
		return Object::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// UIValue
//************************************************************************************************

DEFINE_CLASS_HIDDEN (UIValue, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

UIValue::UIValue ()
: type (kNil)
{
	::memset (&data, 0, sizeof(data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::reset ()
{
	type = kNil;
	::memset (&data, 0, sizeof(data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::copyFrom (const IUIValue* value)
{
	UIValue* v = unknown_cast<UIValue> (const_cast<IUIValue*> (value));
	if(v == nullptr)
		return false;

	type = v->type;
	data = v->data;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIValue::Type CCL_API UIValue::getType () const
{
	return type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromPoint (PointRef p)
{
	type = kPoint;
	reinterpret_cast<Point&> (data.point) = p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toPoint (Point& p) const
{
	if(type != kPoint)
		return false;

	p = asPointRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromRect (RectRef r)
{
	type = kRect;
	reinterpret_cast<Rect&> (data.rect) = r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toRect (Rect& r) const
{
	if(type != kRect)
		return false;

	r = asRectRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromTransform (TransformRef t)
{
	type = kTransform;
	reinterpret_cast<Transform&> (data.transform) = t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toTransform (Transform& t) const
{
	if(type != kTransform)
		return false;

	t = asTransformRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromColor (ColorRef c)
{
	type = kColor;
	reinterpret_cast<Color&> (data.color) = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toColor (Color& color) const
{
	if(type != kColor)
		return false;

	color = asColorRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromColorF (ColorFRef c)
{
	type = kColorF;
	reinterpret_cast<ColorF&> (data.colorF) = c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toColorF (ColorF& color) const
{
	if(type != kColorF)
		return false;

	color = asColorFRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromPointF (PointFRef p)
{
	type = kPointF;
	reinterpret_cast<PointF&> (data.pointF) = p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toPointF (PointF& p) const
{
	if(type != kPointF)
		return false;

	p = asPointFRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromRectF (RectFRef r)
{
	type = kRectF;
	reinterpret_cast<RectF&> (data.rectF) = r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toRectF (RectF& r) const
{
	if(type != kRectF)
		return false;

	r = asRectFRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromPointF3D (PointF3DRef p)
{
	type = kPointF3D;
	reinterpret_cast<PointF3D&> (data.pointF3d) = p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toPointF3D (PointF3D& p) const
{
	if(type != kPointF3D)
		return false;

	p = asPointF3DRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromPointF4D (PointF4DRef p)
{
	type = kPointF4D;
	reinterpret_cast<PointF4D&> (data.pointF4d) = p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toPointF4D (PointF4D& p) const
{
	if(type != kPointF4D)
		return false;

	p = asPointF4DRef ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UIValue::fromTransform3D (Transform3DRef t)
{
	type = kTransform3D;
	data.transform3d = t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UIValue::toTransform3D (Transform3D& t) const
{
	if(type != kTransform3D)
		return false;

	t = asTransform3DRef ();
	return true;
}
