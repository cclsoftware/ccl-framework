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
// Filename    : ccl/extras/audio/runtime/audiotransport.h
// Description : Audio Transport
//
//************************************************************************************************

#ifndef _ccl_audiotransport_h
#define _ccl_audiotransport_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/framework/idleclient.h"

#include "ccl/public/audio/iaudiotransport.h"

namespace CCL {

//************************************************************************************************
// AudioTransport
//************************************************************************************************

class AudioTransport: public Object,
					  public IAudioTransport,
					  private IdleClient
{
public:
	DECLARE_CLASS (AudioTransport, Object)

	~AudioTransport ();

	void setClock (IAudioTransportClock* clock);
	
	void setActive (bool state);

	// IAudioTransport
	TSeconds CCL_API getCurrentTime () const override;
	TSeconds CCL_API getMaxDuration () const override;
	tbool CCL_API isActive () const override;
	tresult CCL_API start () override;	
	tresult CCL_API stop () override;
	tbool CCL_API isPlaying () const override;
	tresult CCL_API seek (TSeconds time) override;
	tresult CCL_API setSeekOverride (tbool state) override;
	tbool CCL_API isSeekOverride () const override;
	tresult CCL_API setRecordActive (tbool state) override;
	tbool CCL_API isRecording () const override;
	tresult CCL_API addClient (IAudioTransportClient* client) override;
	tresult CCL_API removeClient (IAudioTransportClient* client) override;
	tresult CCL_API addObserver (IAudioTransportObserver* observer) override;
	tresult CCL_API removeObserver (IAudioTransportObserver* observer) override;

	CLASS_INTERFACE3 (IAudioTransport, IAudioTransportClock, ITimerTask, Object)

protected:
	static constexpr int kUpdateInterval = 100;
	static constexpr TSeconds kUndefinedTime = -999.;

	bool active = false;
	bool playing = false;
	bool recording = false;
	bool seekOverride = false;
	TSeconds seekOverrideTime = 0.;
	TSeconds lastReportedTime = kUndefinedTime;
	TSeconds lastReportedDuration = kUndefinedTime;
	SharedPtr<IAudioTransportClock> clock;
	Vector<IAudioTransportClient*> clients;
	Vector<IAudioTransportObserver*> observers;

	void signalClients (const AudioTransportEvent& e);
	
	void updateObserverState ();
	void updateObserverTime (TSeconds time);
	void updateObserverDuration (TSeconds duration);

	void syncState ();
	void syncTime ();

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _ccl_audiotransport_h
