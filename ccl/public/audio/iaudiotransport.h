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
// Filename    : ccl/public/audio/iaudiotransport.h
// Description : Audio Transport Interfaces
//
//************************************************************************************************

#ifndef _ccl_iaudiotransport_h
#define _ccl_iaudiotransport_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/audio/cclaudiotypes.h"

namespace CCL {

//************************************************************************************************
// AudioTransportEvent
//************************************************************************************************

struct AudioTransportEvent
{
	DEFINE_ENUM (Type)
	{
		kStarted,
		kStopped,
		kSeek,
		kRecordStarted,
		kRecordStopped
	};

	Type type = kStarted;
	TSeconds time = 0;
};

//************************************************************************************************
// IAudioTransportClient
/** A client syncing its state to a shared transport instance for playback and/or recording. */
//************************************************************************************************

interface IAudioTransportClient: IUnknown
{
	DEFINE_ENUM (StateFlags)
	{
		kPlaying = 1<<0,	///< client is currently playing
		kRecording = 1<<1	///< client is currently recording
	};
	
	/** Get current client state (queried periodically by transport). */
	virtual StateFlags CCL_API getClientState () const = 0;

	/** Handle transport event (start, stop, seek, etc.) */
	virtual void CCL_API onTransportEvent (const AudioTransportEvent& e) = 0;
		
	DECLARE_IID (IAudioTransportClient)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioTransportClient, 0x4f535306, 0x4c53, 0x4783, 0x8c, 0xc9, 0x42, 0xb5, 0x5a, 0x89, 0x11, 0xa3)
#endif

//************************************************************************************************
// IAudioTransportObserver
/** An observer notified periodically by a shared transport instance on state/time changes. */
//************************************************************************************************

interface IAudioTransportObserver: IUnknown
{
	/** Transport state has changed (called occasionally). 
		
		State changes include:
		- active
		- playing
		- recording
		- seek override 
	*/
	virtual void CCL_API onTransportStateChanged () = 0;
	
	/** Transport duration has changed (called occasionally). */
	virtual void CCL_API onTransportDurationChanged (TSeconds duration) = 0;

	/** Current transport time has changed (called periodically while playing). */
	virtual void CCL_API onTransportTimeChanged (TSeconds time) = 0;

	DECLARE_IID (IAudioTransportObserver)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioTransportObserver, 0xfcbd697a, 0xfef9, 0x4beb, 0xbd, 0xca, 0x8a, 0xc7, 0x69, 0xf7, 0xe3, 0xf6)
#endif

//************************************************************************************************
// IAudioTransportClock
/** A clock used (and provided) by a shared transport instance. */
//************************************************************************************************

interface IAudioTransportClock: IUnknown
{
	/** Current transport time in seconds, advancing during playback. */
	virtual TSeconds CCL_API getCurrentTime () const = 0;
	
	/** Maximum transport duration in seconds (-1 if unbound). */
	virtual TSeconds CCL_API getMaxDuration () const = 0;

	static constexpr TSeconds kMaxDurationUnbound = -1.;

	DECLARE_IID (IAudioTransportClock)
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioTransportClock, 0xd15bbeef, 0x9732, 0x4e8b, 0xa7, 0x23, 0x77, 0xb0, 0x34, 0x11, 0xca, 0xf6)
#endif

//************************************************************************************************
// IAudioTransport
/**	A shared transport instance to control playback/recording of one or more clients.
	It provides updates to observers for state and time changes. 

	Threading Policy:
	Everything happens on the main thread. 
*/
//************************************************************************************************

interface IAudioTransport: IAudioTransportClock
{	
	/**	Check if this transport instance is currently active. */
	virtual tbool CCL_API isActive () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Playback
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Start playback for all clients. */
	virtual tresult CCL_API start () = 0;
	
	/** Stop playback for all clients. */
	virtual tresult CCL_API stop () = 0;
	
	/** Returns true if any clients are playing. */
	virtual tbool CCL_API isPlaying () const = 0;
	
	/** Seek all clients to given position. */
	virtual tresult CCL_API seek (TSeconds time) = 0;

	/** Enables seek override so that seeking only changes the displayed time. */
	virtual tresult CCL_API setSeekOverride (tbool state) = 0;

	/** Check if seek override is currently active. */
	virtual tbool CCL_API isSeekOverride () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Recording
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Start/stop recording for all clients. */
	virtual tresult CCL_API setRecordActive (tbool state) = 0;
	
	/** Returns true if any clients are recording. */
	virtual tbool CCL_API isRecording () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Clients/Observers
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Add client to sync to this transport instance (not retained). */
	virtual tresult CCL_API addClient (IAudioTransportClient* client) = 0;
	
	/** Remove client. */
	virtual tresult CCL_API removeClient (IAudioTransportClient* client) = 0;

	/** Add observer to get state/time notifications (not retained). */
	virtual tresult CCL_API addObserver (IAudioTransportObserver* observer) = 0;
	
	/** Remove observer. */
	virtual tresult CCL_API removeObserver (IAudioTransportObserver* observer) = 0;

	DECLARE_IID (IAudioTransport)

	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Shortcut to rewind to zero. */
	tresult rewind () { return seek (0.); }
};

#ifdef _CCL_AUDIO_IID_SYMBOLS_ENABLED_
DEFINE_IID (IAudioTransport, 0xc271d2b0, 0xce90, 0x4abd, 0x88, 0xd, 0xcd, 0xda, 0xd0, 0x18, 0xba, 0x50)
#endif

} // namespace CCL

#endif // _ccl_iaudiotransport_h
