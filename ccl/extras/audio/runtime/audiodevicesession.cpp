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
// Filename    : ccl/extras/audio/runtime/audiodevicesession.cpp
// Description : Audio Device Session
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiodevicesession.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// AudioDeviceSession::StateGuard
//************************************************************************************************

class AudioDeviceSession::StateGuard
{
public:
	StateGuard (AudioDeviceSession& session)
	: session (session),
	  oldState (session.getState ())
	{}

	~StateGuard ()
	{
		State newState = session.getState ();
		if(newState != oldState)
			session.signal (Message (kChanged));
	}

protected:
	AudioDeviceSession& session;
	State oldState;
};

//************************************************************************************************
// AudioDeviceSession::CallbackHandler
//************************************************************************************************

AudioDeviceSession::CallbackHandler::CallbackHandler (AudioDeviceSession& session)
: session (session)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceSession::CallbackHandler::processAudio (const AudioCallbackContext& context, 
																   const AudioIOData& ioData)
{
	if(!session.processor)
		return kResultOk;

	PerformanceGuard performanceGuard (session.performanceMeterActive ? &session.performanceMeter : nullptr);
	
	if(get_flag (context.flags, AudioCallbackContext::kOverload))
		session.performanceMeter.setOverload (true);

	AudioProcessContext& processContext = session.externalContext ?
										  *session.externalContext : 
										  session.fallbackContext;
	
	processContext.systemTime = context.systemTime;
	processContext.samplePos = context.samplePos;

	return session.processor->process ({sizeof(AudioProcessData), processContext, ioData, {}});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceSession::CallbackHandler::postProcess ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioDeviceSession::CallbackHandler::onDeviceNotification (IAudioComponent* component, EventType eventType)
{
	(NEW Message (kDeviceNotification, eventType))->post (&session);
}

//************************************************************************************************
// AudioDeviceSession::SessionMeter
//************************************************************************************************

void CCL_API AudioDeviceSession::SessionMeter::setMaxPeriod (double seconds)
{
	PerformanceMeter::setMaxPeriod (seconds);

	recoveryPeriods = ccl_max (1, int(ccl_round<0> (kRecoveryTime / seconds)));
	goodPeriods = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::SessionMeter::setPerformance (double p)
{
	if(overload && p < 1.)
	{
		if(++goodPeriods >= recoveryPeriods)
		{
			// back within the time budget: clear the overload and start from the current value
			overload = false;
			overAccumulated = 0.;
			performance = p;
			goodPeriods = 0;
		}
	}
	else
		goodPeriods = 0;

	PerformanceMeter::setPerformance (p);
}

//************************************************************************************************
// AudioDeviceSession
//************************************************************************************************

int AudioDeviceSession::blockDurationToFrames (TSeconds duration, SampleRate sampleRate)
{
	return int(ccl_round<0> (duration * sampleRate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AudioDeviceSession, Object)
DEFINE_STRINGID_MEMBER_ (AudioDeviceSession, kConfigurationChanged, "ConfigurationChanged")
DEFINE_STRINGID_MEMBER_ (AudioDeviceSession, kDeviceLost, "DeviceLost")
DEFINE_STRINGID_MEMBER_ (AudioDeviceSession, kCapturePermissionChanged, "CapturePermissionChanged")
DEFINE_STRINGID_MEMBER_ (AudioDeviceSession, kDeviceNotification, "DeviceNotification")

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceSession::AudioDeviceSession ()
{
	callbackHandler = NEW CallbackHandler (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceSession::~AudioDeviceSession ()
{
	closeDevice ();

	cancelSignals ();

	ccl_release (deviceFactory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioDeviceFactory* AudioDeviceSession::getDeviceFactory ()
{
	if(!deviceFactory)
	{
		deviceFactory = ccl_new<IAudioDeviceFactory> (ClassID::AudioDeviceFactory);
		ASSERT (deviceFactory) // audio service must be started
	}
	return deviceFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceSession::State AudioDeviceSession::getState () const
{
	if(device)
	{
		if(device->isActive ())
			return kActive;
		else
			return kOpen;
	}
	else
	{
		if(storedDescription.isValid ())
			return kSuspended;
		else
			return kClosed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::openDevice (const AudioDeviceComponentDesc& description, bool wantExclusiveMode)
{
	StateGuard stateGuard (*this);

	tresult result = createDevice (description, wantExclusiveMode);
	if(result != kResultOk && wantExclusiveMode)
		result = createDevice (description, false);

	if(result == kResultOk)
	{
		storedDescription = description;
		storedExclusiveMode = wantExclusiveMode;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::closeDevice ()
{
	StateGuard stateGuard (*this);

	releaseCurrentDevice ();

	storedDescription = AudioDeviceComponentDesc ();
	storedExclusiveMode = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::releaseDevice ()
{
	State state = getState ();
	ASSERT (state == kOpen || state == kActive)
	if(!(state == kOpen || state == kActive))
		return kResultUnexpected;

	StateGuard stateGuard (*this);

	releaseCurrentDevice ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::restoreDevice ()
{
	State state = getState ();
	
	ASSERT (state == kSuspended)
	if(state != kSuspended)
		return kResultUnexpected;

	return openDevice (storedDescription, storedExclusiveMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent* AudioDeviceSession::getDevice () const
{
	return device;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::setRequestedSampleRate (SampleRate sampleRate)
{
	requestedSampleRate = sampleRate;
	return applyConfiguration ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::setRequestedBlockSize (int blockSize)
{
	requestedBlockSize = blockSize;
	requestedBlockDuration = 0;
	return applyConfiguration ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::setRequestedBlockDuration (TSeconds duration)
{
	requestedBlockDuration = duration;
	requestedBlockSize = 0;
	return applyConfiguration ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::clearRequestedBlockSize ()
{
	requestedBlockSize = 0;
	requestedBlockDuration = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDeviceSession::isAudioCaptureAllowed () const
{
	// unknown: nothing to ask for on this platform, just try
	ISystemPermissionProvider& permissions = System::GetSystemShell ().getPermissions ();
	PermissionState state = permissions.getPermissionState (System::kPermissionAudioCapture);
	return state == System::kPermissionGranted || state == System::kPermissionUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::requestAudioCapturePermission ()
{
	if(capturePermissionPending)
		return;

	ISystemPermissionProvider& permissions = System::GetSystemShell ().getPermissions ();
	PermissionState state = permissions.getPermissionState (System::kPermissionAudioCapture);
	if(state == System::kPermissionUndetermined)
	{
		capturePermissionPending = true;

		SharedPtr<AudioDeviceSession> self (this); // stay alive until the user has answered
		Promise (permissions.requestPermission (System::kPermissionAudioCapture)).then ([self] (IAsyncOperation& operation)
		{
			self->onCapturePermissionResult (operation.getResult ().asInt () == System::kPermissionGranted);
		});
	}
	else if(state == System::kPermissionDenied)
		signal (Message (kCapturePermissionChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::onCapturePermissionResult (bool granted)
{
	capturePermissionPending = false;

	// an open device does not pick up the new permission, create it again
	if(granted && device && !storedDescription.captureDeviceId.isEmpty ())
	{
		AudioDeviceComponentDesc description = storedDescription;
		openDevice (description, storedExclusiveMode);
	}

	signal (Message (kCapturePermissionChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDeviceSession::makeRequestedConfiguration (AudioProcessConfiguration& configuration, bool force) const
{
	if(!force && requestedSampleRate <= 0 && requestedBlockSize <= 0 && requestedBlockDuration <= 0)
		return false;

	device->getProcessConfiguration (configuration);

	if(requestedSampleRate > 0)
		configuration.sampleRate = requestedSampleRate;

	if(requestedBlockSize > 0)
		configuration.maxBlockSize = requestedBlockSize;
	else if(requestedBlockDuration > 0)
		configuration.maxBlockSize = blockDurationToFrames (requestedBlockDuration, configuration.sampleRate);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::applyConfiguration (bool force)
{
	if(!device)
		return kResultOk; // remembered, applied the next time a device is created

	StateGuard stateGuard (*this);

	bool wasActive = device->isActive ();
	if(wasActive)
		device->setActive (false);

	deactivateProcessor ();

	tresult result = kResultOk;
	AudioProcessConfiguration configuration;
	if(makeRequestedConfiguration (configuration, force))
		result = device->setProcessConfiguration (configuration);

	if(activateProcessor ())
	{
		if(wasActive)
			device->setActive (true);
	}

	signal (Message (kConfigurationChanged));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AudioDeviceSession::createDevice (const AudioDeviceComponentDesc& description, bool useExclusiveMode)
{
	releaseCurrentDevice ();

	IAudioComponent* newDevice = nullptr;
	if(getDeviceFactory ())
		getDeviceFactory ()->createInstance (newDevice, description);
	if(!newDevice)
		return kResultFailed;

	if(useExclusiveMode)
	{
		if(UnknownPtr<IAudioDeviceSpecifics> audioDeviceSpecifics = newDevice)
			audioDeviceSpecifics->setDeviceOption (AudioDeviceOptions::kExclusiveMode, true);
	}

	tresult result = newDevice->initialize (callbackHandler->asUnknown ());
	if(result != kResultOk)
	{
		newDevice->release ();
		return result;
	}

	device = newDevice;

	AudioProcessConfiguration configuration;
	if(makeRequestedConfiguration (configuration))
		device->setProcessConfiguration (configuration); // best-effort, mismatch tolerated

	if(activateProcessor ())
	{
		result = device->setActive (true);
		ASSERT (result == kResultOk)
	}

	signal (Message (kConfigurationChanged));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::releaseCurrentDevice ()
{
	if(device)
	{
		if(device->isActive ())
			device->setActive (false);

		device->terminate ();
		device.release ();
		resumeAfterInterruption = false;

		cancelSignals ();

		deactivateProcessor ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::setProcessContext (AudioProcessContext* processContext)
{
	ASSERT (getState () != kActive)

	externalContext = processContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::setProcessor (IAudioProcessor* newProcessor)
{
	StateGuard stateGuard (*this);

	if(processor != newProcessor)
	{
		if(device)
		{
			if(device->isActive ())
				device->setActive (false);

			deactivateProcessor ();
		}

		UnknownPtr<IAudioComponent> newComponent (newProcessor);

		processor = newProcessor;
		processorComponent = newComponent;

		if(device)
		{
			if(activateProcessor ())
				device->setActive (true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioProcessor* AudioDeviceSession::getProcessor () const
{
	return processor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AudioDeviceSession::activateProcessor ()
{
	if(!processor)
		return false;

	AudioProcessConfiguration configuration;
	device->getProcessConfiguration (configuration);
	ASSERT (configuration.isValid ())

	performanceMeter.setMaxPeriod (double(configuration.maxBlockSize) / configuration.sampleRate);

	if(processorComponent.isValid ())
	{
		processorComponent->setProcessConfiguration (configuration);
		fallbackContext.sampleRate = configuration.sampleRate;

		if(UnknownPtr<IAudioDisplayLatency> displayLatency = processor)
		{
			int renderLatency = 0;
			device->getLatency (renderLatency, kAudioRenderDirection);
			displayLatency->setDisplayLatency (renderLatency);
		}

		processorComponent->setActive (true);
	}

	processor->setProcessing (true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioDeviceSession::setPerformanceMeterActive (tbool state)
{
	performanceMeterActive = state;
	if(!state)
		performanceMeter.reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceSession::deactivateProcessor ()
{
	if(processor)
		processor->setProcessing (false);

	if(processorComponent.isValid ())
		processorComponent->setActive (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioDeviceSession::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDeviceNotification)
	{
		auto eventType = static_cast<IAudioDeviceNotificationCallback::EventType> (msg[0].asInt ());
		if(!device)
			return;

		switch(eventType)
		{
		case IAudioDeviceNotificationCallback::kDeviceLost :
			closeDevice (); // there's nothing to restore for a device that is gone
			signal (Message (kDeviceLost));
			break;

		case IAudioDeviceNotificationCallback::kConfigurationChanged :
			applyConfiguration (true);
			break;

		case IAudioDeviceNotificationCallback::kInterruptionBegan :
			if(device->isActive ())
			{
				StateGuard stateGuard (*this);
				resumeAfterInterruption = true;
				device->setActive (false);
			}
			break;

		case IAudioDeviceNotificationCallback::kInterruptionEnded :
			if(resumeAfterInterruption)
			{
				StateGuard stateGuard (*this);
				resumeAfterInterruption = false;
				device->setActive (true);
			}
			break;

		case IAudioDeviceNotificationCallback::kDeviceUnlocked :
			signal (Message (kChanged));
			break;
		}
	}
	else
		SuperClass::notify (subject, msg);
}
