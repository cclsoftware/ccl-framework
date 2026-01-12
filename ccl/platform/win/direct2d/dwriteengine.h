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
// Filename    : ccl/platform/win/direct2d/dwriteengine.h
// Description : DirectWrite Engine
//
//************************************************************************************************

#ifndef _ccl_dwriteengine_h
#define _ccl_dwriteengine_h

#include "ccl/base/singleton.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/win/system/cclcom.h"
#include "ccl/public/base/buffer.h"

#include <dwrite_1.h>
#include <math.h>

namespace CCL {
namespace Win32 {

class DWFontManager;
class DWTextFormatCache;

//************************************************************************************************
// DWRenderingParamsDescription
//************************************************************************************************

struct DWRenderingParamsDescription
{
	float gamma;
	float enhancedContrast;
	float clearTypeLevel;
	DWRITE_PIXEL_GEOMETRY pixelGeometry;
	DWRITE_RENDERING_MODE renderingMode;

	DWRenderingParamsDescription ()
	: gamma (0.f),
	  enhancedContrast (0.f),
	  clearTypeLevel (0.f),
	  pixelGeometry (DWRITE_PIXEL_GEOMETRY_FLAT),
	  renderingMode (DWRITE_RENDERING_MODE_DEFAULT)
	{}
};

//************************************************************************************************
// DWriteEngine
//************************************************************************************************

class DWriteEngine: public Object,
					public StaticSingleton<DWriteEngine>
{
public:
	bool startup ();
	void shutdown ();

	IFontTable* collectFonts (int flags);

	bool isAvailable () const { return factory.isValid (); }
	IDWriteFactory* operator -> () { return factory; }

	bool installFontFromMemory (const void* memory, uint32 size, StringRef fileName);
	bool beginFontInstallation (bool state);

	IDWriteTextFormat* createTextFormat (FontRef font);
	IDWriteTextFormat* createCachedTextFormat (FontRef font);
	IDWriteTextFormat* createCachedTextFormatAndFont (ComPtr<IDWriteFont>& dwFont, FontRef font);
	IDWriteFont* createCachedDWFont (FontRef font);

	HFONT createGdiFont (FontRef font);

	//IDWriteTextLayout* createTextLayout (StringRef text, IDWriteTextFormat* textFormat, float maxWidth, float maxHeight);
	IDWriteTextLayout* createTextLayoutWithFontAttributes (StringRef text, IDWriteTextFormat* textFormat, float maxWidth, float maxHeight, FontRef font);

	IDWriteBitmapRenderTarget* createBitmapRenderTarget (HDC hdc = NULL, int width = 1, int height = 1); ///< used with GDI rendering
	IDWriteRenderingParams* createDefaultRenderingParams ();
	IDWriteRenderingParams* getCachedDefaultRenderingParams ();
	IDWriteRenderingParams* createRenderingParams (const DWRenderingParamsDescription& description);
	void getRenderingParamsDescription (DWRenderingParamsDescription& description, IDWriteRenderingParams* params);

protected:
	ComPtr<IDWriteFactory> factory;
	AutoPtr<DWFontManager> fontManager;
	AutoPtr<DWTextFormatCache> cachedTextFormats;
	ComPtr<IDWriteRenderingParams> cachedDefaultRenderingParams;

	bool lookupDWFont (ComPtr<IDWriteFont>& dwFont, IDWriteFontCollection* collection, StringRef familyName, StringRef faceName) const;
};

//************************************************************************************************
// DWTextFormatCache
//************************************************************************************************

class DWTextFormatCache: public CCL::Object
{
public:
	DWTextFormatCache ();

	IDWriteTextFormat* createFormat (FontRef font);
	IDWriteTextFormat* createFormatAndFont (ComPtr<IDWriteFont>& dwFont, FontRef font);
	IDWriteFont* createDWFont (FontRef font);
	void removeAll ();

protected:
	enum Constants { kStylesUsed = Font::kBold|Font::kItalic };

	struct CachedFormat
	{
		Font font;
		ComPtr<IDWriteTextFormat> textFormat;
		ComPtr<IDWriteFont> dwFont;
	};

	static const int kMaxChacheEntries = 128;
	Vector<CachedFormat> entries;

	CachedFormat* lookup (FontRef font) const;
	CachedFormat* createEntry (FontRef font);
	CachedFormat* add (FontRef font, IDWriteTextFormat* textFormat); ///< shared!
	void createDWFont (CachedFormat& entry);

