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
// Filename    : ccl/public/gui/framework/ipalette.h
// Description : Palette Interface
//
//************************************************************************************************

#ifndef _ccl_ipalette_h
#define _ccl_ipalette_h

#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface ITranslationTable;
interface IVisualStyle;
interface IPalette;
interface IParameter;
interface IParamPreviewHandler;
interface IItemView;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ColorPalette, 0x26368A5A, 0x631F, 0x49E9, 0xA0, 0x77, 0x30, 0x4D, 0x7B, 0x3E, 0x2C, 0x85);
	DEFINE_CID (ImagePalette, 0x193761d7, 0xdd8c, 0x4b28, 0xb2, 0x91, 0xca, 0x52, 0x85, 0x7f, 0x27, 0x4);
	DEFINE_CID (PaletteModel, 0xF6951DE4, 0x4EAB, 0x4854, 0xB5, 0x47, 0x5B, 0x34, 0x1D, 0x5A, 0x82, 0x9B);
	DEFINE_CID (ColorPaletteModel, 0x60EDF04B, 0x5A5B, 0x433D, 0x90, 0xCF, 0x3B, 0x64, 0x8D, 0x07, 0x4E, 0x46);
};

//************************************************************************************************
// IPaletteProvider
/**
	\ingroup gui */
//************************************************************************************************

interface IPaletteProvider: IUnknown
{
	/** Get associated palette. */
	virtual IPalette* CCL_API getPalette () const = 0;

	/** Set associated palette. */
	virtual void CCL_API setPalette (IPalette* palette) = 0;

	DECLARE_IID (IPaletteProvider)
};

DEFINE_IID (IPaletteProvider, 0x9226d84d, 0xae00, 0x4baa, 0xb8, 0x38, 0xff, 0xf0, 0x17, 0x2, 0xb3, 0x34)

//************************************************************************************************
// IPalette
/**
	\ingroup gui */
//************************************************************************************************

interface IPalette: IUnknown
{
	/** Get number of elements. */
	virtual int CCL_API getCount () const = 0;

	/** Get element at index. */
	virtual Variant CCL_API getAt (int index) const = 0;

	/** Get index of element. */
	virtual int CCL_API getIndex (VariantRef element) const = 0;

	/** Get display dimensions. */
	virtual tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const = 0;

	/** Create icon for element. */
	virtual IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const = 0;

	/** Get title of element (optional). */
	virtual tbool CCL_API getTitle (String& title, int index) const = 0;

	/** Get identifier of element (optional). */
	virtual tbool CCL_API getID (MutableCString& id, int index) const = 0;
	
	/** Get category of element (optional). */
	virtual tbool CCL_API getCategory (String& category, int index) const = 0;

	/** Check if element is enabled. */
	virtual tbool CCL_API isEnabled (int index) const = 0;

	DECLARE_IID (IPalette)
};

DEFINE_IID (IPalette, 0xbc2a3778, 0x2bb7, 0x4144, 0x93, 0x2d, 0xdd, 0x5e, 0x85, 0xf4, 0x14, 0x15)

//************************************************************************************************
// IPaletteItemModel
/**
	\ingroup gui */
//************************************************************************************************

interface IPaletteItemModel: IUnknown
{
	/** Initialize with palette, parameter, previewHandler. */
	virtual void CCL_API initModel (IPalette* palette, IParameter* param, IParamPreviewHandler* previewHandler = nullptr) = 0;

	/** Get index of focus element. */
	virtual int CCL_API getFocusIndex () const = 0;

	/** Set focus element by index. */
	virtual void CCL_API setFocusIndex (int index) = 0;

	/** return first itemview of model */
	virtual IItemView* CCL_API getItemView () const = 0;
	
	/** to be called when previewhandler was active (focus was set) */
	virtual	void CCL_API finishPreview () = 0;

	DECLARE_IID (IPaletteItemModel)
};

DEFINE_IID (IPaletteItemModel, 0xCC06D74A, 0xC310, 0x495B, 0xA0, 0x66, 0xB1, 0xEE, 0xDB, 0x37, 0xC2, 0x2E)

//************************************************************************************************
// IColorPaletteModel
/**
	\ingroup gui */
//************************************************************************************************

interface IColorPaletteModel: IUnknown
{
	/** Add/Insert color in palette at specified index - append color if no index (-1) is passed */
	virtual void CCL_API addColor (ColorRef color, int index = -1) = 0;

	/** Remove color palette item at specified index - remove focussed color palette item if no index (-1) is passed  */
	virtual void CCL_API removeColor (int index = -1) = 0;
	
	/** Get color of focussed color palette item */
	virtual Color CCL_API getFocusColor () const = 0;
	
	/** Set color of focussed color palette item */
	virtual void CCL_API setFocusColor (ColorRef color) = 0;

	DECLARE_STRINGID_MEMBER (kFocusColorChanged)

	DECLARE_IID (IColorPaletteModel)
};

DEFINE_IID (IColorPaletteModel, 0x7B03EFB8, 0x6A40, 0x41D2, 0x94, 0xC8, 0xCE, 0x58, 0x72, 0xF2, 0x3E, 0xA9)

