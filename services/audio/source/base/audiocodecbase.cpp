//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audiocodecbase.cpp
// Description : Audio Codec Base Classes
//
//************************************************************************************************

#include "audiocodecbase.h"

using namespace CCL;

//************************************************************************************************
// AudioCodecFactoryBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioCodecFactoryBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioCodecFactoryBase::addCodec (const AudioCodecDesc& description,
									  AudioDecoderConstructor decoderConstructor,
									  AudioEncoderConstructor encoderConstructor)
{
	CodecClass c;
	c.description = description;
	if(decoderConstructor)
	{
		c.description.capabilities |= AudioCodecDesc::kCanDecode;
		c.decoderConstructor = decoderConstructor;
	}
	if(encoderConstructor)
	{
		c.description.capabilities |= AudioCodecDesc::kCanEncode;
		c.encoderConstructor = encoderConstructor;
	}
	
	codecList.add (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const AudioCodecFactoryBase::CodecClass* AudioCodecFactoryBase::findCodec (StringRef contentType) const
{
	for(auto& codec : codecList)
		if(codec.description.fileType.getMimeType ().compare (contentType, false) == Text::kEqual)
			return &codec;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioCodecFactoryBase::getCodecCount () const
{
	return codecList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioCodecFactoryBase::getCodecDescription (AudioCodecDesc& description, int index) const
{
	ASSERT (codecList.isValidIndex (index))
	if(!codecList.isValidIndex (index))
		return kResultInvalidArgument;

	description = codecList[index].description;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioCodecFactoryBase::createDecoderInstance (IAudioSource*& source, IStream& stream,
															  StringRef contentType, const AudioContentFormat& format)
{
	const CodecClass* codecClass = findCodec (contentType);
	if(!codecClass)
		return kResultClassNotFound;

	if(!codecClass->decoderConstructor)
		return kResultUnexpected;

	source = (*codecClass->decoderConstructor) (stream, format);
	return source ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioCodecFactoryBase::createEncoderInstance (IAudioSink*& sink, IStream& stream,
															  StringRef contentType, const AudioContentFormat& format)
{
	const CodecClass* codecClass = findCodec (contentType);
	if(!codecClass)
		return kResultClassNotFound;

	if(!codecClass->encoderConstructor)
		return kResultUnexpected;

	sink = (*codecClass->encoderConstructor) (stream, format);
	return sink ? kResultOk : kResultFailed;
}

//************************************************************************************************
// AudioDecoderBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioDecoderBase, Object)

//************************************************************************************************
// AudioEncoderBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioEncoderBase, Object)
