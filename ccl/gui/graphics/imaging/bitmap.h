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
// Filename    : ccl/gui/graphics/imaging/bitmap.h
// Description : Bitmap class
//
//************************************************************************************************

#ifndef _ccl_bitmap_h
#define _ccl_bitmap_h

#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/public/gui/graphics/ibitmap.h"

namespace CCL {

interface IStream;
class NativeBitmap;
class NativeGraphicsDevice;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap File Types
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace FileTypes
{
	extern FileType bmp;
	extern FileType png;
	extern FileType jpg;
	extern FileType gif;
}

//************************************************************************************************
// Bitmap
/** Bitmap class. */
//************************************************************************************************

class Bitmap: public Image,
			  public IBitmap
{
public:
	DECLARE_CLASS (Bitmap, Image)

	/** Construct bitmap of given size and pixel format. */
	Bitmap (int width, int height, PixelFormat format = kRGB, float contentScaleFactor = 1.f);

	/** Construct from native bitmap (takes ownership). */
	Bitmap (NativeBitmap* nativeBitmap);

	/** Bitmap destructor. */
	~Bitmap ();

	/** Get associated native bitmap. */
	NativeBitmap* getNativeBitmap ();

	/** Save bitmap to stream with given format. */
	virtual bool saveToStream (IStream& stream, const FileType& format, const IAttributeList* encoderOptions = nullptr) const;

	// IBitmap
	Point CCL_API getPixelSize () const override;
	PixelFormat CCL_API getPixelFormat () const override;
	float CCL_API getContentScaleFactor () const override;
	tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;

	// Image
	ImageType CCL_API getType () const override;
	int CCL_API getFrameCount () const override;
	int CCL_API getCurrentFrame () const override;
	void CCL_API setCurrentFrame (int frameIndex) override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;

	/** Try to get original bitmap portion the given image represents. */
	static Bitmap* getOriginalBitmap (Rect& originalRect, Image* image, bool deep = false);

	enum ResolutionNamingMode
	{
		kStandardResolution,	///< 1x scaling (standard)
		kMultiResolution,		///< 1x/2x scaling (standard and high), desktop platforms
		kHighResolution,		///< 2x scaling (high)
		kExtraHighResolution	///< 3x scaling (extra high), mobile plaforms only
	};

	// Enable/disable support for high resolution naming convention
	static void setResolutionNamingMode (ResolutionNamingMode mode);
	static float getDefaultContentScaleFactor ();
	static bool isHighResolutionFile (UrlRef path);
	static inline bool isHighResolutionScaling (float factor) { return factor >= 1.25f; }
	static inline ResolutionNamingMode chooseResolutionMode (float factor) { return factor >= 2.25f ? kExtraHighResolution : (factor >= 1.25f ? kHighResolution : kStandardResolution); }

	CLASS_INTERFACE (IBitmap, Image)

protected:
	NativeBitmap* nativeBitmap;

	Bitmap ();
	Bitmap (const Bitmap&);

	// Constructor for bitmap handler
	friend class BitmapHandler;
	Bitmap (IStream& stream, const FileType& format); ///< load bitmap from stream
	
	void assign (NativeBitmap* nativeBitmap);
	bool isValid () const;
};

//************************************************************************************************
// MultiResolutionBitmap
/** Bitmap with multiple resolutions (currently limited to 1x and 2x scaling). */
//************************************************************************************************

class MultiResolutionBitmap: public Bitmap,
							 public IMultiResolutionBitmap
{
public:
	DECLARE_CLASS_ABSTRACT (MultiResolutionBitmap, Bitmap)

	MultiResolutionBitmap (int width, int height, PixelFormat format = kRGB);
	MultiResolutionBitmap (NativeBitmap* bitmap1x, NativeBitmap* bitmap2x);
	~MultiResolutionBitmap ();

	/** Get associated high resolution native bitmap. */
	NativeBitmap* getNativeBitmap2x ();

	// IMultiResolutionBitmap
	int CCL_API getRepresentationCount () const override;
	void CCL_API setCurrentRepresentation (int index) override;
	int CCL_API getCurrentRepresentation () const override;

	// Bitmap
	bool saveToStream (IStream& stream, const FileType& format, const IAttributeList* encoderOptions = nullptr) const override;
	tresult CCL_API lockBits (BitmapLockData& data, PixelFormat format, int mode) override;
	tresult CCL_API unlockBits (BitmapLockData& data) override;
	tresult CCL_API scrollPixelRect (const Rect& rect, const Point& delta) override;
	Point CCL_API getPixelSize () const override;
	PixelFormat CCL_API getPixelFormat () const override;
	float CCL_API getContentScaleFactor () const override;
	tresult draw (GraphicsDevice& graphics, PointRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, PointFRef pos, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectRef src, RectRef dst, const ImageMode* mode = nullptr) override;
	tresult draw (GraphicsDevice& graphics, RectFRef src, RectFRef dst, const ImageMode* mode = nullptr) override;
	tresult tile (GraphicsDevice& graphics, int method, RectRef src, RectRef dest, RectRef clip, RectRef margins) override;

	CLASS_INTERFACE (IMultiResolutionBitmap, Bitmap)

protected:
	NativeBitmap* nativeBitmap2;
	int currentRepresentation;

	NativeBitmap* getCurrentBitmap () const;
	NativeBitmap* selectBitmap (NativeGraphicsDevice* graphics);
};

} // namespace CCL

#endif // _ccl_bitmap_h
