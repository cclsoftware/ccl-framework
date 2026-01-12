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
// Filename    : ccl/gui/graphics/imaging/codecs/pngcodec.cpp
// Description : PNG Bitmap Codec
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/codecs/pngcodec.h"

#include "core/gui/corepnghandler.h"

namespace CCL {

//************************************************************************************************
// PNGBitmapDecoder
//************************************************************************************************

class PNGBitmapDecoder: public BitmapDecoder
{
public:
	PNGBitmapDecoder (IMemoryStream& stream);

	// BitmapDecoder
	tresult CCL_API getPixelSize (Point& size) override;
	tresult CCL_API getPixelData (BitmapData& data) override;

private:
	enum State
	{
		kInitialized,
		kSizeRequested,
		kDataRequested
	};

	State state;
	CoreStream streamAdapter;
	Core::PNGHandler pngHandler;
};

namespace FileTypes
{
	extern FileType png;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// PNGBitmapCodec
//************************************************************************************************

const FileType& CCL_API PNGBitmapCodec::getFileType () const
{
	return FileTypes::png;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapDecoder* CCL_API PNGBitmapCodec::createBitmapDecoder (IMemoryStream& stream)
{
	return NEW PNGBitmapDecoder (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapEncoder* CCL_API PNGBitmapCodec::createBitmapEncoder (IStream& stream)
{
	// not implemented
	return nullptr;
}

//************************************************************************************************
// PNGBitmapDecoder
//************************************************************************************************

PNGBitmapDecoder::PNGBitmapDecoder (IMemoryStream& stream)
: BitmapDecoder (stream),
  state (kInitialized),
  streamAdapter (stream),
  pngHandler (streamAdapter)
{
	pngHandler.construct ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PNGBitmapDecoder::getPixelSize (Point& size)
{
	ASSERT (state == kInitialized)
	if(state != kInitialized)
		return kResultUnexpected;

	bool hasAlpha = false;
	if(!pngHandler.readInfo (size.x, size.y, hasAlpha))
		return kResultFailed;
	
	state = kSizeRequested;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PNGBitmapDecoder::getPixelData (BitmapData& data)
{
	ASSERT (state == kSizeRequested)
	if(state != kSizeRequested)
		return kResultUnexpected;

	if(!pngHandler.readBitmapData (data))
		return kResultFailed;

	state = kDataRequested;
	return kResultOk;
}
