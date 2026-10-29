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
// Filename    : ccl/extras/audio/audioprocessor.h
// Description : Audio Processor Base Class
//
//************************************************************************************************

#ifndef _ccl_audioprocessor_h
#define _ccl_audioprocessor_h

#include "ccl/extras/audio/audiocomponent.h"

#include "ccl/public/audio/iaudioprocessor.h"

namespace CCL {

//************************************************************************************************
// AudioProcessorBase
//************************************************************************************************

class AudioProcessorBase: public AudioComponentBase,
						  public IAudioProcessor,
						  public IAudioDisplayLatency
{
public:
	DECLARE_CLASS_ABSTRACT (AudioProcessorBase, AudioComponentBase)

	bool isPrepared () const { return processConfiguration.isValid (); }

	// IAudioProcessor
	tbool CCL_API isProcessing () const override;
	tresult CCL_API setProcessing (tbool state) override;
	tresult CCL_API process (const AudioProcessData& data) override;

	// IAudioDisplayLatency
	void CCL_API setDisplayLatency (int latencyInSamples) override;

	// AudioComponentBase
	tresult CCL_API getProcessConfiguration (AudioProcessConfiguration& configuration) const override;
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tbool CCL_API isActive () const override;
	tresult CCL_API setActive (tbool state) override;

	CLASS_INTERFACE2 (IAudioProcessor, IAudioDisplayLatency, AudioComponentBase)

protected:
	AudioProcessConfiguration processConfiguration;
	bool active = false;
	bool processing = false;

	virtual void onConfigurationChanged () {}
	virtual void onActiveChanged (bool state) {}
	virtual void onProcessingChanged (bool state) {}
};

} // namespace CCL

#endif // _ccl_audioprocessor_h