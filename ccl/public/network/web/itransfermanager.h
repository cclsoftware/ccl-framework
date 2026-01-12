//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/network/web/itransfermanager.h
// Description : Transfer Manager Interface
//
//************************************************************************************************

#ifndef _ccl_itransfermanager_h
#define _ccl_itransfermanager_h

#include "ccl/public/base/datetime.h"
#include "ccl/public/base/iasyncoperation.h"

namespace CCL {

interface ITriggerAction;
interface IUnknownIterator;
interface IProgressNotify;
interface IStream;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Transfer Manager Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to transfer manager. */
	DEFINE_STRINGID (kTransfers, "CCL.Transfers")

		/** Reveal given transfer args[0]: ITransfer args[1]: force (tbool) */
		DEFINE_STRINGID (kRevealTransfer, "RevealTransfer")
		
		/** Transfer is paused args[0]: ITransfer args[1]: state (tbool) */
		DEFINE_STRINGID (kTransferPaused, "TransferPaused")
}

namespace Web {

interface IWebCredentials;
interface IWebHeaderCollection;

//************************************************************************************************
// ITransfer
/**	Interface representing a queued transfer (upload or download).
	Additional interfaces: IObserver. */
//************************************************************************************************

interface ITransfer: IUnknown
{
	/** Transfer direction. */
	DEFINE_ENUM (Direction)
	{
		kUpload,		///< upload (local to server)
		kDownload		///< download (server to local)
	};

	/** Transfer states */
	DEFINE_ENUM (State)
	{
		kNone,
		kTransferring,
		kPaused,
		kCompleted,
		kCanceled,
		kFailed
	};
	
	/** Add action to be executed when transfer is done (takes ownership). */
	virtual void CCL_API addFinalizer (ITriggerAction* action) = 0;

	/** Remove all finalizer actions. */
	virtual void CCL_API removeFinalizers () = 0;

	/** Get transfer direction (upload/download). */
	virtual Direction CCL_API getDirection () const = 0;

	/** Get file name. */
	virtual StringRef CCL_API getFileName () const = 0;

	/** Get file size. */
	virtual int64 CCL_API getFileSize () const = 0;

	/** Get beautified string describing the source. */
	virtual StringRef CCL_API getSrcDisplayString () const = 0;

	/** Set beautified string describing the source. */
	virtual void CCL_API setSrcDisplayString (StringRef displayString) = 0;

	/** Get beautified string describing the destination. */
	virtual StringRef CCL_API getDstDisplayString () const = 0;

	/** Set beautified string describing the destination. */
	virtual void CCL_API setDstDisplayString (StringRef displayString) = 0;

	/** Get source URL. */
	virtual UrlRef CCL_API getSrcLocation () const = 0;

	/** Get destination URL. */
	virtual UrlRef CCL_API getDstLocation () const = 0;

	/** Get credentials. */
	virtual IWebCredentials* CCL_API getCredentials () const = 0;

	/** Get current progress value. */
	virtual double CCL_API getProgressValue () const = 0;

	/** Get transfer speed factor in bytes per second. */
	virtual double CCL_API getBytesPerSecond () const = 0;

	/** Returns true for chunked transfer. */
	virtual tbool CCL_API isChunked () const = 0;

	/** Check if file name hasn't been determined yet. */
	virtual tbool CCL_API isUndeterminedFileName () const = 0;

	/** Assign arbitrary data with transfer (shared). */
	virtual void CCL_API setUserData (IUnknown* data) = 0;

	/** Get arbitrary data associated with transfer. */
	virtual IUnknown* CCL_API getUserData () const = 0;

	/** Get time when transfer happened. */
	virtual const DateTime& getTimestamp () const = 0;

	/** Relocate local file if it has been moved, fails if transfer is not complete. */
	virtual tresult CCL_API relocate (UrlRef newLocation) = 0;

	/** Check if re-starting a canceled or failed transfer is possible */
	virtual tbool CCL_API isRestartAllowed () const = 0;

	/** Check if resuming a paused  transfer is possible */
	virtual tbool CCL_API isResumable () const = 0;
	
	/** Check if the transfer is capable of running in the background (when application is suspended) */
	virtual tbool CCL_API canTransferInBackground () const = 0;

	/** Returns the current state. */
	virtual State CCL_API getState () const = 0;
	
	DECLARE_IID (ITransfer)
};

DEFINE_IID (ITransfer, 0x29e799f8, 0x7bfd, 0x47b9, 0xa5, 0xb9, 0x3d, 0xf4, 0xac, 0x1f, 0xfb, 0x97)

//************************************************************************************************
// ITransferHandler
/*	Interface to implement custom-type transfers. ITransfer object reacts to:
	- kContentLengthNotify
	- kBackgroundProgressNotify
	- kDownloadComplete or kUploadComplete

	Threading Policy:
	Transfer handler is called from main thread.
*/
//************************************************************************************************

interface ITransferHandler: IUnknown
{
	/** Transfer options. */
	enum TransferOptions
	{
		kNoLocalStream = 1<<0, ///< if set, the handler has to be used without providing a local stream
		kResumable = 1<<1, ///< if set, the handler is able to resume a  transfer that was paused or interrupted
		kBackgroundSupport = 1<<2 ///< if set, the handler can continue the transfer even when the running application is suspended
	};

	/** Start transfer from/to local stream (stream is optional) */
	virtual void CCL_API startTransfer (ITransfer& t, IStream* localStream) = 0;

