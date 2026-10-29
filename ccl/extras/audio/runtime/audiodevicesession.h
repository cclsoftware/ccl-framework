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
// Filename    : ccl/extras/audio/runtime/audiodevicesession.h
// Description : Audio Device Session
//
//************************************************************************************************

#ifndef _ccl_audiodevicesession_h
#define _ccl_audiodevicesession_h

#include "ccl/base/object.h"
#include "ccl/base/performance.h"

#include "ccl/public/audio/iaudioservice.h"

namespace CCL {

//************************************************************************************************
// AudioDeviceSession
/**	Owns one audio device instance and a single, replaceable IAudioProcessor to drive it.
	Also an IPerformanceProvider, reporting how much of the realtime budget the processor
	is consuming. */
//************************************************************************************************

class AudioDeviceSession: public Object,
						  public IPerformanceProvider
{
public:
	DECLARE_CLASS (AudioDeviceSession, Object)

	AudioDeviceSession ();
	~AudioDeviceSession ();

	enum State
	{
		kClosed,		///< no device, nothing remembered
		kOpen,			///< device created
		kActive,		///< device is active
		kSuspended		///< device released, but can be restored
	};

	State getState () const;

	bool isAudioCaptureAllowed () const;
	void requestAudioCapturePermission ();

	void setProcessContext (AudioProcessContext* processContext);
	void setProcessor (IAudioProcessor* processor);
	IAudioProcessor* getProcessor () const;

	tresult openDevice (const AudioDeviceComponentDesc& description, bool wantExclusiveMode = false);
	void closeDevice ();
	IAudioComponent* getDevice () const;

	tresult releaseDevice ();
	tresult restoreDevice ();

	tresult setRequestedSampleRate (SampleRate sampleRate);
	SampleRate getRequestedSampleRate () const { return requestedSampleRate; }

	tresult setRequestedBlockSize (int blockSize);
	tresult setRequestedBlockDuration (TSeconds duration);
	int getRequestedBlockSize () const { return requestedBlockSize; }
	TSeconds getRequestedBlockDuration () const { return requestedBlockDuration; }
	void clearRequestedBlockSize ();

	// IPerformanceProvider
	IPerformanceMeter& CCL_API getPerformanceMeter () override { return performanceMeter; }
	void CCL_API setPerformanceMeterActive (tbool state) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IPerformanceProvider, Object)

	DECLARE_STRINGID_MEMBER (kConfigurationChanged) ///< Signaled when the device configuration (sample rate, block size, latency) changed
	DECLARE_STRINGID_MEMBER (kDeviceLost) ///< Signaled after the device has been lost, the session is closed
	DECLARE_STRINGID_MEMBER (kCapturePermissionChanged) ///< Signaled after a request for the audio capture permission has completed

	static int blockDurationToFrames (TSeconds duration, SampleRate sampleRate);

protected:
	class StateGuard;

	class CallbackHandler: public Object,
						   public IAudioProcessCallback,
						   public IAudioDeviceNotificationCallback
	{
	public:
		CallbackHandler (AudioDeviceSession& session);

		// IAudioProcessCallback
		tresult CCL_API processAudio (const AudioCallbackContext& context, 
									  const AudioIOData& ioData) override;
		tresult CCL_API postProcess () override;

		// IAudioDeviceNotificationCallback
		void CCL_API onDeviceNotification (IAudioComponent* component, EventType eventType) override;

		CLASS_INTERFACE2 (IAudioProcessCallback, IAudioDeviceNotificationCallback, Object)

	protected:
		AudioDeviceSession& session;
	};

	class SessionMeter: public PerformanceMeter
	{
	public:
		// PerformanceMeter
		void CCL_API setMaxPeriod (double seconds) override;

	protected:
		static constexpr double kRecoveryTime = 1.;

		int recoveryPeriods = 0;
		int goodPeriods = 0;

		// PerformanceMeter
		void setPerformance (double p) override;
	};

	DECLARE_STRINGID_MEMBER (kDeviceNotification)

	IAudioDeviceFactory* deviceFactory = nullptr;
	AutoPtr<CallbackHandler> callbackHandler;
	AutoPtr<IAudioComponent> device;
	AudioDeviceComponentDesc storedDescription;
	bool storedExclusiveMode = false;
	SampleRate requestedSampleRate = 0;
	int requestedBlockSize = 0;
	TSeconds requestedBlockDuration = 0;
	bool resumeAfterInterruption = false;
	bool capturePermissionPending = false;

	AudioProcessContext* externalContext = nullptr;
	AudioProcessContext fallbackContext;
	SharedPtr<IAudioProcessor> processor;
	SharedPtr<IAudioComponent> processorComponent; // (optional)
	SessionMeter performanceMeter;
	bool performanceMeterActive = false;

	IAudioDeviceFactory* getDeviceFactory ();
	tresult createDevice (const AudioDeviceComponentDesc& description, bool useExclusiveMode);
	void releaseCurrentDevice ();

	bool activateProcessor ();
	void deactivateProcessor ();
	bool makeRequestedConfiguration (AudioProcessConfiguration& configuration, bool force = false) const;
	tresult applyConfiguration (bool force = false);
	void onCapturePermissionResult (bool granted);
};

} // namespace CCL

#endif // _ccl_audiodevicesession_h
