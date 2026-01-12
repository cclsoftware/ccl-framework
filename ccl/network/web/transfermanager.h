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
// Filename    : ccl/network/web/transfermanager.h
// Description : Transfer Manager
//
//************************************************************************************************

#ifndef _ccl_transfermanager_h
#define _ccl_transfermanager_h

#include "ccl/base/trigger.h"
#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/network/web/iwebcredentials.h"
#include "ccl/public/network/web/itransfermanager.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// Transfer
//************************************************************************************************

class Transfer: public Object,
				public ITransfer
{
public:
	DECLARE_CLASS (Transfer, Object)

	Transfer (Direction direction = kUpload);
	~Transfer ();
	
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (srcTitle, SrcTitle)
	PROPERTY_STRING (dstTitle, DstTitle)
	PROPERTY_OBJECT (Url, srcUrl, SrcUrl)
	PROPERTY_OBJECT (Url, dstUrl, DstUrl)
	PROPERTY_VARIABLE (int64, size, Size)
	PROPERTY_VARIABLE (double, progress, Progress)
	PROPERTY_VARIABLE (double, bytesPerSecond, Speed)
	PROPERTY_SHARED_AUTO (IWebCredentials, credentials, WebCredentials)
	PROPERTY_SHARED_AUTO (ITransferHandler, handler, Handler)
	PROPERTY_BOOL (fileNameNeeded, FileNameNeeded)
	PROPERTY_OBJECT (DateTime, timestamp, Time)
		
	void setState (State state);
	void setRestartAllowed (bool allowed) { restartAllowed = allowed; }
	void makeDstUnique ();
	Attributes& getResumeData () { return resumeData; }
	
	// ITransfer
	void CCL_API addFinalizer (ITriggerAction* action) override;
	void CCL_API removeFinalizers () override;

	State CCL_API getState () const override					{ return state; }
	Direction CCL_API getDirection () const override			{ return direction; }
	StringRef CCL_API getFileName () const override				{ return name; }
	int64 CCL_API getFileSize () const override					{ return size; }
	StringRef CCL_API getSrcDisplayString () const override		{ return srcTitle; }
	void CCL_API setSrcDisplayString (StringRef s) override		{ srcTitle = s; }
	StringRef CCL_API getDstDisplayString () const override		{ return dstTitle; }
	void CCL_API setDstDisplayString (StringRef s) override		{ dstTitle = s; }
	UrlRef CCL_API getSrcLocation () const override				{ return srcUrl; }
	UrlRef CCL_API getDstLocation () const override				{ return dstUrl; }
	IWebCredentials* CCL_API getCredentials () const override	{ return credentials; }
	double CCL_API getProgressValue () const override			{ return progress; }
	double CCL_API getBytesPerSecond () const override			{ return bytesPerSecond; }
	tbool CCL_API isChunked () const override					{ return chunked; }
	tbool CCL_API isUndeterminedFileName () const override		{ return fileNameNeeded; }
	void CCL_API setUserData (IUnknown* data) override			{ userData = data; }
	IUnknown* CCL_API getUserData () const override				{ return userData; }
	const DateTime& getTimestamp () const override				{ return timestamp; }
	tresult CCL_API relocate (UrlRef newLocation) override;
	tbool CCL_API isRestartAllowed () const override			{ return restartAllowed; }
	tbool CCL_API isResumable () const override;
	tbool CCL_API canTransferInBackground () const override;
	
	// Object
	bool equals (const Object& obj) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (ITransfer, Object)

protected:
	static constexpr int kRetryCount = 3;
	
	State state;
	Direction direction;
	bool chunked;
	int64 lastSpeedTime;
	int64 lastBytesDone;
	InterfaceList<ITriggerAction> finalizers;
	SharedPtr<IUnknown> userData;
	bool restartAllowed;
	Attributes resumeData;
	int failureCount;

	void calcSpeed ();
	void executeFinalizers ();
};

//************************************************************************************************
// TransferHandler
//************************************************************************************************

class TransferHandler: public Object,
					   public ITransferHandler,
					   public Singleton<TransferHandler>
{
public:
	// ITransferHandler
	void CCL_API startTransfer (ITransfer& t, IStream* localStream) override;
	void CCL_API cancelTransfer (ITransfer& t) override;
	void CCL_API pauseTransfer (ITransfer& t) override;
	tresult CCL_API resumeTransfer (ITransfer& t) override;
	int CCL_API getTransferOptions () const override;
	void CCL_API onHeadersReceived (ITransfer& t, IWebHeaderCollection& headers) override;

	CLASS_INTERFACE (ITransferHandler, Object)

protected:
	static StringID kResumeETagID;
	static StringID kResumePathID;
};

//************************************************************************************************
// TransferManager
//************************************************************************************************

class TransferManager: public Object,
					   public ITransferManager,
					   public Singleton<TransferManager>
{
public:
	DECLARE_CLASS (TransferManager, Object)

	TransferManager ();
	~TransferManager ();

	static const String kDownloadPartFileName;
	
	PROPERTY_POINTER (ITransferHandler, systemHandler, SystemHandler)
	
	void triggerNext ();
	bool removeFile (UrlRef path, bool deferred = false);
	
	// ITransferManager
	ITransfer* CCL_API createTransfer (UrlRef dst, UrlRef src, ITransfer::Direction dir, IWebCredentials* credentials = nullptr,
									   ITransferHandler* handler = nullptr) override;
	tresult CCL_API queue (ITransfer* transfer, int options = 0) override;
	tresult CCL_API cancel (ITransfer* transfer) override;
	tresult CCL_API restart (ITransfer* transfer) override;
	tresult CCL_API pause (ITransfer* transfer) override;
	tresult CCL_API resume (ITransfer* transfer) override;
	tresult CCL_API remove (ITransfer* transfer, tbool force = false) override;
	tresult CCL_API removeAll (tbool force = false) override;
	IUnknownIterator* CCL_API createIterator () const override;
	ITransfer* CCL_API find (ITransfer* transfer) const override;
	void CCL_API getActivity (ActivityInfo& activity) const override;
	tresult CCL_API perform (ITransfer* transfer, IProgressNotify* progress = nullptr) override;
	tresult CCL_API downloadFile (IUrl& dst, UrlRef src, IWebCredentials* credentials = nullptr, IProgressNotify* progress = nullptr) override;
	void CCL_API setFormatter (ITransferFormatter* formatter) override;
	tresult CCL_API restore () override;
	tresult CCL_API store () override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (ITransferManager, Object)

protected:
	ObjectArray transfers;
	SharedPtr<ITransferFormatter> formatter;
	bool restored;

	void start (Transfer& t);
	bool isAnyTransferActive () const;
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_transfermanager_h
