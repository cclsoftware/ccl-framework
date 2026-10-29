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
// Filename    : audiodevicebase.cpp
// Description : Audio Device Base Classes
//
//************************************************************************************************

#include "audiodevicebase.h"

using namespace CCL;

//************************************************************************************************
// AudioDeviceFactoryBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioDeviceFactoryBase, Object)

//************************************************************************************************
// AudioDeviceBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioDeviceBase, AudioComponentBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceBase::initialize (IUnknown* context)
{
	processCallback = context;
	notificationCallback = context;
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceBase::terminate ()
{
	processCallback.release ();
	notificationCallback.release ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API AudioDeviceBase::getCapabilities () const
{
	return kCapabilityKnownStreamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API AudioDeviceBase::getStreamLayout () const
{
	return streamLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceBase::setDeviceOption (StringID optionId, VariantRef value)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceBase::getDeviceOption (Variant& value, StringID optionId) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioDeviceBase::getBlockSizeRange (AudioBlockSizeRange& range) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AudioDeviceBase::sendNotification (IAudioDeviceNotificationCallback::EventType eventType)
{
	if(notificationCallback)
		notificationCallback->onDeviceNotification (this, eventType);
}
