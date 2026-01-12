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
// Filename    : ccl/platform/win/direct2d/dwriteengine.cpp
// Description : DirectWrite Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/direct2d/dwriteengine.h"
#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/public/math/mathprimitives.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DWriteGdiFontHelper
//************************************************************************************************

class DWriteGdiFontHelper: public IGdiFontCompatibilityHelper
{
public:
	HFONT createGdiFont (FontRef font) const override
	{
		return DWriteEngine::instance ().createGdiFont (font);
	}
};

//************************************************************************************************
// LocalizedString - IDWriteLocalizedStrings helper
//************************************************************************************************

class LocalizedString: public CCL::String
{
public:	
	static bool assign (String& target, IDWriteLocalizedStrings* collection, UINT32 index)
	{
		ASSERT (collection != nullptr)
		if(collection == nullptr)
			return false;

		UINT32 length = 0;
		HRESULT hr = collection->GetStringLength (index, &length);
		
		if(SUCCEEDED (hr) && length > 0)			
		{
			static const UINT32 kMaxNameLength = 128;
			if(length > kMaxNameLength)
				length = kMaxNameLength;
	
			WCHAR* stringBuffer = NEW WCHAR[length+1];
			VectorDeleter<WCHAR> bufferDeleter (stringBuffer);
			hr = collection->GetString (index, stringBuffer, length+1);
			
			if(SUCCEEDED (hr))
			{
				stringBuffer[length] = 0;
				target.assign (stringBuffer, length);
			}
			return true;
		}
		return false;
	}
	
	LocalizedString (IDWriteLocalizedStrings* collection)
	{
		ASSERT (collection != nullptr)
		if(collection == nullptr)
			return;
		
		UINT32 index = 0;
		BOOL exists = false;
		if(collection->GetCount () > 1)
		{
			HRESULT hr = collection->FindLocaleName (L"en-us", &index, &exists);		
			if(SUCCEEDED (hr) == false)
				exists = false;
		}
	
		SOFT_ASSERT ((collection->GetCount () == 1 || exists), "More than one string in collection!\n")
	
		// if the locale doesn't exist, select the first in the list
		if(!exists)
			index = 0;

		assign (*this, collection, index);
	}

	static bool contains (IDWriteLocalizedStrings* collection, StringRef toFind)
	{
		if(collection)
		{
			UINT32 count = collection->GetCount ();
			for(UINT32 index = 0; index < count; index++)
			{
				String str;
				if(assign (str, collection, index))
				{
					if(toFind == str)
						return true;
				}
			}
		}
		return false;		
	}
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

#pragma comment (lib, "dwrite.lib")

//************************************************************************************************
// DWriteEngine
//************************************************************************************************

bool DWriteEngine::startup ()
{
	// Note: This could lock the process if called from DllMain!
	HRESULT hr = ::DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<::IUnknown**> (static_cast<void**> (factory)));
	if(FAILED (hr))
		return false;	

	fontManager = NEW DWFontManager;
	
	hr = factory->RegisterFontFileLoader (fontManager);
    if(FAILED(hr))
        return false;

    hr = factory->RegisterFontCollectionLoader (fontManager);
    if(FAILED(hr))
        return false;

