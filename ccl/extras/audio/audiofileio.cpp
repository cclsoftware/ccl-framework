//************************************************************************************************
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
// Filename    : ccl/extras/audio/audiofileio.cpp
// Description : Audio File I/O
//
//************************************************************************************************

#include "ccl/extras/audio/audiofileio.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// AudioFileBase
//************************************************************************************************

AutoPtr<IAudioCodecFactory> AudioFileBase::codecFactory;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioFileBase::initCodecs (bool registerFileTypes)
{
	codecFactory = ccl_new<IAudioCodecFactory> (ClassID::AudioCodecFactory);
	if(!codecFactory)
		return false;

	if(registerFileTypes == true)
	{
		FileTypeFilter fileTypes;
		getFileTypes (fileTypes);

		for(auto fileType : fileTypes.getContent ())
			System::GetFileTypeRegistry ().registerFileType (fileType);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioFileBase::exitCodecs ()
{
	ccl_release (codecFactory.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioFileBase::getFileTypes (FileTypeFilter& fileTypes, int capabilityFilter)
{
	bool all = capabilityFilter == 0;
	bool wantEncode = get_flag (capabilityFilter, AudioCodecDesc::kCanEncode);
	bool wantDecode = get_flag (capabilityFilter, AudioCodecDesc::kCanDecode);

	if(codecFactory)
		for(int i = 0, count = codecFactory->getCodecCount (); i < count; i++)
		{
			AudioCodecDesc description;
			codecFactory->getCodecDescription (description, i);
			if(all || (description.canEncode () == wantEncode && description.canDecode () == wantDecode))
			{
				fileTypes.addFileType (description.fileType);
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& AudioFileBase::getWaveFileType ()
{
	static const String kWaveContentType (CCLSTR (AudioContentTypes::kWave));
	if(const FileType* knownType = System::GetFileTypeRegistry ().getFileTypeByMimeType (kWaveContentType))
		return *knownType;

	static const FileType fallbackType (nullptr, "wav", AudioContentTypes::kWave);
	return fallbackType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioFileBase, FileStreamResource)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileBase::AudioFileBase ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileBase::AudioFileBase (UrlRef path)
: FileStreamResource (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileBase::AudioFileBase (const AudioFileBase& other)
: FileStreamResource (other)
{}

//************************************************************************************************
// AudioFileSource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioFileSource, AudioFileBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSource::AudioFileSource ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSource::AudioFileSource (UrlRef path)
: AudioFileBase (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSource::AudioFileSource (const AudioFileSource& other)
: AudioFileBase (other),
  decoder (nullptr) // don't copy decoder
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSource::~AudioFileSource ()
{
	ensureClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioFileSource::openFile (int mode)
{
	lastOpenMode = mode;

	if(!SuperClass::openFile (mode))
		return false;

	StringRef mimeType = getPath ().getFileType ().getMimeType ();
	if(!codecFactory || codecFactory->createDecoderInstance (decoder, *getStream (), mimeType, AudioContentFormat ()) != kResultOk)
	{
		SuperClass::closeFile ();
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioFileSource::closeFile ()
{
	decoder.release ();
	return SuperClass::closeFile ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioFileSource::setPath (UrlRef newPath)
{
	if(!isOpen ())
		return SuperClass::setPath (newPath);

	// already open: close the current file/decoder, switch path, and re-open for the new one
	if(!close ())
		return false;

	if(!SuperClass::setPath (newPath))
		return false;

	return open (lastOpenMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioFileSource::getFormat (AudioContentFormat& format) const
{
	if(!decoder)
		return kResultFailed;

	if(UnknownPtr<IAudioFormatProvider> formatProvider = decoder)
		return formatProvider->getFormat (format);

	format = AudioContentFormat ();
	format.channelCount = decoder->getNumChannels ();
	format.sampleRate = decoder->getSampleRate ();
	format.speakerFormat = decoder->getSpeakerFormat ();
	format.makeFloat ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioFileSource::getNumChannels () const
{
	return decoder ? decoder->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API AudioFileSource::getSampleRate () const
{
	return decoder ? decoder->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API AudioFileSource::getFrameCount () const
{
	return decoder ? decoder->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API AudioFileSource::getSpeakerFormat () const
{
	return decoder ? decoder->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioFileSource::read (AudioData& data, int sampleFrames)
{
	return decoder ? decoder->read (data, sampleFrames) : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API AudioFileSource::seek (TSamples position)
{
	return decoder ? decoder->seek (position) : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API AudioFileSource::tell () const
{
	return decoder ? decoder->tell () : -1;
}

//************************************************************************************************
// AudioFileSink
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioFileSink, AudioFileBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSink::AudioFileSink ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSink::AudioFileSink (UrlRef path)
: AudioFileBase (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSink::AudioFileSink (const AudioFileSink& other)
: AudioFileBase (other),
  encoder (nullptr), // don't copy encoder
  format (other.format)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioFileSink::~AudioFileSink ()
{
	ensureClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioFileSink::setPath (UrlRef newPath)
{
	if(!isOpen ())
		return SuperClass::setPath (newPath);

	// already open: close the current file/encoder, switch path, and re-create for the new one
	if(!close ())
		return false;

	if(!SuperClass::setPath (newPath))
		return false;

	return create (lastCreateMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioFileSink::createFile (int mode)
{
	lastCreateMode = mode;

	ASSERT (format.isValid ()) // encoding has no native format to fall back to
	if(!format.isValid ())
		return false;

	if(!SuperClass::createFile (mode))
		return false;

	StringRef mimeType = getPath ().getFileType ().getMimeType ();
	if(!codecFactory || codecFactory->createEncoderInstance (encoder, *getStream (), mimeType, format) != kResultOk)
	{
		SuperClass::closeFile ();
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioFileSink::closeFile ()
{
	encoder.release ();
	return SuperClass::closeFile ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioFileSink::getNumChannels () const
{
	return encoder ? encoder->getNumChannels () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SampleRate CCL_API AudioFileSink::getSampleRate () const
{
	return encoder ? encoder->getSampleRate () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TSamples CCL_API AudioFileSink::getFrameCount () const
{
	return encoder ? encoder->getFrameCount () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpeakerBitField CCL_API AudioFileSink::getSpeakerFormat () const
{
	return encoder ? encoder->getSpeakerFormat () : SpeakerDef::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AudioFileSink::write (const AudioData& data, int sampleFrames)
{
	return encoder ? encoder->write (data, sampleFrames) : -1;
}
