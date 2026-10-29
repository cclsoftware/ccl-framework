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
// Filename    : ccl/extras/audio/runtime/audioplayercomponent.cpp
// Description : Audio Player Component
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audioplayercomponent.h"

#include "ccl/extras/audio/waveform/audiowaveformview.h"
#include "ccl/extras/audio/audiofileio.h"
#include "ccl/extras/audio/audioformatter.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/system/formatter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum AudioPlayerComponentTags
	{
		kRepeat = 100,
		kFileName,
		kFileInfo
	};
}

//************************************************************************************************
// AudioPlayerComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioPlayerComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioPlayerComponent::AudioPlayerComponent (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("AudioPlayer") : name),
  player (NEW AudioPlayer),
  waveform (NEW AudioWaveform)
{
	paramList.addParam (CSTR ("repeat"), Tag::kRepeat)->enable (false);
	paramList.addString (CSTR ("fileName"), Tag::kFileName)->setReadOnly ();
	paramList.addString (CSTR ("fileInfo"), Tag::kFileInfo)->setReadOnly ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayerComponent::setTransport (IAudioTransport* _transport)
{
	transport = _transport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioPlayerComponent::loadFile (UrlRef path)
{
	// player and waveform need separate instances for accessing the file
	AutoPtr<AudioFileSource> audioFile = NEW AudioFileSource (path);
	AutoPtr<AudioFileSource> audioFile2 = NEW AudioFileSource (path);
	bool opened = audioFile->open () && audioFile2->open ();
	if(!opened)
		return false;

	player->setSource (audioFile2);

	String fileName;
	path.getName (fileName);

	AudioContentFormat format;
	audioFile->getFormat (format);

	String infoString;
	if(format.formatType == kAudioFormatPCM)
	{
		infoString.appendFormat ("%(1), %(2), %(3), %(4)",
									Format::SampleRate::print (format.sampleRate),
									Format::BitDepth::print (format.bitDepth, format.isFloat ()),
									Format::Channels::print (format.channelCount),
									Format::Duration::print (audioFile->getDuration ()));
	}
	else
	{
		infoString.appendFormat ("%(1), %(2), %(3)",
									Format::SampleRate::print (format.sampleRate),
									Format::Channels::print (format.channelCount),
									Format::Duration::print (audioFile->getDuration ()));
	}

	waveform->stopBuilding ();
	waveform->setAudioSource (audioFile);
	waveform->startBuilding ();

	paramList.byTag (Tag::kRepeat)->enable (true);
	paramList.byTag (Tag::kFileName)->fromString (fileName);
	paramList.byTag (Tag::kFileInfo)->fromString (infoString);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioPlayerComponent::unloadFile ()
{
	player->stop ();
	player->clearSource ();

	waveform->stopBuilding ();
	waveform->setAudioSource (nullptr);
	
	paramList.byTag (Tag::kRepeat)->enable (false);
	paramList.byTag (Tag::kFileName)->fromString (String::kEmpty);
	paramList.byTag (Tag::kFileInfo)->fromString (String::kEmpty);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API AudioPlayerComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "WaveformView")
		return *NEW AudioWaveformView (waveform, transport, bounds);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioPlayerComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kRepeat :
		player->setRepeat (param->getValue ());
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioPlayerComponent::terminate ()
{
	unloadFile ();
	
	return SuperClass::terminate ();
}
