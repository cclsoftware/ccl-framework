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

#include "ccl/network/web/transfermanager.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/text/istringdict.h"

#include "ccl/public/network/web/httpstatus.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/network/web/iwebrequest.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Network Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ITransferManager& CCL_API System::CCL_ISOLATED (GetTransferManager) ()
{
	return TransferManager::instance ();
}

//************************************************************************************************
// Transfer
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Transfer, Object, "WebTransfer")

//////////////////////////////////////////////////////////////////////////////////////////////////

Transfer::Transfer (Direction direction)
: direction (direction),
  state (kNone),
  size (-1), // -1: unknown size
  progress (0),
  fileNameNeeded (false),
  restartAllowed (true),
  chunked (false),
  bytesPerSecond (0),
  lastSpeedTime (0),
  lastBytesDone (0),
  failureCount (0)
{
	handler.share (&TransferHandler::instance ()); // init with default handler
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Transfer::~Transfer ()
{
	signal (Message (kDestroyed));

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Transfer::addFinalizer (ITriggerAction* action)
{
	ASSERT (action != nullptr)
	finalizers.append (action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Transfer::removeFinalizers ()
{
	finalizers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transfer::executeFinalizers ()
{
	if(direction == kDownload)
	{
		if(state == kFailed || state == kCanceled)
		{
			// remove file for failed downloads
			if(!TransferManager::instance ().removeFile (dstUrl))
				TransferManager::instance ().removeFile (dstUrl, true); // try again later
		}
		else if(state == kCompleted)
		{
			String nameOnDisk;
			dstUrl.getName (nameOnDisk);
			String fileName = LegalFileName (name);
			if(fileName != nameOnDisk)
			{
				Url newDst (dstUrl);
				newDst.ascend ();
				newDst.descend (fileName);
				newDst.makeUnique ();
				if(System::GetFileSystem ().moveFile (newDst, dstUrl))
					dstUrl = newDst;
				else
					CCL_WARN ("Failed to rename file from \"%s\" to \"%s\" after download!\n", MutableCString (nameOnDisk).str (), MutableCString (fileName).str ())
			}
			
			// signal that a new file has been created
			SignalSource (Signals::kFileSystem).signal (Message (Signals::kFileCreated, dstUrl.asUnknown ()));
		}
	}

	ListForEach (finalizers, ITriggerAction*, action)
		action->execute (this);
	EndFor
	finalizers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Transfer::isResumable () const
{
	return get_flag<int> (handler->getTransferOptions (), ITransferHandler::kResumable) && (resumeData.isEmpty () == false) && direction == kDownload;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Transfer::canTransferInBackground () const
{
	return get_flag<int> (handler->getTransferOptions (), ITransferHandler::kBackgroundSupport);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transfer::setState (State newState)
{
	if(state != newState)
	{
		state = newState;
		
		// reset values
		if(state < Transfer::kTransferring)
		{
			progress = 0.;
			chunked = false;
			bytesPerSecond = 0.;
			lastSpeedTime = 0;
			lastBytesDone = 0;
		}

		signal (Message (kChanged));

		// execute finalizers if canceled
		if(newState == kCanceled)
		{
			ASSERT (System::IsInMainThread ())
			executeFinalizers ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transfer::makeDstUnique ()
{
	Url newDstUrl (getDstUrl ());
	newDstUrl.makeUnique ();
	if(newDstUrl != getDstUrl ())
	{
		setDstUrl (newDstUrl);
		
		String fileName;
		newDstUrl.getName (fileName);
		setName (fileName);
		setDstTitle (UrlDisplayString (newDstUrl));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Transfer::notify (ISubject* subject, MessageRef msg)
{
	if(state == kCanceled || state == kCompleted || state == kFailed) // ignore notifications if already done
		return;

	if(msg == Meta::kBackgroundProgressNotify)
	{
		setProgress (msg[0]);
		calcSpeed ();
	}
	else if(msg == Meta::kContentLengthNotify)
	{
		int64 length = msg[0];
		setSize (length);

		UnknownPtr<IWebHeaderCollection> headers;
		if(msg.getArgCount () >= 2)
			headers = msg[1].asUnknown ();
		SOFT_ASSERT (headers != nullptr, "Web headers not set!")

		chunked = headers && headers->isChunkedTransfer () != 0;

		if(isFileNameNeeded ()) // get file name from response headers
		{
			String fileName;
			if(headers && headers->parseFileName (fileName))
			{
				setName (fileName);
				setFileNameNeeded (false);
			}
			else
				CCL_WARN ("Could not parse file name from Content-Disposition header!", 0)
		}
		
		if(handler && headers)
			handler->onHeadersReceived (*this, *headers);
	
		calcSpeed ();
	}
	else if(msg == Meta::kDownloadComplete || msg == Meta::kUploadComplete)
	{
		int status = 0;
		tresult result = msg[0].asResult ();
		bool success = result == kResultOk; // error check at network level
		if(success && msg.getArgCount () > 1)
		{
			// optional check for errors at application level
			MutableCString protocol (direction == kDownload ? srcUrl.getProtocol () : dstUrl.getProtocol ());
			if(protocol == Meta::kHTTP || protocol == Meta::kHTTPS)
			{
				status = msg[1].asInt ();
				success = HTTP::isSuccessStatus (status);
			}
		}
		
		if(!success && isResumable () && msg == Meta::kDownloadComplete)
		{
			failureCount++;
			if(kRetryCount >= failureCount)
			{
				// keep transfer state and try to resume download where we left off
				if(TransferManager::instance ().resume (this) == kResultOk)
					return;
			}
		}
		
		setState (success ? kCompleted : kFailed);

		// log errors
		if(success == false)
		{
			MutableCString message;
			if(direction == kDownload)
				message.appendFormat ("Download of '%s'", MutableCString (name).str ());
			else
				message.appendFormat ("Upload of '%s'", MutableCString (name).str ());
			
			message.appendFormat (" failed at %.02lf %% (result = 0x%08X, status = %d).", 
								  progress * 100., result, status);
			
			CCL_WARN (message, 0)
		}

		// update timestamp
		System::GetSystem ().getLocalTime (timestamp);

		// execute finalizers (we are in main thread here)
		SharedPtr<Transfer> keeper (this);
		executeFinalizers ();

		// store finished transfers
		TransferManager::instance ().store ();

		// give manager a chance to start next transfer
		TransferManager::instance ().triggerNext ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Transfer::calcSpeed ()
{
	if(state == kTransferring && size != -1) // size must be known
	{
		int64 now = System::GetSystemTicks ();
		if(lastSpeedTime <= 0)
			lastSpeedTime = now;

		if((now - lastSpeedTime) >= 1000) // measure each second
		{
			int64 bytesDone = chunked ? size : (int64)(progress * size);
			int64 bytesDiff = bytesDone - lastBytesDone;
			if(bytesDiff >= 0)
			{
				double secondsDiff = (double)(now - lastSpeedTime) / 1000.;
				bytesPerSecond = (double)bytesDiff / secondsDiff;
			}

			lastBytesDone = bytesDone;
			lastSpeedTime = now;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Transfer::relocate (UrlRef newLocation)
{
	ASSERT (state == kCompleted)
	if(state != kCompleted)
		return kResultUnexpected;

	if(direction == kDownload)
	{
		String fileName;
		newLocation.getName (fileName);
		setName (fileName);
		setDstUrl (newLocation);
	}
	else
		setSrcUrl (newLocation);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transfer::equals (const Object& obj) const
{
	const Transfer* other = ccl_cast<Transfer> (&obj);
	if(other == nullptr)
		return SuperClass::equals (obj);

	if(direction != other->direction)
		return false;

	if(srcUrl != other->srcUrl)
		return false;

	if(credentials && other->credentials)
		if(credentials->getUserName () != other->credentials->getUserName ())
			return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transfer::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	direction = a.getInt ("direction");
	a.get (name, "fileName");
	a.getInt64 (size, "fileSize");
	Format::PortableDateTime::scan (timestamp, a.getString ("timestamp"));

	if(direction == kDownload)
	{
		a.get (dstUrl, "dst");
		a.get (srcTitle, "srcTitle");
		a.get (resumeData, "resumeData");
	}
	else
	{
		a.get (srcUrl, "src");
		a.get (dstTitle, "dstTitle");
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Transfer::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("direction", direction);
	a.set ("fileName", name);
	if(size != -1)
		a.set ("fileSize", size);
	if(timestamp != DateTime ())
		a.set ("timestamp", Format::PortableDateTime::print (timestamp));

	if(direction == kDownload)
	{
		a.set ("dst", dstUrl, true);
		a.set ("srcTitle", srcTitle);
		a.set ("resumeData", resumeData);
	}
	else
	{
		a.set ("src", srcUrl, true);
		a.set ("dstTitle", dstTitle);
	}

	return true;
}

//************************************************************************************************
// TransferHandler
//************************************************************************************************

DEFINE_SINGLETON (TransferHandler)

StringID TransferHandler::kResumeETagID = "eTag";
StringID TransferHandler::kResumePathID = "path";

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferHandler::startTransfer (ITransfer& _t, IStream* localStream)
{
	ASSERT (localStream)
	if(localStream == nullptr)
		return;

	Transfer* t = unknown_cast<Transfer> (&_t);
	ASSERT (t != nullptr)

	if(t->getDirection () == Transfer::kUpload)
	{
		AutoPtr<Web::IWebHeaderCollection> headers = System::GetWebService ().createHeaderCollection ();
		headers->getEntries ().setEntry (Web::Meta::kContentType, Meta::kBinaryContentType);

		System::GetWebService ().uploadInBackground (t, t->getDstUrl (), *localStream, headers, nullptr, t->getCredentials ());
	}
	else
	{
		if(UnknownPtr<INativeFileStream> file = localStream)
		{
			Url streamUrl;
			file->getPath (streamUrl);
			t->getResumeData ().set (kResumePathID, UrlFullString (streamUrl));
		}

		System::GetWebService ().downloadInBackground (t, t->getSrcUrl (), *localStream, t->getCredentials ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferHandler::cancelTransfer (ITransfer& _t)
{
	Transfer* t = unknown_cast<Transfer> (&_t);
	ASSERT (t != nullptr)

	System::GetWebService ().cancelOperation (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferHandler::pauseTransfer (ITransfer& _t)
{
	Transfer* t = unknown_cast<Transfer> (&_t);
	ASSERT (t != nullptr)

	System::GetWebService ().cancelOperation (t);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferHandler::resumeTransfer (ITransfer& _t)
{
	Transfer* t = unknown_cast<Transfer> (&_t);
	ASSERT (t != nullptr)

	if(t->getDirection () == Transfer::kUpload)
		return kResultUnexpected;
	
	String path;
	t->getResumeData ().get (path, kResumePathID);
	Url streamUrl;
	streamUrl.setUrl (path);
	AutoPtr<IStream> localStream = System::GetFileSystem ().openStream (streamUrl, IStream::kWriteMode | IStream::kReadMode);
	ASSERT (localStream)
	if(!localStream)
		return kResultFailed;
	
	AutoPtr<IWebHeaderCollection> headers = System::GetWebService ().createHeaderCollection ();
	String eTag;
	t->getResumeData ().get (eTag, TransferHandler::kResumeETagID);
	if(!eTag.isEmpty ())
	{
		localStream->seek (0, IStream::kSeekEnd);
		headers->getEntries ().appendEntry (Meta::kIfRange, MutableCString (eTag)); // this means : the server uses the range header only if the eTag matches, else sends the whole file
		headers->setRangeBytes (localStream->tell ());
	}
	else
		localStream->rewind (); // we need start from beginning when eTag is unknown
		
	return System::GetWebService ().downloadInBackground (t, t->getSrcUrl (), *localStream, t->getCredentials (), headers);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TransferHandler::getTransferOptions () const
{
	return kResumable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferHandler::onHeadersReceived (ITransfer& _t, IWebHeaderCollection& headers)
{
	Transfer* t = unknown_cast<Transfer> (&_t);
	ASSERT (t != nullptr)
	
	MutableCString eTag = headers.getEntries ().lookupValue (Meta::kETag);
	SOFT_ASSERT (!eTag.isEmpty (), "TransferHandler::onHeadersReceived")
	t->getResumeData ().set (kResumeETagID, eTag);
}

//************************************************************************************************
// TransferManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TransferManager, Object)
DEFINE_SINGLETON (TransferManager)
const String TransferManager::kDownloadPartFileName ("Download.part");

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManager::TransferManager ()
: restored (false),
  systemHandler (nullptr)
{
	transfers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransferManager::~TransferManager ()
{
	cancelSignals ();

	//ASSERT (transfers.isEmpty ())
	ASSERT (formatter == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITransfer* CCL_API TransferManager::createTransfer (UrlRef dst, UrlRef src, ITransfer::Direction dir, IWebCredentials* credentials,
													ITransferHandler* handler)
{
	Transfer* t = NEW Transfer (dir);
	t->setSrcUrl (src);
	t->setWebCredentials (credentials);
	
	if(handler != nullptr) // override default handler
		t->setHandler (handler);
	else if(systemHandler != nullptr)
		t->setHandler (systemHandler);
	
	if(dir == Transfer::kDownload && dst.isFolder ())
	{
		// use file name from server-side, can be updated via response headers
		// Note: URL might point to a script, and not to the data file directly. We do not want to display the script name to the user.
		String srcName;
		if(src.isFile () && System::GetFileTypeRegistry ().getFileTypeByUrl (src) != nullptr)
			src.getName (srcName);

		if(srcName.isEmpty ())
			srcName = kDownloadPartFileName;

		Url tempPath (dst);
		tempPath.descend (srcName);
		t->setDstUrl (tempPath);
		t->setFileNameNeeded (true);
	}
	else
		t->setDstUrl (dst);

	if(dir == Transfer::kDownload)
	{
		String fileName;
		t->getDstUrl ().getName (fileName);
		t->setName (fileName);
		t->setSrcTitle (src.getHostName ());
		t->setDstTitle (UrlDisplayString (dst));
	} 
	else // Transfer::kUpload
	{
		if(src.isFile ())
		{
			FileInfo fileInfo;
			System::GetFileSystem ().getFileInfo (fileInfo, src);
			t->setSize (fileInfo.fileSize);
		}

		String fileName;
		src.getName (fileName);
		t->setName (fileName);

		t->setSrcTitle (UrlDisplayString (src));
		t->setDstTitle (dst.getHostName ());
	}

	return t;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManager::triggerNext ()
{
	ArrayForEach (transfers, Transfer, t)
		if(t->getState () == Transfer::kNone)
		{
			start (*t);
			break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManager::removeFile (UrlRef path, bool deferred)
{
	if(deferred == true)
	{
		AutoPtr<IUrl> path2 = NEW Url (path);
		(NEW Message ("removeFile", static_cast<IUrl*> (path2)))->post (this, 1000);
		return true;
	}
	else
	{
		if(!System::GetFileSystem ().fileExists (path))
			return true;

		ErrorContextGuard context;
		bool removed = System::GetFileSystem ().removeFile (path) != 0;
		if(!removed)
		{
			if(context.hasErrors ())
				CCL_WARN (MutableCString (context->getEvent (0).message).str (), 0)
		}
		return removed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "removeFile")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path != nullptr)
		if(path)
			removeFile (*path, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::queue (ITransfer* transfer, int options)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;

	ASSERT (!transfers.contains (t) && t->getState () == Transfer::kNone)
	if(transfers.contains (t) || t->getState () != Transfer::kNone)
		return kResultUnexpected;

	transfers.add (t);
	transfer->retain ();
	signal (Message (kTransferAdded, t->asUnknown ()));

	if(options & kPreventRestart)
		t->setRestartAllowed (false);

	// start the transfer
	bool shouldStart = true;
	if(options & kNonSimultaneous)
		if(isAnyTransferActive ())
			shouldStart = false;

	if(shouldStart)
	{
		if(!(options & kSuppressSignals))
			SignalSource (Signals::kTransfers).signal (Message (Signals::kRevealTransfer, t->asUnknown ()));

		start (*t);
		
		if(t->getState () == ITransfer::kFailed)
			return kResultFailed;
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::cancel (ITransfer* transfer)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;

	ASSERT (transfers.contains (t))
	if(!transfers.contains (t))
		return kResultUnexpected;

	if(t->getState () <= Transfer::kPaused)
	{
		if(t->getState () == Transfer::kTransferring || t->getState () == Transfer::kPaused)
			t->getHandler ()->cancelTransfer (*t);
		t->setState (Transfer::kCanceled);
	}

	triggerNext ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::restart (ITransfer* transfer)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;

	ASSERT (transfers.contains (t))
	if(!transfers.contains (t))
		return kResultUnexpected;

	if(t->getState () == Transfer::kCanceled || t->getState () == Transfer::kFailed)
	{
		// reset state
		t->setState (Transfer::kNone);

		// start the transfer
		start (*t);
		return kResultOk;
	}

	CCL_DEBUGGER ("Transfer can't be restarted!\n")
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::pause (ITransfer* transfer)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;
	
	ASSERT (transfers.contains (t))
	if(!transfers.contains (t))
		return kResultUnexpected;

	if(t->getState () == Transfer::kTransferring && t->isResumable ())
	{
		t->getHandler ()->pauseTransfer (*t);
		t->setState (Transfer::kPaused);
		SignalSource (Signals::kTransfers).signal (Message (Signals::kTransferPaused, t->asUnknown (), true));
		return kResultOk;
	}

	CCL_DEBUGGER ("Transfer can't be paused")
	return kResultFalse;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::resume (ITransfer* transfer)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;
	
	ASSERT (transfers.contains (t))
	if(!transfers.contains (t))
		return kResultUnexpected;
	
	if(t->getHandler () != nullptr && t->isResumable ())
	{
		if(t->getHandler ()->resumeTransfer (*t) == kResultOk)
		{
			t->setState (Transfer::kTransferring);
			SignalSource (Signals::kTransfers).signal (Message (Signals::kTransferPaused, t->asUnknown (), false));
			return kResultOk;
		}
	}

	CCL_DEBUGGER ("Transfer can't be resumed")
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::remove (ITransfer* transfer, tbool force)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;

	ASSERT (transfers.contains (t))
	if(!transfers.contains (t))
		return kResultUnexpected;

	if(force)
		cancel (transfer);

	switch(t->getState ())
	{
	case Transfer::kCompleted :
	case Transfer::kFailed :
	case Transfer::kCanceled :
	case Transfer::kPaused :
		transfers.remove (t);
		t->setState (Transfer::kNone);
		signal (Message (kTransferRemoved, t->asUnknown ()));
		t->release ();
		return kResultOk;

	default :
		CCL_DEBUGGER ("Transfer is active!\n")
		return kResultFalse;
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::removeAll (tbool force)
{	
	ObjectArray candidates;
	candidates.objectCleanup (true);
	candidates.add (transfers, Container::kShare);

	tresult totalResult = kResultOk;
	ArrayForEach (candidates, Transfer, t)
		tresult result = remove (t, force);
		if(result != kResultOk)
			totalResult = result;
	EndFor
	return totalResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API TransferManager::createIterator () const
{
	return transfers.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITransfer* CCL_API TransferManager::find (ITransfer* transfer) const
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	Transfer* result = t ? (Transfer*)transfers.findEqual (*t) : nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TransferManager::start (Transfer& t)
{
	ASSERT (t.getState () == Transfer::kNone)

	bool streamNeeded = true;
	auto options = t.getHandler ()->getTransferOptions ();
	if(get_flag<int> (options, ITransferHandler::kNoLocalStream))
		streamNeeded = false;
	
	AutoPtr<IStream> localStream;
	if(streamNeeded)
	{
		if(t.getDirection () == Transfer::kUpload)
		{
			if(t.getSrcUrl ().isFile ())
				localStream = System::GetFileSystem ().openStream (t.getSrcUrl ());
			else
				streamNeeded = false;
		}
		else // Transfer::kDownload
		{
			if(t.getDstUrl ().isFile ())
			{
				t.makeDstUnique ();
				localStream = System::GetFileSystem ().openStream (t.getDstUrl (), IStream::kCreateMode);
			}
			else
				streamNeeded = false;
		}
	}

	ASSERT ((streamNeeded && localStream) || streamNeeded == false)
	if(streamNeeded && localStream == nullptr)
	{
		t.setState (Transfer::kFailed);
		return;
	}

	t.getHandler ()->startTransfer (t, localStream);
	t.setState (Transfer::kTransferring);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::perform (ITransfer* transfer, IProgressNotify* progress)
{
	Transfer* t = unknown_cast<Transfer> (transfer);
	ASSERT (t)
	if(!t)
		return kResultInvalidArgument;
	
	ASSERT (t->getState () == Transfer::kNone)
	if(t->getState () != Transfer::kNone)
		return kResultInvalidArgument;

	start (*t);

	const int kInterval = 20;
	int64 lastUpdateTime = System::GetSystemTicks ();
	UnknownPtr<IProgressDetails> progressDetails (progress);

	while(t->getState () < Transfer::kCompleted)
	{
		int64 now = System::GetSystemTicks ();
		if(progress && (now - lastUpdateTime) >= kInterval)
		{
			lastUpdateTime = now;

			// check if canceled
			if(progress->isCanceled ())
			{
				t->getHandler ()->cancelTransfer (*t);
				t->setState (Transfer::kCanceled);
				break;
			}

			if(t->isChunked () || t->getSize () == -1) // size must be known
				progress->updateAnimated ();
			else
				progress->updateProgress (t->getProgressValue ());

			// update details
			if(progressDetails && formatter)
			{
				String text;
				formatter->printState (text, *t, t->getState (), t->getProgressValue (), t->getBytesPerSecond ());
				progressDetails->setDetailText (0, text);
			}
		}

		System::ThreadSleep (1);
	}

	if(t->getState () == Transfer::kCanceled)
		return kResultAborted;
	return t->getState () == Transfer::kCompleted ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::downloadFile (IUrl& dst, UrlRef src, IWebCredentials* credentials, IProgressNotify* progress)
{
	AutoPtr<ITransfer> t = createTransfer (dst, src, Transfer::kDownload, credentials);
	tresult tr = perform (t, progress);
	dst.assign (t->getDstLocation ());
	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManager::setFormatter (ITransferFormatter* formatter)
{
	this->formatter = formatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TransferManager::getActivity (ActivityInfo& activity) const
{
	if(!transfers.isEmpty ())
	{
		// TODO: calculate overall progress!
		activity.numTotal = transfers.count ();
		ArrayForEach (transfers, Transfer, t)
			if(t->getState () == Transfer::kTransferring)
			{
				activity.numActive++;
				if(t->isResumable ())
					activity.numResumable++;
			}
			else if(t->getState () == Transfer::kPaused)
				activity.numPaused++;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TransferManager::isAnyTransferActive () const
{
	ActivityInfo activity;
	getActivity (activity);
	return activity.numActive > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::restore ()
{
	XmlSettings settings ("TransferManager");
	settings.restore ();

	Attributes& completed = settings.getAttributes ("completedTransfers");
	Transfer* t = nullptr;
	while((t = completed.unqueueObject<Transfer> (nullptr)) != nullptr)
	{
		t->setState (ITransfer::kCompleted);
		transfers.add (t);
		signal (Message (kTransferAdded, t->asUnknown ()));
	}

	restored = true;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TransferManager::store ()
{
	if(restored == false) // persistence must be enabled via restore() first!
		return kResultFalse;

	XmlSettings settings ("TransferManager");
	Attributes& completed = settings.getAttributes ("completedTransfers");
	ArrayForEach (transfers, Transfer, t)
		if(t->getState () == ITransfer::kCompleted)
			completed.queue (nullptr, t);
	EndFor
	
	settings.flush ();
	return kResultOk;
}
