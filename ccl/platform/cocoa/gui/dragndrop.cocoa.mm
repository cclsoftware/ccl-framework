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
// Filename    : ccl/platform/cocoa/gui/dragndrop.cocoa.mm
// Description : Mac OS Drag-and-Drop
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/dragndrop.cocoa.h"

#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/system/systemtimer.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/imaging/offscreen.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/objectconverter.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/system/isysteminfo.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

using namespace CCL;
using namespace MacOS;

CocoaDragSession* gInsideDrag = nil;
Sprite* gDragSprite = nil;

NSArray* getDragTypes ()
{
	return @[NSPasteboardTypeString, NSPasteboardTypeFileURL];
}

//************************************************************************************************
// DragSource
//************************************************************************************************

@interface CCL_ISOLATED (DragSource): NSObject
{
	CocoaDragSession* currentDragSession;
}
- (id)initWithSession:(CocoaDragSession*)session;
- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)flag;
@end

@implementation CCL_ISOLATED (DragSource)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithSession:(CocoaDragSession*)session
{
	self = [super init];
	self->currentDragSession = session;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
	return NSDragOperationEvery;
}

@end

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::create (IUnknown* source, int inputDevice)
{
	return NEW CocoaDragSession (source, inputDevice);
}

//************************************************************************************************
// CocoaDragSession
//************************************************************************************************

