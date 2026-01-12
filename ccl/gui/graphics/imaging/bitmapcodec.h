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
// Filename    : ccl/gui/graphics/imaging/bitmapcodec.h
// Description : Bitmap Codec
//
//************************************************************************************************

#ifndef _ccl_bitmapcodec_h
#define _ccl_bitmapcodec_h

#include "ccl/public/base/istream.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/collections/vector.h"

#include "ccl/base/singleton.h"

namespace CCL {

interface IAttributeList;
interface IBitmapEncoder;
interface IBitmapDecoder;

//************************************************************************************************
// IBitmapCodec
// LATER TODO: Make interface public and allow external bitmap codecs
//************************************************************************************************

interface IBitmapCodec: IUnknown
{
	virtual const FileType& CCL_API getFileType () const = 0;
	
	virtual IBitmapDecoder* CCL_API createBitmapDecoder (IMemoryStream& stream) = 0;

	virtual IBitmapEncoder* CCL_API createBitmapEncoder (IStream& stream) = 0;

	DECLARE_IID (IBitmapCodec)
};

//************************************************************************************************
// IBitmapDecoder
/**	Bitmap Decoder.
*
* Notes:
* 
* - Consumers must call 'getPixelSize()' exactly once and use the returned pixel size to allocate
*   a pixel buffer to be passed in the 'data' argument to 'getPixelData()'.
* - Consumers must release the interface instance after calling 'getPixelData()'. */
//************************************************************************************************

interface IBitmapDecoder: IUnknown
{
	virtual tresult CCL_API getPixelSize (Point& size) = 0;

	virtual tresult CCL_API getPixelData (BitmapData& data) = 0;

	DECLARE_IID (IBitmapDecoder)
};

//************************************************************************************************
// IBitmapEncoder
//************************************************************************************************

interface IBitmapEncoder: IUnknown
{
	virtual tresult CCL_API setEncoderOptions (const IAttributeList& options) = 0;

	virtual tresult CCL_API encodePixelData (const BitmapData& data) = 0;

	DECLARE_IID (IBitmapEncoder)
};

//************************************************************************************************
// BitmapCodec
//************************************************************************************************

class BitmapCodec: public Object,
				   public IBitmapCodec
{
public:
	DECLARE_CLASS_ABSTRACT (BitmapCodec, Object)
	CLASS_INTERFACE (IBitmapCodec, Object)
};

//************************************************************************************************
// BitmapDecoder
//************************************************************************************************

class BitmapDecoder: public Object,
					 public IBitmapDecoder
{
public:
	DECLARE_CLASS_ABSTRACT (BitmapDecoder, Object)

	BitmapDecoder (IMemoryStream& stream);
	
	CLASS_INTERFACE (IBitmapDecoder, Object)

protected:
	SharedPtr<IMemoryStream> stream;
};

//************************************************************************************************
// BitmapEncoder
//************************************************************************************************

class BitmapEncoder: public Object,
					 public IBitmapEncoder
{
public:
	DECLARE_CLASS_ABSTRACT (BitmapEncoder, Object)

	BitmapEncoder (IStream& stream);
	
	CLASS_INTERFACE (IBitmapEncoder, Object)

protected:
	SharedPtr<IStream> stream;
};

//************************************************************************************************
// CustomBitmapCodecs
//************************************************************************************************

class CustomBitmapCodecs: public Object,
						  public Singleton<CustomBitmapCodecs>
{
public:
	~CustomBitmapCodecs ();

	void addCodec (IBitmapCodec* bitmapCodec);
	void collectFileTypes (Vector<const FileType*>& fileTypes) const;
	IBitmapCodec* findCodec (const FileType& fileType) const;

	bool encodeBitmap (IStream& stream, IBitmap& bitmap, const FileType& fileType,
					   const IAttributeList* options = nullptr);

protected:
	Vector<IBitmapCodec*> codecs;
};

} // namespace CCL

#endif // _ccl_bitmapcodec_h
