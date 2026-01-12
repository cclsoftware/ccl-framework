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
// Filename    : ccl/gui/theme/palette.h
// Description : Palette
//
//************************************************************************************************

#ifndef _ccl_palette_h
#define _ccl_palette_h

#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cstring.h"

#include "ccl/base/object.h"

namespace CCL {

//************************************************************************************************
// PaletteBase
//************************************************************************************************

class PaletteBase: public Object,
				   public IPalette
{
public:
	DECLARE_CLASS_ABSTRACT (PaletteBase, Object)
	DECLARE_METHOD_NAMES (PaletteBase)
	DECLARE_PROPERTY_NAMES (PaletteBase)

	Variant getNext (VariantRef element) const;

	CLASS_INTERFACE (IPalette, Object)

protected:
	int firstAutoIndex;
	int lastAutoIndex;

	class IndexRange;

	PaletteBase ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ColorPalette
//************************************************************************************************

class ColorPalette: public PaletteBase,
					public IColorPalette
{
public:
	DECLARE_CLASS (ColorPalette, PaletteBase)
	DECLARE_METHOD_NAMES (ColorPalette)

	ColorPalette ();
	~ColorPalette ();
	
	static void linkColorPalette ();

	// IPalette
	int CCL_API getCount () const override;
	Variant CCL_API getAt (int index) const override;
	int CCL_API getIndex (VariantRef element) const override;
	tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const override;
	IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const override;
	tbool CCL_API getTitle (String& title, int index) const override;
	tbool CCL_API getID (MutableCString& id, int index) const override;
	tbool CCL_API getCategory (String& category, int index) const override;
	tbool CCL_API isEnabled (int index) const override;

	// IColorPalette
	tbool CCL_API fromStyle (const IVisualStyle& style) override;
	tbool CCL_API setColors (const Color colors[], int count, int startIndex = 0) override;
	const Color& CCL_API getColorAt (int index) const override;
	const Color& CCL_API getNextColor (const Color& color, tbool wrap = true, tbool autoRange = false) const override;
	const Color& CCL_API getPrevColor (const Color& color, tbool wrap = true, tbool autoRange = false) const override;
	tbool CCL_API removeColors (int startIndex = 0, int count = 1) override;

	CLASS_INTERFACE (IColorPalette, PaletteBase)

protected:
	Vector<Color> colors;
	int columns;
	int cellWidth;
	int cellHeight;
	int cellMargin;
	float cellRadius;
	
	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ImagePalette
//************************************************************************************************

class ImagePalette: public PaletteBase,
					public IImagePalette
{
public:
	DECLARE_CLASS (ImagePalette, PaletteBase)

	ImagePalette ();

	// IPalette
	int CCL_API getCount () const override;
	Variant CCL_API getAt (int index) const override;
	int CCL_API getIndex (VariantRef element) const override;
	tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const override;
	IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const override;
	tbool CCL_API getTitle (String& title, int index) const override;
	tbool CCL_API getID (MutableCString& id, int index) const override;
	tbool CCL_API getCategory (String& category, int index) const override;
	tbool CCL_API isEnabled (int index) const override;

	// IImagePalette
	tbool CCL_API fromStyle (const IVisualStyle& style) override;
	tbool CCL_API addImages (StringID skinID, StringRef folderName, int options = kAddRecursive, 
							 ITranslationTable* stringTable = nullptr, StringID scope = nullptr) override;
	tbool CCL_API addImages (UrlRef path, int options = kAddRecursive, 
							 ITranslationTable* stringTable = nullptr, StringID scope = nullptr) override;

	CLASS_INTERFACE (IImagePalette, PaletteBase)

protected:
	struct Item
	{
		SharedPtr<IImage> image;
		MutableCString id;
		String title;
		String category;

		Item (IImage* image = nullptr, StringID id = nullptr, StringRef title = nullptr, StringRef category = nullptr)
		: image (image),
		  id (id),
		  title (title),
		  category (category)
		{}
	};

	Vector<Item> images;
	int columns;
	int cellWidth;
	int cellHeight;

	void addImages (UrlRef folder, int options, const IUrl* baseFolder, Vector<String>& uniqueNames,
					ITranslationTable* stringTable, StringID scope);
};

} // namespace CCL

#endif // _ccl_palette_h
