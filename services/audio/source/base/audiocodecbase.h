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
// Filename    : audiocodecbase.h
// Description : Audio Codec Base Classes
//
//************************************************************************************************

#ifndef _audiocodecbase_h
#define _audiocodecbase_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/audio/iaudioservice.h"

namespace CCL {

class AudioDecoderBase;
class AudioEncoderBase;

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef AudioDecoderBase* (*AudioDecoderConstructor) (IStream& stream, const AudioContentFormat& format);
typedef AudioEncoderBase* (*AudioEncoderConstructor) (IStream& stream, const AudioContentFormat& format);

//************************************************************************************************
// AudioCodecFactoryBase
//************************************************************************************************

class AudioCodecFactoryBase: public Object,
							 public IAudioCodecFactory,
							 public PluginInstance
{
public:
	DECLARE_CLASS (AudioCodecFactoryBase, Object)

	void addCodec (const AudioCodecDesc& description,
				   AudioDecoderConstructor decoderConstructor,
				   AudioEncoderConstructor encoderConstructor);

	// IAudioCodecFactory
	int CCL_API getCodecCount () const override;
	tresult CCL_API getCodecDescription (AudioCodecDesc& description, int index) const override;	
	tresult CCL_API createDecoderInstance (IAudioSource*& source, IStream& stream,
										  StringRef contentType, const AudioContentFormat& format) override;
	tresult CCL_API createEncoderInstance (IAudioSink*& sink, IStream& stream,
										   StringRef contentType, const AudioContentFormat& format) override;

	CLASS_INTERFACE2 (IAudioCodecFactory, IPluginInstance, Object)

protected:
	struct CodecClass
	{
		AudioCodecDesc description;
		AudioDecoderConstructor decoderConstructor = nullptr;
		AudioEncoderConstructor encoderConstructor = nullptr;
	};

	Vector<CodecClass> codecList;

	const CodecClass* findCodec (StringRef contentType) const;
};

//************************************************************************************************
// AudioDecoderBase
//************************************************************************************************

class AudioDecoderBase: public Object,
						public IAudioFormatProvider,
						public IAudioSource
						
{
public:
	DECLARE_CLASS (AudioDecoderBase, Object)

	// IAudioFormatProvider
	tresult CCL_API getFormat (AudioContentFormat& format) const override { return kResultNotImplemented; }

	// IAudioSource
	int CCL_API getNumChannels () const override { return 0; }
	SampleRate CCL_API getSampleRate () const override { return 0; }
	TSamples CCL_API getFrameCount () const override { return 0; }
	SpeakerBitField CCL_API getSpeakerFormat () const override { return SpeakerDef::kEmpty; }
	int CCL_API read (AudioData& buffer, int sampleFrames) override { return -1; }
	TSamples CCL_API seek (TSamples position) override { return -1; }
	TSamples CCL_API tell () const override { return -1; }

	CLASS_INTERFACE3 (IAudioFormatProvider, IAudioContent, IAudioSource, Object)
};

//************************************************************************************************
// AudioEncoderBase
//************************************************************************************************

class AudioEncoderBase: public Object,
						public IAudioSink
{
public:
	DECLARE_CLASS (AudioEncoderBase, Object)

	// IAudioSink
	int CCL_API getNumChannels () const override { return 0; }
	SampleRate CCL_API getSampleRate () const override { return 0; }
	TSamples CCL_API getFrameCount () const override { return 0; }
	SpeakerBitField CCL_API getSpeakerFormat () const override { return SpeakerDef::kEmpty; }
	int CCL_API write (const AudioData& buffer, int sampleFrames) override { return -1; }

	CLASS_INTERFACE2 (IAudioContent, IAudioSink, Object)
};

} // namespace CCL

#endif // _audiocodecbase_h