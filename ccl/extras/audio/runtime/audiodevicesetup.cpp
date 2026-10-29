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
// Filename    : ccl/extras/audio/runtime/audiodevicesetup.cpp
// Description : Audio Device Setup
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiodevicesetup.h"
#include "ccl/extras/audio/runtime/audiodevicesession.h"
#include "ccl/extras/audio/audioformatter.h"

#include "ccl/app/params.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum AudioDeviceSetupTags
	{
		kRenderDeviceList = 100,
		kCaptureDeviceList,
		kExclusiveMode,
		kSampleRate,
		kRequestedBlockSize,
		kBlockSize,
		kRenderChannels,
		kCaptureChannels,
		kRenderLatency,
		kCaptureLatency,
		kRefresh
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Audio")
	XSTRING (None, "None")
END_XSTRINGS

//************************************************************************************************
// AudioDeviceSetup::DeviceItem
//************************************************************************************************

AudioDeviceSetup::DeviceItem::DeviceItem (IDeviceDescription* _description)
{
	description.share (_description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDeviceSetup::DeviceItem::toString (String& string, int flags) const
{
	if(description)
		string = description->getDeviceName ();
	else
		string = XSTR (None);
	return true;
}

//************************************************************************************************
// AudioDeviceSetup
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AudioDeviceSetup, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceSetup::AudioDeviceSetup (AudioDeviceSession& session, StringRef name)
: Component (name.isEmpty () ? CCLSTR ("AudioDeviceSetup") : name),
  session (session)
{
	session.retain ();

	deviceEnumerator = ccl_new<IDeviceEnumerator> (ClassID::AudioDeviceEnumerator);
	ASSERT (deviceEnumerator != nullptr)

	paramList.addList (CSTR ("renderDeviceList"), Tag::kRenderDeviceList);
	paramList.addList (CSTR ("captureDeviceList"), Tag::kCaptureDeviceList);
	paramList.addParam (CSTR ("exclusiveMode"), Tag::kExclusiveMode)->enable (false);	
	
	IParameter* requestedBlockSizeList = paramList.addList (CSTR ("requestedBlockSize"), Tag::kRequestedBlockSize);
	requestedBlockSizeList->enable (false);

	paramList.addString (CSTR ("sampleRate"), Tag::kSampleRate)->setReadOnly ();
	paramList.addString (CSTR ("blockSize"), Tag::kBlockSize)->setReadOnly ();
	paramList.addString (CSTR ("renderChannels"), Tag::kRenderChannels)->setReadOnly ();
	paramList.addString (CSTR ("captureChannels"), Tag::kCaptureChannels)->setReadOnly ();
	paramList.addString (CSTR ("renderLatency"), Tag::kRenderLatency)->setReadOnly ();
	paramList.addString (CSTR ("captureLatency"), Tag::kCaptureLatency)->setReadOnly ();
	paramList.addParam (CSTR ("refresh"), Tag::kRefresh);

	updateDeviceLists ();
	updateStatus ();

	session.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceSetup::~AudioDeviceSetup ()
{
	session.removeObserver (this);
	session.release ();

	ccl_release (deviceEnumerator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSetup::updateDeviceLists ()
{
	auto* renderList = paramList.byTag<ListParam> (Tag::kRenderDeviceList);
	renderList->removeAll ();
	renderList->appendObject (NEW DeviceItem);

	auto* captureList = paramList.byTag<ListParam> (Tag::kCaptureDeviceList);
	captureList->removeAll ();
	captureList->appendObject (NEW DeviceItem);

	UnknownList descriptions;
	if(deviceEnumerator)
		deviceEnumerator->enumerateDevices (descriptions, IDeviceDescription::kMediaDuplexCapability);

	ForEachUnknown (descriptions, unk)
		if(UnknownPtr<IDeviceDescription> description = unk)
		{
			if(get_flag (description->getDeviceFlags (), IDeviceDescription::kMediaRenderCapability))
				renderList->appendObject (NEW DeviceItem (description));

			if(get_flag (description->getDeviceFlags (), IDeviceDescription::kMediaCaptureCapability))
				captureList->appendObject (NEW DeviceItem (description));
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSetup::refreshDeviceLists ()
{
	AudioDeviceComponentDesc oldSelection = getSelectedDeviceDescription ();

	updateDeviceLists ();

	auto reselect = [] (ListParam& list, StringRef deviceId) -> bool
	{
		Object* match = nullptr;
		if(!deviceId.isEmpty ())
		{
			match = list.findObject ([&] (const Object* object)
			{
				auto* item = static_cast<const DeviceItem*> (object);
				return item->getDescription () && item->getDescription ()->getDeviceID () == deviceId;
			});
		}

		if(match)
		{
			list.selectObject (match, false);
			return true;
		}

		// not found, fall back to "None"
		list.selectObject (list.getObject<Object> (0), false);
		return deviceId.isEmpty ();
	};

	bool renderAvailable = reselect (*paramList.byTag<ListParam> (Tag::kRenderDeviceList), oldSelection.renderDeviceId);
	bool captureAvailable = reselect (*paramList.byTag<ListParam> (Tag::kCaptureDeviceList), oldSelection.captureDeviceId);

	if(!renderAvailable || !captureAvailable)
		makeDevice (getSelectedDeviceDescription ()); // one of them vanished, let the session react

	updateStatus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceComponentDesc AudioDeviceSetup::getSelectedDeviceDescription () const
{
	auto* renderItem = static_cast<DeviceItem*> (paramList.byTag<ListParam> (Tag::kRenderDeviceList)->getSelectedObject ());
	auto* captureItem = static_cast<DeviceItem*> (paramList.byTag<ListParam> (Tag::kCaptureDeviceList)->getSelectedObject ());

	AudioDeviceComponentDesc description;
	if(renderItem && renderItem->getDescription ())
		description.renderDeviceId = renderItem->getDescription ()->getDeviceID ();
	if(captureItem && captureItem->getDescription ())
		description.captureDeviceId = captureItem->getDescription ()->getDeviceID ();
	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSetup::makeDevice (const AudioDeviceComponentDesc& description)
{
	if(!description.isValid ())
		session.closeDevice ();
	else
		session.openDevice (description, wantExclusiveMode);
	
	updateStatus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSetup::updateBlockSizeList ()
{
	auto* blockSizeList = paramList.byTag<ListParam> (Tag::kRequestedBlockSize);
	blockSizeList->removeAll ();

	IAudioComponent* audioDevice = session.getDevice ();
	if(!audioDevice)
	{
		blockSizeList->setFormatter (nullptr);
		blockSizeList->enable (false);
		return;
	}

	AudioBlockSizeRange range;
	if(UnknownPtr<IAudioDeviceSpecifics> audioDeviceSpecifics = audioDevice)
		audioDeviceSpecifics->getBlockSizeRange (range);

	if(blockSizeList->getFormatter () == nullptr || range.unit != requestedBlockSizeUnit)
	{
		requestedBlockSizeUnit = range.unit;
		if(requestedBlockSizeUnit == AudioBlockSizeRange::kMilliseconds)
			blockSizeList->setFormatter (AutoPtr<IFormatter> (NEW Format::SecondsMS));
		else
			blockSizeList->setFormatter (AutoPtr<IFormatter> (NEW Format::BlockSize));
	}

	// collect candidates in the unit of the range
	Vector<int> sizes;
	if(range.unit == AudioBlockSizeRange::kMilliseconds)
	{
		// use fixed list of durations
		constexpr int kMilliseconds[] = {10, 20, 30, 50, 75, 100};
		for(int milliseconds : kMilliseconds)
			if(milliseconds >= range.minBlockSize && milliseconds <= range.maxBlockSize)
				sizes.add (milliseconds);
	}
	else
	{
		ASSERT (range.unit == AudioBlockSizeRange::kSampleFrames)

		if(range.step == AudioBlockSizeRange::kStepPowerOfTwo)
		{
			for(int size = range.minBlockSize; size <= range.maxBlockSize; size *= 2)
				sizes.add (size);
		}
		else if(range.step > 0)
		{
			for(int size = range.minBlockSize; size <= range.maxBlockSize; size += range.step)
				sizes.add (size);
		}
		else
		{
			ASSERT (range.step == AudioBlockSizeRange::kStepAny)
			for(int size = 32; size <= range.maxBlockSize; size *= 2)
				if(size >= range.minBlockSize)
					sizes.add (size);
		}
	}

	// keep the list short: sample evenly across it, always keeping the first, the last and the preferred entry
	constexpr int kMaxEntries = 10;
	if(sizes.count () > kMaxEntries)
	{
		int lastIndex = sizes.count () - 1;
		int preferredIndex = 0;
		for(int i = 1; i <= lastIndex; i++)
			if(ccl_abs (sizes[i] - range.preferredBlockSize) < ccl_abs (sizes[preferredIndex] - range.preferredBlockSize))
				preferredIndex = i;

		Vector<int> indices;
		int closest = 0;
		for(int i = 0; i < kMaxEntries; i++)
		{
			int index = int(ccl_round<0> (double(i) * lastIndex / (kMaxEntries - 1)));
			indices.add (index);
			if(ccl_abs (index - preferredIndex) < ccl_abs (indices[closest] - preferredIndex))
				closest = i;
		}
		indices[closest] = preferredIndex;

		Vector<int> thinned;
		for(int index : indices)
			thinned.add (sizes[index]);
		sizes.takeVector (thinned);
	}

	for(int size : sizes)
	{
		if(range.unit == AudioBlockSizeRange::kMilliseconds)
			blockSizeList->appendValue (TSeconds(size) / 1000.);
		else
			blockSizeList->appendValue (size);
	}

	blockSizeList->enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSetup::updateStatus ()
{
	updateBlockSizeList ();

	IAudioComponent* audioDevice = session.getDevice ();
	if(audioDevice)
	{
		AudioProcessConfiguration configuration;
		audioDevice->getProcessConfiguration (configuration);

		paramList.byTag (Tag::kSampleRate)->fromString (Format::SampleRate::print (configuration.sampleRate));
		paramList.byTag (Tag::kBlockSize)->fromString (Format::BlockSize::print (configuration.maxBlockSize));

		// show the requested block size (or the actual one if nothing was requested) in the unit of the list
		Variant target;
		bool useSeconds = requestedBlockSizeUnit == AudioBlockSizeRange::kMilliseconds;
		if(session.getRequestedBlockDuration () > 0)
		{
			TSeconds duration = session.getRequestedBlockDuration ();
			target = useSeconds ? duration : double(AudioDeviceSession::blockDurationToFrames (duration, configuration.sampleRate));
		}
		else
		{
			int frames = session.getRequestedBlockSize () > 0 ? session.getRequestedBlockSize () : configuration.maxBlockSize;
			target = useSeconds ? double(frames) / configuration.sampleRate : double(frames);
		}
		paramList.byTag<ListParam> (Tag::kRequestedBlockSize)->selectNearestValue (target, false);

		int renderLatency = 0, captureLatency = 0;
		audioDevice->getLatency (renderLatency, kAudioRenderDirection);
		audioDevice->getLatency (captureLatency, kAudioCaptureDirection);

		paramList.byTag (Tag::kRenderLatency)->fromString (Format::SecondsMS::print (double(renderLatency)/configuration.sampleRate));
		paramList.byTag (Tag::kCaptureLatency)->fromString (Format::SecondsMS::print (double(captureLatency)/configuration.sampleRate));

		auto updateChannels = [&] (AudioStreamDirection direction, int tag)
		{
			const IAudioStreamLayout& streamLayout = audioDevice->getStreamLayout ();
			if(streamLayout.getStreamCount (direction) > 0)
			{
				AudioStreamDescription description;
				streamLayout.getStreamDescription (description, direction, 0);
				
				paramList.byTag (tag)->fromString (Format::Channels::print (description.channelCount, true));
			}
			else
				paramList.byTag (tag)->fromString (String::kEmpty);
		};

		updateChannels (kAudioRenderDirection, Tag::kRenderChannels);
		updateChannels (kAudioCaptureDirection, Tag::kCaptureChannels);

		Variant canExclusiveMode, exclusiveMode;
		if(UnknownPtr<IAudioDeviceSpecifics> audioDeviceSpecifics = audioDevice)
		{
			audioDeviceSpecifics->getDeviceOption (canExclusiveMode, AudioDeviceOptions::kCanExclusiveMode);
			audioDeviceSpecifics->getDeviceOption (exclusiveMode, AudioDeviceOptions::kExclusiveMode);
		}
		paramList.byTag (Tag::kExclusiveMode)->setValue (exclusiveMode.asBool ());
		paramList.byTag (Tag::kExclusiveMode)->enable (canExclusiveMode.asBool ());
	}
	else
	{
		paramList.byTag (Tag::kSampleRate)->fromString (String::kEmpty);
		paramList.byTag (Tag::kBlockSize)->fromString (String::kEmpty);
		paramList.byTag (Tag::kRenderLatency)->fromString (String::kEmpty);
		paramList.byTag (Tag::kCaptureLatency)->fromString (String::kEmpty);
		paramList.byTag (Tag::kRenderChannels)->fromString (String::kEmpty);
		paramList.byTag (Tag::kCaptureChannels)->fromString (String::kEmpty);
		paramList.byTag (Tag::kExclusiveMode)->enable (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioDeviceSetup::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kRenderDeviceList :
	case Tag::kCaptureDeviceList :
		makeDevice (getSelectedDeviceDescription ());
		break;

	case Tag::kExclusiveMode :
		wantExclusiveMode = param->getValue ().asBool ();
		session.clearRequestedBlockSize ();
		makeDevice (getSelectedDeviceDescription ());
		break;

	case Tag::kRequestedBlockSize :
		{
			Variant value = UnknownPtr<IListParameter> (param)->getSelectedValue ();
			if(requestedBlockSizeUnit == AudioBlockSizeRange::kMilliseconds)
				session.setRequestedBlockDuration (value.asDouble ());
			else
				session.setRequestedBlockSize (value.asInt ());
		}
		break;

	case Tag::kRefresh :
		refreshDeviceLists ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioDeviceSetup::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &session)
	{
		if(msg == kChanged || msg == AudioDeviceSession::kConfigurationChanged)
		{
			updateStatus ();
		}
		else if(msg == AudioDeviceSession::kCapturePermissionChanged)
		{
			refreshDeviceLists (); // capture devices may be listed differently now
		}
		else if(msg == AudioDeviceSession::kDeviceLost)
		{
			updateDeviceLists ();
		}
	}
	else
		SuperClass::notify (subject, msg);
}