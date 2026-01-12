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
// Filename    : ccl/gui/graphics/imaging/imagecache.h
// Description : Image Cache singleton class
//
//************************************************************************************************

#ifndef _ccl_imagecache_h
#define _ccl_imagecache_h

#include "ccl/base/object.h"
#include "ccl/base/singleton.h"

#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/iimagecache.h"

#include "core/public/corevector.h"

namespace CCL {

class BitmapFilter;
class Shape;
	
//************************************************************************************************
// ImageCache
//************************************************************************************************

class ImageCache: public Object,
				  public Singleton<ImageCache>
{
public:
	DECLARE_CLASS (ImageCache, Object)

	ImageCache ();
	~ImageCache ();

	IImage* requestImage (IImage* source, Coord width, Coord height);
	void releaseImage (IImage* cached);

protected:
	struct CacheEntry 
	{
		bool operator == (const CacheEntry& other) { return source == other.source && width == other.width && height == other.height; }

		IImage* source;
		IImage* cached;
		Coord width;
		Coord height;
		int useCount;
	};

	LinkedList<CacheEntry> cache;
};

//************************************************************************************************
// CachedImage
//************************************************************************************************

class CachedImage
{
public:
	CachedImage ();
	~CachedImage ();

	IImage* operator = (IImage* image);
	operator IImage* ();
	bool operator == (IImage* image);
	operator bool ();
	IImage* operator -> ();

	void update (Coord width, Coord height);

private:
	SharedPtr<IImage> source;
	IImage* cached;
};

//************************************************************************************************
// ModifiedImageCache
//************************************************************************************************

class ModifiedImageCache: public Object,
						  public IImageCache,
						  public Singleton<ModifiedImageCache>
{
public:
	DECLARE_CLASS (ModifiedImageCache, Object)
	
	ModifiedImageCache ();
	~ModifiedImageCache ();
	
	static IImage* createModifiedImage (IImage* source, BitmapFilter* ownedFilter);

	// IImageCache
	IImage* CCL_API lookup (IImage* image, ColorRef color, tbool drawAsTemplate = false) override;

	CLASS_INTERFACE (IImageCache, Object)

private:
	void applyShapeModificationDeep (ColorRef color, Shape* shape, bool colorizeTemplate);
	
	struct CachedModification
	{
		CachedModification () : image (nullptr) {}
		CachedModification (IImage* image, Color color) : image (image), color (color) {}
		IImage* image;
		Color color;
	};
	
	struct CacheEntry
	{
		static const int kMaxCachedModifications = 6;
		typedef Core::FixedSizeVector<CachedModification, kMaxCachedModifications> CachedModification;
		
		CacheEntry () {}
		CacheEntry (IImage* source) : source (source) {}
		SharedPtr<IImage> source;
		CachedModification modifications;
	};

	LinkedList<CacheEntry> cache;
		
	IImage* addModification (CacheEntry& entry, ColorRef color, bool colorizeTemplate);
	IImage* createModifiedShape (IImage* source, ColorRef color, bool colorizeTemplate);
	

	void removeAll ();
};

} // namespace CCL

#endif // _ccl_imagecache_h
