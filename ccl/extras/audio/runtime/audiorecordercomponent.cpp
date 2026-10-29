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
// Filename    : ccl/extras/audio/runtime/audiorecordercomponent.cpp
// Description : Audio Recorder Component
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiorecordercomponent.h"

#include "ccl/extras/audio/waveform/audiowaveformview.h"
#include "ccl/extras/audio/audiofileio.h"
#include "ccl/extras/audio/audioformatter.h"

#include "ccl/public/gui/iparameter.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum AudioRecorderComponentTags
	{
		kChannels = 100,
		kBitDepth,
		kFileName
	};
}

//************************************************************************************************
// AudioRecorderComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioRecorderComponent, Component)
const String AudioRecorderComponent::kDefaultFileName ("Audio Recording");

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioRecorderComponent::AudioRecorderComponent (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("AudioRecorder") : name),
  waveform (NEW AudioWaveform)
{
	recorder = NEW AudioRecorder (*this);

	IParameter* channelsParam = paramList.addInteger (1, 2, CSTR ("channels"), Tag::kChannels);
	channelsParam->setFormatter (AutoPtr<IFormatter> (NEW Format::Channels));
	channelsParam->setValue (1);

	IParameter* bitDepthParam = paramList.addList (CSTR ("bitDepth"), Tag::kBitDepth);
	bitDepthParam->setFormatter (AutoPtr<IFormatter> (NEW Format::BitDepth));
	UnknownPtr<IListParameter> bitDepthList (bitDepthParam);
	bitDepthList->appendValue (16);
	bitDepthList->appendValue (24);
	bitDepthList->appendValue (32);

	paramList.addString (CSTR ("fileName"), Tag::kFileName)->fromString (kDefaultFileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioRecorderComponent::~AudioRecorderComponent ()
{
	setTransport (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorderComponent::setTransport (IAudioTransport* newTransport)
{
	if(transport != newTransport)
	{
		if(transport)
			transport->removeObserver (this);

		transport = newTransport;

		if(transport)
			transport->addObserver (this);

		onTransportStateChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioSink* AudioRecorderComponent::createRecordSink (SampleRate sampleRate)
{
	ASSERT (busy == false) // only one take at a time supported
	if(busy)
		return nullptr;
	
	int channelCount = paramList.byTag (Tag::kChannels)->getValue ();
	int bitDepth = UnknownPtr<IListParameter> (paramList.byTag (Tag::kBitDepth))->getSelectedValue ();

	AudioContentFormat format;
	format.channelCount = channelCount;
	format.sampleRate = sampleRate;
	format.speakerFormat = SpeakerDef::getDefault (channelCount);
	format.makeDefaultForBitDepth (bitDepth);

	String fileName;
	paramList.byTag (Tag::kFileName)->toString (fileName);
	fileName.trimWhitespace ();
	if(fileName.isEmpty ())
		fileName = kDefaultFileName;

	Url path;
	System::GetSystem ().getLocation (path, System::kUserMusicFolder);
	path.descend (LegalFileName (fileName), Url::kFile);
	path.setFileType (AudioFileSink::getWaveFileType ());
	path.makeUnique ();

	AutoPtr<AudioFileSink> fileSink = NEW AudioFileSink (path);
	fileSink->setFormat (format);
	if(!fileSink->create ())
	{
		CCL_WARN ("AudioRecorderComponent: Failed to create file \"%s\".\n", 
				  MutableCString (UrlDisplayString (path)).str ())
		return nullptr;
	}

	waveform->startLive (format);

	AutoPtr<AudioPeakBuilderSink> peakSink = NEW AudioPeakBuilderSink;
	peakSink->setSink (fileSink);
	peakSink->setBuilder (waveform->getLiveBuilder ());

	path.getName (fileName, false);
	paramList.byTag (Tag::kFileName)->fromString (fileName);

	busy = true;
	return peakSink.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioRecorderComponent::onRecordTakeFinished (IAudioSink* sink, tresult result)
{
	auto* peakSink = unknown_cast<AudioPeakBuilderSink> (sink);
	auto* fileSink = peakSink ? unknown_cast<AudioFileSink> (peakSink->getSink ()) : nullptr;
	ASSERT (peakSink != nullptr && fileSink != nullptr)
	if(!peakSink || !fileSink)
		return;

	// the recorder has drained the sink, so nothing is written anymore
	peakSink->setBuilder (nullptr);

	TSamples frameCount = fileSink->getFrameCount ();
	waveform->finishLive (frameCount);

	fileSink->close ();

	if(result != kResultOk || frameCount <= 0)
		fileSink->deletePhysical ();
	else
		lastRecordedFile = NEW Url (fileSink->getPath ());

	busy = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioRecorderComponent::onTransportStateChanged ()
{
	bool recording = transport && transport->isRecording ();

	paramList.byTag (Tag::kChannels)->enable (!recording);
	paramList.byTag (Tag::kBitDepth)->enable (!recording);	
	paramList.byTag (Tag::kFileName)->enable (!recording);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioRecorderComponent::onTransportDurationChanged (TSeconds duration)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioRecorderComponent::onTransportTimeChanged (TSeconds time)
{
	if(waveform->isLive ())
		waveform->updateLive ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API AudioRecorderComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "WaveformView")
		return *NEW AudioWaveformView (waveform, nullptr, bounds); // no transport: no cursor on a take
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioRecorderComponent::terminate ()
{
	// makes sure current take ends cleanly
	recorder->stopRecording ();
	
	return SuperClass::terminate ();
}
