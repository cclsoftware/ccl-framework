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
// Filename    : ccl/extras/audio/runtime/audiotransportcomponent.cpp
// Description : Audio Transport Component
//
//************************************************************************************************

#include "ccl/extras/audio/runtime/audiotransportcomponent.h"

#include "ccl/public/system/formatter.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum AudioTransportComponentTags
	{
		kStart = 100,
		kStop,
		kRewind,
		kRecord,
		kTime
	};
}

//************************************************************************************************
// AudioTransportComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AudioTransportComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioTransportComponent::AudioTransportComponent (IAudioTransport& _transport, StringRef name)
: Component (name.isEmpty () ? CCLSTR ("AudioTransport") : name),
  transport (&_transport)
{
	paramList.addParam (CSTR ("start"), Tag::kStart);
	paramList.addParam (CSTR ("stop"), Tag::kStop);
	paramList.addParam (CSTR ("rewind"), Tag::kRewind);
	paramList.addParam (CSTR ("record"), Tag::kRecord);

	timeParam = paramList.addFloat (0., 0., CSTR ("time"), Tag::kTime);
	timeParam->setFormatter (AutoPtr<IFormatter> (NEW Format::Duration));

	transport->addObserver (this);
	onTransportStateChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AudioTransportComponent::~AudioTransportComponent ()
{
	transport->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioTransportComponent::onTransportStateChanged ()
{
	bool enabled = transport && transport->isActive ();
	bool playing = enabled && transport->isPlaying ();
	bool recording = enabled && transport->isRecording ();

	paramList.byTag (Tag::kStart)->enable (enabled);
	paramList.byTag (Tag::kStop)->enable (enabled);
	paramList.byTag (Tag::kRecord)->enable (enabled);
	paramList.byTag (Tag::kRewind)->enable (enabled);
	timeParam->enable (enabled);

	paramList.byTag (Tag::kStart)->setValue (playing);
	paramList.byTag (Tag::kStop)->setValue (!playing);
	paramList.byTag (Tag::kRecord)->setValue (recording);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioTransportComponent::onTransportDurationChanged (TSeconds duration)
{
	if(duration == IAudioTransport::kMaxDurationUnbound)
		timeParam->setMax (10. * TSeconds(DateTime::kSecondsInDay));
	else
		timeParam->setMax (duration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioTransportComponent::onTransportTimeChanged (TSeconds time)
{
	if(!timeParam->isEditing ())
		timeParam->setValue (time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AudioTransportComponent::paramEdit (IParameter* param, tbool begin)
{
	switch(param->getTag ())
	{
	case Tag::kTime :
		param->setEditing (begin);
		transport->setSeekOverride (begin);
		if(!begin)
			transport->seek (param->getValue ());
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AudioTransportComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kStart :
		transport->start ();
		param->setValue (transport->isPlaying ());
		break;

	case Tag::kStop :
		if(transport->isPlaying ())
			transport->stop ();
		else
			transport->rewind ();
		param->setValue (!transport->isPlaying ());
		break;

	case Tag::kRewind :
		transport->rewind ();
		break;

	case Tag::kRecord :
		transport->setRecordActive (param->getValue ().asBool ());
		param->setValue (transport->isRecording ());
		break;

	case Tag::kTime :
		transport->seek (param->getValue ());
		break;
	}
	return true;
}
