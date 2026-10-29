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
// Filename    : ccl/public/audio/iaudioservice.h
// Description : Audio Service Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudioservice_h
#define _ccl_iaudioservice_h

#include "ccl/public/storage/filetype.h"

#include "ccl/public/devices/ideviceenumerator.h"

#include "ccl/public/audio/iaudiocallback.h"
#include "ccl/public/audio/iaudiocomponent.h"
#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/audio/iaudiograph.h"

namespace CCL {
	
interface IStream;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_AUDIOSERVICE_NAME_	"CCL Audio Service"
#define CCL_AUDIOSERVICE_NAME	CCLSTR (CCL_AUDIOSERVICE_NAME_)

#define PLUG_CATEGORY_AUDIOSERVICEUTILITY CCLSTR ("AudioServiceUtility")

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Audio Device Enumerator [IDeviceEnumerator] */
	DEFINE_CID (AudioDeviceEnumerator, 0x3c37a1d0, 0x323b, 0x4c67, 0x9f, 0xc7, 0xaf, 0xa7, 0x25, 0x17, 0x71, 0xf3)

	/** Audio Device Factory [IAudioDeviceFactory] */
	DEFINE_CID (AudioDeviceFactory, 0xf7638536, 0x25a8, 0x46b6, 0xbc, 0x67, 0xdd, 0xdf, 0xe8, 0x2b, 0x3b, 0x1)

	/** Audio Codec Factory [IAudioCodecFactory] */
	DEFINE_CID (AudioCodecFactory, 0xe31f00e7, 0x4007, 0x4592, 0xb7, 0xaa, 0xc7, 0x4, 0xfe, 0xe4, 0x88, 0x6c)

	/** Buffered Audio Source [IBufferedAudioSource] */
	DEFINE_CID (BufferedAudioSource, 0x7f7f3392, 0xec2, 0x4b64, 0x94, 0x56, 0x80, 0xf5, 0xb1, 0xb0, 0x9c, 0x4d)

	/** Buffered Audio Sink [IBufferedAudioSink] */
	DEFINE_CID (BufferedAudioSink, 0x5a34cae8, 0x11b7, 0x4ac0, 0x8e, 0x45, 0x29, 0x7f, 0xd1, 0x6b, 0x93, 0x02)

	/** Resampling Audio Source [IResamplingAudioSource] */
	DEFINE_CID (ResamplingAudioSource, 0xb5b1be09, 0xeeb7, 0x4b2f, 0x93, 0xd4, 0xcb, 0xed, 0x10, 0x50, 0x5c, 0x11)

	/** Resampling Audio Sink [IResamplingAudioSink] */
	DEFINE_CID (ResamplingAudioSink, 0xfd79ec19, 0x9b88, 0x4452, 0xbe, 0x55, 0x82, 0x0b, 0xeb, 0x84, 0x6b, 0x86)

	/** Converting Audio Source [IConvertingAudioSource] */
	DEFINE_CID (ConvertingAudioSource, 0xd41c8a6f, 0x7e29, 0x4f03, 0xae, 0x51, 0x63, 0x2b, 0x9d, 0x11, 0xf4, 0x88)

	/** Converting Audio Sink [IConvertingAudioSink] */
	DEFINE_CID (ConvertingAudioSink, 0x6b92e857, 0x1a4d, 0x4c76, 0x8f, 0x3e, 0x0a, 0x5c, 0x77, 0x21, 0xb6, 0x9d)

	/** Audio Graph [IAudioGraph] */
	DEFINE_CID (AudioGraph, 0xd385faec, 0xad76, 0x4136, 0xa4, 0x9d, 0xda, 0x10, 0x84, 0xc4, 0xbb, 0x23)

	/** Audio Source Node [IAudioSourceNode] */
	DEFINE_CID (AudioSourceNode, 0x7bc0be5d, 0x8d52, 0x4c4c, 0x81, 0xc7, 0xea, 0xbc, 0x56, 0xf8, 0xfd, 0xc0)