	/** Cancel transfer. */
	virtual void CCL_API cancelTransfer (ITransfer& t) = 0;

	/** Stop transfer but keep resume data. */
	virtual void CCL_API pauseTransfer (ITransfer& t) = 0;
	
	/** Resume transfer after pause. */
	virtual tresult CCL_API resumeTransfer (ITransfer& t) = 0;
	
	/** Get capabilities of the handler. */
	virtual int CCL_API getTransferOptions () const = 0;

	/** Called when HTTP headers have been received from the server. */
	virtual void CCL_API onHeadersReceived (ITransfer& t, IWebHeaderCollection& headers) = 0;
	
	DECLARE_IID (ITransferHandler)
};

DEFINE_IID (ITransferHandler, 0x9d4a4231, 0x5d41, 0x4a73, 0x92, 0x22, 0xe9, 0x59, 0x84, 0xb0, 0xde, 0x16)

//************************************************************************************************
// ITransferFormatter
//************************************************************************************************

interface ITransferFormatter: IUnknown
{
	/** Print state of given transfer. */
	virtual void CCL_API printState (String& string, ITransfer& transfer,
									 ITransfer::State state, double progress, double speed) = 0;

	DECLARE_IID (ITransferFormatter)
};

DEFINE_IID (ITransferFormatter, 0x621b7667, 0x192e, 0x4b2a, 0xad, 0x7a, 0xfb, 0x93, 0x35, 0x61, 0xe3, 0x75)

//************************************************************************************************
// ITransferManager
/*
	Threading Policy:
	The current implementation is NOT thread-safe! It must be called from the main thread only.
*/
//************************************************************************************************

interface ITransferManager: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Transfer Queue
	//////////////////////////////////////////////////////////////////////////////////////////////

	DECLARE_STRINGID_MEMBER (kTransferAdded)		///< arg[0]: ITransfer
	DECLARE_STRINGID_MEMBER (kTransferRemoved)		///< arg[0]: ITransfer

	/** Create transfer object. */
	virtual ITransfer* CCL_API createTransfer (UrlRef dst, UrlRef src, ITransfer::Direction dir,
												IWebCredentials* credentials = nullptr,
												ITransferHandler* handler = nullptr) = 0;

	/** Queue options. */
	enum QueueOptions
	{
		kNonSimultaneous = 1<<0,	///< don't start immediately if other transfers active
		kSuppressSignals = 1<<1,	///< don't emit "reveal" signal for transfer
		kPreventRestart = 1<<2		///< do not allow to restart this transfer
	};

	/** Add transfer to queue. The transfer will be shared. */
	virtual tresult CCL_API queue (ITransfer* transfer, int options = 0) = 0;

	/** Cancel transfer. */
	virtual tresult CCL_API cancel (ITransfer* transfer) = 0;

	/** Restart transfer. */
	virtual tresult CCL_API restart (ITransfer* transfer) = 0;

	/** Pause transfer. */
	virtual tresult CCL_API pause (ITransfer* transfer) = 0;

	/** Resume transfer. */
	virtual tresult CCL_API resume (ITransfer* transfer) = 0;

	/** Remove transfer. If not forced, a working transfer will remain active. */
	virtual tresult CCL_API remove (ITransfer* transfer, tbool force = false) = 0;

	/** Remove (and optionally cancel) all transfers. */
	virtual tresult CCL_API removeAll (tbool force = false) = 0;

	/** Create iterator of transfer objects. */
	virtual IUnknownIterator* CCL_API createIterator () const = 0;

	/** Find existing instance of given transfer. */
	virtual ITransfer* CCL_API find (ITransfer* transfer) const = 0;

	/** Transfer activity information. */
	struct ActivityInfo
	{
		int numActive;			///< number of active transfers
		int numTotal;			///< number of total transfers
		int numResumable;		///< number of resumable transfers (can be paused)
		int numPaused;			///< number of paused transfers
		float progressValue;	///< total progress value

		ActivityInfo ()
		: numActive (0),
		  numTotal (0),
		  numResumable (0),
		  numPaused (0),
		  progressValue (0)
		{}
	};

	/** Get transfer activity. */
	virtual void CCL_API getActivity (ActivityInfo& activity) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Synchronous Transfers
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Perform given transfer synchronously. The transfer object is not added to the queue. */
	virtual tresult CCL_API perform (ITransfer* transfer, IProgressNotify* progress = nullptr) = 0;

	/** Shortcut to download file synchronously. Safe to be called from main thread. Destination URL can accommodate to file name from server. */
	virtual tresult CCL_API downloadFile (IUrl& dst, UrlRef src, IWebCredentials* credentials = nullptr, IProgressNotify* progress = nullptr) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Other
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Assign formatter for transfer status. */
	virtual void CCL_API setFormatter (ITransferFormatter* formatter) = 0;

	/** Restore finished transfers. */
	virtual tresult CCL_API restore () = 0;

	/** Store finished transfers. */
	virtual tresult CCL_API store () = 0;

	DECLARE_IID (ITransferManager)
};

DEFINE_IID (ITransferManager, 0xba5c1244, 0xad7b, 0x4e69, 0x93, 0xde, 0x6e, 0x57, 0xc1, 0x80, 0x18, 0xed)
DEFINE_STRINGID_MEMBER (ITransferManager, kTransferAdded, "transferAdded")
DEFINE_STRINGID_MEMBER (ITransferManager, kTransferRemoved, "transferRemoved")

} // namespace Web
} // namespace CCL

#endif // _ccl_itransfermanager_h