DEFINE_CLASS (CocoaDragSession, DragSession)
DEFINE_CLASS_UID (CocoaDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaDragSession::CocoaDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  dragInfo (0),
  dragGuard (nullptr)
{
	ASSERT (gInsideDrag == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaDragSession::CocoaDragSession (id <NSDraggingInfo> dragInfo, int inputDevice)
: DragSession (inputDevice),
  dragInfo (dragInfo),
  dragGuard (nullptr)
{
	ASSERT (gInsideDrag == nullptr)
	convertNativeItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaDragSession::~CocoaDragSession ()
{
	if(dragGuard)
		delete dragGuard;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API CocoaDragSession::dragAsync ()
{
	IWindow* parentWindow = nullptr;

	UnknownPtr<IView> view = source;
	if(view)
		parentWindow = view->getIWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getDialogParentWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getApplicationWindow ();
	Window* window = unknown_cast<Window> (parentWindow);

	ASSERT (gDragSprite == nullptr)
	if(dragImage && window)
	{
		AutoPtr<IDrawable> drawable (NEW ImageDrawable (dragImage, 0.7f));
		Rect size;
		dragImage->getSize (size);
		gDragSprite = NEW FloatingSprite (window, drawable, size, ISprite::kKeepOnTop);
	}

	NSEvent* nsEvent = [NSApp currentEvent];
	if(nsEvent == nil)
		return AsyncOperation::createCompleted (IDragSession::kDropNone);

	NSEventType eventType = [nsEvent type];
	if(eventType == NSEventTypeLeftMouseDown  || eventType == NSEventTypeLeftMouseDragged || eventType == NSEventTypeRightMouseDown || eventType == NSEventTypeRightMouseDragged || eventType == NSEventTypeOtherMouseDown || eventType == NSEventTypeOtherMouseDragged || eventType == NSEventTypeTabletPoint)
	{
		NSWindow* nsWindow = [nsEvent window];
		if(nsWindow == nil)
			return AsyncOperation::createCompleted (IDragSession::kDropNone);

		@try
		{
			NSObj<NSMutableArray> pasteBoardItems = [[NSMutableArray alloc] init];
			NSObj<NSMutableArray> draggingItems = [[NSMutableArray alloc] init];

			ForEachUnknown (getItems (), unknown)
				NSObj<NSPasteboardItem> item = [[NSPasteboardItem alloc] init];

				// String representaion
				String string;
				if(Clipboard::instance ().toText (string, unknown))
				{
					NSObj<NSString> nativeString = string.createNativeString<NSString*> ();
					[item setString:nativeString forType:NSPasteboardTypeString];
				}

				// URL representaion
				AutoPtr<IUrl> url = ObjectConverter::toInterface<IUrl> (unknown);
				if(url && url->isNativePath ())
				{
					NSURL* nativeUrl = [NSURL alloc];
					MacUtils::urlToNSUrl (*url, nativeUrl);
					[item setString:[nativeUrl absoluteString] forType:NSPasteboardTypeFileURL];
				}

				if([[item types] count] > 0)
					[pasteBoardItems addObject:item];
			EndFor

			// Placeholder
			if([pasteBoardItems count] == 0)
				[pasteBoardItems addObject:@""];
			
			for(id item in (NSMutableArray*)pasteBoardItems)
			{
				NSObj<NSDraggingItem> dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:item];
				[dragItem setDraggingFrame:NSMakeRect (0, 0, 3, 3) contents:nil];
				[draggingItems addObject:dragItem];
			}

			GUI.hideTooltip ();

			id dragSource = [[[CCL_ISOLATED (DragSource) alloc] initWithSession:this] autorelease];
			[[nsWindow contentView] beginDraggingSessionWithItems:draggingItems event:nsEvent source:dragSource];
		}
 		@catch(NSException *e)
		{
			ASSERT (0)
			Debugger::printf ("Exception", 0);
		}

		ASSERT (!dragGuard)
		dragGuard = NEW DragGuard (*this);
		gInsideDrag = this;
		retain (); // released in onDragFinished
		AsyncOperation* operation = NEW AsyncOperation;
		operation->setState (AsyncOperation::kStarted);
		setDragOperation (operation);
		return operation;
	}
	else
	{
		// there is no mouse event, so this drag was initiated by a TUIO touch operation, handle this in a modal way
		if(!window)
		{
			UnknownPtr<IView> view = source;
			if(view)
				parentWindow = view->getIWindow ();
			window = unknown_cast<Window> (parentWindow);
		}	
		
		if(!window)
			return AsyncOperation::createCompleted (IDragSession::kDropNone);
		NSWindow* nsWindow = toNSWindow (window);
		if(nsWindow == nil)
			return AsyncOperation::createCompleted (IDragSession::kDropNone);
					
		DragGuard dragGuard (*this);
		gInsideDrag = this;
		
        DragEvent dragEvent (*this, DragEvent::kDragEnter, getOffset ());
        dragEvent.keys.keys |= KeyState::kLButton;
        window->onDragEnter (dragEvent);

		NSModalSession session = [NSApp beginModalSessionForWindow:nsWindow];
		while(!(wasCanceled () || isDropped ()))
		{
			[NSApp runModalSession:session];
			NSEvent* nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.025] inMode:NSDefaultRunLoopMode dequeue:YES];
			SystemTimer::serviceTimers ();
		}
		[NSApp endModalSession:session];
		gInsideDrag = nullptr;
		if(gDragSprite)
		{
			gDragSprite->hide ();
			gDragSprite->release ();
			gDragSprite = nullptr;
		}
		return AsyncOperation::createCompleted (getResult ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaDragSession::convertNativeItems ()
{
	for(id item in [[dragInfo draggingPasteboard] readObjectsForClasses:@[[NSURL class], [NSString class]] options:@{}])
	{
		if([item isKindOfClass:[NSString class]])
		{
			Boxed::String* string = NEW Boxed::String ();
			string->appendNativeString ((NSString*)(item));
			string->normalize (Text::kNormalizationC);
			
			IUnknown* obj = Clipboard::fromText (*string);
			if(obj)
				string->release ();
			else
				obj = string->asUnknown ();
			items.add (obj, false);
		}
		else if([item isKindOfClass:[NSURL class]])
		{
			IUrl* url = NEW Url;
			bool useBookmark = System::GetSystem ().isProcessSandboxed ();
			MacUtils::urlFromNSUrl (*url, (NSURL*)(item), IUrl::kFile, useBookmark);
			if(IUrl* packageUrl = System::GetFileUtilities ().translatePathInMountedFolder (*url))
			{
				url->release ();
				url = packageUrl;
			}
			items.add (url, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaDragSession::showNativeDragImage (bool state)
{
	if(gDragSprite)
	{
		if(state)
		{
			Rect size (gDragSprite->getSize ());
			Point pos = getDragImagePosition ();
			if(getInputDevice () == kTouchInput)
				pos.offset(0, -60);
			else
				pos.offset(0, 30);
				
			gDragSprite->getView ()->windowToClient (pos);
			size.moveTo (pos);

			gDragSprite->move (size);
			gDragSprite->show ();
		}
		else
			gDragSprite->hide ();

		dragImageVisible (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaDragSession::onDragFinished (const DragEvent& event)
{
	// clean up only after the drag operation concludes
	if(!isDropped () && !wasCanceled ())
		return;

	if(gDragSprite)
	{
		gDragSprite->hide ();
		gDragSprite->release ();
		gDragSprite = nullptr;
	}

	if(dragOperation)
	{
		dragOperation->setResult (getResult ());
		dragOperation->setState (AsyncOperation::kCompleted);
	}

	if(gInsideDrag == this)
	{
		gInsideDrag = nullptr;
		release ();
	}
	else
	{
		ASSERT (gInsideDrag == nullptr)
	}
}
