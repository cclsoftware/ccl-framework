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
// Filename    : services/audio/source/system/miniaudiogenerator.cpp
// Description : Audio Generators based on miniaudio
//
//************************************************************************************************

#include "miniaudiogenerator.h"

#include "ccl/extras/audio/cclaudiodataops.h"
#include "ccl/extras/audio/audioformatter.h"
#include "ccl/extras/audio/audiocurves.h"

#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

//************************************************************************************************
// MiniAudioGenerator
//************************************************************************************************

void MiniAudioGenerator::addBaseParameterDefinitions (ParameterDefinitionList& paramList)
{
	paramList.add (ParameterDefinition ()
					.setType (kParamTypeToggle)
					.setName (CSTR ("on"))
					.setTitle (CCLSTR ("On"))
					.setTag (kOn))
				.add (ParameterDefinition ()
					.setType (kParamTypeFloat)
					.setName (CSTR ("amplitude"))
					.setTitle (CCLSTR ("Amplitude"))
					.setTag (kAmplitude)
					.setRange (Math::factorToDb (0.), +24.)
					.setDefault (-24.)
					.setUnit (Format::kDB)
					);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (MiniAudioGenerator, AudioProcessorBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioGenerator::MiniAudioGenerator (const ParameterDefinitionList& paramList)
: paramList (paramList)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MiniAudioGenerator::countParameters () const
{
	return paramList.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGenerator::getParameterAt (ParameterDefinition& definition, int index) const
{
	if(!paramList.isValidIndex (index))
		return kResultInvalidArgument;

	definition = paramList.at (index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MiniAudioGenerator::setOn (bool state)
{
	on = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGenerator::setAmplitude (float newAmplitude)
{
	amplitude = newAmplitude;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGenerator::applyDefaultValues ()
{
	for(const ParameterDefinition& p : paramList)
	{
		AudioParamValue value (p.defaultValue.asDouble ());
		applyValue (p.tag, value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioGenerator::applyValue (int tag, AudioParamValue value)
{
	switch(tag)
	{
	case kOn : setOn (value.asBool ()); break;
	case kAmplitude : setAmplitude (Math::dbToFactor (value.asFloat ())); break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API MiniAudioGenerator::getCapabilities () const
{
	return kCapabilityKnownStreamLayout|kCapabilityMutableStreamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGenerator::setStreamLayout (const IAudioStreamLayout& layout)
{
	// generator always replaces the output signal - an input stream, if present, is accepted
	// for topology purposes only and is never read in process()
	bool layoutSupported = AudioLayoutHelper (layout).matchesStreamCount (0, 1) ||
						   AudioLayoutHelper (layout).matchesStreamCount (1, 1);
	ASSERT (layoutSupported)
	if(layoutSupported == false)
		return kResultInvalidArgument;

	streamLayout.removeAll ();

	AudioStreamDescription outputDescription;
	layout.getStreamDescription (outputDescription, kAudioOutputDirection, 0);
	streamLayout.add (kAudioOutputDirection,
		NEW AudioStream (outputDescription.name, AudioStream::kActive, outputDescription.channelCount, outputDescription.speakerType));

	if(layout.getStreamCount (kAudioInputDirection) > 0)
	{
		AudioStreamDescription inputDescription;
		layout.getStreamDescription (inputDescription, kAudioInputDirection, 0);
		streamLayout.add (kAudioInputDirection,
			NEW AudioStream (inputDescription.name, AudioStream::kActive, inputDescription.channelCount, inputDescription.speakerType));
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API MiniAudioGenerator::getStreamLayout () const
{
	return streamLayout; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGenerator::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{
	ASSERT (configuration.floatType == kAudioFloat32) // miniaudio has no float64 support
	if(configuration.floatType != kAudioFloat32)
		return kResultInvalidArgument;

	return SuperClass::setProcessConfiguration (configuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MiniAudioGenerator::process (const AudioProcessData& data)
{
	ASSERT (data.ioData.numOutputs > 0)
	int numChannels = data.ioData.outputs[0].numChannels;
	ASSERT (numChannels > 0)
	int numSamples = data.ioData.numSamples;

	// apply parameter changes
	if(!data.paramData.isEmpty ())
		for(int i = 0; i < data.paramData.numEvents; i++)
		{
			const AudioParamEvent& e = data.paramData.events[i];
			applyValue (e.tag, e.value);
		}

	// process audio
	float32* firstChannel = data.ioData.outputs[0].buffers32[0];

	// gain changes (amplitude, on/off) are ramped within the block to avoid zipper noise
	float targetGain = on ? amplitude : 0.f;

	if(appliedGain == 0.f && targetGain == 0.f)
		AudioVectorOpsGeneric::vectorClear (firstChannel, numSamples);
	else
	{
		generate (firstChannel, numSamples);

		int maxRampSamples = int(processConfiguration.sampleRate * kRampTime);
		AudioVectorOpsGeneric::vectorRamp (firstChannel, numSamples, appliedGain, targetGain, maxRampSamples);
	}
	appliedGain = targetGain;

	for(int ch = 1; ch < numChannels; ch++)
		AudioVectorOpsGeneric::vectorCopy (data.ioData.outputs[0].buffers32[ch], firstChannel, numSamples);

	return kResultOk;
}

//************************************************************************************************
// MiniAudioOscillator
//************************************************************************************************

ma_waveform_type MiniAudioOscillator::toMaWaveType (WaveType type)
{
	switch(type)
	{
	case kSquare : return ma_waveform_type_square;
	case kTriangle : return ma_waveform_type_triangle;
	case kSawtooth : return ma_waveform_type_sawtooth;
	default :
	case kSine : return ma_waveform_type_sine;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr MiniAudioOscillator::kWaveTypeStrings[]
{
	"Sine",		// kSine
	"Square",	// kSquare
	"Triangle",	// kTriangle
	"Sawtooth"	// kSawtooth
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const ParameterDefinitionList& MiniAudioOscillator::getParamList ()
{
	static ParameterDefinitionList paramList;
	if(paramList.isEmpty ())
	{
		addBaseParameterDefinitions (paramList);
		
		paramList.add (ParameterDefinition ()
					   .setType (kParamTypeList)
					   .setName (AudioParamID::Oscillator::kWaveType)
					   .setTitle (CCLSTR ("Wave Type"))
					   .setTag (kWaveType)
					   .setListValue ({kWaveTypeStrings, ARRAY_COUNT(kWaveTypeStrings)})
					   .setDefault (kSine))
				 .add (ParameterDefinition ()
					   .setType (kParamTypeFloat)
					   .setName (AudioParamID::Oscillator::kFrequency)
					   .setTitle (CCLSTR ("Frequency"))
					   .setTag (kFrequency)
					   .setRange (1., 22000.)
					   .setDefault (440.)
					   .setCurve (CurveNames::kFrequency)
					   .setUnit (Format::kFrequency)
					);
	}
	return paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (MiniAudioOscillator, MiniAudioGenerator)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioOscillator::MiniAudioOscillator ()
: MiniAudioGenerator (getParamList ())
{
	ma_waveform_config config = ma_waveform_config_init (ma_format_f32, 1, ma_uint32(kDefaultSampleRate),
														 toMaWaveType (waveType), 
														 1.,
														 double(frequency));
	ma_result result = ma_waveform_init (&config, &waveform);
	ASSERT (result == MA_SUCCESS)

	applyDefaultValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioOscillator::~MiniAudioOscillator ()
{
	ma_waveform_uninit (&waveform);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOscillator::setWaveType (WaveType _type)
{
	waveType = ccl_bound<WaveType> (_type, kWaveTypeFirst, kWaveTypeLast);
	ma_waveform_set_type (&waveform, toMaWaveType (waveType));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOscillator::setFrequency (float newFrequency)
{
	frequency = newFrequency;
	ma_waveform_set_frequency (&waveform, double(frequency));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOscillator::applyValue (int tag, AudioParamValue value)
{
	switch(tag)
	{
	case kWaveType : setWaveType (value.asInt ()); break;
	case kFrequency : setFrequency (value.asFloat ()); break;
	default :
		SuperClass::applyValue (tag, value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOscillator::onConfigurationChanged ()
{
	ma_waveform_set_sample_rate (&waveform, ma_uint32(processConfiguration.sampleRate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioOscillator::generate (float32* output, int numSamples)
{
	ma_uint64 framesRead = 0;
	ma_result result = ma_waveform_read_pcm_frames (&waveform, output, ma_uint64(numSamples), &framesRead);
	ASSERT (result == MA_SUCCESS)
}

//************************************************************************************************
// MiniAudioNoiseGenerator
//************************************************************************************************

ma_noise_type MiniAudioNoiseGenerator::toMaNoiseType (NoiseType type)
{
	switch(type)
	{
	case kPink : return ma_noise_type_pink;
	case kBrown : return ma_noise_type_brownian;
	default :
	case kWhite : return ma_noise_type_white;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const CStringPtr MiniAudioNoiseGenerator::kNoiseTypeStrings[]
{
	"White",	// kWhite
	"Pink",		// kPink
	"Brown"		// kBrown
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const ParameterDefinitionList& MiniAudioNoiseGenerator::getParamList ()
{
	static ParameterDefinitionList paramList;
	if(paramList.isEmpty ())
	{
		addBaseParameterDefinitions (paramList);
		
		paramList.add (ParameterDefinition ()
					   .setType (kParamTypeList)
					   .setName (AudioParamID::NoiseGenerator::kNoiseType)
					   .setTitle (CCLSTR ("Noise Type"))
					   .setTag (kNoiseType)
					   .setListValue ({kNoiseTypeStrings, ARRAY_COUNT(kNoiseTypeStrings)})
					   .setDefault (kWhite))
				 .add (ParameterDefinition ()
					   .setType (kParamTypeInteger)
					   .setName (AudioParamID::NoiseGenerator::kSeed)
					   .setTitle (CCLSTR ("Seed"))
					   .setTag (kSeed)
					   .setRange (1, 9999)
					   .setDefault (4321)
					);
	}
	return paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (MiniAudioNoiseGenerator, MiniAudioGenerator)

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioNoiseGenerator::MiniAudioNoiseGenerator ()
: MiniAudioGenerator (getParamList ())
{
	for(NoiseType noiseType = kNoiseTypeFirst; noiseType <= kNoiseTypeLast; noiseType++)
	{
		ma_noise_config config = ma_noise_config_init (ma_format_f32, 1,
													  toMaNoiseType (noiseType), 
													  seed, 1.);
		ma_result result = ma_noise_init (&config, nullptr, &noise[noiseType]);
		ASSERT (result == MA_SUCCESS)
	}

	applyDefaultValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MiniAudioNoiseGenerator::~MiniAudioNoiseGenerator ()
{
	for(int i = 0; i < kNoiseTypeCount; i++)
		ma_noise_uninit (&noise[i], nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNoiseGenerator::setNoiseType (NoiseType _type)
{
	noiseType = ccl_bound<NoiseType> (_type, kNoiseTypeFirst, kNoiseTypeLast);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNoiseGenerator::setSeed (int32 newSeed)
{
	seed = newSeed;
	for(int i = 0; i < kNoiseTypeCount; i++)
		ma_noise_set_seed (&noise[i], seed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNoiseGenerator::applyValue (int tag, AudioParamValue value)
{
	switch(tag)
	{
	case kNoiseType : setNoiseType (value.asInt ()); break;
	case kSeed : setSeed (value.asInt ()); break;
	default :
		SuperClass::applyValue (tag, value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MiniAudioNoiseGenerator::generate (float32* output, int numSamples)
{
	ma_uint64 framesRead = 0;
	ma_result result = ma_noise_read_pcm_frames (&noise[noiseType], output, ma_uint64(numSamples), &framesRead);
	ASSERT (result == MA_SUCCESS)
	ASSERT (framesRead == ma_uint64(numSamples))
}