	/** Audio Gain Node [IAudioGainNode] */
	DEFINE_CID (AudioGainNode, 0x9e4a1d7c, 0x3b8f, 0x4a1e, 0x9c, 0x02, 0x5f, 0x3d, 0x8e, 0x21, 0x6a, 0x44)

	/** Audio Input Node [IAudioInputNode] */
	DEFINE_CID (AudioInputNode, 0x4b7e2a90, 0x6f14, 0x4c8d, 0x8a, 0x21, 0x9d, 0x6c, 0x3f, 0x50, 0xb7, 0x1e)

	/** Audio Output Node [IAudioOutputNode] */
	DEFINE_CID (AudioOutputNode, 0xc2915e3a, 0x7d68, 0x4f21, 0xb6, 0x4c, 0x1a, 0x8e, 0x53, 0xd9, 0x0c, 0x72)

	/** Audio Pan Node [IAudioPanNode] */
	DEFINE_CID (AudioPanNode, 0x3f6a1d2c, 0x8e57, 0x4b93, 0xa1, 0x0d, 0x6c, 0x2f, 0x91, 0x4e, 0x7b, 0x38)

	/** Audio Meter Node [IAudioMeterNode] */
	DEFINE_CID (AudioMeterNode, 0x8d24c6a1, 0x4f39, 0x4b78, 0xa2, 0x6e, 0x5d, 0x91, 0x3c, 0x7a, 0x4e, 0x02)

	/** Audio Processor Node [IAudioProcessorNode] */
	DEFINE_CID (AudioProcessorNode, 0x63837b95, 0x9266, 0x48d4, 0x86, 0x12, 0x37, 0x4b, 0x25, 0xb6, 0x11, 0x0a)

	/** Audio Oscillator [IAudioComponent/IAudioProcessor] */
	DEFINE_CID (AudioOscillator, 0x1e825a4f, 0x6d93, 0x4c7b, 0x9e, 0x02, 0x5a, 0x8f, 0x3c, 0x61, 0x7d, 0x24)

	/** Audio Noise Generator [IAudioComponent/IAudioProcessor] */
	DEFINE_CID (AudioNoiseGenerator, 0x7b4c9e21, 0xa835, 0x4f6d, 0x9c, 0x71, 0x0e, 0x4a, 0x2d, 0x8b, 0x53, 0x6f)
}

//************************************************************************************************
// AudioDeviceComponentDesc
//************************************************************************************************

struct AudioDeviceComponentDesc
{
	String renderDeviceId;
	String captureDeviceId;
	int capabilities = 0;

	bool hasRenderId () const;
	bool hasCaptureId () const;
	int getDuplexCapability () const;

	bool isRenderOnly () const;
	bool isCaptureOnly () const;
	bool isFullDuplex () const;

	bool isValid () const;
};

//************************************************************************************************
// AudioDeviceOptions
//************************************************************************************************

namespace AudioDeviceOptions
{
	/**	Check if audio device supports exclusive mode [tbool, read-only]
		(Windows WASAPI only). */
	DEFINE_STRINGID (kCanExclusiveMode, "canExclusiveMode")

	/**	Turn exclusive mode on or off [tbool, read/write], device must be re-initialized to apply
		(Windows WASAPI only). */
	DEFINE_STRINGID (kExclusiveMode, "exclusiveMode")
}

//************************************************************************************************
// IAudioDeviceFactory
//************************************************************************************************

interface IAudioDeviceFactory: IUnknown
{	
	/**	Create new audio device component instance.
	 
		Specify render and/or capture device identifiers from enumeration, see IDeviceEnumerator.
		If device identitifers are empty, system defaults are selected. 
		Optionally specificy which capabilites (render/capture) are requested (see IDeviceDescription). 
	*/
	virtual tresult CCL_API createInstance (IAudioComponent*& component,
											const AudioDeviceComponentDesc& description) = 0;

