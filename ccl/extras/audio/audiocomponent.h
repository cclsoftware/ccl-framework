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
// Filename    : ccl/extras/audio/audiocomponent.h
// Description : Audio Component Base Class
//
//************************************************************************************************

#ifndef _ccl_audiocomponent_h
#define _ccl_audiocomponent_h

#include "ccl/extras/audio/audiostreamlayout.h"

#include "ccl/public/audio/iaudiocomponent.h"

namespace CCL {

//************************************************************************************************
// AudioComponentBase
//************************************************************************************************

class AudioComponentBase: public Object,
						  public IAudioComponent
{
public:
	DECLARE_CLASS_ABSTRACT (AudioComponentBase, Object)

	// IAudioComponent
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;
	CapabilityFlags CCL_API getCapabilities () const override;
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API setStreamActive (AudioStreamDirection direction, int index, tbool active) override;
	tresult CCL_API getProcessConfiguration (AudioProcessConfiguration& configuration) const override;
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API getLatency (int& latencyInSamples, AudioStreamDirection direction) const override;
	tbool CCL_API isActive () const override;
	tresult CCL_API setActive (tbool state) override;

	CLASS_INTERFACE2 (IComponent, IAudioComponent, Object)

protected:
	SharedPtr<IUnknown> hostContext;
};

} // namespace CCL

#endif // _ccl_audiocomponent_h