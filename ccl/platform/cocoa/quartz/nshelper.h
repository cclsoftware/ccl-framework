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
// Filename    : ccl/platform/cocoa/quartz/nshelper.h
// Description : Quartz Helper
//
//************************************************************************************************

#ifndef _ccl_nshelper_h
#define _ccl_nshelper_h

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/platform/cocoa/interfaces/iquartzbitmap.h"

#include "ccl/platform/cocoa/cclcocoa.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// Bitmap helper
//************************************************************************************************

inline NSImage* nsImageFromBitmap (Bitmap& bitmap)
{
	NativeBitmap* nativeBitmap = bitmap.getNativeBitmap ();
	if(!nativeBitmap)
		return 0;
		
	UnknownPtr<IQuartzBitmap> quartzBitmap (bitmap.getNativeBitmap ()->asUnknown ());
	if(!quartzBitmap)
		return 0;
		
	NSSize size = NSMakeSize (bitmap.getWidth (), bitmap.getHeight ());
	NSImage* nsImage = [[NSImage alloc] initWithSize:size];
	CGImageRef cgImage = quartzBitmap->getCGImage ();
	if(!cgImage)
		return 0;
	
	NSImage* nsImage1x = [[NSImage alloc] initWithCGImage:quartzBitmap->getCGImage () size:size];
	NSData* tiffRep1x = [nsImage1x TIFFRepresentation];
	[nsImage addRepresentation:[NSBitmapImageRep imageRepWithData:tiffRep1x]];
	[nsImage1x release];
	
	if(MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (&bitmap))
		if(NativeBitmap* bitmap2x = multiBitmap->getNativeBitmap2x ())
		{
			UnknownPtr<IQuartzBitmap> nativeBitmap2x (bitmap2x->asUnknown ());
			if(nativeBitmap2x)
			{
				NSImage* nsImage2x = [[NSImage alloc] initWithCGImage:nativeBitmap2x->getCGImage () size:size];
				NSData* tiffRep2x = [nsImage2x TIFFRepresentation];
				[nsImage addRepresentation:[NSBitmapImageRep imageRepWithData:tiffRep2x]];
				[nsImage2x release];
			}
		}
	
	return nsImage;
}

//************************************************************************************************
// Rect helper
//************************************************************************************************

inline CCL::Rect& fromNSRect (CCL::Rect& dst, const ::NSRect& src)
{
	dst.left = (Coord)src.origin.x;
	dst.top = (Coord)src.origin.y;
	dst.right = (Coord)(src.origin.x + src.size.width);
	dst.bottom = (Coord)(src.origin.y + src.size.height);
	return dst;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ::NSRect& toNSRect (::NSRect& dst, const CCL::Rect& src)
{
	dst.origin.x = src.left;
	dst.origin.y = src.top;
	dst.size.width = src.right - src.left;
	dst.size.height = src.bottom - src.top;
	return dst;
}
    
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool rectIntersects (CCL::RectRef cclRect, const ::NSRect& nsRect)
{
    if(cclRect.left > nsRect.origin.x + nsRect.size.width)
        return false;
    if(cclRect.right < nsRect.origin.x)
        return false;
    if(cclRect.bottom < nsRect.origin.y)
        return false;
    if(cclRect.top > nsRect.origin.y + nsRect.size.height)
        return false;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int inline screenHeight ()
{
	return (int)([[[NSScreen screens] objectAtIndex:0] frame].size.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
int inline flipCoord (int y)
{
	return screenHeight () - y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int inline flipCoord (float y)
{
	return screenHeight () - (Coord)y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int inline flipCoord (double y)
{
	return screenHeight () - (Coord)y;
}

//************************************************************************************************
// NSClipRegion
//************************************************************************************************

class NSClipRegion: public Unknown, 
					public IUpdateRegion
{
public:
    NSClipRegion (NSRect& dirtyRect, const NSRect* rects, NSInteger count)
    : dirtyRect (dirtyRect),
      rects (rects),
      count (count)
    {}
    
    // IUpdateRegion
    tbool CCL_API rectVisible (RectRef rect) const override
    {
        if(rectIntersects (rect, dirtyRect) == false)
            return false;
        
        for(int i = 0; i < count; i++)
        {
            if(rectIntersects (rect, rects[i]))
                return true;
        }
        return false;
    }

	CCL::Rect CCL_API getBoundingBox () const override
	{
		CCL::Rect bounds;
		return fromNSRect (bounds, dirtyRect);
	}

    CLASS_INTERFACE (IUpdateRegion, Unknown)
    
protected:
    NSRect& dirtyRect;
    const NSRect* rects;
    NSInteger count;
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_nshelper_h
