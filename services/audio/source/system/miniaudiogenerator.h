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
// Filename    : services/audio/source/system/miniaudiogenerator.h
// Description : Audio Generators based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiogenerator_h
#define _miniaudiogenerator_h

#include "miniaudioglue.h"

#include "ccl/extras/audio/audioprocessor.h"

#include "ccl/public/audio/audioparamids.h"
#include "ccl/public/gui/iparamprovider.h"
#include "ccl/public/plugins/pluginst.h"

namespace CCL {

//************************************************************************************************
// MiniAudioGenerator
/** Shared base class for signal generators. */
//************************************************************************************************

class MiniAudioGenerator: public AudioProcessorBase,
						  public IParameterDefinitionProvider,
						  public PluginInstance
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioGenerator, AudioProcessorBase)

	static constexpr SampleRate kDefaultSampleRate = 44100.;
	static constexpr TSeconds kRampTime = 0.005; ///< max. duration of gain ramps (5 ms)

	MiniAudioGenerator (const ParameterDefinitionList& paramList);

	// IParameterDefinitionProvider
	int CCL_API countParameters () const override;
	tresult CCL_API getParameterAt (ParameterDefinition& definition, int index) const override;

	// AudioProcessorBase
	CapabilityFlags CCL_API getCapabilities () const override;
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE2 (IParameterDefinitionProvider, IPluginInstance, AudioProcessorBase)

protected:
	static void addBaseParameterDefinitions (ParameterDefinitionList& paramList);

	enum ParamTag
	{
		kOn = 1,
		kAmplitude,
		kGeneratorLast = kAmplitude
	};

	AudioStreamLayout streamLayout;
	const ParameterDefinitionList& paramList;
	bool on = false;
	float amplitude = 1.f;
	float appliedGain = 0.f;

	void applyDefaultValues ();

	void setOn (bool state);
	void setAmplitude (float amplitude);

	virtual void applyValue (int tag, AudioParamValue value);
	virtual void generate (float32 output[], int numSamples) = 0;
};

//************************************************************************************************
// MiniAudioOscillator
//************************************************************************************************

class MiniAudioOscillator: public MiniAudioGenerator
{
public:
	DECLARE_CLASS (MiniAudioOscillator, MiniAudioGenerator)

	MiniAudioOscillator ();
	~MiniAudioOscillator ();

	using WaveType = AudioParamID::Oscillator::WaveType;
	
	static constexpr WaveType kSine = AudioParamID::Oscillator::kSine;
	static constexpr WaveType kSquare = AudioParamID::Oscillator::kSquare;
	static constexpr WaveType kTriangle = AudioParamID::Oscillator::kTriangle;
	static constexpr WaveType kSawtooth = AudioParamID::Oscillator::kSawtooth;
	
	static constexpr WaveType kWaveTypeFirst = kSine;
	static constexpr WaveType kWaveTypeLast = kSawtooth;

protected:
	static ma_waveform_type toMaWaveType (WaveType type);
	static const CStringPtr kWaveTypeStrings[];
	static const ParameterDefinitionList& getParamList ();

	enum ParamTag
	{
		kWaveType = kGeneratorLast + 1,
		kFrequency
	};

	ma_waveform waveform = {};
	WaveType waveType = kSine;
	float frequency = 440.f;

	void setWaveType (WaveType type);
	void setFrequency (float frequency);

	// MiniAudioGenerator
	void onConfigurationChanged () override;
	void applyValue (int tag, AudioParamValue value) override;
	void generate (float32* output, int numSamples) override;
};

//************************************************************************************************
// MiniAudioNoiseGenerator
//************************************************************************************************

class MiniAudioNoiseGenerator: public MiniAudioGenerator
{
public:
	DECLARE_CLASS (MiniAudioNoiseGenerator, MiniAudioGenerator)

	MiniAudioNoiseGenerator ();
	~MiniAudioNoiseGenerator ();

	using NoiseType = AudioParamID::NoiseGenerator::NoiseType;
	
	static constexpr NoiseType kWhite = AudioParamID::NoiseGenerator::kWhite;
	static constexpr NoiseType kPink = AudioParamID::NoiseGenerator::kPink;
	static constexpr NoiseType kBrown = AudioParamID::NoiseGenerator::kBrown;

	static constexpr NoiseType kNoiseTypeFirst = kWhite;
	static constexpr NoiseType kNoiseTypeLast = kBrown;

protected:
	static ma_noise_type toMaNoiseType (NoiseType type);
	static const CStringPtr kNoiseTypeStrings[];
	static const ParameterDefinitionList& getParamList ();

	static constexpr int kNoiseTypeCount = kNoiseTypeLast - kNoiseTypeFirst + 1;

	enum ParamTag
	{
		kNoiseType = kGeneratorLast + 1,
		kSeed
	};

	ma_noise noise[kNoiseTypeCount] = {};
	NoiseType noiseType = kWhite;
	int32 seed = 0;

	void setNoiseType (NoiseType type);
	void setSeed (int32 seed);

	// MiniAudioGenerator
	void applyValue (int tag, AudioParamValue value) override;
	void generate (float32* output, int numSamples) override;
};

} // namespace CCL

#endif // _miniaudiogenerator_h
