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
// Filename    : ccl/platform/cocoa/quartz/fontcache.h
// Description : ATSUI font style cache
//
//************************************************************************************************

#ifndef _ccl_fontcache_h
#define _ccl_fontcache_h

#include "ccl/base/singleton.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/gui/graphics/types.h"

#include <CoreText/CoreText.h>
#include <CoreFoundation/CoreFoundation.h>

namespace CCL {

//************************************************************************************************
// FontCache
//************************************************************************************************

class FontCache: public Object,
				 public Singleton<FontCache>
{
public:
	FontCache ();
	~FontCache ();
	
	CTFontRef createFont (FontRef font, float& ascent, float& descent, float& leading);
	void removeAll ();
	void addStyledFont (StringRef familyName, int fontStyle, StringRef fullName);
	void addUserFont (StringRef familyName);
	bool isUserFont (StringRef familyName) const;
	CTFontRef getStyledFont (FontRef font, int style);
	const LinkedList<String>& getUserFonts () const { return userFontList; }
protected:
	struct FontCacheRecord
	{
		FontCacheRecord (FontRef _font, CTFontRef _fontRef, float _ascent, float _descent, float _leading)
		: font (_font), 
		  fontRef (_fontRef),
		  ascent (_ascent),
		  descent (_descent),
		  leading (_leading)
		{
			CFRetain (fontRef);
		}
		
		FontCacheRecord () 
		: fontRef (0) 
		{
		}
		
		FontCacheRecord (const FontCacheRecord& other)
		: font (other.font),
		  fontRef (other.fontRef),
		  ascent (other.ascent),
		  descent (other.descent),
		  leading (other.leading)
		{
			if(fontRef)
				CFRetain (fontRef);
		}
		
		FontCacheRecord& operator = (const FontCacheRecord& other)
		{
			font = other.font;
			ascent = other.ascent;
			descent = other.descent;
			leading = other.leading;
			fontRef = other.fontRef;
			if(fontRef)
				CFRetain (fontRef);
			return *this;
		}
		
		~FontCacheRecord () 
		{
			if(fontRef)
				CFRelease (fontRef);
		}
		
		Font font;
		CTFontRef fontRef;
		float ascent, descent, leading;
	};
	
	struct StyledFont
	{
		StyledFont (StringRef _familyName, int _fontStyle, StringRef _fullName)
		: familyName (_familyName),
		fontStyle (_fontStyle),
		fullName (_fullName)
		{}
		
		StyledFont ()
		: familyName (""),
		fontStyle (0),
		fullName ("")
		{}
		
		String familyName;
		int fontStyle;
		String fullName;
	};
	
	LinkedList<FontCacheRecord> cache;
	LinkedList<StyledFont> styledFontList;
	LinkedList<String> userFontList;
};

} // namespace CCL

#endif // _ccl_fontcache_h
