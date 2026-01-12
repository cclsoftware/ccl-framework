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
// Filename    : ccl/platform/cocoa/gui/fileselector.ios.mm
// Description : platform-specific file selector code
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/dialogs/fileselector.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/gui/gui.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/window.ios.h"

#include "ccl/public/base/ccldefpush.h"
#import <MobileCoreServices/MobileCoreServices.h>

@class CCL_ISOLATED (FSDelegate);
@class CCL_ISOLATED (FolderSelectorDelegate);

namespace CCL {

//************************************************************************************************
// IOSFileSelector
//************************************************************************************************

class IOSFileSelector: public NativeFileSelector
{
public:
	IOSFileSelector ();
	~IOSFileSelector ();
	
	DECLARE_CLASS (IOSFileSelector, NativeFileSelector)

	PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)

	void onSelectionFinished (NSArray<NSURL*>* urls);

	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	int CCL_API getSaveBehavior () const override;
	void CCL_API setSaveContent (UrlRef url) override;

protected:
	UIDocumentPickerViewController* controller;
	CCL_ISOLATED (FSDelegate)* fsDelegate;
	Url saveContent;
	int failedDownloads;
	int numPendingDownloads;

	void onFileUpdateDone (IAsyncOperation& operation);
	void completeFileSelection ();
	bool hasPendingDownloads ();
};

//************************************************************************************************
// IOSFolderSelector
//************************************************************************************************

class IOSFolderSelector: public NativeFolderSelector
{
public:
    IOSFolderSelector ();
    ~IOSFolderSelector ();
    
	DECLARE_CLASS (IOSFolderSelector, NativeFolderSelector)
    
    PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)
    
    void onSelectionFinished (NSURL* urls);

	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
protected:
    UIDocumentPickerViewController* controller;
    CCL_ISOLATED (FolderSelectorDelegate)* folderSelectorDelegate;
    
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FSDelegate
//************************************************************************************************

@interface CCL_ISOLATED (FSDelegate) : NSObject<UIDocumentPickerDelegate>
{
	IOSFileSelector* fileSelector;
}

- (id)initWithFileSelector:(IOSFileSelector*)fileSelector;
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls;
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentAtURL:(NSURL *)url;
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (FSDelegate)

- (id)initWithFileSelector:(IOSFileSelector*)_fileSelector
{
	if(self = [super init])
		fileSelector = _fileSelector;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
	if(fileSelector)
		fileSelector->onSelectionFinished (urls);
}

//////////////////////////////////////////////////////////////////////////////////////////////////


- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentAtURL:(NSURL*)url
{
	NSArray<NSURL*>* urls = [NSArray arrayWithObject:url];
	[self documentPicker:controller didPickDocumentsAtURLs:urls];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
	if(fileSelector)
		fileSelector->onSelectionFinished (nil);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

@end

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileSelector")
	XSTRING (Reopen, "Files are still being downloaded. Please try to open the file again later.")
END_XSTRINGS

//************************************************************************************************
// IOSFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (IOSFileSelector, NativeFileSelector, "FileSelector")
DEFINE_CLASS_UID (IOSFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFileSelector::IOSFileSelector ()
: controller (nil),
  fsDelegate (nil),
  failedDownloads (0),
  numPendingDownloads (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFileSelector::~IOSFileSelector ()
{
	if(controller)
		[controller release];
	if(fsDelegate)
		[fsDelegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	CCL_WARN ("synchronous FileSelector not supported!", 0)
	Promise p (runPlatformSelectorAsync (type, title, filterIndex, window));
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* IOSFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	if(controller)
	{
		[controller release];
		controller = nil;
	}

	if(fsDelegate)
	{
		[fsDelegate release];
		fsDelegate = nil;
	}

	if(type == kSaveFile)
	{
		ASSERT(!saveContent.isEmpty ())
		if(!saveContent.isEmpty ())
		{
			NSURL* nsUrl = [NSURL alloc];
			MacUtils::urlToNSUrl (saveContent, nsUrl);
			controller =  [[UIDocumentPickerViewController alloc] initWithURL:nsUrl inMode:UIDocumentPickerModeExportToService];
		}
	}
	else // kOpenFile or kOpenMultipleFiles
	{
		NSMutableArray* utis = [[[NSMutableArray alloc] init] autorelease];
		if(filters.isEmpty ())
			[utis addObject:@"public.item"];
		else
			for(auto fileType : iterate_as<Boxed::FileType> (filters))
			{
				String mimeType (fileType->getMimeType ());
				NSString* mimeString = [mimeType.createNativeString<NSString*> () autorelease];
				NSString* uti = [(NSString*)UTTypeCreatePreferredIdentifierForTag (kUTTagClassMIMEType, (CFStringRef)mimeString, 0) autorelease];
				[utis addObject:uti];

				// extract extension if specified in MIME type and add explicitly
				NSArray* components = [mimeString componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"+ "]];
				if(components.count > 1)
					[utis addObject:[(NSString*)UTTypeCreatePreferredIdentifierForTag (kUTTagClassFilenameExtension, (CFStringRef)[components objectAtIndex:1], 0) autorelease]];
			}
		controller = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:utis inMode:UIDocumentPickerModeOpen];
	}

	if(!controller)
		return nullptr;

	if(type == kOpenMultipleFiles)
		[controller setAllowsMultipleSelection:YES];

	fsDelegate = [[CCL_ISOLATED (FSDelegate) alloc] initWithFileSelector:this];
	controller.delegate = fsDelegate;

	retain (); // will be released in completeFileSelection

    IOSWindow* iosWindow = IOSWindow::cast (unknown_cast<Window> (window));
    if(iosWindow)
    {
        UIViewController* parentController = (UIViewController*)(iosWindow->getTopViewController ());
        [parentController presentViewController:controller animated:YES completion:nil];

        asyncOperation.share (NEW AsyncOperation);
        asyncOperation->setState (AsyncOperation::kStarted);
    }
    else
        return nullptr;

	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSFileSelector::onSelectionFinished (NSArray<NSURL*>* urls)
{
	paths.empty ();

	for(NSURL* url in urls)
	{
		url = [url URLByResolvingSymlinksInPath];
		BOOL isDir = NO;
		[[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&isDir];

		Url* result = NEW Url;
		MacUtils::urlFromNSUrl (*result, url, (isDir ? IUrl::kFolder : IUrl::kFile), true);

		paths.add (result);

		if(!isDir)
		{
			AutoPtr<IAsyncOperation> op = System::GetFileManager ().triggerFileUpdate (*result);
			if(op && op->getState () == AsyncOperation::kStarted)
			{
				numPendingDownloads++;
				Promise p (op.detach ());
				p.then (this, &IOSFileSelector::onFileUpdateDone);
			}
		}
	}

	if(hasPendingDownloads () == false)
		completeFileSelection ();
}


//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API IOSFileSelector::getSaveBehavior () const
{
	return IFileSelector::kSaveNeedsContent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IOSFileSelector::setSaveContent (UrlRef url)
{
	saveContent = url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSFileSelector::onFileUpdateDone (IAsyncOperation& operation)
{
	if(operation.getState () != AsyncOperation::kCompleted)
	{
		failedDownloads += 1;
	}

    numPendingDownloads--;

	if(hasPendingDownloads () == false)
		completeFileSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSFileSelector::hasPendingDownloads ()
{
	return numPendingDownloads > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSFileSelector::completeFileSelection ()
{
	// release asyncOperation when leaving this method
	SharedPtr<AsyncOperation> asyncOp (asyncOperation);
	asyncOperation = nullptr;

	if(failedDownloads > 0)
	{
		Promise (Alert::errorWithContextAsync (String ().append (XSTR (Reopen))));
		asyncOp->setState (AsyncOperation::kFailed);
	}
	else
	{
		if(asyncOp)
		{
			asyncOp->setResult (!paths.isEmpty ());
			asyncOp->setState (AsyncOperation::kCompleted);
		}
	}

	release (); // was retained in runPlatformSelectorAsync
}

//************************************************************************************************
// FolderSelectorDelegate
//************************************************************************************************

@interface CCL_ISOLATED (FolderSelectorDelegate) : NSObject <UIDocumentPickerDelegate>
{
    IOSFolderSelector* folderSelector;
}

- (id)initWithFolderSelector:(IOSFolderSelector*)_folderSelector;
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls;
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentAtURL:(NSURL*)url;
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (FolderSelectorDelegate)

- (id)initWithFolderSelector:(IOSFolderSelector*)_folderSelector
{
    if(self = [super init])
        folderSelector = _folderSelector;

    return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    if(folderSelector)
    {
        folderSelector->onSelectionFinished ((urls && urls.count == 1) ? [urls objectAtIndex:0] : nil);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////


- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentAtURL:(NSURL*)url
{
    if(folderSelector)
    {
        folderSelector->onSelectionFinished (url ? url : nil);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    if(folderSelector)
    {
        folderSelector->onSelectionFinished (nil);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

@end

//************************************************************************************************
// IOSFolderSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (IOSFolderSelector, NativeFolderSelector, "FolderSelector")
DEFINE_CLASS_UID (IOSFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFolderSelector::IOSFolderSelector ()
: controller (nil),
  folderSelectorDelegate (nil)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSFolderSelector::~IOSFolderSelector ()
{
    if(controller)
        [controller release];
    if(folderSelectorDelegate)
        [folderSelectorDelegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* IOSFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
    if(controller)
    {
        [controller release];
        controller = nil;
    }
    
    if(folderSelectorDelegate)
    {
        [folderSelectorDelegate release];
        folderSelectorDelegate = nil;
    }
    
    NSString* uti = (NSString*)kUTTypeFolder;
    
    controller = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[uti] inMode:UIDocumentPickerModeOpen];
    folderSelectorDelegate = [[CCL_ISOLATED (FolderSelectorDelegate) alloc] initWithFolderSelector:this];
    controller.delegate = folderSelectorDelegate;

//	TODO: Setting directoryURL does not work on iPhone and iPad. Waiting for investigation by Apple (ticket is open).
//  NSURL* directoryURL = [NSURL alloc];
//  MacUtils::urlToNSUrl (getPath(), directoryURL);
//  controller.directoryURL = directoryURL;

    [controller setAllowsMultipleSelection:NO];
    
    IOSWindow* iosWindow = IOSWindow::cast (unknown_cast<Window>(window));
    
    if(iosWindow)
    {
        UIViewController* parentController = (UIViewController*)(iosWindow->getTopViewController ());
        [parentController presentViewController:controller animated:YES completion:nil];
        
        retain (); // release in onResult
        
        asyncOperation.share (NEW AsyncOperation);
        asyncOperation->setState (AsyncOperation::kStarted);
    }
    else
    {
        return nullptr;
    }

    return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSFolderSelector::onSelectionFinished (NSURL* url)
{
    SharedPtr<AsyncOperation> asyncOperation (this->asyncOperation);
    this->asyncOperation = nullptr;
    
    if(url)
    {
        Url result;
        MacUtils::urlFromNSUrl (result, url, IUrl::kFolder, true);
        this->setPath (result);
    }
    
    release ();
    
    if(asyncOperation)
    {
        if(url)
        {
            asyncOperation->setResult (true);
            asyncOperation->setState (AsyncOperation::kCompleted);
        }
        else
        {
            asyncOperation->setResult (false);
            asyncOperation->setState (AsyncOperation::kCanceled);
        }
    }
}
