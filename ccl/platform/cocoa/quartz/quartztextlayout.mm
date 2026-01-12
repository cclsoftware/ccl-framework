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
// Filename    : ccl/platform/cocoa/quartz/quartztextlayout.mm
// Description : Core Text based Text Layout
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/quartz/quartztextlayout.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/quartz/fontcache.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/macutils.h"

#if CCL_PLATFORM_IOS
#define COLORTYPE UIColor
#else
#define COLORTYPE NSColor
#endif

#define ATTRIBUTED_STRING (NSMutableAttributedString*)string

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// ColorFormat
//************************************************************************************************

class ColorFormat: public Object
{
public:
	ColorFormat (QuartzTextLayout::Range _range, Color _color)
	: range (_range),
	  color (_color)
	{}
	
	QuartzTextLayout::Range range;
	Color color;
};

//************************************************************************************************
// QuartzContext
//************************************************************************************************

class QuartzContext: public Object
{
public:
	QuartzContext (CGContextRef _context = NULL)
	: context (_context)
	{
		if(!context)
		{
			CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB ();
			context = CGBitmapContextCreate (NULL, 1, 1, 8, 0, colorSpace, kCGImageAlphaPremultipliedLast);
			CGContextSetShouldAntialias (context, true);
			CFRelease (colorSpace);
		}
		else
			CFRetain (context);
	}
	
	~QuartzContext ()
	{
		if(context)
			CFRelease (context);
	}
	
	const CGContextRef getContext () { return context; }
	
protected:
	CGContextRef context;
};

//************************************************************************************************
// QuartzTextLayout
//************************************************************************************************

static const CoordF kPaddingLeft = 2.f;
static const CoordF kPaddingRight = 2.f;
static const CoordF kPaddingTop = 2.f;
static const CoordF kPaddingBottom = 2.f;

AutoPtr<QuartzContext> hiddenContext = NEW QuartzContext ();