	DECLARE_IID (IAudioDeviceFactory)
};

DEFINE_IID (IAudioDeviceFactory, 0x5b13799e, 0xd339, 0x462f, 0xb9, 0x7d, 0x37, 0xce, 0x27, 0xde, 0xc2, 0x6a)

//************************************************************************************************
// AudioContentTypes
//************************************************************************************************

namespace AudioContentTypes
{
	DEFINE_STRINGID (kWave, "audio/wav")
	DEFINE_STRINGID (kFLAC, "audio/flac")
	DEFINE_STRINGID (kMP3, "audio/mpeg")
}

//************************************************************************************************
// AudioCodecDesc
//************************************************************************************************

struct AudioCodecDesc
{
	static constexpr int kCanDecode = 1<<0;
	static constexpr int kCanEncode = 1<<1;

	int capabilities = 0;
	FileType fileType;

	bool canDecode () const;
	bool canEncode () const;
};

//************************************************************************************************
// IAudioCodecFactory
/** Creates IAudioSource/IAudioSink instances for decoding/encoding an IStream. */
//************************************************************************************************

interface IAudioCodecFactory: IUnknown
{
	/** Get number of built-in codecs. */
	virtual int CCL_API getCodecCount () const = 0;

	/** Get description of built-in codec at given index. */
	virtual tresult CCL_API getCodecDescription (AudioCodecDesc& description, int index) const = 0;

	/**	Create new IAudioSource for decoding from the given stream.
		The stream is retained for the lifetime of the source. */
	virtual tresult CCL_API createDecoderInstance (IAudioSource*& source, IStream& stream,
												   StringRef contentType, const AudioContentFormat& format) = 0;

	/**	Create new IAudioSink for encoding into the given stream.
		The stream is retained for the lifetime of the sink. */
	virtual tresult CCL_API createEncoderInstance (IAudioSink*& sink, IStream& stream,
												   StringRef contentType, const AudioContentFormat& format) = 0;

	DECLARE_IID (IAudioCodecFactory)
};

DEFINE_IID (IAudioCodecFactory, 0x5e98e2e1, 0x3cf1, 0x4b5a, 0x93, 0xc3, 0x3c, 0x25, 0x74, 0x68, 0x14, 0xe8)

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioDeviceComponentDesc inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioDeviceComponentDesc::hasRenderId () const { return !renderDeviceId.isEmpty (); }
inline bool AudioDeviceComponentDesc::hasCaptureId () const { return !captureDeviceId.isEmpty (); }

inline int AudioDeviceComponentDesc::getDuplexCapability () const
{
	return capabilities & IDeviceDescription::kMediaDuplexCapability;
}

inline bool AudioDeviceComponentDesc::isRenderOnly () const
{
	return	(hasRenderId () == true && hasCaptureId () == false) ||
			(getDuplexCapability () == IDeviceDescription::kMediaRenderCapability);
}

inline bool AudioDeviceComponentDesc::isCaptureOnly () const
{
	return	(hasRenderId () == false && hasCaptureId () == true) ||
			(getDuplexCapability () == IDeviceDescription::kMediaCaptureCapability);
}

inline bool AudioDeviceComponentDesc::isFullDuplex () const 
{
	return	(hasRenderId () == true && hasCaptureId () == true) ||
			(getDuplexCapability () == IDeviceDescription::kMediaDuplexCapability);
}

inline bool AudioDeviceComponentDesc::isValid () const
{
	return isRenderOnly () || isCaptureOnly () || isFullDuplex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioCodecDesc inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioCodecDesc::canDecode () const
{
	return get_flag (capabilities, kCanDecode);
}

inline bool AudioCodecDesc::canEncode () const
{
	return get_flag (capabilities, kCanEncode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iaudioservice_h
