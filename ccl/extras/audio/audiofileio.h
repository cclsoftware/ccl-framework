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
// Filename    : ccl/extras/audio/audiofileio.h
// Description : Audio File I/O
//
//************************************************************************************************

#ifndef _ccl_audiofileio_h
#define _ccl_audiofileio_h

#include "ccl/base/storage/fileresource.h"

#include "ccl/public/audio/iaudioservice.h"

namespace CCL {

//************************************************************************************************
// AudioFileBase
/** Common base for AudioFileSource/AudioFileSink; owns the shared codec factory used to
	decode/encode audio files. */
//************************************************************************************************

class AudioFileBase: public FileStreamResource
{
public:
	DECLARE_CLASS_ABSTRACT (AudioFileBase, FileStreamResource)

	AudioFileBase ();
	AudioFileBase (UrlRef path);
	AudioFileBase (const AudioFileBase&);

	/** Create the shared codec factory; call once at startup. */
	static bool initCodecs (bool registerFileTypes = true);

	/** Release the shared codec factory; call once at shutdown. */
	static void exitCodecs ();
	
	/** Collect the file types the codec factory can decode, encode, or both. */
	static void getFileTypes (FileTypeFilter& fileTypes, int capabilityFilter = 0);

	/** Get file type for .wav. */
	static const FileType& getWaveFileType ();

protected:
	static AutoPtr<IAudioCodecFactory> codecFactory;
};

//************************************************************************************************
// AudioFileSource
/** Reads audio directly from an existing file, decoding via the shared codec factory. */
//************************************************************************************************

class AudioFileSource: public AudioFileBase,
					   public IAudioFormatProvider,
					   public IAudioSource
{
public:
	DECLARE_CLASS (AudioFileSource, AudioFileBase)

	AudioFileSource ();
	AudioFileSource (UrlRef path);
	AudioFileSource (const AudioFileSource&);
	~AudioFileSource ();

	// IAudioFormatProvider
	tresult CCL_API getFormat (AudioContentFormat& format) const override;

	// IAudioSource
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API read (AudioData& data, int sampleFrames) override;
	TSamples CCL_API seek (TSamples position) override;
	TSamples CCL_API tell () const override;

	// AudioFileBase
	tbool CCL_API setPath (UrlRef path) override;

	CLASS_INTERFACE3 (IAudioFormatProvider, IAudioContent, IAudioSource, AudioFileBase)

protected:
	AutoPtr<IAudioSource> decoder;
	int lastOpenMode = 0;

	// AudioFileBase
	bool openFile (int mode) override;
	bool createFile (int mode) override { ASSERT (0) return false; } // read-only
	bool closeFile () override;
};

//************************************************************************************************
// AudioFileSink
/** Writes audio directly to a new file, encoding via the shared codec factory. */
//************************************************************************************************

class AudioFileSink: public AudioFileBase,
					 public IAudioSink
{
public:
	DECLARE_CLASS (AudioFileSink, AudioFileBase)

	AudioFileSink ();
	AudioFileSink (UrlRef path);
	AudioFileSink (const AudioFileSink&);
	~AudioFileSink ();

	/** Must be called before creating file, determines the encoder format. */
	void setFormat (const AudioContentFormat& newFormat) { format = newFormat; }

	// IAudioSink
	int CCL_API getNumChannels () const override;
	SampleRate CCL_API getSampleRate () const override;
	TSamples CCL_API getFrameCount () const override;
	SpeakerBitField CCL_API getSpeakerFormat () const override;
	int CCL_API write (const AudioData& data, int sampleFrames) override;

	// AudioFileBase
	tbool CCL_API setPath (UrlRef path) override;

	CLASS_INTERFACE2 (IAudioContent, IAudioSink, AudioFileBase)

protected:
	AutoPtr<IAudioSink> encoder;
	AudioContentFormat format;
	int lastCreateMode = 0;

	// AudioFileBase
	bool openFile (int mode) override { ASSERT (0) return false; } // write-only
	bool createFile (int mode) override;
	bool closeFile () override;
};

} // namespace CCL

#endif // _ccl_audiofileio_h
