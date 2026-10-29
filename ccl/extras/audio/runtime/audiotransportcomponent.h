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
// Filename    : ccl/extras/audio/runtime/audiotransportcomponent.h
// Description : Audio Transport Component
//
//************************************************************************************************

#ifndef _ccl_audiotransportcomponent_h
#define _ccl_audiotransportcomponent_h

#include "ccl/app/component.h"

#include "ccl/public/audio/iaudiotransport.h"

namespace CCL {

//************************************************************************************************
// AudioTransportComponent
//************************************************************************************************

class AudioTransportComponent: public Component,
							   public IAudioTransportObserver
{
public:
	DECLARE_CLASS_ABSTRACT (AudioTransportComponent, Component)

	AudioTransportComponent (IAudioTransport& transport, StringRef name = nullptr);
	~AudioTransportComponent ();

	// IAudioTransportObserver
	void CCL_API onTransportStateChanged () override;
	void CCL_API onTransportDurationChanged (TSeconds duration) override;
	void CCL_API onTransportTimeChanged (TSeconds time) override;

	// Component
	void CCL_API paramEdit (IParameter* param, tbool begin) override;
	tbool CCL_API paramChanged (IParameter* param) override;

	CLASS_INTERFACE (IAudioTransportObserver, Component)

protected:
	SharedPtr<IAudioTransport> transport;

	IParameter* timeParam = nullptr;
};

} // namespace CCL

#endif // _ccl_audiotransportcomponent_h