	static int getUsedStyle (FontRef font) {return font.getStyle () & kStylesUsed;}
};

//************************************************************************************************
// DWTextLayout
//************************************************************************************************

class DWTextLayout: public NativeTextLayout
{
public:
	static float getBaseline (IDWriteTextLayout* layout);

	DECLARE_CLASS_ABSTRACT (DWTextLayout, NativeTextLayout)

	static const float kLineSpacingBaselineFactor;

	DWTextLayout ();

	IDWriteTextLayout* getLayout () { return layout; }
	PROPERTY_VARIABLE (Alignment, alignment, Alignment)
	PROPERTY_OBJECT (Font, font, Font)

	// ITextLayout
	tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API resize (Coord width, Coord height) override;
	tresult CCL_API resize (CoordF width, CoordF height) override;
	tresult CCL_API setFontStyle (const Range& range, int style, tbool state) override;
	tresult CCL_API setFontSize (const Range& range, float size) override;
	tresult CCL_API setSpacing (const Range& range, float spacing) override;
	tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) override;
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;
	tresult CCL_API getBounds (Rect& bounds, int flags = 0) const override;
	tresult CCL_API getBounds (RectF& bounds, int flags = 0) const override;
	tresult CCL_API getImageBounds (RectF& bounds) const override;
	tresult CCL_API getBaselineOffset (PointF& offset) const override;
	tresult CCL_API hitTest (int& textIndex, PointF& position) const override;
	tresult CCL_API getCharacterBounds (RectF& offset, int textIndex) const override;
	tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;
	StringRef CCL_API getText () const override;

protected:
	ComPtr<IDWriteTextLayout> layout;
	PointF layoutSize;
	mutable PointF baselineOffset;
	String text;
	bool wordWrap;

	template<class TCoord>
	Core::TPoint<TCoord> getTextPosition (const Core::TRect<TCoord>& textBounds) const;
};

//************************************************************************************************
// DWFontManager
//************************************************************************************************

class DWFontManager: public Object, 
	                 public IDWriteFontCollectionLoader, 
					 public IDWriteFontFileLoader
{
public:
	struct FontCollection;
	struct FontFile;
	struct InstalledFont;
	
	DWFontManager ();

	bool installFontFromMemory (IDWriteFactory* factory, const void* memory, uint32 size, StringRef fileName);
	void beginFontInstallation (bool state);

	const InstalledFont* lookupInstalledFont (StringRef name, int fontStyle) const;
	
	void removeInstalledFonts ();
	const Vector<AutoPtr<FontCollection>>& getInstalledCollections () const {return fontCollections;}

	// IUnknown
	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	
	// IDWriteFontCollectionLoader
	HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey (IDWriteFactory* factory,  void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator) override;
   	
	// IDWriteFontFileLoader
	HRESULT STDMETHODCALLTYPE CreateStreamFromKey (void const* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream) override;

private:
	class MemoryFontFileEnumerator;
	class MemoryFontFileStream;

	PROPERTY_POINTER (IDWriteFactory, factory, Factory)
	Vector<AutoPtr<FontCollection>> fontCollections;
	FontCollection* pendingCollection;
	int nextKey;
	bool inFontInstallationScope;

	int makeKey ();
	void loadPendingCollection ();
	FontFile* getFontFileFromKey (void const* fontKey, UINT32 fontKeySize) const;
	FontCollection* getCollectionFromKey (void const* collectionKey, UINT32 collectionKeySize) const;
};

//************************************************************************************************
// DWFontManager::FontCollection
//************************************************************************************************

struct DWFontManager::FontCollection: public Unknown
{	
	FontCollection (int key): key (key)
	{}
	
	PROPERTY_VARIABLE (int, key, Key)
	Vector<AutoPtr<FontFile>> fontFiles;
	Vector<AutoPtr<InstalledFont>> fonts;
	ComPtr<IDWriteFontCollection> collection;
};

//************************************************************************************************
// DWFontManager::InstalledFont
//************************************************************************************************

struct DWFontManager::FontFile: public Unknown
{
	FontFile (Buffer* data, int key, StringRef fileName)
	: fontData (data),
	  key(key),	
	  fileName (fileName)
	{}

	PROPERTY_STRING (fileName, FileName)
	PROPERTY_VARIABLE (int, key, Key)
	AutoPtr<Buffer> fontData;
};

//************************************************************************************************
// DWFontManager::InstalledFont
//************************************************************************************************

struct DWFontManager::InstalledFont: public Unknown
{
	InstalledFont (FontCollection* c, StringRef familyName, StringRef faceName, DWRITE_FONT_WEIGHT dwFontWeight)
	: owner (c), 
	  familyName (familyName),
	  faceName (faceName),
	  dwFontWeight (dwFontWeight)
	{}

