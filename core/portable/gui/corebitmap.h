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
// Filename    : core/portable/gui/corebitmap.h
// Description : Bitmap class
//
//************************************************************************************************

#ifndef _corebitmap_h
#define _corebitmap_h

#include "core/public/corestream.h"
#include "core/public/corehashmap.h"
#include "core/public/coreintrusivelist.h"
#include "core/public/corestringbuffer.h"
#include "core/public/coremacros.h"
#include "core/public/corebuffer.h"
#include "core/public/coreobserver.h"
#include "core/public/gui/corebitmapdata.h"
#include "core/gui/corebitmapprimitives.h"
#include "core/gui/coreskinformat.h"

#include "core/portable/coreattributes.h"
#include "core/portable/coresingleton.h"

namespace Core {
namespace Portable {

class FilePackage;

//************************************************************************************************
// ResourceAttributes
//************************************************************************************************

namespace ResourceAttributes
{
	using namespace Skin::ResourceAttributes;

	INLINE uint32 nameToInt (CStringPtr string) { return CStringFunctions::hashDJB (string); }
	int hashIntKey (const uint32& key, int size);

	CStringPtr makeHiResFileName (CString256& result, CStringPtr fileName);

	void decodeSize (Rect& size, const AttributeValue& a);
	int64 packRect (RectRef r);
	void unpackRect (Rect& r, int64 value);

	Rect getSize (const Attributes& a, CStringPtr name);
	Rect getSize (const Attributes& a);
}

//************************************************************************************************
// DpiSetting
/** \ingroup core_gui */
//************************************************************************************************

class DpiSetting: public StaticSingleton<DpiSetting>
{
public:
	DpiSetting ()
	: scaleFactor (1.f)
	{}

	PROPERTY_VARIABLE (float, scaleFactor, ScaleFactor)

	bool isHighResolution () const { return scaleFactor > 1.f; }
	
	void scaleCoord (Coord& c) const;
	void scaleRect (Rect& r) const;

protected:
	int scale (int value) const;
};

//************************************************************************************************
// Bitmap
/** \ingroup core_gui */
//************************************************************************************************

class Bitmap
{
public:
	enum Options { kUninitialized = 1<<0, kTopDown = 1<<1, kDoubleWordAligned = 1<<2  };

	/** [LIGHT] Default constructor, no allocation. */
	Bitmap ();
	/** [LIGHT] Wrap bitmap data, no allocation. */
	Bitmap (const BitmapData& data);
	/** [HEAVY] Allocates bitmap on heap. */
	Bitmap (int width, int height, BitmapPixelFormat format = kBitmapRGBAlpha, int options = 0);
	/** [LIGHT] Does not allocate or copy any data, works for BMP format only. */
	Bitmap (const uint8* bitmapFileData, uint32 bitmapFileLength);
	virtual ~Bitmap ();

	void construct (int width, int height, BitmapPixelFormat format, int options, 
					const uint8* externalBuffer = nullptr, uint32 externalBufferSize = 0);

	static Bitmap* loadPNGImage (IO::Stream& stream, BitmapPixelFormat requestedFormat = kBitmapAny, bool explicitFormat = true);
	static Bitmap* loadBMPImage (IO::Stream& stream);
	static bool saveBMPImage (IO::Stream& stream, const Bitmap& bitmap);
	static uint32 getTotalBitmapMemory ();

	PROPERTY_BOOL (alphaChannelUsed, AlphaChannelUsed)
	PROPERTY_VARIABLE (int, frameCount, FrameCount)

	int getWidth () const { return data.width; }
	int getHeight () const { return data.height; }
	Rect& getSize (Rect& size) const { return size (0, 0, data.width, data.height); }
	Rect getSize () const { return Rect (0, 0, data.width, data.height); }
	Rect& getFrame (Rect& frameRect, int frameIndex = 0) const;

	BitmapPixelFormat getFormat () const { return (BitmapPixelFormat)data.format; } 

	BitmapData& accessForWrite () { return data; }
	const BitmapData& accessForRead () const { return data; }

	bool copyFrom (const Bitmap& bitmap);
	bool copyFrom (const Bitmap& bitmap, RectRef rect);

	void* getBufferAddress () { return pixelBuffer.getAddressAligned (); }
	const void* getBufferAddress () const { return pixelBuffer.getAddressAligned (); }
	uint32 getBufferSize () const { return pixelBuffer.getSize (); }

	// use count maintained by BitmapReference
	void use () { useCount++; }
	void unuse () { useCount--; ASSERT (useCount >= 0) }
	bool isReferenced () const { return useCount > 0; }
	
protected:
	static const int kAlignment = 8;
	static uint32 totalBitmapMemory;

	BitmapData data;
	IO::Buffer pixelBuffer;
	int useCount;
};

//************************************************************************************************
// BitmapFileFormat
//************************************************************************************************

namespace BitmapFileFormat
{
	enum Format
	{
		kUnknown = -1,
		kPNG,
		kBMP
	};

