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
// Filename    : ccl/gui/graphics/imaging/codecs/webpcodec.cpp
// Description : WebP Bitmap Codec (https://developers.google.com/speed/webp)
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/codecs/webpcodec.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/text/translation.h"

#include "libwebp/src/webp/decode.h"
#include "libwebp/src/webp/encode.h"

namespace CCL {

//************************************************************************************************
// WebPBitmapDecoder
//************************************************************************************************

class WebPBitmapDecoder: public BitmapDecoder
{
public:
	WebPBitmapDecoder (IMemoryStream& stream);

	// BitmapDecoder
	tresult CCL_API getPixelSize (Point& size) override;
	tresult CCL_API getPixelData (BitmapData& data) override;
};

//************************************************************************************************
// WebPBitmapEncoder
//************************************************************************************************

class WebPBitmapEncoder: public BitmapEncoder
{
public:
	WebPBitmapEncoder (IStream& stream);

	// BitmapEncoder
	tresult CCL_API setEncoderOptions (const IAttributeList& options) override;
	tresult CCL_API encodePixelData (const BitmapData& data) override;

private:
	Attributes options;

	static int writerFunction (const uint8_t* data, size_t size, const WebPPicture* picture);
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (WebPFile, "WebP File")
END_XSTRINGS

//************************************************************************************************
// WebPBitmapCodec
//************************************************************************************************

const FileType& CCL_API WebPBitmapCodec::getFileType () const
{
	static FileType fileType (nullptr, "webp", "image/webp");
	return FileTypes::init (fileType, XSTR (WebPFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapDecoder* CCL_API WebPBitmapCodec::createBitmapDecoder (IMemoryStream& stream)
{
	return NEW WebPBitmapDecoder (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapEncoder* CCL_API WebPBitmapCodec::createBitmapEncoder (IStream& stream)
{
	return NEW WebPBitmapEncoder (stream);
}

//************************************************************************************************
// WebPBitmapDecoder
//************************************************************************************************

WebPBitmapDecoder::WebPBitmapDecoder (IMemoryStream& stream)
: BitmapDecoder (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebPBitmapDecoder::getPixelSize (Point& size)
{	
	const uint8_t* webpData = static_cast<uint8_t*> (stream->getMemoryAddress ());
	size_t dataSize = stream->getBytesWritten ();
	if(!WebPGetInfo (webpData, dataSize, &size.x, &size.y))
		return kResultFailed;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebPBitmapDecoder::getPixelData (BitmapData& data)
{
	WebPDecoderConfig config {};
	if(!WebPInitDecoderConfig (&config))
		return kResultFailed;

	#if (CORE_BITMAP_PLATFORM_FORMAT == CORE_BITMAP_FORMAT_RGBA)
	config.output.colorspace = MODE_rgbA;
	#else
	config.output.colorspace = MODE_bgrA;
	#endif

	config.output.height = data.height;
	config.output.is_external_memory = 1;
	config.output.u.RGBA.rgba = reinterpret_cast<uint8_t*> (data.scan0);
	config.output.u.RGBA.size = abs(data.rowBytes) * data.height;
	config.output.u.RGBA.stride = data.rowBytes;
	config.output.width = data.width;

	const uint8_t* webpData = static_cast<uint8_t*> (stream->getMemoryAddress ());
	size_t dataSize = stream->getBytesWritten ();	
	if(WebPDecode (webpData, dataSize, &config) != VP8_STATUS_OK)
		return kResultFailed;

	return kResultOk;
}

//************************************************************************************************
// WebPBitmapEncoder
//************************************************************************************************

WebPBitmapEncoder::WebPBitmapEncoder (IStream& stream)
: BitmapEncoder (stream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebPBitmapEncoder::setEncoderOptions (const IAttributeList& _options)
{
	options.copyFrom (_options);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebPBitmapEncoder::encodePixelData (const BitmapData& data)
{
	ASSERT (data.bitsPerPixel == 32)
	if(data.bitsPerPixel != 32)
		return kResultInvalidArgument;

	// set up encoder configuration
	WebPConfig config {};
	if(!WebPConfigInit (&config))
		return kResultFailed;

	config.lossless = false;
	config.quality = 85.f;

	Variant value;
	if(options.getAttribute (value, ImageEncoding::kLossless))
		config.lossless = value.parseBool ();

	if(options.getAttribute (value, ImageEncoding::kQuality))
		config.quality = value.parseInt ();

	if(!WebPValidateConfig (&config))
		return kResultFailed;

	// set up picture data object
	WebPPicture picture {};
	if(!WebPPictureInit (&picture))
		return kResultFailed;

	picture.width = data.width;
	picture.height = data.height;
	picture.use_argb = true;

	if(!WebPPictureImportBGRA (&picture, reinterpret_cast<uint8_t*> (data.scan0), data.rowBytes))
		return kResultFailed;

	picture.writer = &writerFunction;
	picture.custom_ptr = stream.as_plain ();

	// encode and free picture object afterwards
	int result = WebPEncode (&config, &picture);

	WebPPictureFree (&picture);

	if(!result)
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WebPBitmapEncoder::writerFunction (const uint8_t* data, size_t size, const WebPPicture* picture)
{
	IStream* stream = static_cast<IStream*> (picture->custom_ptr);
	if(stream->write (data, int (size)) != size)
		return false;

	return true;
}
