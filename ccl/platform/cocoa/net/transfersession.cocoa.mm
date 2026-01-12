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
// Filename    : ccl/platform/cocoa/net/transfersession.cocoa.mm
// Description : Web transfer handler implementation based on Apple's NSURLSession API
//
//************************************************************************************************

#define DEBUG_LOG 0
#define ENABLE_TRANSFER_SESSION CCL_PLATFORM_IOS || (0 && DEBUG)

#if ENABLE_TRANSFER_SESSION
#define OUT_OF_PROCESS_TRANSFERS 1

#include "ccl/platform/cocoa/net/transfersession.cocoa.h"

#include "ccl/base/message.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/network/web/transfermanager.h"
#include "ccl/network/web/webrequest.h"

#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/netservices.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace Web;

@interface CCL_ISOLATED (SessionDelegate) : NSObject<NSURLSessionDelegate, NSURLSessionTaskDelegate, NSURLSessionDownloadDelegate>
{
@public
	CococaTransferSession* transferSession;
}

- (id)initWithSession:(CococaTransferSession*)session;
- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge completionHandler:(void(^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential* credential))completionHandler;
- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didResumeAtOffset:(int64_t)fileOffset expectedTotalBytes:(int64_t)expectedTotalBytes;
- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite;
- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didFinishDownloadingToURL:(NSURL*)location;
- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didCompleteWithError:(NSError*)error;
- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didSendBodyData:(int64_t)bytesSent totalBytesSent:(int64_t)totalBytesSent totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend;
- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task willPerformHTTPRedirection:(NSHTTPURLResponse*)response newRequest:(NSURLRequest*)request completionHandler:(void(^)(NSURLRequest*))completionHandler;

@end

@interface CCL_ISOLATED (FileStream) : NSInputStream
{
@public
	SharedPtr<IStream> cclStream;
}

- (id)initWithStream:(IStream*)stream;
- (NSInteger)read:(uint8_t*)buffer maxLength:(NSUInteger)len;
- (BOOL)getBuffer:(uint8_t**)buffer length:(NSUInteger*)len;
- (BOOL)hasBytesAvailable;

@end

//************************************************************************************************
// SessionDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (SessionDelegate)

