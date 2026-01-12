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
// Filename    : ccl/gui/system/dragndrop.cpp
// Description : Drag-and-Drop
//
//************************************************************************************************

#include "ccl/gui/system/dragndrop.h"

#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/controls/autoscroller.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/dialogs/alert.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/objectconverter.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/collections/container.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/guievent.h"

using namespace CCL;

//************************************************************************************************
// DragSession::DeferredDrop
//************************************************************************************************

class DragSession::DeferredDrop: public Object
{
public:
	DeferredDrop (IDragHandler* handler, const DragEvent& dragEvent, DragSession* session, View* dragView)
	: dragEvent  (dragSession, dragEvent.eventType, dragEvent.where, dragEvent.keys)
	{
		ASSERT (handler && session)
		ASSERT (&dragEvent.session == session)
		ASSERT (session->getDragHandler () == handler || session->getSourceDragHandler () == handler)
		
		if(handler && session)
		{
			// make copy of drag session
			this->dragSession.copyFrom (*session);
			this->dragSession.setResult (session->getResult ());
			this->dragSession.setDragHandler (handler);
			this->dragSession.getAttributes ().copyFrom (session->getAttributes ());

			// remember window of target view
			if(dragView)
				dragWindow = dragView->getWindow ();

			(NEW Message ("deferDrop"))->post (this);
		}
	}
	
	~DeferredDrop ()
	{
		cancelSignals ();
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == "deferDrop" && dragSession.getHandler ())
		{
			auto canDeliver = [&] ()
			{
				// don't deliver during an alert
				IDialogInformation* dialogInfo = AlertService::instance ().getCurrentDialog ();
				if(dialogInfo && dialogInfo->getDialogType () == IDialogInformation::kStandardAlert) // surprise: can also be a dialog!
					return false;

				// if there's a modal dialog, we can only deliver a deferred drop for that dialog
				IWindow* topModal = Desktop.getTopWindow (kDialogLayer);
				if(topModal && dragWindow && topModal != dragWindow)
					return false;

				return true;
			};

			if(getActiveSession ())
			{
				// we must not deliver until the platform drag loop is done (and the mouseDown that called it)
				// a single deferred message does not to guarantee this on all platforms
				CCL_PRINT ("DeferredDrop: still in drag session, defer further\n")
				(NEW Message (msg))->post (this, 10);
			}
			else if(canDeliver ())
			{
				dragSession.getHandler ()->afterDrop (dragEvent);
				release ();
			}
			else
			{
				CCL_PRINT ("defering Drop further\n")
				(NEW Message (msg))->post (this, 1000);
			}
		}
	}

private:
	DragSession dragSession;
	DragEvent dragEvent;
	ObservedPtr<IWindow> dragWindow;
};

//************************************************************************************************
// DragSession::DragGuard
//************************************************************************************************

