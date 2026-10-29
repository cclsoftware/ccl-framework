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
// Filename    : ccl/extras/audio/runtime/audiodevicesetup.h
// Description : Audio Device Setup
//
//************************************************************************************************

#ifndef _ccl_audiodevicesetup_h
#define _ccl_audiodevicesetup_h

#include "ccl/app/component.h"

#include "ccl/public/audio/iaudioservice.h"

namespace CCL {

class AudioDeviceSession;

//************************************************************************************************
// AudioDeviceSetup
/** Setup component for audio device session with parameters to select render/capture device, etc. */
//************************************************************************************************

class AudioDeviceSetup: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (AudioDeviceSetup, Object)

	AudioDeviceSetup (AudioDeviceSession& session, StringRef name = nullptr);
	~AudioDeviceSetup ();

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	class DeviceItem: public Object
	{
	public:
		DeviceItem (IDeviceDescription* description = nullptr);

		PROPERTY_SHARED_AUTO (IDeviceDescription, description, Description)

		// Object
		bool toString (String& string, int flags) const override;
	};

	AudioDeviceSession& session;
	IDeviceEnumerator* deviceEnumerator = nullptr;
	bool wantExclusiveMode = false;
	int requestedBlockSizeUnit = AudioBlockSizeRange::kSampleFrames;

	void updateDeviceLists ();
	void refreshDeviceLists ();
	AudioDeviceComponentDesc getSelectedDeviceDescription () const;
	void makeDevice (const AudioDeviceComponentDesc& description);
	void updateBlockSizeList ();
	void updateStatus ();
};

} // namespace CCL

#endif // _ccl_audiodevicesetup_h