	// override GDI font creation
	static DWriteGdiFontHelper theDirectWriteHelper;
	theGdiFontHelper = &theDirectWriteHelper;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWriteEngine::shutdown ()
{
	cachedTextFormats.release ();
	cachedDefaultRenderingParams.release ();

	fontManager->removeInstalledFonts ();

	factory->UnregisterFontCollectionLoader (fontManager);
	factory->UnregisterFontFileLoader (fontManager);

	fontManager.release ();
	factory.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DWriteEngine::installFontFromMemory (const void* memory, uint32 size, StringRef fileName)
{
	return fontManager->installFontFromMemory (factory, memory, size, fileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DWriteEngine::beginFontInstallation (bool state)
{
	if(fontManager.isValid ())
	{
		fontManager->beginFontInstallation (state);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DWriteEngine::lookupDWFont (ComPtr<IDWriteFont>& result, IDWriteFontCollection* collection, StringRef familyName, StringRef styleName) const
{
	ComPtr<IDWriteFontCollection> systemFonts;
	if(collection == nullptr)
	{
		factory->GetSystemFontCollection (systemFonts, FALSE);
		collection = systemFonts;
	}
	
	if(collection)
	{	
		UINT32 familyIndex = 0;
		BOOL valid = false;
		collection->FindFamilyName (StringChars (familyName), &familyIndex, &valid);
		if(valid)
		{
			ComPtr<IDWriteFontFamily> fontFamily;
			collection->GetFontFamily (familyIndex, fontFamily);        
			if(fontFamily)
			{
				int fontCount = fontFamily->GetFontCount ();					
				for(int fontIndex = 0; fontIndex < fontCount; fontIndex++)
				{
					ComPtr<IDWriteFont> dwFont;
					fontFamily->GetFont (fontIndex, dwFont);
					if(dwFont)
					{
						ComPtr<IDWriteLocalizedStrings> faceNames;
						dwFont->GetFaceNames (faceNames);			
						if(LocalizedString::contains (faceNames, styleName))
						{
							result = dwFont;
							return true;								
						}
					}
				}	
			}
		}
	}			

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextFormat* DWriteEngine::createTextFormat (FontRef font)
{
	DWRITE_FONT_WEIGHT fontWeight = font.isBold () ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
	DWRITE_FONT_STYLE fontStyle = font.isItalic () ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
	DWRITE_FONT_STRETCH fontStretch = DWRITE_FONT_STRETCH_NORMAL;
	FLOAT fontSize = font.getSize ();
	String fontFace (font.getFace ());
	
	// the collection must always be checked for installed fonts, because CreateTextFormat never fails but returns a fallback
	IDWriteFontCollection* collection = nullptr;
	if(const DWFontManager::InstalledFont* installedFont = fontManager->lookupInstalledFont (fontFace, font.getStyle ()))
	{
		ASSERT (installedFont->getOwner ())
		if(installedFont->getOwner ())
		{
			if(fontFace == installedFont->getGdiFamilyName ()) // take weight of installed font when its gdi name is used
				fontWeight = installedFont->getDWFontWeight ();
			
			fontFace = installedFont->getFamilyName ();
			collection = installedFont->getOwner ()->collection;
		}
	}

	// when style is requested by name, lookup according font and use its style parameters
	if(font.getStyleName ().isEmpty () == false)
	{
		ComPtr<IDWriteFont> dwFont;
		if(lookupDWFont (dwFont, collection, font.getFace (), font.getStyleName ()))
		{
			fontWeight = dwFont->GetWeight ();
			fontStyle = dwFont->GetStyle ();
			fontStretch = dwFont->GetStretch ();
		}
	}

	IDWriteTextFormat* textFormat = nullptr;
	HRESULT hr = factory->CreateTextFormat (StringChars (fontFace), collection, fontWeight, fontStyle, fontStretch, fontSize, L"", &textFormat);

	ASSERT (SUCCEEDED (hr))
	return textFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextFormat* DWriteEngine::createCachedTextFormat (FontRef font)
{
	if(!cachedTextFormats)
		cachedTextFormats = NEW DWTextFormatCache;
	return cachedTextFormats->createFormat (font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextFormat* DWriteEngine::createCachedTextFormatAndFont (ComPtr<IDWriteFont>& dwFont, FontRef font)
{
	if(!cachedTextFormats)
		cachedTextFormats = NEW DWTextFormatCache;
	return cachedTextFormats->createFormatAndFont (dwFont, font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteFont* DWriteEngine::createCachedDWFont (FontRef font)
{
	if(!cachedTextFormats)
		cachedTextFormats = NEW DWTextFormatCache;
	return cachedTextFormats->createDWFont (font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HFONT DWriteEngine::createGdiFont (FontRef font)
{
	ComPtr<IDWriteGdiInterop> gdiInterop;
	HRESULT hr = factory->GetGdiInterop (gdiInterop);
	ASSERT (SUCCEEDED (hr))
	if(!gdiInterop)
		return NULL;

	ComPtr<IDWriteFont> dwFont;
	ComPtr<IDWriteTextFormat> dwFormat = createCachedTextFormatAndFont (dwFont, font);

	LOGFONTW logFont = {0};
	BOOL isSystemFont = FALSE;
	gdiInterop->ConvertFontToLOGFONT (dwFont, &logFont, &isSystemFont);

	logFont.lfHeight = -(int)(font.getSize () + .5f);
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality = font.getMode () == Font::kNone ? NONANTIALIASED_QUALITY : font.getMode () == Font::kAntiAlias ? ANTIALIASED_QUALITY : CLEARTYPE_QUALITY;
	logFont.lfPitchAndFamily = VARIABLE_PITCH;

	return ::CreateFontIndirectW (&logFont);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFontTable* DWriteEngine::collectFonts (int flags)
{
	AutoPtr<SimpleFontTable> result = NEW SimpleFontTable;
	
	ComPtr<IDWriteFontCollection> systemFontCollection;
	factory->GetSystemFontCollection (systemFontCollection, FALSE);
	
	bool collectSymbolicFonts = (flags & Font::kCollectSymbolicFonts) != 0;
	bool collectAppFonts = (flags & Font::kCollectAppFonts) != 0;
	bool collectSimulatedFonts = (flags & Font::kCollectSimulatedFonts) != 0;

	auto& installedCollections = fontManager->getInstalledCollections ();
	int installedCollectionCount = collectAppFonts ? installedCollections.count () : 0;

	for(int collectionIndex = -1; collectionIndex < installedCollectionCount; collectionIndex++)
	{
		IDWriteFontCollection* collection = nullptr;
		if(collectionIndex < 0)
			collection = systemFontCollection;
		else
			collection = installedCollections.at (collectionIndex)->collection;
		
		if(collection)
		{	
			int familyCount = collection->GetFontFamilyCount ();
			for(int familyIndex = 0; familyIndex < familyCount; familyIndex++)
			{
				ComPtr<IDWriteFontFamily> fontFamily;
				collection->GetFontFamily (familyIndex, fontFamily);        
				if(fontFamily)
				{
					ComPtr<IDWriteLocalizedStrings> familyNames;
					fontFamily->GetFamilyNames (familyNames);

					AutoPtr<SimpleFontTable::FontFamily> resultFamily = NEW SimpleFontTable::FontFamily;
					resultFamily->name = LocalizedString (familyNames);

					int fontCount = fontFamily->GetFontCount ();					
					for(int fontIndex = 0; fontIndex < fontCount; fontIndex++)
					{
						ComPtr<IDWriteFont> font;
						fontFamily->GetFont (fontIndex, font);

						if(font)
						{
							DWRITE_FONT_SIMULATIONS simulations = font->GetSimulations ();
							if(simulations == 0 || collectSimulatedFonts) // check if simulated fonts should be collected or only 'real' fonts
							{
								bool isSymbolic = font->IsSymbolFont ();
								if(isSymbolic == false || collectSymbolicFonts) 
								{						
									ComPtr<IDWriteLocalizedStrings> faceNames;
									font->GetFaceNames (faceNames);					
									resultFamily->styles.add (LocalizedString (faceNames));

									if(resultFamily->exampleText.isEmpty ())
									{
										BOOL exists = FALSE;
										ComPtr<IDWriteLocalizedStrings> infoStrings;
										font->GetInformationalStrings (DWRITE_INFORMATIONAL_STRING_SAMPLE_TEXT, infoStrings, &exists);
										if(exists)
											resultFamily->exampleText = LocalizedString (infoStrings);
										
										else if(isSymbolic)
										{
											// make 4 line example text by iterating supported characters
											StringWriter<32> writer (resultFamily->exampleText);											
											int counter = 0, lineCounter = 0;
											for(uchar c = 33; c < 0xFFFFF && lineCounter < 4; c++)
											{
												exists = false;
												font->HasCharacter (c, &exists);
												if(exists)
												{
													writer.append (c);							
													if(++counter >= 20)
														writer.append ('\n'), counter = 0, lineCounter++;
												}
											}
											writer.flush ();																				
										}
									}
								}
							}
						}
					}

					if(resultFamily->styles.isEmpty () == false)
						result->addFamilySorted (resultFamily.detach ());
				}
   			}				
		}
	}
	return result.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*IDWriteTextLayout* DWriteEngine::createTextLayout (StringRef text, IDWriteTextFormat* textFormat, float maxWidth, float maxHeight)
{
	ComPtr<IDWriteTextLayout> textLayout;
	StringChars textChars (text);
	UINT textLength = text.length ();

	HRESULT hr = factory->CreateTextLayout (textChars, textLength, textFormat, maxWidth, maxHeight, textLayout);
	ASSERT (SUCCEEDED (hr))

	return textLayout.detach ();
}*/

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextLayout* DWriteEngine::createTextLayoutWithFontAttributes (StringRef text, IDWriteTextFormat* textFormat, float maxWidth, float maxHeight, FontRef font)
{
	ComPtr<IDWriteTextLayout> textLayout;
	StringChars textChars (text);
	UINT textLength = text.length ();

	HRESULT hr = factory->CreateTextLayout (textChars, textLength, textFormat, maxWidth, maxHeight, textLayout);
	ASSERT (SUCCEEDED (hr))
	if(textLayout)
	{	
		DWRITE_TEXT_RANGE textRange = {0, textLength};
		// NOTE: bold/italic is already part of IDWriteTextFormat!

		if(font.isUnderline ())
		{
			hr = textLayout->SetUnderline (TRUE, textRange);
			ASSERT (SUCCEEDED (hr))
		}
		if(font.isStrikeout ())
		{
			hr = textLayout->SetStrikethrough (TRUE, textRange);
			ASSERT (SUCCEEDED (hr))
		}

		if(font.getSpacing () != 0.f)
		{
			ComPtr<IDWriteTextLayout1> textLayout1;
			if(SUCCEEDED (textLayout.as (textLayout1)))
			{
				hr = textLayout1->SetCharacterSpacing (0.f, font.getSpacing (), 0.f, textRange);
				ASSERT (SUCCEEDED (hr))
			}
		}

		if(font.getLineSpacing () != 1.f)
		{
			ComPtr<IDWriteTextLayout1> textLayout1;
			if(SUCCEEDED (textLayout.as (textLayout1)))
			{
				float lineSpacing = font.getLineSpacing ();
				hr = textLayout1->SetLineSpacing (DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, lineSpacing, DWTextLayout::kLineSpacingBaselineFactor * lineSpacing);
				ASSERT (SUCCEEDED (hr))
			}
		}
	}
	return textLayout.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteBitmapRenderTarget* DWriteEngine::createBitmapRenderTarget (HDC hdc, int width, int height)
{
	ComPtr<IDWriteGdiInterop> gdiInterop;
	HRESULT hr = factory->GetGdiInterop (gdiInterop);
	ASSERT (SUCCEEDED (hr))
	if(!gdiInterop)
		return nullptr;

	IDWriteBitmapRenderTarget* renderTarget = nullptr;
	hr = gdiInterop->CreateBitmapRenderTarget (hdc, width, height, &renderTarget);
	ASSERT (SUCCEEDED (hr))
	return renderTarget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// The Old New Thing: How do I get the handle of the primary monitor?
static HMONITOR GetPrimaryMonitorHandle ()
{
	const POINT ptZero = {0, 0};
	return ::MonitorFromPoint (ptZero, MONITOR_DEFAULTTOPRIMARY);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteRenderingParams* DWriteEngine::createDefaultRenderingParams ()
{
	IDWriteRenderingParams* renderingParams = nullptr;
	HRESULT hr = factory->CreateMonitorRenderingParams (GetPrimaryMonitorHandle (), &renderingParams);
	ASSERT (SUCCEEDED (hr))
	return renderingParams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteRenderingParams* DWriteEngine::getCachedDefaultRenderingParams ()
{
	if(!cachedDefaultRenderingParams)
	{
		#if 0 // EXPERIMENTAL: customize rendering parameters
		ComPtr<IDWriteRenderingParams> defaultParams = createDefaultRenderingParams ();
		DWRenderingParamsDescription description;
		getRenderingParamsDescription (description, defaultParams);
		description.clearTypeLevel = 0.f;
		description.enhancedContrast = 0.f;
		//description.renderingMode = DWRITE_RENDERING_MODE_NATURAL;
		cachedDefaultRenderingParams = createRenderingParams (description);		
		#else
		cachedDefaultRenderingParams = createDefaultRenderingParams ();
		#endif
	}
	return cachedDefaultRenderingParams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteRenderingParams* DWriteEngine::createRenderingParams (const DWRenderingParamsDescription& description)
{
	IDWriteRenderingParams* renderingParams = nullptr;
	HRESULT hr = factory->CreateCustomRenderingParams (description.gamma, description.enhancedContrast, 
													   description.clearTypeLevel, description.pixelGeometry,
													   description.renderingMode, &renderingParams);
	ASSERT (SUCCEEDED (hr))
	return renderingParams;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWriteEngine::getRenderingParamsDescription (DWRenderingParamsDescription& description, IDWriteRenderingParams* params)
{
	description.gamma = params->GetGamma ();
	description.enhancedContrast = params->GetEnhancedContrast ();
	description.clearTypeLevel = params->GetClearTypeLevel ();
	description.pixelGeometry = params->GetPixelGeometry ();
	description.renderingMode = params->GetRenderingMode ();
}

//************************************************************************************************
// DWTextFormatCache
//************************************************************************************************

DWTextFormatCache::DWTextFormatCache ()
: entries (kMaxChacheEntries)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DWTextFormatCache::CachedFormat* DWTextFormatCache::lookup (FontRef font) const
{
	for(int i = 0; i < entries.count (); i++)
	{
		const CachedFormat& e = entries[i];
		if(e.font.getFace () == font.getFace () && 
		   e.font.getSize () == font.getSize () && 
		   ((font.getStyleName ().isEmpty () && e.font.getStyleName ().isEmpty ())
				? getUsedStyle (e.font) == getUsedStyle (font) // ignore underline, etc.
				: e.font.getStyleName () == font.getStyleName ()))
			return const_cast<CachedFormat*> (&e);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DWTextFormatCache::CachedFormat* DWTextFormatCache::createEntry (FontRef font)
{
	CachedFormat* entry = lookup (font);
	if(entry == nullptr)
	{
		// check for max. cache size
		if(entries.count () >= kMaxChacheEntries)
		{
			entries.removeAll ();
			entries.resize (kMaxChacheEntries);
		}

		IDWriteTextFormat* textFormat = DWriteEngine::instance ().createTextFormat (font);
		if(textFormat)				
			entry = add (font, textFormat);
	}
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DWTextFormatCache::CachedFormat* DWTextFormatCache::add (FontRef font, IDWriteTextFormat* textFormat)
{
	CachedFormat e;
	e.font = font;
	e.textFormat = textFormat;
	entries.add (e);
	return &entries.last ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWTextFormatCache::removeAll ()
{
	entries.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextFormat* DWTextFormatCache::createFormat (FontRef font)
{
	IDWriteTextFormat* textFormat = nullptr;
	if(CachedFormat* entry = createEntry (font))
	{
		textFormat = entry->textFormat;
		textFormat->AddRef ();
	}
	return textFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteTextFormat* DWTextFormatCache::createFormatAndFont (ComPtr<IDWriteFont>& dwFont, FontRef font)
{
	IDWriteTextFormat* textFormat = nullptr;
	if(CachedFormat* entry = createEntry (font))
	{
		textFormat = entry->textFormat;
		textFormat->AddRef ();

		createDWFont (*entry);
		dwFont.share (entry->dwFont);
	}
	return textFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDWriteFont* DWTextFormatCache::createDWFont (FontRef font)
{
	CachedFormat* entry = createEntry (font);
	if(entry)
	{
		createDWFont (*entry);

		if(entry->dwFont)
			entry->dwFont->AddRef ();
		return entry->dwFont;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWTextFormatCache::createDWFont (CachedFormat& entry)
{
	if(!entry.dwFont)
	{
		ComPtr<IDWriteFontCollection> collection;
		entry.textFormat->GetFontCollection (collection);
		if(collection)
		{
			TCHAR name[64];
			entry.textFormat->GetFontFamilyName (name, 64);

			UINT32 findex = 0;
			BOOL exists = false;
			collection->FindFamilyName (name, &findex, &exists);

			if(exists)
			{
				ComPtr<IDWriteFontFamily> ffamily;
				collection->GetFontFamily (findex, ffamily);
				if(ffamily)
				{
					IDWriteFont* dwFont = nullptr;
					ffamily->GetFirstMatchingFont (entry.textFormat->GetFontWeight (), entry.textFormat->GetFontStretch (), entry.textFormat->GetFontStyle (), &dwFont);
					entry.dwFont = dwFont;
				}
			}
		}
	}
}

//************************************************************************************************
// DWTextLayout
//************************************************************************************************

float DWTextLayout::getBaseline (IDWriteTextLayout* layout)
{
	if(layout)
	{
		DWRITE_LINE_METRICS metrics = {};
		UINT32 actualLineCount = 0;
		layout->GetLineMetrics (&metrics, 1, &actualLineCount);
		return metrics.baseline;	
	}
	return 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DWTextLayout, Object)

const float DWTextLayout::kLineSpacingBaselineFactor = 0.8f;

//////////////////////////////////////////////////////////////////////////////////////////////////

DWTextLayout::DWTextLayout ()
: baselineOffset (kMinCoord, kMinCoord),
  wordWrap (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format)
{
	return construct (text, (float)width, (float)height, font, lineMode, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format)
{
	ComPtr<IDWriteTextFormat> textFormat = DWriteEngine::instance ().createCachedTextFormat (font);
	ASSERT (textFormat.isValid ()) // fails e.g. if font size is zero
	if(!textFormat)
		return kResultFailed;

	wordWrap = lineMode == kMultiLine && format.isWordBreak ();
	DWInterop::applyAlignment (textFormat, format.getAlignment ());
	DWInterop::setWordWrapping (textFormat, wordWrap);
	//if(lineMode == kSingleLine)
	//	DWInterop::setCharacterTrimming (textFormat, true);

	// when layout is used to measure how many lines are needed in a multiline layout for a given width, 
	// sometimes the height is too small. This seems to fix it:
	if(wordWrap && width > 1)
		width--; 

	setAlignment (format.getAlignment ());
	layout = DWriteEngine::instance ().createTextLayoutWithFontAttributes (text, textFormat, width, height, font);
	if(layout)
	{
		if(lineMode == kMultiLine)
			DWInterop::adjustTabStops (layout);
	}

	layoutSize (width, height);
	this->font = font;
	this->text = text;
	return layout ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::resize (Coord width, Coord height)
{
	return resize (CoordF (width), CoordF (height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::resize (CoordF width, CoordF height)
{
	// when layout is used to measure how many lines are needed in a multiline layout for a given width, 
	// sometimes the height is too small. This seems to fix it:
	if(wordWrap && width > 1)
		width--;

	if(layout)
	{
		HRESULT hr = layout->SetMaxWidth (width);
		if(FAILED (hr))
			return kResultFailed;
		hr = layout->SetMaxHeight (height);
		if(FAILED (hr))
			return kResultFailed;
	}
	layoutSize (width, height);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setFontStyle (const Range& range, int style, tbool state)
{
	ASSERT (layout != 0)
	if(!layout)
		return kResultUnexpected;

	DWRITE_TEXT_RANGE textRange = {(UINT32)range.start, (UINT32)range.length};

	if(style & Font::kBold)
	{
		if(state)
			layout->SetFontWeight (DWRITE_FONT_WEIGHT_BOLD, textRange);
		else
			layout->SetFontWeight (DWRITE_FONT_WEIGHT_NORMAL, textRange);
	}

	if(style & Font::kItalic)
	{
		if(state)
			layout->SetFontStyle (DWRITE_FONT_STYLE_ITALIC, textRange);
		else
			layout->SetFontStyle (DWRITE_FONT_STYLE_NORMAL, textRange);
	}

	if(style & Font::kUnderline)
		layout->SetUnderline (state, textRange);

	if(style & Font::kStrikeout)
		layout->SetStrikethrough (state, textRange);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setFontSize (const Range& range, float size)
{
	ASSERT (layout != 0)
	if(!layout)
		return kResultUnexpected;

	DWRITE_TEXT_RANGE textRange = {(UINT32)range.start, (UINT32)range.length};
	return static_cast<tresult> (layout->SetFontSize (size, textRange));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setSpacing (const Range& range, float spacing)
{
	DWRITE_TEXT_RANGE textRange = {(UINT32)range.start, (UINT32)range.length};

	ComPtr<IDWriteTextLayout1> textLayout1;
	HRESULT hr = layout.as (textLayout1);
	if(textLayout1.isValid ())
		hr = textLayout1->SetCharacterSpacing (0.f, spacing, 0.f, textRange);

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setLineSpacing (const Range& range, float lineSpacing)
{
	DWRITE_TEXT_RANGE textRange = {(UINT32)range.start, (UINT32)range.length};

	ComPtr<IDWriteTextLayout1> textLayout1;
	HRESULT hr = layout.as (textLayout1);
	if(textLayout1.isValid ())
		hr = textLayout1->SetLineSpacing (DWRITE_LINE_SPACING_METHOD_PROPORTIONAL, lineSpacing, kLineSpacingBaselineFactor * lineSpacing);

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setTextColor (const Range& range, Color color)
{
	CCL_NOT_IMPL ("Text color must be handled by derived class!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setBaselineOffset (const Range& range, float offset)
{
	CCL_NOT_IMPL ("Baseline offset must be handled by derived class!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setSuperscript (const Range& range)
{
	CCL_NOT_IMPL ("Superscript must be handled by derived class!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::setSubscript (const Range& range)
{
	CCL_NOT_IMPL ("Subscript must be handled by derived class!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getBaselineOffset (PointF& offset) const
{
	if(baselineOffset.x == kMinCoord)
		baselineOffset (0, getBaseline (layout));	
	
	offset = baselineOffset;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
TPoint<TCoord> DWTextLayout::getTextPosition (const TRect<TCoord>& textBounds) const
{
	TPoint<TCoord> textPos;
	switch(alignment.getAlignH ())
	{
		case Alignment::kHCenter :
			textPos.x = (TCoord(layoutSize.x) - textBounds.getWidth ()) / 2;
			break;
		case Alignment::kRight :
			textPos.x = TCoord(layoutSize.x) - textBounds.getWidth ();
			break;
		default : // left aligned
			break;
	}

	switch(alignment.getAlignV ())
	{
		case Alignment::kVCenter :
			textPos.y = (TCoord(layoutSize.y) - textBounds.getHeight ()) / 2;
			break;
		case Alignment::kBottom :
			textPos.y = (TCoord(layoutSize.y) - textBounds.getHeight ());
			break;

		default : // top aligned
			break;
	}
	return textPos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getBounds (Rect& bounds, int flags) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	DWInterop::getTextMetrics (bounds, layout);
	if(!(flags & kNoMargin))
		DWInterop::adjustTextMetrics (bounds);
	bounds.offset (getTextPosition (bounds));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getBounds (RectF& bounds, int flags) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	DWInterop::getTextMetrics (bounds, layout);
	if(!(flags & kNoMargin))
		DWInterop::adjustTextMetrics (bounds);
	bounds.offset (getTextPosition (bounds));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getImageBounds (RectF& bounds) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	// Zero unbounded layout extents (represented by kMaxCoord) before measuring
	// as limited float precision around kMaxCoord hurts accuracy otherwise.
	PointF measureSize (layoutSize.x < (CoordF)kMaxCoord ? layoutSize.x : 0.f, layoutSize.y < (CoordF)kMaxCoord ? layoutSize.y : 0.f);

	layout->SetMaxWidth (measureSize.x);
	layout->SetMaxHeight (measureSize.y);

	DWRITE_OVERHANG_METRICS overhangMetrics = { 0 };
	HRESULT hr = layout->GetOverhangMetrics (&overhangMetrics);
	bounds (-overhangMetrics.left, -overhangMetrics.top, measureSize.x + overhangMetrics.right, measureSize.y + overhangMetrics.bottom);

	layout->SetMaxWidth (layoutSize.x);
	layout->SetMaxHeight (layoutSize.y);

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::hitTest (int& textIndex, PointF& position) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	DWRITE_HIT_TEST_METRICS hitTestMetrics = {};
	BOOL isTrailingHit = false;
	BOOL isInside = false;
	HRESULT hr = layout->HitTestPoint (position.x, position.y, &isTrailingHit, &isInside, &hitTestMetrics);
	textIndex = hitTestMetrics.textPosition;
	position.x = hitTestMetrics.left;
	position.y = hitTestMetrics.top;
	if(isTrailingHit)
		textIndex += 1;

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getCharacterBounds (RectF& offset, int textIndex) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	DWRITE_HIT_TEST_METRICS hitTestMetrics = {};
	float x = 0;
	float y = 0;
	HRESULT hr = layout->HitTestTextPosition (textIndex, false, &x, &y, &hitTestMetrics);
	offset.left = x;
	offset.top = y;
	offset.setHeight (hitTestMetrics.height);
	offset.setWidth (hitTestMetrics.width);

	return static_cast<tresult> (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getTextBounds (IMutableRegion& bounds, const Range& range) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	UINT32 count = 0;
	HRESULT hr = layout->HitTestTextRange (range.start, range.length, 0, 0, nullptr, 0, &count);
	if(hr != E_NOT_SUFFICIENT_BUFFER)
		return kResultUnexpected;

	DWRITE_HIT_TEST_METRICS* metrics = NEW DWRITE_HIT_TEST_METRICS[count];
	VectorDeleter<DWRITE_HIT_TEST_METRICS> deleter (metrics);
	hr = layout->HitTestTextRange (range.start, range.length, 0, 0, metrics, count, &count);
	if(FAILED (hr))
		return kResultFailed;

	ComPtr<IDWriteFont> dwFont;
	ComPtr<IDWriteTextFormat> textFormat = DWriteEngine::instance ().createCachedTextFormatAndFont (dwFont, getFont ());
	float endOfLineIndicatorWidth = ccl_max (textFormat->GetFontSize () / 6.f, 2.f);

	for(int i = 0; i < count; i++)
	{
		int lastPosition = metrics[i].textPosition + metrics[i].length - 1;
		bool isEndOfLine = lastPosition < text.length () && (text[lastPosition] == '\r' || text[lastPosition] == '\n');
		bounds.addRect (Rect (ccl_to_int (metrics[i].left), ccl_to_int (metrics[i].top), ccl_to_int (metrics[i].left + metrics[i].width + (isEndOfLine ? endOfLineIndicatorWidth : 0)), ccl_to_int (metrics[i].top + metrics[i].height)));
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWTextLayout::getLineRange (Range& range, int textIndex) const
{
	ASSERT (layout.isValid ())
	if(!layout)
		return kResultUnexpected;

	UINT32 count = 0;
	HRESULT hr = layout->GetLineMetrics (nullptr, 0, &count);
	if(hr != E_NOT_SUFFICIENT_BUFFER)
		return kResultUnexpected;

	DWRITE_LINE_METRICS* metrics = NEW DWRITE_LINE_METRICS[count];
	VectorDeleter<DWRITE_LINE_METRICS> deleter (metrics);
	hr = layout->GetLineMetrics (metrics, count, &count);
	if(FAILED (hr))
		return kResultFailed;

	UINT32 currentTextPosition = 0;
	for(UINT32 i = 0; i < count; i++)
	{
		if(currentTextPosition + metrics[i].length > textIndex)
		{
			range.start = currentTextPosition;
			range.length = metrics[i].length;
			return kResultOk;
		}
		currentTextPosition += metrics[i].length;
	}

	range.start = currentTextPosition;
	range.length = 0;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DWTextLayout::getText () const
{
	return text;
}

//************************************************************************************************
// DWFontManager::MemoryFontFileEnumerator
//************************************************************************************************

class DWFontManager::MemoryFontFileEnumerator: public Object, 
	                                           public IDWriteFontFileEnumerator
{
public:
    MemoryFontFileEnumerator (IDWriteFactory* _factory, IDWriteFontFileLoader* _loader, FontCollection* fontCollection)
	: currentFile (nullptr), currentFileIndex (0), loader (_loader), fontCollection (fontCollection)
	{
		factory.share (_factory);		
	}

	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (IDWriteFontFileEnumerator)
		return Object::queryInterface (iid, ptr);	
	}

    // IDWriteFontFileEnumerator methods
    HRESULT STDMETHODCALLTYPE MoveNext (OUT BOOL* hasCurrentFile) override
	{
		*hasCurrentFile = FALSE;
		HRESULT hr = S_OK;
		if(factory != 0)
		{
			if(currentFileIndex < fontCollection->fontFiles.count ())
			{
				currentFile.release ();

				FontFile* fontFile = fontCollection->fontFiles.at (currentFileIndex);
				int key = fontFile->getKey ();
				hr = factory->CreateCustomFontFileReference (&key, sizeof(int), loader, currentFile);
				if(SUCCEEDED(hr))
				{
					*hasCurrentFile = TRUE;
					currentFileIndex++;
				}
			}
		}
		return hr;
	}

    HRESULT STDMETHODCALLTYPE GetCurrentFontFile (OUT IDWriteFontFile** fontFile) override
	{
		if(currentFile && fontFile)
		{
			currentFile->AddRef ();
			*fontFile = currentFile;
			return S_OK;
		}
		return S_FALSE;
	}

private:
    ComPtr<IDWriteFactory> factory;
	ComPtr<IDWriteFontFile> currentFile;
	IDWriteFontFileLoader* loader;
	SharedPtr<FontCollection> fontCollection;
	int currentFileIndex;
};

//************************************************************************************************
// DWFontManager::MemoryFontFileStream
//************************************************************************************************

class DWFontManager::MemoryFontFileStream : public Object, 
	                                        public IDWriteFontFileStream
{
public:
	MemoryFontFileStream (Buffer* buffer)
	: buffer (buffer), 
	  memory (buffer->getAddress ()), 
	  memSize (buffer->getSize ())
	{}	

	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (IDWriteFontFileStream)
		return Object::queryInterface (iid, ptr);	
	}

    HRESULT STDMETHODCALLTYPE ReadFileFragment (void const** fragmentStart, UINT64 fileOffset,  UINT64 fragmentSize,  OUT void** fragmentContext) override
	{
		if(fileOffset <= memSize && fragmentSize <= memSize - fileOffset)
		{
			*fragmentStart = static_cast<BYTE const*>(memory) + static_cast<size_t>(fileOffset);
			*fragmentContext = nullptr;
			return S_OK;
		}
		else
		{
			*fragmentStart = nullptr;
			*fragmentContext = nullptr;
			return E_FAIL;
		}
	}

	void STDMETHODCALLTYPE ReleaseFileFragment (void* fragmentContext) override {}

	HRESULT STDMETHODCALLTYPE GetFileSize (OUT UINT64* fileSize) override
	{
		*fileSize = memSize;
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE GetLastWriteTime(UINT64* lastWriteTime) override
	{
		// The concept of last write time does not apply to this loader.
		*lastWriteTime = 0;
		return E_NOTIMPL;
	}

private:
	SharedPtr<Buffer> buffer;
	const void* memory;
	uint32 memSize;
};

//************************************************************************************************
// DWFontManager
//************************************************************************************************

DWFontManager::DWFontManager ()
: nextKey (1),
  factory (nullptr),
  pendingCollection (nullptr),
  inFontInstallationScope (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DWFontManager::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDWriteFontCollectionLoader)
	QUERY_COM_INTERFACE (IDWriteFontFileLoader)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DWFontManager::CreateEnumeratorFromKey (IDWriteFactory* factory, void const* collectionKey, UINT32 collectionKeySize, IDWriteFontFileEnumerator** fontFileEnumerator)
{
	// IDWriteFontCollectionLoader
	FontCollection* collection = getCollectionFromKey (collectionKey, collectionKeySize);
	if(collection == nullptr)
	{
		*fontFileEnumerator = nullptr;
		return 	E_INVALIDARG;
	}
	
	MemoryFontFileEnumerator* enumerator = NEW MemoryFontFileEnumerator (factory, this, collection);
	if(enumerator == nullptr)
	{
		*fontFileEnumerator = nullptr;
		return E_OUTOFMEMORY;
	}

	*fontFileEnumerator = enumerator;    
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DWFontManager::CreateStreamFromKey (void const* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream)
{
   // IDWriteFontFileLoader
   *fontFileStream = nullptr;

	FontFile* fontFile = getFontFileFromKey (fontFileReferenceKey, fontFileReferenceKeySize);
	if(fontFile && fontFile->fontData)
	{
		MemoryFontFileStream* stream = NEW MemoryFontFileStream (fontFile->fontData);
		if(stream == nullptr)
			return E_OUTOFMEMORY;

		*fontFileStream = stream;
		return S_OK;
	}
	return E_INVALIDARG;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
static inline void printFontFamilyDetails (IDWriteFontFamily* fontFamily)
{
	ComPtr<IDWriteLocalizedStrings> familyNames;
	fontFamily->GetFamilyNames (familyNames);					
	LocalizedString familyName (familyNames);
	UINT32 fontCount = fontFamily->GetFontCount ();

	Debugger::println (String () << "Font family = \"" << familyName << "\" font count = " << int(fontCount));

	for(UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
	{
		ComPtr<IDWriteFont> font;
		fontFamily->GetFont (fontIndex, font);
		if(font)
		{
			ComPtr<IDWriteLocalizedStrings> faceNames;
			font->GetFaceNames (faceNames);
			LocalizedString faceName (faceNames);

			Debugger::println (String () << "- Font " << int(fontIndex) << ": face = \"" << faceName << "\"");

			static const struct { DWRITE_INFORMATIONAL_STRING_ID key; CStringPtr name; } idList[] = 
			{
				{DWRITE_INFORMATIONAL_STRING_FULL_NAME, "Full Name"},
				{DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, "Win32 Family Name"},
				{DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES, "Win32 Subfamily Name"},
				{DWRITE_INFORMATIONAL_STRING_PREFERRED_FAMILY_NAMES, "Preferred Family Name"},
				{DWRITE_INFORMATIONAL_STRING_PREFERRED_SUBFAMILY_NAMES, "Preferred Subfamily Name"}
			};

			for(int idIndex = 0; idIndex < ARRAY_COUNT (idList); idIndex++)
			{
				BOOL exists = FALSE;
				ComPtr<IDWriteLocalizedStrings> infoStrings;
				font->GetInformationalStrings (idList[idIndex].key, infoStrings, &exists);
				String infoString;
				if(exists)
					infoString = LocalizedString (infoStrings);

				Debugger::println (String () << "-- " << idList[idIndex].name << " = \"" << infoString << "\"");
			}
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

DWFontManager::FontFile* DWFontManager::getFontFileFromKey (void const* fontKeyPtr, UINT32 fontKeySize) const
{
	if(fontKeySize == sizeof(int))
	{
		int fontKey = *reinterpret_cast<const int*> (fontKeyPtr);

		for(int collectionIndex = 0; collectionIndex < fontCollections.count (); collectionIndex++)
		{
			FontCollection* collection = fontCollections.at (collectionIndex);
			for(int fontIndex = 0; fontIndex < collection->fontFiles.count (); fontIndex++)
			{
				FontFile* fontFile = collection->fontFiles.at (fontIndex);
				if(fontFile->getKey () == fontKey)
					return fontFile;
			}		
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DWFontManager::FontCollection* DWFontManager::getCollectionFromKey (void const* collectionKeyPtr, UINT32 collectionKeySize) const
{
	if(collectionKeySize == sizeof(int))
	{
		int collectionKey = *reinterpret_cast<const int*> (collectionKeyPtr);

		for(int collectionIndex = 0; collectionIndex < fontCollections.count (); collectionIndex++)
		{
			FontCollection* collection = fontCollections.at (collectionIndex);
			if(collection->getKey () == collectionKey)
				return collection;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DWFontManager::makeKey ()
{
	return nextKey++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DWFontManager::installFontFromMemory (IDWriteFactory* factory, const void* memory, uint32 size, StringRef fileName)
{	
	if(memory && size > 0)
	{				
		// check if already installed (reload skin)
		for(int collectionIndex = 0; collectionIndex < fontCollections.count (); collectionIndex++)
		{
			FontCollection* collection = fontCollections.at (collectionIndex);
			for(int fontIndex = 0; fontIndex < collection->fontFiles.count (); fontIndex++)
			{
				FontFile* fontFile = collection->fontFiles.at (fontIndex);
				if(fontFile->fontData->getSize () == size &&  memcmp (memory, fontFile->fontData->getAddress (), size) == 0) 
					return false;
			}		
		}

		setFactory (factory);

		if(pendingCollection == nullptr)
		{
			pendingCollection = NEW FontCollection (makeKey ());
			fontCollections.add (pendingCollection);
		}

		Buffer* buffer = NEW Buffer (const_cast<void*>(memory), size);
		FontFile* fontFile = NEW FontFile (buffer, makeKey (), fileName);
		pendingCollection->fontFiles.add (fontFile);

		if(inFontInstallationScope == false)
			loadPendingCollection ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWFontManager::beginFontInstallation (bool state)
{
	inFontInstallationScope = state;
	if(inFontInstallationScope == false)
		loadPendingCollection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWFontManager::loadPendingCollection ()
{
	if(factory && pendingCollection)
	{
		int key = pendingCollection->getKey ();
		factory->CreateCustomFontCollection (this, (void*)&key, sizeof(int), pendingCollection->collection);
		ASSERT (pendingCollection->collection != 0)

		if(pendingCollection->collection)
		{
			IDWriteFontCollection* dwCollection = pendingCollection->collection;
			
			UINT32 familyCount = dwCollection->GetFontFamilyCount ();
			for(UINT32 familyIndex = 0; familyIndex < familyCount; familyIndex++)
			{
				ComPtr<IDWriteFontFamily> fontFamily;
				dwCollection->GetFontFamily (familyIndex, fontFamily);
				if(fontFamily)
				{
					#if 0 && DEBUG
					printFontFamilyDetails (fontFamily);
					#endif
					ComPtr<IDWriteLocalizedStrings> familyNames;
					fontFamily->GetFamilyNames (familyNames);					
					LocalizedString familyName (familyNames);
												
					UINT32 fontCount = fontFamily->GetFontCount ();
					for(UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
					{
						ComPtr<IDWriteFont> font;
						fontFamily->GetFont (fontIndex, font);
						if(font && font->GetSimulations () == 0)
						{
							ComPtr<IDWriteLocalizedStrings> faceNames;
							font->GetFaceNames (faceNames);					
							LocalizedString faceName (faceNames);

							// add installed font
							InstalledFont* installed = NEW InstalledFont (pendingCollection, familyName, faceName, font->GetWeight ());
							pendingCollection->fonts.add (installed);
								
							BOOL exists = FALSE;
							ComPtr<IDWriteLocalizedStrings> infoStrings;
							font->GetInformationalStrings (DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, infoStrings, &exists);
							if(exists)
							{
								LocalizedString gdiFamilyName (infoStrings);
								if(!gdiFamilyName.isEmpty () && familyName != gdiFamilyName)
									installed->setGdiFamilyName (gdiFamilyName);
							}
						}
					}
				}        
			}
		}

		if(pendingCollection->collection == 0)
		{
			int index = fontCollections.index (AutoPtr<FontCollection> ().share (pendingCollection));
			if(index >= 0)
				fontCollections.removeAt (index);
		}		

		pendingCollection = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DWFontManager::removeInstalledFonts ()
{
	fontCollections.removeAll (); 
	pendingCollection = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const DWFontManager::InstalledFont* DWFontManager::lookupInstalledFont (StringRef name, int fontStyle) const
{
	ASSERT (pendingCollection == nullptr)
	
	bool bold = (fontStyle & Font::kBold) != 0;

	InstalledFont* matchingFont = nullptr;
	for(int collectionIndex = 0; collectionIndex < fontCollections.count (); collectionIndex++)
	{
		FontCollection* collection = fontCollections.at (collectionIndex);
		for(int fontIndex = 0; fontIndex < collection->fonts.count (); fontIndex++)
		{
			InstalledFont* font = collection->fonts.at (fontIndex);
			if(name == font->getGdiFamilyName ()) // ignore fontStyle when the gdi name is used
				return font;

			if(name == font->getFamilyName ())
			{				
				if(font->isLight () == false) // do not find light fonts here, they are not expressed as fontStyle
				{								
					if(font->isBold () == bold)
						return font;

					matchingFont = font;
				}
			}
		}		
	}
	return matchingFont;
}
