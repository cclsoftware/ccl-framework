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
// Filename    : ccl/public/gui/graphics/ibitmap.h
// Description : Bitmap Interface
//
//************************************************************************************************

#ifndef _ccl_ibitmap_h
#define _ccl_ibitmap_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

#include "core/public/gui/corebitmapdata.h"

namespace CCL {

using Core::BitmapData;
using Core::RGBA;
using Core::Pixel;

struct BitmapLockData;

//************************************************************************************************
// IBitmap
/** Bitmap interface
    Don't implement this interface yourself. Use framework implementations instead.
	\ingroup gui_graphics */
//************************************************************************************************

interface IBitmap: IUnknown
{
	DEFINE_ENUM (PixelFormat)
	{
		kAny = Core::kBitmapAny,
		kRGB = Core::kBitmapRGB,
		kRGBAlpha = Core::kBitmapRGBAlpha
	};

	enum LockModes
	{
		kLockRead, //< lock for read-only access
		kLockWrite //< lock for read and write access
	};

	virtual Point CCL_API getPixelSize () const = 0;

	virtual PixelFormat CCL_API getPixelFormat () const = 0;

	virtual float CCL_API getContentScaleFactor () const = 0;

	virtual tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) = 0;

	virtual tresult CCL_API unlockBits (BitmapLockData& data) = 0;
	
	/** Move area \a rect by offset \a delta. All parameters are interpreted as pixel coordinates. */
	virtual tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) = 0;

	DECLARE_IID (IBitmap)
};

DEFINE_IID (IBitmap, 0x58212a64, 0xe3dd, 0x40b7, 0x85, 0x81, 0x39, 0x59, 0xbf, 0x3d, 0x58, 0xe6)

//************************************************************************************************
// IMultiResolutionBitmap
/** Additional bitmap interface for multiple resolutions (1x/2x scaling).
	\ingroup gui_graphics */
//************************************************************************************************

interface IMultiResolutionBitmap: IUnknown
{
	/** Get number of representations. */
	virtual int CCL_API getRepresentationCount () const = 0;

	/** Set current representation for IBitmap::lockBits(), IBitmap::getPixelSize(), etc. */
	virtual void CCL_API setCurrentRepresentation (int index) = 0;

	/** Get currently selected representation. */
	virtual int CCL_API getCurrentRepresentation () const = 0;

	/** Helper to select current representation. */
	struct RepSelector
	{
		IMultiResolutionBitmap* bitmap;
		int oldIndex;
		
		RepSelector (IMultiResolutionBitmap* bitmap, int index)
		: bitmap (bitmap),
		  oldIndex (bitmap ? bitmap->getCurrentRepresentation () : 0)
		{ if(bitmap) bitmap->setCurrentRepresentation (index); }
		
		~RepSelector ()
		{ if(bitmap) bitmap->setCurrentRepresentation (oldIndex); }
	};

	DECLARE_IID (IMultiResolutionBitmap)
};

DEFINE_IID (IMultiResolutionBitmap, 0xba98496e, 0x7e2a, 0x4120, 0x98, 0xf7, 0xf9, 0x82, 0x74, 0x72, 0x9b, 0x4d)

//************************************************************************************************
// BitmapLockData
/** Data of a locked bitmap \see IBitmap::lockBits
	\ingroup gui_graphics */
//************************************************************************************************

struct BitmapLockData: BitmapData
{
	int mode;				///< mode flags passed to IBitmap::lockBits()
	void* nativeData;		///< reserved for internal use

	BitmapLockData ()
	: mode (0),
	  nativeData (nullptr)
	{}
};

//************************************************************************************************
// BitmapDataLocker
/**
	\ingroup gui_graphics */
//************************************************************************************************

struct BitmapDataLocker
{
	BitmapDataLocker (IBitmap* bitmap, IBitmap::PixelFormat format, int mode)
	: bitmap (bitmap),
	  result (kResultFalse)
	{
		if(bitmap)
			result = bitmap->lockBits (data, format, mode);
	}

	~BitmapDataLocker ()
	{
		if(bitmap && result == kResultOk)
			bitmap->unlockBits (data);
	}

	IBitmap* bitmap;
	BitmapLockData data;
	tresult result;
};

}; // namespace CCL

#endif // _ccl_ibitmap_h