	PROPERTY_STRING (familyName, FamilyName)
	PROPERTY_STRING (faceName, FaceName)
	PROPERTY_STRING (gdiFamilyName, GdiFamilyName) // GDI-compatible family name
	PROPERTY_VARIABLE (DWRITE_FONT_WEIGHT, dwFontWeight, DWFontWeight)
	PROPERTY_POINTER (FontCollection, owner, Owner)

	bool isBold () const
	{
		return dwFontWeight >= DWRITE_FONT_WEIGHT_DEMI_BOLD;
	}
	bool isLight () const
	{
		return dwFontWeight <= DWRITE_FONT_WEIGHT_SEMI_LIGHT;
	}
};

//************************************************************************************************
// DirectWrite interoperability helpers
//************************************************************************************************

namespace DWInterop {

inline DWRITE_TEXT_ALIGNMENT toTextAlignment (CCL::AlignmentRef alignment)
{
	int alignH = alignment.getAlignH ();
	return	alignH == Alignment::kLeft ? DWRITE_TEXT_ALIGNMENT_LEADING :
			alignH == Alignment::kHCenter ? DWRITE_TEXT_ALIGNMENT_CENTER : 
			DWRITE_TEXT_ALIGNMENT_TRAILING;
}

inline DWRITE_PARAGRAPH_ALIGNMENT toParagraphAlignment (CCL::AlignmentRef alignment)
{
	int alignV = alignment.getAlignV ();
	return	alignV == Alignment::kTop ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR :
			alignV == Alignment::kVCenter ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
			DWRITE_PARAGRAPH_ALIGNMENT_FAR;
}

inline void applyAlignment (IDWriteTextFormat* textFormat, CCL::AlignmentRef alignment)
{
	textFormat->SetTextAlignment (toTextAlignment (alignment));
	textFormat->SetParagraphAlignment (toParagraphAlignment (alignment));
}

inline void setWordWrapping (IDWriteTextFormat* textFormat, bool state)
{
	textFormat->SetWordWrapping (state ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
}

inline void setCharacterTrimming (IDWriteTextFormat* textFormat, bool state)
{
	const DWRITE_TRIMMING trimming = {state ? DWRITE_TRIMMING_GRANULARITY_CHARACTER : DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0};
	textFormat->SetTrimming (&trimming, nullptr);
}

inline void adjustTabStops (IDWriteTextLayout* textLayout)
{
	float v = textLayout->GetIncrementalTabStop ();
	textLayout->SetIncrementalTabStop (v/2.f);
}

inline void getTextMetrics (Rect& size, IDWriteTextLayout* textLayout)
{
	DWRITE_TEXT_METRICS textMetrics = {0};
	HRESULT hr = textLayout->GetMetrics (&textMetrics);
	ASSERT (SUCCEEDED (hr))

	size (0, 0, (Coord)(ceil (textMetrics.width)), (Coord)(ceil (textMetrics.height)));
}

inline void getTextMetrics (RectF& size, IDWriteTextLayout* textLayout)
{
	DWRITE_TEXT_METRICS textMetrics = {0};
	HRESULT hr = textLayout->GetMetrics (&textMetrics);
	ASSERT (SUCCEEDED (hr))

	size (0, 0, textMetrics.width, textMetrics.height);
}

static const int kTextLayoutMargin = 1;

template<class Rect>
inline void adjustTextMetrics (Rect& size)
{
	size.right += 2 * kTextLayoutMargin;
	size.bottom += 2 * kTextLayoutMargin;
}

template<class Point>
inline void adjustLayoutPos (Point& p, Alignment align)
{
	if(align.getAlignH () == Alignment::kLeft)
		p.x += kTextLayoutMargin;
	if(align.getAlignV () == Alignment::kTop)
		p.y += kTextLayoutMargin;
}

template<class Rect>
inline void adjustLayoutPosition (Rect& rect, Alignment align)
{
	if(align.getAlignH () == Alignment::kLeft)
		rect.left += kTextLayoutMargin;
	if(align.getAlignV () == Alignment::kTop)
		rect.top += kTextLayoutMargin;
}

} // namespace DWInterop

} // namespace Win32
} // namespace CCL

#endif // _ccl_dwriteengine_h