DragSession::DragGuard::DragGuard (DragSession& session)
: oldSession (DragSession::activeSession)
{
	DragSession::activeSession = &session;
	if(session.getSource ())
		session.getSource ()->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession::DragGuard::~DragGuard ()
{
	if(DragSession::activeSession->getSource ())
		DragSession::activeSession->getSource ()->release ();

	DragSession::activeSession->signal (Message ("endDrag"));
	DragSession::activeSession = oldSession;
}

//************************************************************************************************
// DragSession::DropGuard
//************************************************************************************************

DragSession::DropGuard::DropGuard (DragSession& session)
: oldSession (DragSession::activeSession)
{
	// ensure that there is an activeSession during drop
	if(oldSession == nullptr)
		DragSession::activeSession = &session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession::DropGuard::~DropGuard ()
{
	DragSession::activeSession = oldSession;
}

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::activeSession = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragSession::isInternalDragActive ()
{
	return activeSession && !activeSession->isDropped ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession* DragSession::getActiveSession (bool target)
{
	if(target && activeSession && activeSession->targetSession)
		return activeSession->targetSession;

	return activeSession;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (DragSession, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession::DragSession (IUnknown* source, int inputDevice)
: source (source),
  sourceSession (nullptr),
  targetSession (nullptr),
  dropResult (kDropNone),
  sourceResult (kDropNone),
  dragImage (nullptr),
  canceled (false),
  dropped (false),
  sourceHandlerActive (false),
  autoScroller (nullptr),
  flags (0),
  inputDevice (inputDevice)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession::DragSession (int inputDevice)
: source (nullptr),
  sourceSession (nullptr),
  targetSession (nullptr),
  dropResult (kDropNone),
  sourceResult (kDropNone),
  dragImage (nullptr),
  backColor (Colors::kWhite),
  canceled (false),
  dropped (false),
  sourceHandlerActive (false),
  autoScroller (nullptr),
  flags (0),
  inputDevice (inputDevice)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragSession::~DragSession ()
{
	if(dragImage)
		dragImage->release ();

	if(autoScroller)
		autoScroller->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::copyFrom (const DragSession& other)
{
	setSource (other.getSource ());
	setOffset (other.getOffset ());
	setSize (other.getSize ());
	setTargetID (other.getTargetID ());
	setInputDevice (other.getInputDevice ());

	// share items of other list...
	ForEachUnknown (other.items, obj)
		items.add (obj, true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::setTargetSession (DragSession* session)
{
	targetSession = session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DragSession::drag ()
{
	Promise promise (dragAsync ());
	return getResult ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API DragSession::dragAsync ()
{
	CCL_NOT_IMPL ("DragSession::IAsyncOperation* CCL_API dragAsync")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setSource (IUnknown* _source)
{
	source = _source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setTargetID (StringID _targetID)
{
	targetID = _targetID;

	// copy to source for internal drag operation
	if(sourceSession)
		sourceSession->setTargetID (targetID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API DragSession::getTargetID () const
{
	return targetID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API DragSession::getSource () const
{
	return source;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::setAutoScrollTarget (View* view)
{
	if(dragHandler && !dragHandler->wantsAutoScroll ())
	{
		if(autoScroller)
			autoScroller->setTargetView (nullptr);
	}
	else
	{
		if(autoScroller)
			autoScroller->setTargetView (view);
		else
		{
			autoScroller = NEW AutoScroller (view);
			autoScroller->setDragSession (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::triggerAutoScroll ()
{
	if(autoScroller)
		autoScroller->onMouseMove (MouseHandler::kAutoScroll);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setSize (const Rect& _size)
{
	size = _size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Rect& CCL_API DragSession::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setOffset (const Point& _offset)
{
	offset = _offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Point& CCL_API DragSession::getOffset () const
{
	return offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DragSession::getResult () const
{
	return dropResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setResult (int result)
{
	dropResult = result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DragSession::getTotalResult () const
{
	return sourceResult == kDropNone ? dropResult : sourceResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragSession::wasCanceled () const
{
	return canceled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::setCanceled (bool state)
{
	canceled = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setDragImage (IImage* _image, const Color& backColor)
{
	if(Image* image = unknown_cast<Image> (_image))
	{
		take_shared<Image> (dragImage, image);
		this->backColor = backColor;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::leaveDragHandler (const DragEvent& event)
{
	if(dragHandler)
	{
		dragHandler->dragLeave (event);
		setHandler (nullptr);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragSession::getText (String& text)
{
	if(Boxed::String* string = unknown_cast<Boxed::String> (items.getFirst ()))
	{
		text = *string;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownList& CCL_API DragSession::getItems ()
{
	return items;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList& CCL_API DragSession::getAttributes ()
{
	if(sourceSession)
		return sourceSession->getAttributes ();
	return attributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API DragSession::getDragHandler () const
{
	return getHandler ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setDragHandler (IDragHandler* handler)
{
	setHandler (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* DragSession::getSourceDragHandler () const
{
	if(!sourceDragHandler && sourceSession)
		return sourceSession->getSourceDragHandler ();

	return sourceDragHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::setSourceDragHandler (IDragHandler* handler)
{
	sourceDragHandler = handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::setSourceResult (int result)
{
	sourceResult = result;

	if(sourceSession)
		sourceSession->setSourceResult (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DragSession::getSourceResult () const
{
	return sourceResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::deferDrop (IDragHandler* handler, const DragEvent& dragEvent, View* dragView)
{
	NEW DeferredDrop (handler, dragEvent, this, dragView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragSession::hasVisualFeedback () const
{
	return dragHandler && dragHandler->hasVisualFeedback ();	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::showNativeDragImage (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* DragSession::getDragImage () const
{
	return dragImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DragSession::getInputDevice () const
{
	return inputDevice;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DragSession::setInputDevice (int device)
{
	inputDevice = device;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragSession::containsNativePaths ()
{
	if(!pathsChecked ())
	{
		pathsChecked (true);
		ForEachUnknown (items, obj)
			AutoPtr<IUrl> url = ObjectConverter::toInterface<IUrl> (obj);
			if(url && url->isNativePath ())
			{
				hasNativePaths (true);
				return true;
			}
		EndFor
	}
	return hasNativePaths ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragSession::getNativePaths (Container& urls)
{
	ASSERT (urls.isObjectCleanup ()) 

	ForEachUnknown (items, obj)
		AutoPtr<IUrl> url = ObjectConverter::toInterface<IUrl> (obj);
		if(url && url->isNativePath ())
			urls.add (NEW Url (*url));
	EndFor
	return !urls.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragSession::onDragFinished (const DragEvent& event)
{}