	extern Format detectFormat (CStringPtr fileName);
	extern CStringPtr getExtension (Format format);
	extern uint16 getCustomBitmapHeaderType ();
}

//************************************************************************************************
// BitmapReference
/** \ingroup core_gui */
//************************************************************************************************

class BitmapReference
{
public:
	explicit BitmapReference (Bitmap* _bitmap = nullptr)
	: bitmap (nullptr)
	{ assign (_bitmap); }
	
	BitmapReference (const BitmapReference& other)
	: bitmap (nullptr)
	{ assign (other.bitmap); }
	
	~BitmapReference ()
	{ assign (nullptr); }
	
	BitmapReference& assign (Bitmap* _bitmap)
	{
		if(bitmap != _bitmap)
		{
			if(bitmap)
				bitmap->unuse ();
			bitmap = _bitmap;
			if(bitmap)
				bitmap->use ();			
		}
		return *this;		
	}
		
	BitmapReference& operator = (Bitmap* bitmap) { return assign (bitmap); }
	BitmapReference& operator = (const BitmapReference& other) { return assign (other.bitmap); }

	bool isValid () const { return bitmap != nullptr; }
	Bitmap* getBitmap () const { return bitmap; }

protected:
	Bitmap* bitmap;
};

//************************************************************************************************
// BitmapManager
/** \ingroup core_gui */
//************************************************************************************************

struct BitmapManagerObserver
{
	virtual void onDelayLoadingBitmap (CStringPtr filename) = 0;
		
	virtual void onBitmapLoaded (CStringPtr filename) = 0;
};

//************************************************************************************************
// BitmapManager
/** \ingroup core_gui */
//************************************************************************************************

class BitmapManager: public StaticSingleton<BitmapManager>
{
public:
	BitmapManager ();
	~BitmapManager ();

	/** Set default format for color bitmaps. */
	PROPERTY_VARIABLE (BitmapPixelFormat, defaultFormat, DefaultFormat)	

	/** Set bitmap memory limit for caching (in bytes, default is 0 - no limit). */
	PROPERTY_VARIABLE (uint32, memoryLimit, MemoryLimit)

	/** Load bitmaps from package defined in 'bitmaps.json/.ubj' file. */
	int loadBitmaps (FilePackage& package, bool delayDecoding = false);
	
	/** Get bitmap by name. */
	BitmapReference getBitmap (CStringPtr name);

	/** Give idle time for delayed bitmap decoding (optional). */
	void idle ();
	
	DEFINE_OBSERVER (BitmapManagerObserver)

protected:
	class BitmapDescriptor: public IntrusiveLink<BitmapDescriptor>
	{
	public:
		BitmapDescriptor ()
		: package (nullptr),
		  monochrome (false),
		  frameCount (1),
		  alwaysCached (false),
		  bitmap (nullptr)
		{}

		~BitmapDescriptor ()
		{
			delete bitmap;
		}

		PROPERTY_POINTER (FilePackage, package, Package)
		PROPERTY_CSTRING_BUFFER (128, fileName, FileName)
		PROPERTY_BOOL (monochrome, Monochrome)
		PROPERTY_VARIABLE (int, frameCount, FrameCount)
		PROPERTY_BOOL (alwaysCached, AlwaysCached)

		PROPERTY_POINTER (Bitmap, bitmap, Bitmap)		
		void unload ()
		{
			delete bitmap;
			bitmap = nullptr;
		}
	};

	struct InternalStore
	{
		IntrusiveLinkedList<BitmapDescriptor> descriptors;
		HashMap<uint32, BitmapDescriptor*> hashMap;
		Vector<BitmapDescriptor*> toDecode;

		InternalStore ();
		~InternalStore ();
	};

	InternalStore* store;
	InternalStore& getStore ();

	void addBitmap (CStringPtr name, BitmapDescriptor* descriptor);
	bool loadBitmap (BitmapDescriptor& descriptor);
	bool isAboveMemoryLimit () const;
	void reduceBitmaps ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int DpiSetting::scale (int value) const
{
	return (int)((float)value * scaleFactor);
}

inline void DpiSetting::scaleCoord (Coord& c) const
{
	if(scaleFactor > 1.f)
		c = scale (c);
}

inline void DpiSetting::scaleRect (Rect& r) const
{
	if(scaleFactor > 1.f)
	{
		r.left = scale (r.left);
		r.top = scale (r.top);
		r.right = scale (r.right);
		r.bottom = scale (r.bottom);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ResourceAttributes::decodeSize (Rect& size, const AttributeValue& a)
{
	if(a.getType () == AttributeValue::kInt)
		unpackRect (size, a.getInt ());
	else
		parseSize (size, a.getString ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline int64 ResourceAttributes::packRect (RectRef r)
{
	int64 v;
	int16* a = (int16*)&v;
	a[0] = (int16)r.left;
	a[1] = (int16)r.top;
	a[2] = (int16)r.right;
	a[3] = (int16)r.bottom;
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ResourceAttributes::unpackRect (Rect& r, int64 value)
{
	int16* a = (int16*)&value;
	r.left = a[0];
	r.top = a[1];
	r.right = a[2];
	r.bottom = a[3];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _corebitmap_h