DEFINE_CLASS_HIDDEN (QuartzTextLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzTextLayout::QuartzTextLayout ()
: string (nullptr),
  boundingRect (CGSizeMake (0, 0)),
  totalHeight (0.0),
  ascent (0),
  descent (0),
  leading (0),
  framesetter (NULL),
  line (NULL),
  wordBreak (false)
{
	colorRegions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzTextLayout::~QuartzTextLayout ()
{
	if(string)
		[ATTRIBUTED_STRING release];

	if(line)
		CFRelease (line);
	
	resetFrameSetter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzTextLayout::resetFrameSetter ()
{
	if(framesetter)
		CFRelease (framesetter);
	framesetter = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzTextLayout::resetLine ()
{
	if(line)
		CFRelease (line);
	line = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format)
{
	return construct (text, (CoordF)width, (CoordF)height, font, lineMode, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::construct (StringRef text, CoordF width, CoordF height, FontRef _font, LineMode _lineMode, TextFormatRef format)
{
	this->text = text;
	font = _font;
	NSString* nsString = [text.createNativeString<NSString*> () autorelease];
	QuartzTextLayout::Range fullRange (0, text.length ());
	
	CTFontRef ctFont = FontCache::instance ().createFont (font, ascent, descent, leading);
	ASSERT(ctFont)
	NSMutableParagraphStyle* nsFormat = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];

	alignment = format.getAlignment ();
	wordBreak = format.isWordBreak ();
	lineMode = _lineMode;

	nsString = [NSString stringWithFormat: @"%@\n", nsString]; // framesetter expects that every line is terminated with a newline

	switch(alignment.getAlignH ())
	{
		case Alignment::kHCenter :
			[nsFormat setAlignment:NSTextAlignmentCenter];
			break;
		case Alignment::kRight :
			[nsFormat setAlignment:NSTextAlignmentRight];
			break;
		default : // left aligned
			[nsFormat setAlignment:NSTextAlignmentLeft];
	}
	
	NSLineBreakMode lineBreak = NSLineBreakByClipping;
	if(wordBreak)
		lineBreak = NSLineBreakByWordWrapping;
	[nsFormat setLineBreakMode:lineBreak];
	[nsFormat setLineSpacing:1.];
	[nsFormat setLineHeightMultiple:font.getLineSpacing ()];
	
	NSMutableDictionary* attributes = [[[NSMutableDictionary alloc] init] autorelease];
	[attributes setValue:(id)ctFont forKey:NSFontAttributeName];
	[attributes setValue:nsFormat forKey:NSParagraphStyleAttributeName];
	[attributes setValue:(id)kCFBooleanTrue forKey:(id)kCTForegroundColorFromContextAttributeName];

	if(string)
		[ATTRIBUTED_STRING release];	
	string = [[NSMutableAttributedString alloc] initWithString:nsString attributes:attributes];
	if(font.getStyle () & Font::kUnderline)
		setFontStyle (fullRange, Font::kUnderline, true);
	if(font.getStyle () & Font::kStrikeout)
		setFontStyle (fullRange, Font::kStrikeout, true);

	float spacing = font.getSpacing ();
	if(spacing != 0)
		setSpacing (fullRange, spacing);
		
	resetFrameSetter ();
	resetLine ();

	if(width == kMaxCoord)
		width = 0.f;
	if(height == kMaxCoord)
		height = 0.f;

	boundingRect = CGSizeMake (width, height);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::resize (Coord width, Coord height)
{
	return resize (CoordF (width), CoordF (height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::resize (CoordF width, CoordF height)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;

	boundingRect.width = width;
	boundingRect.height = height;

	resetFrameSetter ();
	resetLine ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setFontStyle (const Range& range, int style, tbool state)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;
	
	NSRange currentRange = NSMakeRange (range.start, range.length);	
	
	int baseStyle = font.getStyle ();
	switch(style)
	{
	case Font::kBold:
		if((baseStyle & Font::kBold) != state)
		{
			CTFontRef newFont = FontCache::instance ().getStyledFont (font, baseStyle ^ Font::kBold);
			if(newFont)
				[ATTRIBUTED_STRING addAttribute:NSFontAttributeName value:(id)newFont range:currentRange];
			else
			{
				CCL_WARN ("CoreText: font %s :", MutableCString (font.getFace ()).str ())
				CCL_WARN ("style %d not available!\n", style)
			}
		}
		break;
	case Font::kItalic:
		if((baseStyle & Font::kItalic) != state)
		{
			CTFontRef newFont = FontCache::instance ().getStyledFont (font, baseStyle ^ Font::kItalic);
			if(newFont)
				[ATTRIBUTED_STRING addAttribute:NSFontAttributeName value:(id)newFont range:currentRange];
			else
			{
				CCL_WARN ("CoreText: font %s :", MutableCString (font.getFace ()).str ())
				CCL_WARN ("style %d not available!\n", style)
			}
		}
		break;
	case Font::kUnderline:
		if(state)
			[ATTRIBUTED_STRING addAttribute:NSUnderlineStyleAttributeName value:[NSNumber numberWithInt:NSUnderlineStyleSingle] range:currentRange];
		else
			[ATTRIBUTED_STRING removeAttribute:NSUnderlineStyleAttributeName range:currentRange];
		break;
	case Font::kStrikeout:
		if(state)
			// not a copy and paste typo, underline is used for strikethrough, too
			[ATTRIBUTED_STRING addAttribute:NSStrikethroughStyleAttributeName value:[NSNumber numberWithInt:NSUnderlineStyleSingle] range:currentRange];
		else
			[ATTRIBUTED_STRING removeAttribute:NSStrikethroughStyleAttributeName range:currentRange];
		break;
	}
	
	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setFontSize (const Range& range, float size)
{
	if(!string)
		return kResultUnexpected;

	NSRange currentRange = NSMakeRange (range.start, range.length);
	CTFontRef ctFont = (CTFontRef)[ATTRIBUTED_STRING attribute:NSFontAttributeName atIndex:currentRange.location effectiveRange:NULL];
	CTFontRef newFont = CTFontCreateCopyWithSymbolicTraits (ctFont, size, NULL, 0, 0);
	ASSERT (newFont)
	[ATTRIBUTED_STRING addAttribute:NSFontAttributeName value:(id)newFont range:currentRange];
	CFRelease (newFont);
	
	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setSpacing (const Range& range, float spacing)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;
	
	[ATTRIBUTED_STRING addAttribute:NSKernAttributeName value:[NSNumber numberWithFloat:spacing] range: NSMakeRange (range.start, range.length)];

	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setLineSpacing (const Range& range, float lineSpacing)
{
	if(!string)
		return kResultUnexpected;

	NSRange currentRange = NSMakeRange (range.start, range.length);
	NSMutableParagraphStyle* paragraphStyle = [[(NSParagraphStyle*)[ATTRIBUTED_STRING attribute:NSParagraphStyleAttributeName atIndex:currentRange.location effectiveRange:nullptr] mutableCopy] autorelease];
	if(paragraphStyle == nil)
		paragraphStyle = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
	[paragraphStyle setLineHeightMultiple:lineSpacing];
	[ATTRIBUTED_STRING addAttribute:NSParagraphStyleAttributeName value:(id)paragraphStyle range:currentRange];
	
	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setTextColor (const Range& range, Color color)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;
	
	colorRegions.add (NEW ColorFormat (range, color));

	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setBaselineOffset (const Range& range, float offset)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;

	[ATTRIBUTED_STRING addAttribute:NSBaselineOffsetAttributeName value:[NSNumber numberWithFloat:offset] range: NSMakeRange (range.start, range.length)];

	resetFrameSetter ();
	resetLine ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setSuperscript (const Range& range)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;

	return setSuperscript (range, kSuperscriptSizeFactor, kSuperscriptBaselineFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::setSubscript (const Range& range)
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;

	return setSuperscript (range, kSubscriptSizeFactor, -kSubscriptBaselineFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult QuartzTextLayout::setSuperscript (const Range& _range, float sizeFactor, float baselineFactor)
{
	auto setStyle = [&] (const Range& range, float fontSize, float baselineOffset)
	{
		setFontSize (range, fontSize * sizeFactor);
		setBaselineOffset (range, baselineOffset + baselineFactor * fontSize);
	};

	auto getMetics = [] (float& fontSize, float& baselineOffset, CFDictionaryRef attributes)
	{
		CTFontRef font = (CTFontRef)CFDictionaryGetValue (attributes, NSFontAttributeName);
		fontSize = CTFontGetSize (font);
		if(NSNumber* offset = (NSNumber*)CFDictionaryGetValue (attributes, NSBaselineOffsetAttributeName))
			baselineOffset = offset.floatValue;
	};

	Range range (_range);

	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		NSArray* runs = (NSArray*)CTLineGetGlyphRuns (line);
		for(id runObj in runs)
		{
			CTRunRef run = (CTRunRef)runObj;
			CFRange currentRange = CTRunGetStringRange (run);

			int currentRangeFrom = int(currentRange.location);
			int currentRangeTo = int(currentRange.location + currentRange.length);
			if(range.start < currentRangeTo && range.start + range.length > currentRangeFrom)
			{
				int overlapStart = ccl_max (range.start, currentRangeFrom);
				int overlapEnd = ccl_min (range.start + range.length, currentRangeTo);
				Range overlapRange (overlapStart, overlapEnd - overlapStart);
				if(CFDictionaryRef attributes = CTRunGetAttributes (run))
				{
					float fontSize = 0.f;
					float baselineOffset = 0.f;
					getMetics (fontSize, baselineOffset, attributes);

					setStyle (overlapRange, fontSize, baselineOffset);

					if(overlapStart > range.start)
					{
						setSuperscript (Range (range.start, overlapStart - range.start), sizeFactor, baselineFactor);
						range.length -= overlapStart - range.start;
						range.start = overlapStart;
					}

					if(overlapEnd < range.start + range.length)
					{
						range.length = overlapEnd - (range.start + range.length);
						range.start = overlapEnd;
					}

					if(overlapStart == range.start && overlapEnd == range.start + range.length)
						return kResultOk;
				}
			}
		}
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getBounds (Rect& bounds, int flags) const
{
	RectF boundsF;
	if(getBounds (boundsF, flags) != kResultOk)
		return kResultUnexpected;

	bounds.left = 0;
	bounds.top = 0;
	bounds.setWidth ((Coord) ceil (boundsF.getWidth ()));
	bounds.setHeight ((Coord) ceil (boundsF.getHeight ()));
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getBounds (RectF& bounds, int flags) const
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;

	CGSize actualSize = getActualSize (!(flags & kNoMargin));
	CGPoint textPos = getTextPosition (PointF ());
	bounds.left = (CoordF)textPos.x - kPaddingLeft;
	bounds.top = (CoordF)(boundingRect.height + descent - (textPos.y + actualSize.height - kPaddingTop));
	bounds.setWidth ((CoordF)actualSize.width);
	bounds.setHeight ((CoordF)actualSize.height);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getImageBounds (RectF& bounds) const
{
	ASSERT (string != nullptr)
	if(!string)
		return kResultUnexpected;
	
	CGPoint textPos = getTextPosition (PointF (0, 0));
	CGContextSetTextPosition (hiddenContext->getContext (), textPos.x, textPos.y);
	
	if(!line)
		line = CTLineCreateWithAttributedString ((CFAttributedStringRef) string);
	if(!line)
		return kResultFailed;

	CGRect imageBounds = CTLineGetImageBounds (line, hiddenContext->getContext ());

	bounds.left = (CoordF)imageBounds.origin.x;
	bounds.top = (CoordF)(boundingRect.height - 0.5f - (imageBounds.origin.y + imageBounds.size.height));
	bounds.setWidth ((CoordF)imageBounds.size.width);
	bounds.setHeight ((CoordF)imageBounds.size.height);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getBaselineOffset (PointF& offset) const
{
	offset (kPaddingLeft, kPaddingTop + ascent - 0.5f);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzTextLayout::initTotalHeight () const
{
	if(lineMode == kSingleLine || boundingRect.width == 0)
	{
		totalHeight = boundingRect.height - 0.5;
		return;
	}
	else if(totalHeight > 0.0)
		return;

	totalHeight = 0.0;

	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	CFIndex lineIndex = 0;
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		CGRect bounds = CTLineGetBoundsWithOptions (line, 0);
		CGPoint lineOrigin = CGPointMake (0, 0);
		CTFrameGetLineOrigins (frame, CFRangeMake (lineIndex, 1), &lineOrigin);
		CGFloat lineTop = lineOrigin.y + bounds.origin.y + bounds.size.height;
		totalHeight = ccl_max (totalHeight, lineTop);
		lineIndex++;
	}

	totalHeight += kPaddingTop;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::hitTest (int& textIndex, PointF& position) const
{
	initTotalHeight ();

	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	CGPathRef path = CTFrameGetPath (frame);
	CGRect pathBounds = CGPathGetBoundingBox (path);
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	CFIndex lineIndex = 0;
	float distance = 0.f;
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		CFRange range = CTLineGetStringRange (line);
		CGRect bounds = CTLineGetBoundsWithOptions (line, 0);
		CGPoint lineOrigin = CGPointMake (0, 0);
		CTFrameGetLineOrigins (frame, CFRangeMake (lineIndex, 1), &lineOrigin);
		bounds.origin.x = pathBounds.origin.x + lineOrigin.x;
		bounds.origin.y += lineOrigin.y;
		bounds.origin.x += kPaddingLeft;
		if(lineMode == kMultiLine)
			bounds.origin.y -= kPaddingTop;
		else
			bounds.origin.y -= kPaddingTop + kPaddingBottom;

		bounds.origin.y = totalHeight - bounds.origin.y;

		CGPoint pos = CGPointMake (position.x, position.y);
		float currentDistance = ccl_abs (bounds.origin.y + descent - ((ascent + descent) / 2.f) - pos.y);
		if(lineIndex > 0 && currentDistance > distance)
			break;

		distance = currentDistance;
		pos.x -= bounds.origin.x;
		pos.y -= bounds.origin.y - bounds.size.height;
		pos.x = ccl_min (pos.x, bounds.size.width);
		if(range.length == 1 && bounds.size.width == 0)
			textIndex = int(range.location);
		else
			textIndex = int(CTLineGetStringIndexForPosition (line, pos));

		lineIndex++;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getCharacterBounds (RectF& characterBounds, int textIndex) const
{
	initTotalHeight ();

	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	CGPathRef path = CTFrameGetPath (frame);
	CGRect pathBounds = CGPathGetBoundingBox (path);
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	CFIndex lineIndex = 0;
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		CFRange range = CTLineGetStringRange (line);
		if(range.location + range.length >= textIndex)
		{
			CGRect bounds = CTLineGetBoundsWithOptions (line, kCTLineBoundsUseOpticalBounds);
			CGPoint lineOrigin = CGPointMake (0, 0);
			CTFrameGetLineOrigins (frame, CFRangeMake (lineIndex, 1), &lineOrigin);
			bounds.origin.x += lineOrigin.x;
			bounds.origin.y += lineOrigin.y;
			bounds.origin.x += kPaddingLeft;
			if(lineMode == kMultiLine)
				bounds.origin.y -= kPaddingTop;
			else
				bounds.origin.y -= kPaddingTop + kPaddingBottom;

			bounds.origin.y = totalHeight - bounds.origin.y;

			characterBounds.top = bounds.origin.y - bounds.size.height;
			characterBounds.bottom = bounds.origin.y;
			characterBounds.left = bounds.origin.x + CTLineGetOffsetForStringIndex (line, textIndex, NULL);
			characterBounds.right = bounds.origin.x + CTLineGetOffsetForStringIndex (line, textIndex + 1, NULL);
			characterBounds.right = ccl_max (characterBounds.right, characterBounds.left + 1);
		}

		if(range.location + range.length > textIndex)
			break;

		lineIndex++;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getTextBounds (IMutableRegion& result, const Range& range) const
{
	initTotalHeight ();

	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	CFIndex lineIndex = 0;
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		CFRange lineRange = CTLineGetStringRange (line);
		if(lineRange.location < range.start + range.length && lineRange.location + lineRange.length > range.start)
		{
			CGRect bounds = CTLineGetBoundsWithOptions (line, kCTLineBoundsUseOpticalBounds);
			CGPoint lineOrigin = CGPointMake (0, 0);
			CTFrameGetLineOrigins (frame, CFRangeMake (lineIndex, 1), &lineOrigin);
			bounds.origin.x += lineOrigin.x;
			bounds.origin.y += lineOrigin.y;
			bounds.origin.x += kPaddingLeft;
			if(lineMode == kMultiLine)
				bounds.origin.y -= kPaddingTop;
			else
				bounds.origin.y -= kPaddingTop + kPaddingBottom;

			bounds.origin.y = totalHeight - bounds.origin.y;

			Rect lineBounds;
			lineBounds.top = bounds.origin.y - bounds.size.height;
			lineBounds.bottom = bounds.origin.y;

			CFIndex start = ccl_max (lineRange.location, CFIndex(range.start));
			CFIndex end = ccl_min (lineRange.location + lineRange.length, CFIndex(range.start + range.length));

			lineBounds.left = bounds.origin.x + CTLineGetOffsetForStringIndex (line, start, NULL);
			lineBounds.right = bounds.origin.x + CTLineGetOffsetForStringIndex (line, end, NULL);
			lineBounds.right = ccl_max (lineBounds.right, lineBounds.left + 1);

			result.addRect (lineBounds);
		}

		lineIndex++;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuartzTextLayout::getLineRange (Range& range, int textIndex) const
{
	CFObj<CTFrameRef> frame = createFrame (PointF (0, 0));
	NSArray* lines = (NSArray*)CTFrameGetLines (frame);
	for(id lineObj in lines)
	{
		CTLineRef line = (CTLineRef)lineObj;
		CFRange cfRange = CTLineGetStringRange (line);
		if(cfRange.location + cfRange.length > textIndex)
		{
			range.start = int(cfRange.location);
			range.length = int(cfRange.length);
			return kResultOk;
		}
	}

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzTextLayout::draw (CGContextRef context, PointF position, Color textColor)
{
	ASSERT (string != nullptr)
	if(!string)
		return;

	initTotalHeight ();
	CGFloat offset = totalHeight;
	if(lineMode == kMultiLine)
		offset += kPaddingTop;

	CGContextTranslateCTM (context, 0, offset);
	CGContextScaleCTM (context, 1.0, -1.0);

	NSRange fullRange = NSMakeRange (0, [ATTRIBUTED_STRING length]);
	
	if(!colorRegions.isEmpty ())
	{
		COLORTYPE* color = [COLORTYPE colorWithRed:textColor.getRedF () green:textColor.getGreenF () blue:textColor.getBlueF () alpha:textColor.getAlphaF ()];
		[ATTRIBUTED_STRING addAttribute:NSForegroundColorAttributeName value:color range:fullRange];
		CGContextSetStrokeColorWithColor (context, color.CGColor);
		CGContextSetFillColorWithColor (context, color.CGColor);
		ForEach(colorRegions, ColorFormat, region)
			COLORTYPE* color = [COLORTYPE colorWithRed:region->color.getRedF () green:region->color.getGreenF () blue:region->color.getBlueF () alpha:region->color.getAlphaF ()];
			[ATTRIBUTED_STRING addAttribute:NSForegroundColorAttributeName value:color range:NSMakeRange (region->range.start, region->range.length)];
		EndFor
		[ATTRIBUTED_STRING addAttribute:(id)kCTForegroundColorFromContextAttributeName value:(id)kCFBooleanFalse range:fullRange];
	}
	else
	{
		CGColorRef cgColor = [COLORTYPE colorWithRed:textColor.getRedF () green:textColor.getGreenF () blue:textColor.getBlueF () alpha:textColor.getAlphaF ()].CGColor;
		[ATTRIBUTED_STRING addAttribute:(id)kCTForegroundColorFromContextAttributeName value:(id)kCFBooleanTrue range:fullRange];
		CGContextSetStrokeColorWithColor (context, cgColor);
		CGContextSetFillColorWithColor (context, cgColor);
	}

	if(lineMode == kSingleLine)
	{
		if(!line)
			line = CTLineCreateWithAttributedString ((CFAttributedStringRef) string);
		if(line)
		{
			if(boundingRect.height == 0)
			{
				CGContextSetTextPosition (context, position.x + kPaddingLeft, -position.y - ascent - kPaddingTop);
				CTLineDraw (line, context);
			}
			else
			{
				CGContextSaveGState (context);

				CGPoint textPos = getTextPosition (position);
				CGContextSetTextPosition (context, textPos.x, textPos.y);
				CTLineDraw (line, context);

				CGContextRestoreGState (context);
			}
		}
	}
	else
	{
		CGContextSaveGState (context);

		CFObj<CTFrameRef> frame = createFrame (position);
		CTFrameDraw (frame, context);

		CGContextRestoreGState (context);
	}
	CGContextScaleCTM (context, 1.0, -1.0);
	CGContextTranslateCTM (context, 0, -offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CTFrameRef QuartzTextLayout::createFrame (const PointF& position) const
{
	CGSize actualSize = getActualSize (true);
	CGRect frameRect;
	frameRect.origin.x = position.x + kPaddingLeft;
	frameRect.origin.y = -position.y + kPaddingBottom;
	switch(alignment.getAlignV ())
	{
		case Alignment::kVCenter :
			frameRect.origin.y -= (boundingRect.height - actualSize.height) / 2 + ascent * 0.05; // ad hoc correction to align with single line mode
			break;
		case Alignment::kTop :
			break;
		default : // bottom aligned
			frameRect.origin.y -= (boundingRect.height - actualSize.height);
			break;
	}

	ASSERT (boundingRect.width > 0)
	frameRect.size.width = boundingRect.width - kPaddingLeft - kPaddingRight;
	frameRect.size.height = boundingRect.height - kPaddingTop - kPaddingBottom;

	CFObj<CGPathRef> cgPath = CGPathCreateWithRect (frameRect, NULL);
	if(!framesetter)
		framesetter = CTFramesetterCreateWithAttributedString ((CFAttributedStringRef) string);

	return CTFramesetterCreateFrame (framesetter, CFRangeMake (0, 0), cgPath, NULL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGPoint QuartzTextLayout::getTextPosition (const PointF& position) const
{
	CGPoint textPos = CGPointMake (0, 0);
	CGSize actualSize = getActualSize (true);
	switch(alignment.getAlignH ())
	{
		case Alignment::kHCenter :
			textPos.x = position.x + (boundingRect.width - actualSize.width + kPaddingLeft + kPaddingRight) / 2;
			break;
		case Alignment::kRight :
			textPos.x = position.x + (boundingRect.width - actualSize.width) + kPaddingRight;
			break;
		default : // left aligned
			textPos.x = position.x + kPaddingLeft;
	}

	switch(alignment.getAlignV ())
	{
		case Alignment::kVCenter :
			textPos.y = - position.y + (boundingRect.height - ascent) / 2 + ascent * 0.1f; // 10% correction needed to look right
			break;
		case Alignment::kTop :
			textPos.y = - position.y + (boundingRect.height - ascent) - kPaddingTop;
			break;
		default : // bottom aligned
			textPos.y = - position.y + descent + kPaddingBottom;
			break;
	}
	
	return textPos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGSize QuartzTextLayout::getActualSize (bool applyMargin) const
{
	if(!string)
		return CGSizeMake (0, 0);
	
	if(!framesetter)
		framesetter = CTFramesetterCreateWithAttributedString ((CFAttributedStringRef) string);
	
	CGFloat width = 0;
	if(wordBreak)
		width = boundingRect.width - kPaddingLeft - kPaddingRight;
	else
		width = CGFLOAT_MAX;
	
	CGSize size = CTFramesetterSuggestFrameSizeWithConstraints (framesetter, CFRangeMake (0, 0), NULL, CGSizeMake (width, CGFLOAT_MAX), NULL);

	if(applyMargin)
	{
		size.height += kPaddingTop + kPaddingBottom;
		size.width += kPaddingLeft + kPaddingRight;
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API QuartzTextLayout::getText () const
{
	return text;
}