- (id)initWithSession:(CococaTransferSession*)session
{
	ASSERT (session)
	if(self = [super init])
		transferSession = session;
		
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential*credential))completionHandler
{
	if(!transferSession || !challenge)
		return;
	
	if(challenge.protectionSpace.authenticationMethod == NSURLAuthenticationMethodServerTrust)
	{
		SecTrustRef trust = challenge.protectionSpace.serverTrust;

		// check the server SSL certificate
		CFObj<SecPolicyRef> policy = SecPolicyCreateWithProperties (kSecPolicyAppleSSL, NULL);
		OSStatus result = SecTrustSetPolicies (trust, policy);
		NSError* error = nil;
		bool accepted = SecTrustEvaluateWithError (trust, (CFErrorRef*)(&error));
		if(!accepted)
		{
			#if DEBUG_LOG
			String errorMessage;
			errorMessage.appendNativeString (error.description);
			CCL_PRINTF ("%s\n", MutableCString (errorMessage).str ())
			#endif
			return;
		}
		
		NSURLCredential* response = [NSURLCredential credentialForTrust:trust];
		completionHandler (NSURLSessionAuthChallengeUseCredential, response);
		return;
	}
	
	completionHandler (NSURLSessionAuthChallengePerformDefaultHandling, nil);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didResumeAtOffset:(int64_t)fileOffset expectedTotalBytes:(int64_t)expectedTotalBytes
{
	if(!transferSession)
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)totalBytesWritten totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite
{
	if(!transferSession)
		return;
	
	transferSession->progress (downloadTask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session downloadTask:(NSURLSessionDownloadTask*)downloadTask didFinishDownloadingToURL:(NSURL*)location
{
	if(!transferSession)
		return;

	transferSession->finishDownload (downloadTask, location, nil);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didCompleteWithError:(NSError*)error
{
	if(!transferSession)
		return;
	
	transferSession->completeTransfer (task, error);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task didSendBodyData:(int64_t)bytesSent totalBytesSent:(int64_t)totalBytesSent totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend
{
	if(!transferSession)
		return;
		
	transferSession->progress (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)URLSession:(NSURLSession*)session task:(NSURLSessionTask*)task willPerformHTTPRedirection:(NSHTTPURLResponse*)response newRequest:(NSURLRequest*)request completionHandler:(void(^)(NSURLRequest*))completionHandler
{
	#if DEBUG_LOG
	Url newUrl;
	MacUtils::urlFromNSUrl (newUrl, request.URL);
	String path;
	newUrl.getUrl (path);
	CCL_PRINTF ("Redirected to : %s\n", MutableCString (path).str ())
	#endif
	
	completionHandler (request);
}

@end

//************************************************************************************************
// CCLFileStream
//************************************************************************************************

@implementation CCL_ISOLATED (FileStream)

- (id)initWithStream:(IStream*)stream
{
	ASSERT (stream)
	if(self = [super init])
		cclStream = stream;
		
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSInteger)read:(uint8_t*)buffer maxLength:(NSUInteger)len
{
	if(!cclStream)
		return 0;
		
	return cclStream->read (buffer, static_cast<int> (len));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)getBuffer:(uint8_t**)buffer length:(NSUInteger*)len
{
	return NO;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (BOOL)hasBytesAvailable
{
	 // May also return YES if a read must be attempted in order to determine the availability of bytes
	return YES;
}

@end

//************************************************************************************************
// CococaTransferSession
//************************************************************************************************

DEFINE_SINGLETON (CococaTransferSession)
StringID CococaTransferSession::kResumeBlobID = "sessionData";

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (CococaTransferSession, kSecondRun)
{
	CococaTransferSession::instance ().initialize ();
	TransferManager::instance ().setSystemHandler (&CococaTransferSession::instance ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (CococaTransferSession, kSecondRun)
{
	TransferManager::instance ().setSystemHandler (nullptr);
	CococaTransferSession::instance ().terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static NSString* kSessionID = @"CCL Session";

//////////////////////////////////////////////////////////////////////////////////////////////////

int CococaTransferSession::hashKey (const NSUInteger& key, int size)
{
	return key % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CococaTransferSession::CococaTransferSession ()
: transfers (64, hashKey)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CococaTransferSession::initialize ()
{
	#if OUT_OF_PROCESS_TRANSFERS
	NSURLSessionConfiguration* config = [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:kSessionID];
	[config setSessionSendsLaunchEvents:YES];
	#else
	NSURLSessionConfiguration* config = [NSURLSessionConfiguration defaultSessionConfiguration];
	#endif

	CCL_ISOLATED (SessionDelegate)* delegate = [[[CCL_ISOLATED (SessionDelegate) alloc] initWithSession:this] autorelease];
	urlSession = [[NSURLSession sessionWithConfiguration:config delegate:delegate delegateQueue:nil] retain];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CococaTransferSession::terminate ()
{
	CCL_ISOLATED (SessionDelegate)* delegate = static_cast<CCL_ISOLATED (SessionDelegate)*> ([urlSession delegate]);
	if(delegate)
		delegate->transferSession = nullptr;
	
	[urlSession invalidateAndCancel];
	
	Threading::ScopedLock scopedLock (tableLock);
	HashMapIterator<NSUInteger, TransferEntry*> iter (transfers);
	while(!iter.done ())
		delete iter.next ();
	transfers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CococaTransferSession::startTransfer (ITransfer& _t, IStream* localStream)
{
	Threading::ScopedLock scopedLock (tableLock);
	
	ASSERT (localStream == nullptr)
	
	Transfer* transfer = unknown_cast<Transfer> (&_t);
	ASSERT (transfer != nullptr)
	
	UrlRef remoteUrl = transfer->getDirection () == Transfer::kUpload ? transfer->getDstUrl () : transfer->getSrcUrl ();
	NSURL* remoteNSUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (remoteUrl, remoteNSUrl);
	NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:remoteNSUrl];
	if(IWebCredentials* credentials = transfer->getCredentials ())
	{
		MutableCString authType (credentials->getAuthType ());
		if(authType.isEmpty () || authType == Meta::kBasic)
		{
			MutableCString string;
			string.append (credentials->getUserName ());
			string.append (":");
			string.append (credentials->getPassword ());

			Security::Crypto::Material material (Security::Crypto::Block (string.str (), string.length ()));

			MutableCString basicAuthentication ("Basic ");
			basicAuthentication.append (material.toCBase64 ());
			[request setValue:[NSString stringWithCString:basicAuthentication.str () encoding:NSASCIIStringEncoding] forHTTPHeaderField:@"Authorization"];
		}
		else if(authType == Meta::kBearer)
		{
			MutableCString bearerAuthentication ("Bearer ");
			bearerAuthentication.append (credentials->getPassword ());
			[request setValue:[NSString stringWithCString:bearerAuthentication.str () encoding:NSASCIIStringEncoding] forHTTPHeaderField:@"Authorization"];
		}
		else if(authType == Meta::kOAuth)
		{
			MutableCString oAuthAuthentication ("OAuth ");
			oAuthAuthentication.append (credentials->getPassword ());
			[request setValue:[NSString stringWithCString:oAuthAuthentication.str () encoding:NSASCIIStringEncoding] forHTTPHeaderField:@"Authorization"];
		}
	}
	
	NSURLSessionTask* task = nil;
	if(transfer->getDirection () == Transfer::kUpload)
	{
		NSURL* sourceNSUrl = [NSURL alloc];
		MacUtils::urlToNSUrl (transfer->getSrcUrl (), sourceNSUrl);
		task = [urlSession uploadTaskWithRequest:request fromFile:sourceNSUrl];
	}
	else
		task = [urlSession downloadTaskWithRequest:request];
	
	if(!task)
		return;
		
	transfers.add (task.taskIdentifier, NEW TransferEntry (&_t));
	[task resume];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NSUInteger CococaTransferSession::getTaskID (ITransfer& t)
{
	Threading::ScopedLock scopedLock (tableLock);

	NSUInteger taskId = 0;
	HashMapIterator<NSUInteger, TransferEntry*> iter (transfers);
	while(!iter.done ())
	{
		TransferEntry* entry = iter.next ();
		if(&(*entry->transfer) == &t)
		{
			transfers.getKey (taskId, entry);
			break;
		}
	}

	return taskId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CococaTransferSession::cancelTransfer (ITransfer& t)
{
	NSUInteger taskId = getTaskID (t);
	if(taskId)
		[urlSession getAllTasksWithCompletionHandler:^(NSArray<NSURLSessionTask*>* tasks)
		{
			for(NSURLSessionTask* task in tasks)
				if(task.taskIdentifier == taskId)
				{
					[task cancel];
					break;
				}
		}];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CococaTransferSession::getTransferOptions () const
{
	return ITransferHandler::kNoLocalStream | ITransferHandler::kResumable | ITransferHandler::kBackgroundSupport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CococaTransferSession::progress (NSURLSessionTask* task)
{
	Threading::ScopedLock scopedLock (tableLock);
	
	if(!task || task.taskIdentifier == 0)
		return;
	
	TransferEntry* entry = transfers.lookup (task.taskIdentifier);
	if(!entry)
	{
		SOFT_ASSERT (0, "Progress on unknown transfer")
		return;
	}
	
	Transfer* transfer = unknown_cast<Transfer> (entry->transfer);
	if(!transfer)
		return;
	
	if([task isKindOfClass:[NSURLSessionDownloadTask class]])
		if(entry->isInitialized == false)
		{
			AutoPtr<WebHeaderCollection> headers = NEW WebHeaderCollection;
			NSHTTPURLResponse* response = static_cast<NSHTTPURLResponse*> (task.response);
			for(NSString* key in response.allHeaderFields)
			{
				NSString* value = [response.allHeaderFields objectForKey:key];
				String keyString;
				keyString.appendNativeString (key);
				String valueString;
				valueString.appendNativeString (value);
				headers->appendEntry (MutableCString (keyString), MutableCString (valueString));
			}
			Message* m = NEW Message (Meta::kContentLengthNotify, task.countOfBytesExpectedToReceive, headers->asUnknown ());
			m->post (transfer);
			entry->isInitialized = true;
		}

	Message* m = NEW Message (Meta::kBackgroundProgressNotify, task.progress.fractionCompleted);
	m->post (transfer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CococaTransferSession::finishDownload (NSURLSessionDownloadTask* task, NSURL* location, NSError* error)
{
	Threading::ScopedLock scopedLock (tableLock);
	
	if(!task || task.taskIdentifier == 0)
		return;
	
	TransferEntry* entry = transfers.lookup (task.taskIdentifier);
	if(!entry)
	{
		SOFT_ASSERT (0, "Finish on unknown transfer")
		return;
	}
	
	if(error == nil && location)
	{
		Url tempFilePath;
		MacUtils::urlFromNSUrl (tempFilePath, location);
		Transfer* transfer = unknown_cast<Transfer> (entry->transfer);
		if(transfer)
		{
			transfer->makeDstUnique ();
			Url dstUrl (transfer->getDstUrl ());
			System::GetFileSystem ().moveFile (dstUrl, tempFilePath);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CococaTransferSession::completeTransfer (NSURLSessionTask* task, NSError* error)
{
	Threading::ScopedLock scopedLock (tableLock);
	
	if(!task || task.taskIdentifier == 0)
		return;
	
	TransferEntry* entry = transfers.lookup (task.taskIdentifier);
	if(!entry)
		return;

	NSHTTPURLResponse* response = static_cast<NSHTTPURLResponse*> (task.response);
	tresult result = kResultOk;
	if(error)
	{
		if(error.code == NSURLErrorCancelled)
			result = kResultAborted;
		else
			result = kResultFailed;
	}

	Transfer* transfer = unknown_cast<Transfer> (entry->transfer);
	Message* m = NEW Message (transfer->getDirection () == Transfer::kUpload ? Meta::kUploadComplete : Meta::kDownloadComplete, result, static_cast<int64> (response.statusCode));
	m->post (transfer);
	transfers.remove (task.taskIdentifier);
	
	dispatch_sync (dispatch_get_main_queue (), ^()
	{
		delete entry; // need to call on main thread because of signal handlers
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CococaTransferSession::pauseTransfer (ITransfer& t)
{
	NSUInteger taskId = getTaskID (t);
	if(taskId == 0)
		return;

	Transfer* transfer = unknown_cast<Transfer> (&t);
	TransferEntry* entry = transfers.lookup (taskId);
	ASSERT (transfer != nullptr && entry != nullptr)
	
	[urlSession getAllTasksWithCompletionHandler:^(NSArray<NSURLSessionTask*>* tasks)
	{
		for(NSURLSessionTask* task in tasks)
			if(task.taskIdentifier == taskId && [task isKindOfClass:[NSURLSessionDownloadTask class]])
			{
				[(NSURLSessionDownloadTask*)task cancelByProducingResumeData:^(NSData* resumeData)
				{
					String encodedResumeData;
					encodedResumeData.appendNativeString ([resumeData base64EncodedStringWithOptions:0]);
					transfer->getResumeData ().set (kResumeBlobID, encodedResumeData);
				}];
				transfers.remove (task.taskIdentifier);
				break;
			}
	}];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CococaTransferSession::resumeTransfer (ITransfer& t)
{
	Transfer* transfer = unknown_cast<Transfer> (&t);
	ASSERT (transfer != 0)
	
	String encodedResumeData;
	transfer->getResumeData ().get (kResumeBlobID, encodedResumeData);
	NSData* resumeData = [[[NSData alloc] initWithBase64EncodedString:[encodedResumeData.createNativeString<NSString*> () autorelease] options:0] autorelease];
	if(resumeData == nullptr)
		return kResultFailed;
	
	NSURLSessionTask* task = [urlSession downloadTaskWithResumeData:resumeData];
	transfers.add (task.taskIdentifier, NEW TransferEntry (&t));
	[task resume];
	return kResultOk;
}

#endif // ENABLE_TRANSFER_SESSION
