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
// Filename    : ccl/extras/audio/meter/audiometercomponent.h
// Description : Audio Meter Component
//
//************************************************************************************************

#ifndef _ccl_audiometercomponent_h
#define _ccl_audiometercomponent_h

#include "ccl/app/component.h"

#include "ccl/extras/audio/meter/audiometerbuffer.h"

#include "ccl/public/audio/iaudiometer.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

//************************************************************************************************
// AudioMeterComponent
/** Binds an IAudioMeterSource to read-only peak/RMS/peak-hold parameters (in dB) and clip
	parameters, polled on an idle timer and smoothed through an AudioMeterBuffer. */
//************************************************************************************************

class AudioMeterComponent: public Component,
						   public IAudioMeterClient,
						   private IdleClient
{
public:
	DECLARE_CLASS (AudioMeterComponent, Component)

	AudioMeterComponent (StringRef name = nullptr, int maxChannelCount = kMaxAudioMeterChannels);
	~AudioMeterComponent ();

	/** Assign the meter to read from (retained). */
	void setMeterSource (IAudioMeterSource* source);

	/** Enable/disable polling. */
	void setMeterActive (bool state);

	int getMaxChannelCount () const;
	int getChannelCount () const;
	IParameter* getPeakParam (int channel) const;
	IParameter* getRMSParam (int channel) const;
	IParameter* getClipParam (int channel) const;
	IParameter* getPeakHoldParam (int channel) const;

	/** Ballistics/display settings, shareable across multiple instances. */
	AudioMeterSettings& getMeterSettings () const { return meterBuffer->getSettings (); }
	void setMeterSettings (AudioMeterSettings* settings) { meterBuffer->setSettings (settings); }

	AudioMeterBuffer& getMeterBuffer () const { return *meterBuffer; }

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (ITimerTask, Component)

protected:
	static constexpr int kMeterUpdateInterval = 50; // milliseconds

	Vector<IParameter*> peakParams;
	Vector<IParameter*> rmsParams;
	Vector<IParameter*> clipParams;
	Vector<IParameter*> peakHoldParams;
	SharedPtr<IAudioMeterSource> meterSource;
	AutoPtr<AudioMeterBuffer> meterBuffer;
	double lastUpdateTime = 0.;

	void updateParamRange ();
	void syncParamValues ();

	// IAudioMeterClient
	void meterDataChanged (const AudioMeterBuffer& buffer) override;
	void meterSettingsChanged (const AudioMeterBuffer& buffer) override;

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _ccl_audiometercomponent_h
