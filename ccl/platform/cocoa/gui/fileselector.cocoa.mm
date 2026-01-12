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
// Filename    : ccl/platform/cocoa/gui/fileselector.cocoa.mm
// Description : platform-specific file selector code
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/dialogs/fileselector.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/system/systemtimer.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// CocoaFileSelector
//************************************************************************************************

class CocoaFileSelector: public NativeFileSelector
{
public:
	DECLARE_CLASS (CocoaFileSelector, NativeFileSelector)

	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	
protected:
	static ChildWindow* createChildWindow (View* customView, NSView* accessoryView);
};

//************************************************************************************************
// CocoaFolderSelector
//************************************************************************************************

class CocoaFolderSelector: public NativeFolderSelector
{
public:
	DECLARE_CLASS (CocoaFolderSelector, NativeFolderSelector)

	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
};

} // namespace CCL

using namespace CCL;

extern void modalStateEnter ();
extern void modalStateLeave ();

//************************************************************************************************
// FSControl
//************************************************************************************************

@interface CCL_ISOLATED (FSControl): NSView
{}
- (BOOL)isFlipped;
@end

@implementation CCL_ISOLATED (FSControl)
- (BOOL)isFlipped
{
    return YES;
}
@end

//************************************************************************************************
// FSDelegate
//************************************************************************************************

@interface CCL_ISOLATED (FSDelegate) : NSObject<NSOpenSavePanelDelegate>
{
	NativeFileSelector* fileSelector;
	NSSavePanel* nsPanel;
	int itemIndex;
}

- (id)initWithFileSelector:(NativeFileSelector*)fileSelector;
- (void)setPanel:(NSSavePanel*)panel;
- (void)panelSelectionDidChange:(id)sender;
- (void)typeSelectionChanged:(id)sender;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (FSDelegate)

