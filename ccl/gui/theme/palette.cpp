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
// Filename    : ccl/gui/theme/palette.cpp
// Description : Palette
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/gui/theme/palette.h"
#include "ccl/gui/theme/visualstyle.h"

#include "ccl/gui/skin/skinregistry.h"
#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skinmodel.h"

#include "ccl/gui/graphics/shapes/shapes.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/filefilter.h"
#include "ccl/base/message.h"

#include "ccl/public/text/itranslationtable.h"

using namespace CCL;

//************************************************************************************************
// PaletteBase::IndexRange
//************************************************************************************************

class PaletteBase::IndexRange
{
public:
	IndexRange (const PaletteBase& palette, tbool autoRange);

	int nextIndex (int index, tbool wrap);
	int prevIndex (int index, tbool wrap);

private:
	int minIndex;
	int maxIndex;
};

//************************************************************************************************
// PaletteBase::IndexRange
//************************************************************************************************

PaletteBase::IndexRange::IndexRange (const PaletteBase& palette, tbool autoRange)
{
	maxIndex = palette.getCount () - 1;
	minIndex = 0;

	if(autoRange)
	{
		minIndex = palette.firstAutoIndex;
		if(palette.lastAutoIndex >= 0)
			maxIndex = palette.lastAutoIndex;
		else // -1 -> always to end (default); -2 -> without last index...
			maxIndex = palette.getCount () + palette.lastAutoIndex;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PaletteBase::IndexRange::nextIndex (int index, tbool wrap)
{
	if(index < minIndex)
		return minIndex;

	if(index >= maxIndex)
		return wrap ? minIndex : maxIndex;

	int count = maxIndex - minIndex + 1;
	return minIndex + (index - minIndex + 1) % count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PaletteBase::IndexRange::prevIndex (int index, tbool wrap)
{
	if(index > maxIndex)
		return maxIndex;

	if(index <= minIndex)
		return wrap ? maxIndex : minIndex;

	int count = maxIndex - minIndex + 1;
	return minIndex + (index - minIndex - 1) % count;
}

//************************************************************************************************
// PaletteBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PaletteBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PaletteBase::PaletteBase ()
: firstAutoIndex (0),
  lastAutoIndex (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant PaletteBase::getNext (VariantRef element) const
{
	int index = getIndex (element);
	return getAt ((index + 1) % getCount ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (PaletteBase)
	DEFINE_PROPERTY_NAME ("count")
END_PROPERTY_NAMES (PaletteBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PaletteBase::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "count")
	{
		var = getCount ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PaletteBase)
	DEFINE_METHOD_NAME ("getAt")
	DEFINE_METHOD_NAME ("getNext")
END_METHOD_NAMES (PaletteBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PaletteBase::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getAt")
	{
		returnValue = getAt (msg[0].asInt ());
		return true;
	}
	else if(msg == "getNext")
	{
		returnValue = getNext (msg[0]);
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ColorPalette
//************************************************************************************************

void ColorPalette::linkColorPalette () {} // force linkage of this file

DEFINE_CLASS (ColorPalette, PaletteBase)
DEFINE_CLASS_UID (ColorPalette, 0x26368A5A, 0x631F, 0x49E9, 0xA0, 0x77, 0x30, 0x4D, 0x7B, 0x3E, 0x2C, 0x85)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPalette::ColorPalette ()
: columns (8),
  cellWidth (30),
  cellHeight (20),
  cellRadius (0),
  cellMargin (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPalette::~ColorPalette ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ColorPalette::getCount () const
{
	return colors.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ColorPalette::getAt (int index) const
{
	uint32 colorCode = getColorAt (index);
	return Variant ((int)colorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Color& CCL_API ColorPalette::getColorAt (int index) const
{
	return colors.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Color& CCL_API ColorPalette::getNextColor (const Color& color, tbool wrap, tbool autoRange) const
{
	if(!colors.isEmpty ())
	{
		int index = getIndex ((int64)(uint32)color);

		IndexRange range (*this, autoRange);
		return getColorAt (range.nextIndex (index, wrap));
	}
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Color& CCL_API ColorPalette::getPrevColor (const Color& color, tbool wrap, tbool autoRange) const
{
	if(!colors.isEmpty ())
	{
		int index = getIndex ((int64)(uint32)color);

		IndexRange range (*this, autoRange);
		return getColorAt (range.prevIndex (index, wrap));
	}
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ColorPalette::getIndex (VariantRef element) const
{
	uint32 colorCode = element.asInt () | 0xFF000000; // interpret as opaque
	Color color = Color::fromInt (colorCode);
	for(int i = 0; i < getCount (); i++)
		if(color == (getColorAt (i) | 0xFF000000)) // interpret as opaque
			return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::getDimensions (int& columns, int& cellWidth, int& cellHeight) const
{
	columns = this->columns;
	cellWidth = this->cellWidth;
	cellHeight = this->cellHeight;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ColorPalette::createIcon (int index, int width, int height, const IVisualStyle& style) const
{
	Color color (getColorAt (index));
	AutoPtr<Shape> shape;

	if((uint32)color == 0) // transparent color: remove color
	{
		if(IImage* emptyCellImage = style.getImage ("emptyCellImage"))
		{
			emptyCellImage->retain ();
			return emptyCellImage;
		}
		
		TriangleShape* triangle = NEW TriangleShape;
		triangle->setP1 (Point (width-1, 0));
		triangle->setP2 (Point (0, height-1));
		triangle->setP3 (Point (width-1, height-1));
		triangle->setStyle (Shape::kFill);
		triangle->setFillBrush (SolidBrush (Colors::kWhite));
		shape = triangle;
	}
	else
	{
		RectShape* rect = NEW RectShape;
		rect->setRect (Rect (0, 0, width-cellMargin, height-cellMargin));
		rect->setStyle (Shape::kStrokeAndFill);
		rect->setRadiusX (cellRadius);
		rect->setRadiusY (cellRadius);
		rect->setFillBrush (SolidBrush (color));
		rect->setStrokePen (Pen (color));
		shape = rect;
	}
	
	return NEW ShapeImage (shape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::getTitle (String& title, int index) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::getID (MutableCString& id, int index) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::getCategory (String& category, int index) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::isEnabled (int index) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::fromStyle (const IVisualStyle& style)
{
	bool result = false;
	int maxColors = style.getMetric<int> ("maxColors", 256);
	IImage* image = style.getImage ("palette");
	UnknownPtr<IBitmap> bitmap (image);
	if(bitmap)
	{
		BitmapLockData data;
		if(bitmap->lockBits (data, IBitmap::kRGBAlpha, IBitmap::kLockRead) == kResultOk)
		{
			result = true;

			Coord margin  = style.getMetric<Coord> ("margin", 0);
			Coord spacing = style.getMetric<Coord> ("spacing", 0);
			columns       = style.getMetric<int> ("columns", 8);
			int rows      = style.getMetric<int> ("rows", 1);
			bool flip     = (style.getMetric<int> ("flip", 0) != 0) ? true : false;

			Coord cellW = (image->getWidth () - 2 * margin + spacing) / columns;
			Coord cellH = (image->getHeight () - 2 * margin + spacing) / rows;

			colors.resize (rows * columns);
			float bitmapScaleFactor = bitmap->getContentScaleFactor ();

			int colorIndex = 0;
			if(flip)
			{
				Coord x = margin + (cellW - spacing) / 2;
				for(int c = 0; c < columns; c++, x += cellW)
				{
					Coord y = margin + (cellH - spacing) / 2;
					for(int r = 0; r < rows; r++, y += cellH)
					{
						PixelPoint p (Point (x, y), bitmapScaleFactor);
						RGBA pixel = data.rgbaAt (p.x, p.y);
						if(colorIndex < maxColors)
						{
							if(pixel.color == 0) // special code for remove color
								colors.add (Color (0, 0, 0, 0));
							else
								colors.add (Color (pixel.red, pixel.green, pixel.blue, 0xff));
						}
						colorIndex++;
					}
				}
				columns = rows;
			}
			else
			{
				Coord y = margin + (cellH - spacing) / 2;
				for(int r = 0; r < rows; r++, y += cellH)
				{
					Coord x = margin + (cellW - spacing) / 2;
					for(int c = 0; c < columns; c++, x += cellW)
					{
						PixelPoint p (Point (x, y), bitmapScaleFactor);
						RGBA pixel = data.rgbaAt (p.x, p.y);
						if(colorIndex < maxColors)
						{
							if(pixel.color == 0) // special code for remove color
								colors.add (Color (0, 0, 0, 0));
							else
								colors.add (Color (pixel.red, pixel.green, pixel.blue, 0xff));
						}
						colorIndex++;
					}
				}
			}
			bitmap->unlockBits (data);
		}
		#if DEBUG
		else
			CCL_DEBUGGER ("Wrong palette bitmap format!")
		#endif
	}

	firstAutoIndex = style.getMetric<int> ("autoFirst", 0);
	lastAutoIndex = style.getMetric<int> ("autoLast", -1);
	if(lastAutoIndex >= 0 && lastAutoIndex >= getCount ())
		lastAutoIndex = -1;
	cellWidth = style.getMetric<int> ("cellwidth", cellWidth);
	cellHeight = style.getMetric<int> ("cellheight", cellHeight);
	columns = style.getMetric<int> ("presentation.columns", columns);
	cellRadius = style.getMetric<int> ("cellradius", cellRadius);
	cellMargin = style.getMetric<int> ("cellmargin", cellMargin);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::setColors (const Color newColors[], int count, int startIndex)
{
	if(startIndex < 0)
		startIndex = colors.count (); // append
	else
	{
		ASSERT (startIndex <= colors.count ()) // can't leave a "hole" between existing and new colors
		ccl_upper_limit (startIndex, colors.count ());
	}

	for(int i = 0; i < count; i++)
	{
		int index = startIndex + i;
		if(index < colors.count ())
			colors.at (index) = newColors[i]; 
		else
			colors.add (newColors[i]);
	}

	// quick fix: adjust dimensions based on color count
	if(colors.count () > 8 && colors.count () < 32)
	{
		columns = get_max (2, colors.count () / 2);
		if(colors.count () % 2)
			columns++;
	}
	
	deferSignal (NEW Message (kChanged));
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::removeColors (int startIndex, int count)
{
	if(startIndex < 0)
		startIndex = 0;

	if(count < 0)
		count = colors.count () - startIndex;
		
	for(int i = startIndex + count - 1; i >= startIndex; i--)
		colors.removeAt (i);
		
	deferSignal (NEW Message (kChanged));
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ColorPalette)
	DEFINE_METHOD_ARGS ("setColor", "index: int, color: int | string")
	DEFINE_METHOD_ARGS ("removeColors", "startIndex: int = 0, count: int = -1")
END_METHOD_NAMES (ColorPalette)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPalette::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setColor")
	{
		int index = msg[0];
		Variant colorValue = msg[1];

		Color color;
		if(colorValue.isString ())
			 Colors::fromString (color, colorValue.asString ());
		else
			color (colorValue.asUInt ());

		setColors (&color, 1, index);
		return true;
	}
	else if(msg == "removeColors")
	{
		int startIndex = msg.getArgCount () > 0 ? msg[0].asInt () : 0;
		int count = msg.getArgCount () > 1 ? msg[1].asInt () : -1;
		removeColors (startIndex, count);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ImagePalette
//************************************************************************************************

DEFINE_CLASS (ImagePalette, PaletteBase)
DEFINE_CLASS_UID (ImagePalette, 0x193761d7, 0xdd8c, 0x4b28, 0xb2, 0x91, 0xca, 0x52, 0x85, 0x7f, 0x27, 0x4)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImagePalette::ImagePalette ()
: columns (1),
  cellWidth (34),
  cellHeight (34)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ImagePalette::getCount () const
{
	return images.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ImagePalette::getAt (int index) const
{
	return Variant (images.at (index).image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ImagePalette::getIndex (VariantRef element) const
{
	IImage* elementImage = UnknownPtr<IImage> (element.asUnknown ());
	if(elementImage)
		for(int i = 0; i < images.count (); i++)
			if(images.at (i).image == elementImage)
				return i;

	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::getDimensions (int& columns, int& cellWidth, int& cellHeight) const
{
	columns = this->columns;
	cellWidth = this->cellWidth;
	cellHeight = this->cellHeight;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ImagePalette::createIcon (int index, int width, int height, const IVisualStyle& style) const
{
	IImage* image = images.at (index).image;
	if(image)
		image->retain ();
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::getTitle (String& title, int index) const
{
	title = images.at (index).title;
	return !title.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::getID (MutableCString& id, int index) const
{
	id = images.at (index).id;
	return !id.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::getCategory (String& category, int index) const
{
	category = images.at (index).category;
	return !category.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::isEnabled (int index) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::fromStyle (const IVisualStyle& style)
{
	columns = style.getMetric<int> ("columns", columns);
	cellWidth = style.getMetric<int> ("cellwidth", cellWidth);
	cellHeight = style.getMetric<int> ("cellheight", cellHeight);

	int count = style.getMetric<int> ("count", 0);
	for(int i = 0; i < count; i++)
	{
		MutableCString imageName;
		imageName.appendFormat ("image%d", i + 1);

		IImage* image = style.getImage (imageName);
		CString imageID = style.getString (imageName);
		ASSERT (image != nullptr)
		if(image)
			images.add (Item (image, imageID));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::addImages (StringID skinID, StringRef folderName, int options, 
									   ITranslationTable* stringTable, StringID scope)
{
	SkinWizard* skin = SkinRegistry::instance ().getSkin (skinID);
	ASSERT (skin != nullptr)
	if(skin == nullptr)
		return false;

	Url path;
	skin->getRoot ().makeSkinUrl (path, folderName, true);
	return addImages (path, options, stringTable, scope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImagePalette::addImages (UrlRef path, int options,
									   ITranslationTable* stringTable, StringID scope)
{
	Vector<String> uniqueNames;
	addImages (path, options, nullptr, uniqueNames, stringTable, scope);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImagePalette::addImages (UrlRef folder, int options, const IUrl* baseFolder, Vector<String>& uniqueNames,
							  ITranslationTable* stringTable, StringID scope)
{	
	bool recursive = get_flag<int> (options, kAddRecursive);
	bool unique = get_flag<int> (options, kAddUnique);
	bool asTemplate = get_flag<int> (options, kAddAsTemplate);
	bool collectStrings = get_flag<int> (options, kCollectStrings);

	FileFilter filter (folder);

	ForEachFile (File (folder).newIterator (), path)
		if(path->isFolder ())
		{
			if(recursive == true)
				addImages (*path, options, baseFolder ? baseFolder : &folder, uniqueNames,
						   stringTable, scope);
		}
		else
		{
			if(!filter.matches (*path))
				continue;
			
			if(Bitmap::isHighResolutionFile (*path)) // ignore @2x, etc.
				continue;

			if(AutoPtr<Image> image = Image::loadImage (*path))
			{
				image->setIsTemplate (asTemplate);

				// make id, title, and category
				String pathName;
				if(baseFolder)
				{
					path->getPathName (pathName);
					pathName.remove (0, baseFolder->getPath ().length () + 1);
				}

				String fileName;
				path->getName (fileName, false);
				
				if(unique == true)
				{
					if(uniqueNames.contains (fileName))
						continue;					
					uniqueNames.add (fileName);
				}
				
				String id = pathName;
				if(!id.isEmpty ())
					id << Url::strPathChar;
				id << fileName;
				id.replace (CCLSTR (" "), String::kEmpty);
				id.toLowercase ();

				String category, title;
				if(stringTable)
				{
					if(collectStrings)
					{
						// table will take care of duplicates
						if(!pathName.isEmpty ())
							stringTable->addStringWithUnicodeKey (scope, pathName, String::kEmpty);
						stringTable->addStringWithUnicodeKey (scope, fileName, String::kEmpty);
					}
					else
					{
						if(!pathName.isEmpty ())
							stringTable->getStringWithUnicodeKey (category, scope, pathName);
						stringTable->getStringWithUnicodeKey (title, scope, fileName);
					}
				}

				if(category.isEmpty ())
					category = pathName;
				if(title.isEmpty ())
					title = fileName;
				
				Item item (image, MutableCString (id), title, category);
				CCL_PRINTF ("Adding image to palette: id = \"%s\" title = \"%s\"\n", item.id.str (), MutableCString (item.title).str ())
				images.add (item);
			}
		}
	EndFor
	
}
