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
// Filename    : ccl/extras/audio/audiocomponent.cpp
// Description : Audio Component Base Class
//
//************************************************************************************************

#include "ccl/extras/audio/audiocomponent.h"

using namespace CCL;

//************************************************************************************************
// AudioComponentBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioComponentBase, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::initialize (IUnknown* context)
{
	hostContext = context;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::terminate ()
{
	hostContext.release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioComponentBase::canTerminate () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAudioComponent::CapabilityFlags CCL_API AudioComponentBase::getCapabilities () const
{
	return kCapabilityNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::setStreamLayout (const IAudioStreamLayout& layout)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAudioStreamLayout& CCL_API AudioComponentBase::getStreamLayout () const
{
	return AudioStreamLayout::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::setStreamActive (AudioStreamDirection direction, int index, tbool active)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::getProcessConfiguration (AudioProcessConfiguration& configuration) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::setProcessConfiguration (const AudioProcessConfiguration& configuration)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::getLatency (int& latencyInSamples, AudioStreamDirection direction) const
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioComponentBase::isActive () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AudioComponentBase::setActive (tbool state)
{
	return kResultNotImplemented;
}