- (id)initWithFileSelector:(NativeFileSelector*)_fileSelector
{
	self = [super init];
	self->fileSelector = _fileSelector;
	itemIndex = 0;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setPanel:(NSSavePanel*)panel
{
	self->nsPanel = panel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)panelSelectionDidChange:(id)sender
{
	UnknownPtr<IFileSelectorHook> hook (fileSelector->getHook ());
	if(hook)
	{
		NSSavePanel* _nsPanel = sender;
		NSURL* nsUrl = [_nsPanel URL];
		
		if(nsUrl != nil)
		{
			BOOL isDir = NO;
			[[NSFileManager defaultManager] fileExistsAtPath:[nsUrl path] isDirectory:&isDir];
			
			Url result;
			MacUtils::urlFromNSUrl (result, nsUrl, (isDir ? IUrl::kFolder : IUrl::kFile));
			hook->onSelectionChanged (*fileSelector, result);

			SystemTimer::serviceTimers ();
			for(NSView* subView in [[_nsPanel accessoryView] subviews])
				[subView setNeedsDisplay:YES];
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)typeSelectionChanged:(id)sender
{
	NSPopUpButton* popup = sender;
	itemIndex = (int)[[popup selectedItem] tag];

	fileSelector->setSelectedType (itemIndex);
	
	switch(itemIndex)
	{
	case 0:
	{
		NSMutableArray* extensionArray = [NSMutableArray array];
		int filterCount = fileSelector->countFilters ();
		for(int i = 0; i < filterCount; i++)
		{
			const CCL::FileType* filter = fileSelector->getFilter (i);
			NSString* extension = filter->getExtension ().createNativeString<NSString*> ();
			[extensionArray addObject:extension];
			[extension release];
		}
		[nsPanel setAllowedFileTypes:extensionArray];
		break;
	}
	case 1:
		[nsPanel setAllowedFileTypes:nil];	
		break;
	default:
		const CCL::FileType* filter = fileSelector->getFilter (itemIndex - 2);
		NSString* extension = filter->getExtension ().createNativeString<NSString*> ();
		[nsPanel setAllowedFileTypes:[NSArray arrayWithObject:extension]];
		[extension release];
		break;
	}
}

@end

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileSelector")
	XSTRING (ReadableDocuments, "All Readable Documents")
	XSTRING (AllDocuments, "All Documents")
END_XSTRINGS

//************************************************************************************************
// CocoaFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CocoaFileSelector, NativeFileSelector, "FileSelector")
DEFINE_CLASS_UID (CocoaFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

ChildWindow* CocoaFileSelector::createChildWindow (View* customView, NSView* accessoryView)
{
	ChildWindow* childWindow = nullptr;
	if(customView)
	{
		Rect size (customView->getSize ());
		size.moveTo (Point ((Coord)(([accessoryView frame].size.width - size.getWidth ()) / 2), 30));
		childWindow = NEW ChildWindow (accessoryView, Window::kWindowModeEmbedding, size);
		childWindow->setTheme (const_cast<Theme*> (&customView->getTheme ()));
		childWindow->setCollectGraphicUpdates (true);
		childWindow->addView (customView);
		if(customView->hasVisualStyle ())
			childWindow->setVisualStyle (unknown_cast<VisualStyle> (&customView->getVisualStyle ()));
		
		Desktop.addWindow (childWindow, kDialogLayer);
	}
	
	return childWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{	
	setSelectedType (0);
	NSString* nsTitle = title.createNativeString<NSString*> ();

	NSView* accessoryView = nil;
	CCL_ISOLATED (FSDelegate)* fsDelegate = [[CCL_ISOLATED (FSDelegate) alloc] initWithFileSelector:this];
	NSInteger result = NSModalResponseCancel;
	NSArray* urls = nil;

	ChildWindow* childWindow = nullptr;
	
	@try 
	{
		View* customView = createCustomView ();
		int minWidth = customView ? customView->getWidth () + 40 : 300;
		int minHeight = customView ? customView->getHeight () + 35 : 35;
		int popupWidth = 300;
		
		NSPopUpButton* popupButton = nil;
		if(countFilters () > 1)
		{
			NSRect frame = { {0, 0}, {static_cast<CGFloat>(minWidth), static_cast<CGFloat>(minHeight)} };
			accessoryView = [[NSView alloc] initWithFrame:frame];
			
			NSRect buttonFrame =  { {static_cast<CGFloat>((minWidth - popupWidth) / 2), 5}, {static_cast<CGFloat>(popupWidth), 23} };
			popupButton = [[NSPopUpButton alloc] initWithFrame:buttonFrame pullsDown:NO];
			[popupButton setTarget:fsDelegate];
			[popupButton setAction:@selector(typeSelectionChanged:)];

			NSMenu* nsMenu = [[NSMenu alloc] initWithTitle:@""];
			if(type != kSaveFile)
			{
				[[nsMenu addItemWithTitle:[XSTR (ReadableDocuments).createNativeString<NSString*> () autorelease] action:nil keyEquivalent:@""] setTag:0];
				[nsMenu addItem:[NSMenuItem separatorItem]];
			}
			
			for(int i = 0; i < countFilters (); i++)
			{
				String title = getFilter (i)->getDescription ();
				title = title << " (." << getFilter (i)->getExtension () << ")";
				NSString* filterTitle = title.createNativeString<NSString*> ();
				
				[[nsMenu addItemWithTitle:filterTitle action:nil keyEquivalent:@""] setTag:i+2];
				title.releaseNativeString (filterTitle);
			}
			
			if(type != kSaveFile)
			{
				[nsMenu addItem:[NSMenuItem separatorItem]];
				[[nsMenu addItemWithTitle:[XSTR (AllDocuments).createNativeString<NSString*> () autorelease]  action:nil keyEquivalent:@""] setTag:1];
			}
			
			[popupButton setMenu:nsMenu];
            [nsMenu release];
			[popupButton setIntValue:filterIndex];
			
			[accessoryView addSubview:popupButton];
            [popupButton release];
		}
		
 		if(type != kSaveFile)
		{
			NSOpenPanel* nsOpenPanel = [[NSOpenPanel openPanel] retain];
			[fsDelegate setPanel:nsOpenPanel];

			NSURL* initialDirectory = nil;
			if(!getInitialFolder ().isEmpty ())
			{
				initialDirectory = [NSURL alloc];
				CCL::Url dir = getInitialFolder ();
				if(!getInitialFileName ().isEmpty ())
					dir.descend (getInitialFileName ());

				MacUtils::urlToNSUrl (dir, initialDirectory);
			}

			[nsOpenPanel setTitle:nsTitle];
			[nsOpenPanel setAllowsMultipleSelection:(type == kOpenMultipleFiles) ? YES : NO];
			[nsOpenPanel setCanChooseFiles:YES];
			[nsOpenPanel setAllowedFileTypes:nil];
			
			[nsOpenPanel setCanChooseDirectories:NO];
			[nsOpenPanel setCanCreateDirectories:NO];
			[nsOpenPanel setDelegate:fsDelegate];	
			if(popupButton)
				[fsDelegate typeSelectionChanged:popupButton];
			else if(countFilters () > 0)
			{
				const CCL::FileType* filter = getFilter (0);
				NSString* extension = filter->getExtension ().createNativeString<NSString*> ();
				[nsOpenPanel setAllowedFileTypes:[NSArray arrayWithObject:extension]];
			}
			
            if(initialDirectory)
                [nsOpenPanel setDirectoryURL:initialDirectory];			
			if(accessoryView)
			{
				[nsOpenPanel setAccessoryView:accessoryView];
				childWindow = createChildWindow (customView, accessoryView);
			}
			
			modalStateEnter ();
			result = [nsOpenPanel runModal];
			modalStateLeave ();
			
			if(result == NSModalResponseOK)
				urls = [nsOpenPanel URLs];
			[nsOpenPanel release];
		}
		else if (type == kSaveFile)
		{
			NSURL* initialDirectory = nil;
			if(!getInitialFolder ().isEmpty ())
			{
				initialDirectory = [NSURL alloc];
				MacUtils::urlToNSUrl (getInitialFolder (), initialDirectory);
			}

			NSString* initialFileName = nil;
			if(!getInitialFileName ().isEmpty ())
				initialFileName = getInitialFileName ().createNativeString<NSString*> ();

			NSSavePanel* nsSavePanel = [[NSSavePanel savePanel] retain];
			[fsDelegate setPanel:nsSavePanel];

			[nsSavePanel setTitle:nsTitle];
			[nsSavePanel setAllowedFileTypes:nil]; 
			[nsSavePanel setCanCreateDirectories:YES];
			[nsSavePanel setDelegate:fsDelegate];
			if(popupButton)
				[fsDelegate typeSelectionChanged:popupButton];
			else if(countFilters () > 0)
			{
				const CCL::FileType* filter = getFilter (0);
				NSString* extension = filter->getExtension ().createNativeString<NSString*> ();
				[nsSavePanel setAllowedFileTypes:[NSArray arrayWithObject:extension]];
			}
					
			if(initialFileName != nil)
				[nsSavePanel setNameFieldStringValue:initialFileName];
			if(initialDirectory) 
				[nsSavePanel setDirectoryURL:initialDirectory];
			if(accessoryView)
			{
				[nsSavePanel setAccessoryView:accessoryView];
				childWindow = createChildWindow (customView, accessoryView);
			}

			modalStateEnter ();
			result = [nsSavePanel runModal];
			modalStateLeave ();
			if(childWindow)
				Desktop.removeWindow (childWindow); 
			
			if(result == NSModalResponseOK)
				urls = [NSArray arrayWithObject:[nsSavePanel URL]];
			[nsSavePanel release];
		}
		
	}
	@catch (NSException* e) 
	{
		[NSApp reportException:e];
		CCL_DEBUGGER ("")
	}
	
    [accessoryView release];
	[nsTitle release];
	
	if(result == NSModalResponseOK && urls != nil)
	{
		NSEnumerator* enumerator = [urls objectEnumerator];
		while(true)
		{
			NSURL* nsUrl = [enumerator nextObject];
			if(nsUrl == nil)
				break;
			
			Url* result = NEW Url;
			MacUtils::urlFromNSUrl (*result, nsUrl, IUrl::kFile, true);			
			
			if(type == kSaveFile)
			{
				if(countFilters() > 0)
				{
					const FileType& fileType = *getFilter (selectedType < 2 ? 0 : selectedType - 2);
						
					// don't replace a pseudo extension entered by the user ("www.mysong.de")
					String path (result->getPath ());
					String ext (".");
					ext << fileType.getExtension ();

					if(!path.endsWith (ext, result->isCaseSensitive ()))
					{
						path << ext; // add it to the path string manually, so the previous part doesn't get replaced
						result->setPath (path);
					}
					result->setFileType (fileType, true);
				}
			}
			paths.add (result);
		}
	}
	
	if(childWindow)
	{
        childWindow->close ();
		childWindow = nullptr;
	}

	if(customView)
	{
		customView->release ();
		customView = nullptr;
	}
	[fsDelegate autorelease];
	
	return paths.count() > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CocoaFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	int result = runPlatformSelector (type, title, filterIndex, window);

	return AsyncOperation::createCompleted (result);
}

//************************************************************************************************
// CocoaFolderSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (CocoaFolderSelector, NativeFolderSelector, "FolderSelector")
DEFINE_CLASS_UID (CocoaFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	NSString* nsTitle = title.createNativeString<NSString*> ();
	
    NSURL* initialDirectory = nil;
    if(!getInitialPath ().isEmpty ())
	{
		initialDirectory = [NSURL alloc];
		MacUtils::urlToNSUrl (getInitialPath (), initialDirectory);	
	}


	NSOpenPanel* nsOpenPanel = [NSOpenPanel openPanel];
	[nsOpenPanel setTitle:nsTitle];
	[nsTitle release];
	[nsOpenPanel setAllowsMultipleSelection:NO];
	[nsOpenPanel setCanChooseFiles:NO];
	[nsOpenPanel setCanChooseDirectories:YES];
	[nsOpenPanel setCanCreateDirectories:YES];
    if(initialDirectory)
		[nsOpenPanel setDirectoryURL:initialDirectory];

	modalStateEnter ();
	NSInteger result = [nsOpenPanel runModal];	
	modalStateLeave ();

	NSURL* nsUrl = [nsOpenPanel URL];
	
	if(result == NSModalResponseOK)
	{
		MacUtils::urlFromNSUrl (*path, nsUrl, IUrl::kFolder, true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CocoaFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
	int result = runPlatformSelector (title, window);

	return AsyncOperation::createCompleted (result);
}
