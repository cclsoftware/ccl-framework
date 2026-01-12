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
// Filename    : ccl/platform/cocoa/quartz/fontcache.mm
// Description : CoreText font cache
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/fontcache.h"

#if CCL_PLATFORM_MAC
#define FONT_CLASS NSFont
#define FONT_DESCRIPTOR_CLASS NSFontDescriptor
#define FONT_NAME_ATTR NSFontNameAttribute
#define FONT_FAMILY_ATTR NSFontFamilyAttribute
#define FONT_FACE_ATTR NSFontFaceAttribute
#else
#define FONT_CLASS UIFont
#define FONT_DESCRIPTOR_CLASS UIFontDescriptor
#define FONT_NAME_ATTR UIFontDescriptorNameAttribute
#define FONT_FAMILY_ATTR UIFontDescriptorFamilyAttribute
#define FONT_FACE_ATTR UIFontDescriptorFaceAttribute
#endif

using namespace CCL;

//************************************************************************************************
// FontCache
//************************************************************************************************

DEFINE_SINGLETON (FontCache)

//////////////////////////////////////////////////////////////////////////////////////////////////

FontCache::FontCache ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontCache::~FontCache ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontCache::removeAll ()
{
	cache.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CTFontRef FontCache::createFont (FontRef font, float& ascent, float& descent, float& leading)
{
	CCL::ListIterator<FontCacheRecord> iter (cache);
	while(!iter.done ()) 
	{
		FontCacheRecord& record = iter.next ();
		if(record.font.isEqual (font))
		{
			ascent = record.ascent;
			descent = record.descent;
			leading = record.leading;
			return record.fontRef;
		}
	}
	
	String fontName = font.getFace ();
	StringRef fontStyleName = font.getStyleName ();
	NSString* familyName = [fontName.createNativeString<NSString*> () autorelease];
	NSString* faceName = [fontStyleName.createNativeString<NSString*> () autorelease];

	CCL_PRINTF ("creating font: %s - %s %f %d\n", MutableCString (fontName).str () , MutableCString (fontStyleName).str (), font.getSize (), font.getMode ())

	FONT_DESCRIPTOR_CLASS* fontDescriptor = [FONT_DESCRIPTOR_CLASS fontDescriptorWithFontAttributes:@{ FONT_FAMILY_ATTR : familyName, FONT_FACE_ATTR : faceName }];
	CTFontRef fontRef = (CTFontRef)[FONT_CLASS fontWithDescriptor:fontDescriptor size:font.getSize ()];
	if(fontRef)
	{
		// on iOS we always get a fontRef, but it might be a fallback font. To make sure we get the
		// desired font, we check it's name.
		NSString* postScriptName = [(NSString*)CTFontCopyPostScriptName (fontRef) autorelease];
		if(![postScriptName isEqualToString:familyName])
			fontRef = NULL;
	}

	if(fontRef == NULL)
	{
		if(!fontStyleName.isEmpty ())
			fontName << " " << fontStyleName;

		NSString* fullName = [fontName.createNativeString<NSString*> () autorelease];
		if(fontStyleName.isEmpty ())
		{
			CCL::ListIterator<StyledFont> iter2 (styledFontList);
			while(!iter2.done ())
			{
				StyledFont& styledFont = iter2.next ();
				if(styledFont.familyName == fontName && styledFont.fontStyle & font.getStyle ())
				{
					fullName = [styledFont.fullName.createNativeString<NSString*> () autorelease];
					break;
				}
			}
		}

		fontDescriptor = [FONT_DESCRIPTOR_CLASS fontDescriptorWithFontAttributes:@{ FONT_NAME_ATTR : fullName }];
		fontRef = (CTFontRef)[FONT_CLASS fontWithDescriptor:fontDescriptor size:font.getSize ()];
		SOFT_ASSERT (fontRef, "Font not available")
		if(fontRef == NULL)
			fontRef = (CTFontRef)[FONT_CLASS systemFontOfSize:font.getSize ()];
	}

	if(fontRef == NULL)
		return NULL;
		
	FontCacheRecord record (font, fontRef, (float)CTFontGetAscent (fontRef), (float)CTFontGetDescent (fontRef), (float)CTFontGetLeading (fontRef));
	cache.append (record);

	#if DEBUG_LOG
	CFObj<CFStringRef> cfname;
	String nameString;
	cfname = CTFontCopyName (fontRef, kCTFontFullNameKey);
	nameString.appendNativeString (cfname);
	CCL_PRINTF ("fullName: %s\n", MutableCString (nameString).str ())
	cfname = CTFontCopyName (fontRef, kCTFontPostScriptNameKey);
	nameString.empty ();
	nameString.appendNativeString (cfname);
	CCL_PRINTF ("postScriptName: %s\n", MutableCString (nameString).str ())
	cfname = CTFontCopyName (fontRef, kCTFontFamilyNameKey);
	nameString.empty ();
	nameString.appendNativeString (cfname);
	CCL_PRINTF ("familyName: %s\n", MutableCString (nameString).str ())
	#endif

	ascent = record.ascent;
	descent = record.descent;
	leading = record.leading;
	
	CCL_PRINTLN ("done creating font\n")
	
	return fontRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontCache::addStyledFont (StringRef familyName, int fontStyle, StringRef fullName)
{
	StyledFont record (familyName, fontStyle, fullName);
	styledFontList.append (record);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CTFontRef FontCache::getStyledFont (FontRef font, int style)
{
	Font styledFont (font);
	styledFont.setStyle (style);

	float unused;
	return createFont (styledFont, unused, unused, unused);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontCache::addUserFont (StringRef familyName)
{
	if(!userFontList.contains (familyName))
		userFontList.append (familyName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontCache::isUserFont (StringRef familyName) const
{
	return userFontList.contains (familyName);
}
