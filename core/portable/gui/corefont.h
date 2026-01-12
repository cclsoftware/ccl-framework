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
// Filename    : core/portable/gui/corefont.h
// Description : Bitmap Font
//
//************************************************************************************************

#ifndef _corefont_h
#define _corefont_h

#include "coregraphics.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// BitmapFont
/** \ingroup core_gui */
//************************************************************************************************

class BitmapFont
{
public:
	BitmapFont (CStringPtr name, Bitmap* bitmap, bool ownBitmap = true);
	~BitmapFont ();

	static const Color kInvertColor; ///< special color to invert monochrome fonts (inconsistent, but so be it)

	PROPERTY_CSTRING_BUFFER (32, name, Name)
	PROPERTY_VARIABLE (int, fontNumber, FontNumber) ///< optional, for external use

	bool loadDescriptor (IO::Stream& stream); ///< load descriptor from .fnt file
	bool loadInplace (const uint8* fontFileData, uint32 fontFileLength); ///< load inplace, doesn't copy or allocate memory

	int getLineHeight () const { return lineHeight; }
	int getStringWidth (CStringPtr string, int length) const;

	void render (Graphics& graphics, Point pos, CStringPtr string, int length, ColorRef color) const;

protected:
	struct CharDescriptor;
	struct KerningPair;

	struct CharMap
	{
		virtual ~CharMap () {}
		virtual bool lookup (CharDescriptor&, uchar32 c) const = 0;
		virtual int getKerningAmount (uchar32 first, uchar32 second) const = 0;
	};

	struct InplaceMap: CharMap
	{
		const void* charData;
		int charCount;
		const void* pairData;
		int pairCount;

		InplaceMap ()
		: charData (nullptr),
		  charCount (0),
		  pairData (nullptr),
		  pairCount (0)
		{}

		// CharMap
		bool lookup (CharDescriptor&, uchar32 c) const override;
		virtual int getKerningAmount (uchar32 first, uchar32 second) const override;
	};

	struct LoadedMap;

	Bitmap* bitmap;
	bool ownBitmap;
	InplaceMap inplaceMap;
	LoadedMap* loadedMap;
	int lineHeight;

	struct ColorEntry
	{
		Bitmap* bitmap;
		Color color;

		ColorEntry (Bitmap* bitmap = nullptr, ColorRef color = Colors::kBlack)
		: bitmap (bitmap),
		  color (color)
		{}
	};

	static const int kMaxColorBitmaps = 5;
	FixedSizeVector<ColorEntry, kMaxColorBitmaps> colorBitmaps;
	bool colorCachingEnabled;

	struct RenderScope
	{
		Graphics& graphics;
		int oldRenderMode;
		bool renderModeChanged;
		Bitmap* bitmap;
		BitmapMode bitmapMode;

		RenderScope (const BitmapFont& font, Graphics& graphics, ColorRef color);
		~RenderScope ();
	};

	Bitmap* prepareColorBitmap (ColorRef color);
	const CharMap& getMap() const;
};

//************************************************************************************************
// BitmapInplaceFont
/** \ingroup core_gui */
//************************************************************************************************

class BitmapInplaceFont: public BitmapFont
{
public:
	BitmapInplaceFont (CStringPtr name,
					   const uint8* bitmapFileData, uint32 bitmapFileLength,
					   const uint8* fontFileData, uint32 fontFileLength);

protected:
	Bitmap fontBitmap;
};

//************************************************************************************************
// FontProvider
/** \ingroup core_gui */
//************************************************************************************************

struct FontProvider
{
	virtual const BitmapFont* getFont (CStringPtr name) const = 0;
};

//************************************************************************************************
// FontManagerObserver
/** \ingroup core_gui */
//************************************************************************************************

struct FontManagerObserver
{
	virtual void onFontAdded (CStringPtr name) = 0;
};

//************************************************************************************************
// FontManager
/** \ingroup core_gui */
//************************************************************************************************

class FontManager: public StaticSingleton<FontManager>,
				   public FontProvider
{
public:
	FontManager ();
	~FontManager ();
	
	PROPERTY_POINTER (BitmapFont, defaultColorFont, DefaultColorFont)
	PROPERTY_POINTER (BitmapFont, defaultMonoFont, DefaultMonoFont)
	PROPERTY_POINTER (FontProvider, externalFontProvider, ExternalFontProvider)

	/** Load fonts from package defined in 'fonts.json/.ubj' file. */
	int loadFonts (FilePackage& package, BitmapFileFormat::Format bitmapFormat = BitmapFileFormat::kPNG);
		
	void addFont (BitmapFont* font);

	DEFINE_OBSERVER (FontManagerObserver)

	// FontProvider
	const BitmapFont* getFont (CStringPtr name) const override;

protected:
	typedef HashMap<uint32, BitmapFont*> FontMap;
	FontMap* fontMap;

	FontMap& getFontMap ();
};

} // namespace Portable
} // namespace Core

#endif // _corefont_h