DEFINE_STRINGID_MEMBER (IColorPaletteModel, kFocusColorChanged, "focusColorChanged")

//************************************************************************************************
// AbstractPalette
/**
	\ingroup gui */
//************************************************************************************************

class AbstractPalette: public IPalette
{
public:
	int CCL_API getCount () const override
	{ return 0; }

	Variant CCL_API getAt (int index) const override
	{ return Variant (index); }

	int CCL_API getIndex (VariantRef element) const override
	{ return element.asInt (); }

	tbool CCL_API getDimensions (int& columns, int& cellWidth, int& cellHeight) const override
	{ return false; }

	IImage* CCL_API createIcon (int index, int width, int height, const IVisualStyle& style) const override
	{ return nullptr; }

	tbool CCL_API getTitle (String& title, int index) const override
	{ return false; }

	tbool CCL_API getID (MutableCString& id, int index) const override
	{ return false; }
	
	tbool CCL_API getCategory (String& category, int index) const override
	{ return false; }

	tbool CCL_API isEnabled (int index) const override
	{ return true; }
};

//************************************************************************************************
// IColorPalette
/**
	\ingroup gui */
//************************************************************************************************

interface IColorPalette: IPalette
{
	/** Get colors from a bitmap named "palette", using the metrics "rows", "columns", "margin", "spacing". */
	virtual tbool CCL_API fromStyle (const IVisualStyle& style) = 0;

	/** Add or replace colors in palette. Colors in the palette will be replaced from startIndex on; use startIndex = -1 to append. */
	virtual tbool CCL_API setColors (const Color colors[], int count, int startIndex = 0) = 0;

	/** Get palette color by index. */
	virtual const Color& CCL_API getColorAt (int index) const = 0;

	/* Get palette color after / before given color (or first color if no match). */
	virtual const Color& CCL_API getNextColor (const Color& color, tbool wrap = true, tbool autoRange = false) const = 0;
	virtual const Color& CCL_API getPrevColor (const Color& color, tbool wrap = true, tbool autoRange = false) const = 0;

	/** Remove colors in palette - 'count' colors in the palette will be removed from startIndex on; use count = -1 to remove all subsequent colors. */
	virtual tbool CCL_API removeColors (int startIndex = 0, int count = 1) = 0;


//////////////////////////////////////////////////////////////////////////////////////////////////

	/** helper method to remove all colors */
	bool removeAll ()
	{
		return removeColors (0, -1);
	}
	
	/** helper method to append single color */
	bool appendColor (ColorRef color)
	{
		return setColors (&color, 1, -1);
	}
	
	DECLARE_IID (IColorPalette)
};

DEFINE_IID (IColorPalette, 0xF1B7737D, 0x26CA, 0x45CE, 0x93, 0x38, 0x1C, 0x69, 0x3C, 0x17, 0xD6, 0x50)

//************************************************************************************************
// IImagePalette
/**
	\ingroup gui */
//************************************************************************************************

interface IImagePalette: IPalette
{
	/** Get dimensions and images from visual style. */
	virtual tbool CCL_API fromStyle (const IVisualStyle& style) = 0;

	enum Options
	{
		kAddRecursive = 1<<0,
		kAddUnique = 1<<1,
		kAddAsTemplate = 1<<2,
		kCollectStrings = 1<<3
	};
	
	/** Add images from skin folder. */
	virtual tbool CCL_API addImages (StringID skinID, StringRef folderName, int options = kAddRecursive, 
									 ITranslationTable* stringTable = nullptr, StringID scope = nullptr) = 0;

	/** Add images from file system location. */
	virtual tbool CCL_API addImages (UrlRef path, int options = kAddRecursive,
									 ITranslationTable* stringTable = nullptr, StringID scope = nullptr) = 0;

	DECLARE_IID (IImagePalette)
};

DEFINE_IID (IImagePalette, 0xe9251115, 0x33a3, 0x408e, 0x88, 0x91, 0xbc, 0x18, 0xac, 0xc, 0x7e, 0x81)

//************************************************************************************************
// ImagePaletteAccessor
/**
	\ingroup gui */
//************************************************************************************************

class ImagePaletteAccessor
{
public:
	ImagePaletteAccessor (IPalette& palette)
	: palette (palette) 
	{}

	IImage* getImageWithId (StringID imageId) const
	{
		for(int i = 0; i < palette.getCount (); i++)
		{
			MutableCString itemId;
			palette.getID (itemId, i);
			if(itemId == imageId)
				if(UnknownPtr<IImage> image = palette.getAt (i).asUnknown ())
					return image; 
		}
		return nullptr;
	}

	bool getIdFromImage (MutableCString& imageId, IImage* image) const
	{
		for(int i = 0, count = palette.getCount (); i < count; i++)
			if(isEqualUnknown (image, palette.getAt (i).asUnknown ()))
			{
				palette.getID (imageId, i);
				return true;
			}
		return false;
	}

protected:
	IPalette& palette;
};

} // namespace CCL

#endif // _ccl_ipalette_h
