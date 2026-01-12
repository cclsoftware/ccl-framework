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
// Filename    : ccl/platform/cocoa/net/transfersession.cocoa.h
// Description : Web transfer handler implementation based on Apple's NSURLSession API
//
//************************************************************************************************

#ifndef _ccl_transfersession_cocoa_h
#define _ccl_transfersession_cocoa_h

#include "ccl/base/singleton.h"

#include "ccl/public/network/web/itransfermanager.h"
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/system/threadsync.h"

#include "ccl/platform/cocoa/macutils.h"

@class NSURLSession;
@class NSURLSessionDownloadTask;
@class NSURL;

namespace CCL {
namespace Web {

//************************************************************************************************
// CococaTransferSession
//************************************************************************************************

class CococaTransferSession: public Object,
							 public ITransferHandler,
					   		 public Singleton<CococaTransferSession>
{
public:
	CococaTransferSession ();

	void initialize ();
	void terminate ();
	void progress (NSURLSessionTask* task);
	void finishDownload (NSURLSessionDownloadTask* task, NSURL* location, NSError* error);
	void completeTransfer (NSURLSessionTask* task, NSError* error);
	
	// ITransferHandler
	void CCL_API startTransfer (ITransfer& t, IStream* localStream);
	void CCL_API cancelTransfer (ITransfer& t);
	void CCL_API pauseTransfer (ITransfer& t);
	CCL::tresult CCL_API resumeTransfer (ITransfer& t);
	int CCL_API getTransferOptions () const;
	void CCL_API onHeadersReceived (ITransfer& t, IWebHeaderCollection& headers) {}
	
	CLASS_INTERFACE (ITransferHandler, Object)

protected:
	struct TransferEntry
	{
		TransferEntry (ITransfer* t)
		: transfer (t), isInitialized (false)
		{}
		
		SharedPtr<ITransfer> transfer;
		bool isInitialized;
	};

	static StringID kResumeBlobID;
	
	NSObj<NSURLSession> urlSession;
	typedef HashMap<NSUInteger, TransferEntry*> TransferTable;
	static int hashKey (const NSUInteger& key, int size);
	TransferTable transfers;
	Threading::CriticalSection tableLock;
	
	NSUInteger getTaskID (ITransfer& t);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_transfersession_cocoa_h
