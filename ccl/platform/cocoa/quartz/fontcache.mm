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
#define FONT_TRAITS_CLASS NSFontDescriptorSymbolicTraits
#define FONT_SYMBOLIC_TRAIT_KEY NSFontSymbolicTrait
#define FONT_TRAITS_ATTR NSFontTraitsAttribute
#define FONT_DESCRIPTOR_TRAIT_BOLD NSFontDescriptorTraitBold
#define FONT_DESCRIPTOR_TRAIT_ITALIC NSFontDescriptorTraitItalic
#else
#define FONT_CLASS UIFont
#define FONT_DESCRIPTOR_CLASS UIFontDescriptor
#define FONT_NAME_ATTR UIFontDescriptorNameAttribute
#define FONT_FAMILY_ATTR UIFontDescriptorFamilyAttribute
#define FONT_FACE_ATTR UIFontDescriptorFaceAttribute
#define FONT_TRAITS_CLASS UIFontDescriptorSymbolicTraits
#define FONT_SYMBOLIC_TRAIT_KEY UIFontSymbolicTrait
#define FONT_TRAITS_ATTR UIFontDescriptorTraitsAttribute
#define FONT_DESCRIPTOR_TRAIT_BOLD UIFontDescriptorTraitBold
#define FONT_DESCRIPTOR_TRAIT_ITALIC UIFontDescriptorTraitItalic
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
		if(record.font.getFace () == font.getFace () &&
		   record.font.getSize () == font.getSize () &&
		   ((font.getStyleName ().isEmpty () && record.font.getStyleName ().isEmpty ())
				? getUsedStyle (record.font) == getUsedStyle (font) // ignore underline, etc.
				: record.font.getStyleName () == font.getStyleName ()))
		{
			ascent = record.ascent;
			descent = record.descent;
			leading = record.leading;
			return record.fontRef;
		}
	}

	StringRef fontFamilyName = font.getFace ();
	StringRef fontStyleName = font.getStyleName ();

	FONT_DESCRIPTOR_CLASS* fontDescriptor = nil;
	if(fontFamilyName.equals (CCLSTR ("System Font")))
		fontDescriptor = [[FONT_CLASS systemFontOfSize:font.getSize ()] fontDescriptor];
	else
	{
	   NSMutableDictionary<NSString *,id>* attributes = [[[NSMutableDictionary alloc] init] autorelease];
	   NSString* familyName = [fontFamilyName.createNativeString<NSString*> () autorelease];
	   [attributes setObject:familyName forKey:FONT_FAMILY_ATTR];

	   if(!fontStyleName.isEmpty ())
		   [attributes setObject:[fontStyleName.createNativeString<NSString*> () autorelease] forKey:FONT_FACE_ATTR];

	   fontDescriptor = [FONT_DESCRIPTOR_CLASS fontDescriptorWithFontAttributes:attributes];
   }

	FONT_TRAITS_CLASS symbolicTraits = 0;
	if(font.getStyle () & Font::kBold) symbolicTraits |= FONT_DESCRIPTOR_TRAIT_BOLD;
	if(font.getStyle () & Font::kItalic) symbolicTraits |= FONT_DESCRIPTOR_TRAIT_ITALIC;
	if(symbolicTraits)
		fontDescriptor = [fontDescriptor fontDescriptorWithSymbolicTraits:symbolicTraits];

	CCL_PRINTF ("creating font: %s - %s %f %d\n", MutableCString (fontFamilyName).str () , MutableCString (fontStyleName).str (), font.getSize (), font.getMode ())
	CTFontRef fontRef = (CTFontRef)[FONT_CLASS fontWithDescriptor:fontDescriptor size:font.getSize ()];
	if(fontRef == NULL)
	{
		NSString* fullName = nil;
		String fontName (fontFamilyName);
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
			if(fullName == nil)
				fullName = [fontName.createNativeString<NSString*> () autorelease];
		}
		else
		{
			fontName << " " << fontStyleName;
			fullName = [fontName.createNativeString<NSString*> () autorelease];
		}

		fontDescriptor = [FONT_DESCRIPTOR_CLASS fontDescriptorWithFontAttributes:@{ FONT_NAME_ATTR : fullName }];
		fontRef = (CTFontRef)[FONT_CLASS fontWithDescriptor:fontDescriptor size:font.getSize ()];
		if(fontRef)
		{
			NSString* name = [(NSString*)CTFontCopyFamilyName (fontRef) autorelease];
			NSString* familyName = [fontName.createNativeString<NSString*> () autorelease];
			if(![familyName hasPrefix:name])
				fontRef = NULL;
		}
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
