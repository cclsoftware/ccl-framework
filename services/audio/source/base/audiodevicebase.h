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
// Filename    : audiodevicebase.h
// Description : Audio Device Base Classes
//
//************************************************************************************************

#ifndef _audiodevicebase_h
#define _audiodevicebase_h

#include "ccl/extras/audio/audiocomponent.h"

#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/audio/iaudioservice.h"

namespace CCL {

//************************************************************************************************
// AudioDeviceFactoryBase
//************************************************************************************************

class AudioDeviceFactoryBase: public Object,
							  public IAudioDeviceFactory,
							  public PluginInstance
{
public:
	DECLARE_CLASS_ABSTRACT (AudioDeviceFactoryBase, Object)

	CLASS_INTERFACE2 (IAudioDeviceFactory, IPluginInstance, Object)
};

//************************************************************************************************
// AudioDeviceBase
//************************************************************************************************

class AudioDeviceBase: public AudioComponentBase,
					   public IAudioDeviceSpecifics
{
public:
	DECLARE_CLASS_ABSTRACT (AudioDeviceBase, AudioComponentBase)

	// IAudioDeviceSpecifics
	tresult CCL_API setDeviceOption (StringID optionId, VariantRef value) override;
	tresult CCL_API getDeviceOption (Variant& value, StringID optionId) const override;
	tresult CCL_API getBlockSizeRange (AudioBlockSizeRange& range) const override;

	// AudioComponentBase
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	CapabilityFlags CCL_API getCapabilities () const override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;

	CLASS_INTERFACE (IAudioDeviceSpecifics, AudioComponentBase)

protected:
	AudioStreamLayout streamLayout;
	UnknownPtr<IAudioProcessCallback> processCallback;
	UnknownPtr<IAudioDeviceNotificationCallback> notificationCallback;	

	void sendNotification (IAudioDeviceNotificationCallback::EventType eventType);
};

} // namespace CCL

#endif // _audiodevicebase_h