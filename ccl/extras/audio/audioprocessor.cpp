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
// Filename    : ccl/extras/audio/audioprocessor.cpp
// Description : Audio Processor Base Class
//
//************************************************************************************************

#include "ccl/extras/audio/audioprocessor.h"

using namespace CCL;

//************************************************************************************************
// AudioProcessorBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioProcessorBase, AudioComponentBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioProcessorBase::getProcessConfiguration (AudioProcessConfiguration& configuration) const
{
	configuration = processConfiguration;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioProcessorBase::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{
	ASSERT (configuration.isValid ())
	if(!configuration.isValid ())
		return kResultInvalidArgument;

	ASSERT (!isActive ())
	if(isActive ())
		return kResultUnexpected;

	if(processConfiguration != configuration)
	{
		processConfiguration = configuration;
		onConfigurationChanged ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioProcessorBase::isActive () const
{
	return active;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioProcessorBase::setActive (tbool state)
{
	bool newState = state != 0;
	if(active != newState)
	{
		ASSERT (!isProcessing ()) // must not be processing (caller's responsibility)

		ASSERT (isPrepared ()) // must be prepared to activate
		if(newState && !isPrepared ())
			return kResultUnexpected;

		active = newState;
		onActiveChanged (active);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioProcessorBase::isProcessing () const
{
	return processing;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioProcessorBase::setProcessing (tbool state)
{
	bool newState = state != 0;
	if(processing != newState)
	{
		ASSERT (isActive ()) // must be activated to process
		if(newState && !isActive ())
			return kResultUnexpected;

		processing = newState;
		onProcessingChanged (processing);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioProcessorBase::process (const AudioProcessData& data)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioProcessorBase::setDisplayLatency (int latencyInSamples)
{}