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
// Filename    : ccl/extras/audio/runtime/audioplayercomponent.h
// Description : Audio Player Component
//
//************************************************************************************************

#ifndef _ccl_audioplayercomponent_h
#define _ccl_audioplayercomponent_h

#include "ccl/app/component.h"

#include "ccl/extras/audio/runtime/audioplayer.h"
#include "ccl/extras/audio/waveform/audiowaveform.h"

namespace CCL {

//************************************************************************************************
// AudioPlayerComponent
/**	Wraps an AudioPlayer with parameters for file name/info and creates an AudioWaveformView
	of the loaded file.

	Rendering is left to the host application. Use getPlayer () to call (un)prepareRender ()/
	renderAudio () from the host's own audio device callback, or to add it to an audio graph. */
//************************************************************************************************

class AudioPlayerComponent: public Component
{
public:
	DECLARE_CLASS (AudioPlayerComponent, Component)

	AudioPlayerComponent (StringRef name = nullptr);

	/** Assign shared transport (optional). */
	void setTransport (IAudioTransport* transport);

	/** Load an audio file into the player and waveform. */
	bool loadFile (UrlRef path);

	/** Unload current file. */
	void unloadFile ();

	/** Access the underlying audio player. */
	AudioPlayer& getPlayer () const { return *player; }

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tresult CCL_API terminate () override;

protected:
	AutoPtr<AudioPlayer> player;
	AutoPtr<AudioWaveform> waveform;
	SharedPtr<IAudioTransport> transport;
};

} // namespace CCL

#endif // _ccl_audioplayercomponent_h
