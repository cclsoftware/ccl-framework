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
// Filename    : audioservice.cpp
// Description : Audio Service
//
//************************************************************************************************

#include "audioservice.h"
#include "plugversion.h"

#include "system/miniaudiosystem.h"
#include "system/miniaudiodevice.h"
#include "system/miniaudiocodec.h"
#include "system/miniaudiosources.h"
#include "system/miniaudiograph.h"
#include "system/miniaudiogenerator.h"

#include "ccl/public/plugins/classfactory.h"

using namespace CCL;

//************************************************************************************************
// AudioService
//************************************************************************************************

tresult CCL_API AudioService::initialize (IUnknown* context)
{
	if(!MiniAudioSystem::instance ().startup ())
		return kResultFailed;

	ServicePlugin::initialize (context);
	ClassFactory& classFactory = getClassFactory ();

	ClassDesc enumeratorClass (ClassID::AudioDeviceEnumerator, PLUG_CATEGORY_DEVICEENUMERATOR, CCL_AUDIOSERVICE_NAME_ " Device Enumerator");
	classFactory.registerClass (enumeratorClass, PluginConstructor<MiniAudioDeviceEnumerator, IDeviceEnumerator>::createInstance);

	ClassDesc deviceFactoryClass (ClassID::AudioDeviceFactory, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Device Factory");
	classFactory.registerClass (deviceFactoryClass, PluginConstructor<MiniAudioDeviceFactory, IAudioDeviceFactory>::createInstance);

	ClassDesc codecFactoryClass (ClassID::AudioCodecFactory, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Codec Factory");
	classFactory.registerClass (codecFactoryClass, PluginConstructor<MiniAudioCodecFactory, IAudioCodecFactory>::createInstance);

	ClassDesc bufferedSourceClass (ClassID::BufferedAudioSource, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Buffered Source");
	classFactory.registerClass (bufferedSourceClass, PluginConstructor<MiniAudioBufferedSource, IBufferedAudioSource>::createInstance);

	ClassDesc bufferedSinkClass (ClassID::BufferedAudioSink, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Buffered Sink");
	classFactory.registerClass (bufferedSinkClass, PluginConstructor<MiniAudioBufferedSink, IBufferedAudioSink>::createInstance);

	ClassDesc resamplingSourceClass (ClassID::ResamplingAudioSource, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Resampling Source");
	classFactory.registerClass (resamplingSourceClass, PluginConstructor<MiniAudioResamplingSource, IResamplingAudioSource>::createInstance);

	ClassDesc resamplingSinkClass (ClassID::ResamplingAudioSink, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Resampling Sink");
	classFactory.registerClass (resamplingSinkClass, PluginConstructor<MiniAudioResamplingSink, IResamplingAudioSink>::createInstance);

	ClassDesc convertingSourceClass (ClassID::ConvertingAudioSource, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Converting Source");
	classFactory.registerClass (convertingSourceClass, PluginConstructor<MiniAudioConvertingSource, IConvertingAudioSource>::createInstance);

	ClassDesc convertingSinkClass (ClassID::ConvertingAudioSink, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Converting Sink");
	classFactory.registerClass (convertingSinkClass, PluginConstructor<MiniAudioConvertingSink, IConvertingAudioSink>::createInstance);

	ClassDesc graphClass (ClassID::AudioGraph, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Graph");
	classFactory.registerClass (graphClass, PluginConstructor<MiniAudioGraph, IAudioGraph>::createInstance);

	ClassDesc sourceNodeClass (ClassID::AudioSourceNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Source Node");
	classFactory.registerClass (sourceNodeClass, PluginConstructor<MiniAudioSourceNode, IAudioSourceNode>::createInstance);

	ClassDesc gainNodeClass (ClassID::AudioGainNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Gain Node");
	classFactory.registerClass (gainNodeClass, PluginConstructor<MiniAudioGainNode, IAudioGainNode>::createInstance);

	ClassDesc inputNodeClass (ClassID::AudioInputNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Input Node");
	classFactory.registerClass (inputNodeClass, PluginConstructor<MiniAudioInputNode, IAudioInputNode>::createInstance);

	ClassDesc outputNodeClass (ClassID::AudioOutputNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Output Node");
	classFactory.registerClass (outputNodeClass, PluginConstructor<MiniAudioOutputNode, IAudioOutputNode>::createInstance);

	ClassDesc panNodeClass (ClassID::AudioPanNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Pan Node");
	classFactory.registerClass (panNodeClass, PluginConstructor<MiniAudioPanNode, IAudioPanNode>::createInstance);

	ClassDesc meterNodeClass (ClassID::AudioMeterNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Meter Node");
	classFactory.registerClass (meterNodeClass, PluginConstructor<MiniAudioMeterNode, IAudioMeterNode>::createInstance);

	ClassDesc processorNodeClass (ClassID::AudioProcessorNode, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Processor Node");
	classFactory.registerClass (processorNodeClass, PluginConstructor<MiniAudioProcessorNode, IAudioProcessorNode>::createInstance);

	ClassDesc oscillatorClass (ClassID::AudioOscillator, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Oscillator");
	classFactory.registerClass (oscillatorClass, PluginConstructor<MiniAudioOscillator, IAudioProcessor>::createInstance);

	ClassDesc noiseGeneratorClass (ClassID::AudioNoiseGenerator, PLUG_CATEGORY_AUDIOSERVICEUTILITY, CCL_AUDIOSERVICE_NAME_ " Noise Generator");
	classFactory.registerClass (noiseGeneratorClass, PluginConstructor<MiniAudioNoiseGenerator, IAudioProcessor>::createInstance);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioService::terminate ()
{
	MiniAudioSystem::instance ().shutdown ();

	return ServicePlugin::terminate ();
}
