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
// Filename    : ccl/platform/android/graphics/paintcache.h
// Description : Cache of Java Paint Objects
//
//************************************************************************************************

#ifndef _ccl_android_paintcache_h
#define _ccl_android_paintcache_h

#include "ccl/public/collections/intrusivelist.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/brush.h"
#include "ccl/public/gui/graphics/pen.h"
#include "ccl/public/gui/graphics/font.h"

#include "core/platform/shared/jni/corejnienvironment.h"

#define PAINT_CACHE_STATISTIC 0

#if PAINT_CACHE_STATISTIC
#include "ccl/public/base/debug.h"
#include "ccl/public/systemservices.h"
#endif

namespace CCL {
namespace Android {

class FrameworkGraphicsFactory;

//************************************************************************************************
// PaintCache
//************************************************************************************************

template<class Data>
class PaintCache
{
public:
	PaintCache (FrameworkGraphicsFactory* graphicsFactory, int maxSize, const char* name = 0);
	~PaintCache ();

	jobject getPaint (const Data& data); // returns Paint to be passed to FrameworkGraphics java draw calls

protected:
	class Item: public IntrusiveLink<Item>
	{
	public:
		Item (int javaIndex)
		: javaIndex (javaIndex)
		{}

		int javaIndex;
		JniObject javaObject;
		Data data;
	};

	FrameworkGraphicsFactory* graphicsFactory;
	IntrusiveLinkedList<Item> items;
	int maxSize;
	int total;
	const char* name;

	#if PAINT_CACHE_STATISTIC
	int numReused;
	int numConfigured;
	int64 lastLog;
	#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Data>
PaintCache<Data>::PaintCache (FrameworkGraphicsFactory* graphicsFactory, int maxSize, const char* name)
: graphicsFactory (graphicsFactory),
  maxSize (maxSize),
  name (name),
#if PAINT_CACHE_STATISTIC
  numReused (0),
  numConfigured (0),
  lastLog (0),
#endif
  total (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Data>
PaintCache<Data>::~PaintCache ()
{
	IntrusiveListForEach (items, Item, item)
		delete item;
	EndFor

	items.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Data>
jobject PaintCache<Data>::getPaint (const Data& data)
{
	// the java side has a simple array of Paint objects, which never change their index
	// our items are managed as a queue (recently used first)
	IntrusiveListForEach (items, Item, item)
		if(item->data == data)
		{
			#if PAINT_CACHE_STATISTIC
			numReused++;
			#endif
			items.prepend (items.removeLast ()); // move item to front
			return item->javaObject;
		}
	EndFor

	Item* item = 0;
	if(total >= maxSize)
		item = items.removeLast (); // reuse oldest item
	else
	{
		item = NEW Item (total);
		total++;
	}

	#if PAINT_CACHE_STATISTIC
	numConfigured++;
	int64 now = System::GetSystemTicks ();
	if(now - lastLog >= 5000)
	{
		int reconfigured = numConfigured - maxSize;
		int totalUsed = numReused + numConfigured;
		float reuseRatio = float(numReused) / totalUsed;
		Debugger::printf ("PaintCache (%s) reused %.1f of %d (%d reconfigured)\n", name, 100.f * reuseRatio, totalUsed, ccl_max (reconfigured, 0));
		lastLog = now;
	}
	#endif

	JniAccessor jni;
	item->data = data;
	item->javaObject.assign (jni, Data::createJavaPaint (jni, item->javaIndex, data));
	items.prepend (item);

	return item->javaObject;
}

//************************************************************************************************
// BitmapPaintData
//************************************************************************************************

struct BitmapPaintData
{
	int alpha;
	bool filtered;

	BitmapPaintData (int alpha = 255, bool filtered = false)
	: alpha (alpha), filtered (filtered)
	{}

	BitmapPaintData (const ImageMode* mode)
	{
		if(mode)
		{
			alpha = mode->getAlphaF () * 255;
			filtered = mode->getInterpolationMode () != ImageMode::kInterpolationPixelQuality;
		}
		else
		{
			alpha = 255;
			filtered = true;
		}
	}

	bool operator== (const BitmapPaintData& data)
	{
		return data.alpha == alpha
			&& data.filtered == filtered;
	}

	static jobject createJavaPaint (const JniAccessor& jni, int javaIndex, const BitmapPaintData& data);
};

//************************************************************************************************
// FillPaintData
//************************************************************************************************

struct FillPaintData
{
	int color;
	bool antiAlias;

	FillPaintData (int color = 0, bool antiAlias = false)
	: color (color), antiAlias (antiAlias)
	{}

	FillPaintData (SolidBrushRef brush, bool antiAlias = false);

	bool operator== (const FillPaintData& data)
	{
		return data.color == color
			&& data.antiAlias == antiAlias;
	}

	static jobject createJavaPaint (const JniAccessor& jni, int javaIndex, const FillPaintData& data);
};

//************************************************************************************************
// DrawPaintData
//************************************************************************************************

struct DrawPaintData: public FillPaintData
{
	float width;
	int penStyle;

	DrawPaintData (int color = 0, float width = 1.f, int penStyle = 0, bool antiAlias = false)
	: FillPaintData (color, antiAlias),
	  width (width),
	  penStyle (penStyle)
	{}

	DrawPaintData (PenRef pen, bool antiAlias);

	bool operator== (const DrawPaintData& data)
	{
		return data.color == color
			&& data.width == width
			&& data.penStyle == penStyle
			&& data.antiAlias == antiAlias;
	}

	static jobject createJavaPaint (const JniAccessor& jni, int javaIndex, const DrawPaintData& data);
};

//************************************************************************************************
// TextPaintData
//************************************************************************************************

struct TextPaintData
{
	jobject typeface;
	int style;
	float fontSize;
	float spacing;
	int color;

	TextPaintData (jobject typeface = 0, int style = 0, float fontSize = 0, float spacing = 0, int color = 0)
	: typeface (typeface), style (style), fontSize (fontSize), spacing (spacing), color (color)
	{}

	TextPaintData (FontRef font, SolidBrushRef brush);
	TextPaintData (FontRef font);

	bool operator== (const TextPaintData& data)
	{
		return data.typeface == typeface
			&& data.style == style
			&& data.fontSize == fontSize
			&& data.spacing == spacing
			&& data.color == color;
	}

	static jobject createJavaPaint (const JniAccessor& jni, int javaIndex, const TextPaintData& data);
};

} // namespace Android
} // namespace CCL


#endif // _ccl_android_paintcache_h
