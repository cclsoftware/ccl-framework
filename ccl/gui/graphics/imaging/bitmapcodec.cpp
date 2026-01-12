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
// Filename    : ccl/gui/graphics/imaging/bitmapcodec.cpp
// Description : Bitmap Codec
//
//************************************************************************************************

#include "ccl/gui/graphics/imaging/bitmapcodec.h"

using namespace CCL;

//************************************************************************************************
// IBitmapCodec / IBitmapDecoder
//************************************************************************************************

DEFINE_IID_ (IBitmapCodec, 0x6277d241, 0x35d3, 0x4b71, 0x9b, 0x7, 0x23, 0x5e, 0xd0, 0x44, 0x8d, 0xc5)
DEFINE_IID_ (IBitmapDecoder, 0x74163fd, 0x104, 0x4419, 0xa8, 0xcf, 0x8a, 0x3d, 0x7a, 0xe2, 0x88, 0x21)
DEFINE_IID_ (IBitmapEncoder, 0x7d1baa18, 0x556f, 0x4270, 0x8e, 0xbc, 0x5a, 0x14, 0xce, 0x58, 0xc5, 0xd5)

//************************************************************************************************
// BitmapCodec
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BitmapCodec, Object)

//************************************************************************************************
// BitmapDecoder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BitmapDecoder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapDecoder::BitmapDecoder (IMemoryStream& _stream)
: stream (&_stream)
{}

//************************************************************************************************
// BitmapEncoder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BitmapEncoder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapEncoder::BitmapEncoder (IStream& _stream)
: stream (&_stream)
{}
	
//************************************************************************************************
// CustomBitmapCodecs
//************************************************************************************************

DEFINE_SINGLETON (CustomBitmapCodecs)

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomBitmapCodecs::~CustomBitmapCodecs ()
{
	for(auto* codec : codecs)
		codec->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomBitmapCodecs::addCodec (IBitmapCodec* bitmapCodec)
{
	codecs.add (bitmapCodec);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomBitmapCodecs::collectFileTypes (Vector<const FileType*>& fileTypes) const
{
	for(auto* codec : codecs)
		fileTypes.add (&codec->getFileType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapCodec* CustomBitmapCodecs::findCodec (const FileType& fileType) const
{
	auto result = codecs.findIf ([fileType] (IBitmapCodec* codec) 
		{
			return codec->getFileType () == fileType;
		});
	return result ? *result : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomBitmapCodecs::encodeBitmap (IStream& stream, IBitmap& bitmap, const FileType& fileType,
										  const IAttributeList* options)
{
	bool result = false;
	if(IBitmapCodec* codec = findCodec (fileType))
	{
		if(AutoPtr<IBitmapEncoder> encoder = codec->createBitmapEncoder (stream))
		{
			if(options)
				encoder->setEncoderOptions (*options);
			
			BitmapDataLocker locker (&bitmap, IBitmap::kAny, IBitmap::kLockRead);
			if(locker.result == kResultOk)
				result = encoder->encodePixelData (locker.data) == kResultOk;
		}		
	}
	return result;
}
