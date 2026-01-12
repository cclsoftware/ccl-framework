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
// Filename    : ccl/platform/cocoa/gui/dragndrop.ios.cpp
// Description : iOS Drag-and-Drop
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/gui.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {

//************************************************************************************************
// IOSDragSession
//************************************************************************************************

class IOSDragSession: public DragSession
{
public:
	DECLARE_CLASS (IOSDragSession, DragSession)

	IOSDragSession (IUnknown* source = 0, int inputDevice = kMouseInput);
	~IOSDragSession ();

	PROPERTY_SHARED_AUTO (AsyncOperation, dragOperation, DragOperation)

	// DragSession
	IAsyncOperation* CCL_API dragAsync () override;
	void showNativeDragImage (bool state) override;
	void onDragFinished (const DragEvent& event) override;

private:
	Sprite* dragSprite;
	DragGuard* dragGuard;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::create (IUnknown* source, int inputDevice)
{
	return NEW IOSDragSession (source, inputDevice);
}

//************************************************************************************************
// IOSDragSession
//************************************************************************************************

DEFINE_CLASS (IOSDragSession, DragSession)
DEFINE_CLASS_UID (IOSDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSDragSession::IOSDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  dragSprite (nullptr),
  dragGuard (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSDragSession::~IOSDragSession ()
{
	delete dragGuard;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API IOSDragSession::dragAsync ()
{
    Point where;
    GUI.getMousePosition (where);

    // find window
    IWindow* parentWindow = nullptr;
    UnknownPtr<IView> view = source;
    if(view)
        parentWindow = view->getIWindow ();
    if(parentWindow == nullptr)
        parentWindow = Desktop.getDialogParentWindow ();
    if(parentWindow == nullptr)
        parentWindow = Desktop.getApplicationWindow ();
    if(parentWindow == nullptr)
        parentWindow = Desktop.findWindow (where);

    Window* window = unknown_cast<Window> (parentWindow);
    if(window)
    {
        GUI.hideTooltip ();

		ASSERT (!dragGuard)
        dragGuard = NEW DragGuard (*this);

        ASSERT (dragSprite == nullptr)
        if(dragImage)
        {
            AutoPtr<IDrawable> drawable (NEW ImageDrawable (dragImage, 0.7f));
			IImage::Selector (dragImage, IImage::kNormal);
            Rect size;
            dragImage->getSize (size);
            dragSprite = NEW FloatingSprite (window, drawable, size, ISprite::kKeepOnTop);

            showNativeDragImage (!hasVisualFeedback ());
        }

        window->screenToClient (where);

        DragEvent dragEvent (*this, DragEvent::kDragEnter, where);
        dragEvent.keys.keys |= KeyState::kLButton;
        window->onDragEnter (dragEvent);
	}

	retain (); // released in onDragFinished

	AsyncOperation* operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);
	setDragOperation (operation);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSDragSession::onDragFinished (const DragEvent& event)
{
    if(dragSprite)
    {
        dragSprite->hide ();
        dragSprite->release ();
        dragSprite = nullptr;
    }

	if(dragOperation)
	{
		dragOperation->setResult (getResult ());
		dragOperation->setState (AsyncOperation::kCompleted);
	}

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSDragSession::showNativeDragImage (bool state)
{
    if(dragSprite)
    {
        if(state)
        {
            Rect size (dragSprite->getSize ());
            CCL_PRINTF ("showNativeDragImage: x = %d y = %d width = %d height = %d\n",
                        size.left, size.top, size.getWidth (), size.getHeight ())

            Point pos;
            GUI.getMousePosition (pos);
            dragSprite->getView ()->screenToClient (pos);
			pos.offset (-size.getWidth () / 2, -size.getHeight () - 10); // place centered above finger
            size.moveTo (pos);

            dragSprite->move (size);
            dragSprite->show ();
        }
        else
            dragSprite->hide ();

		dragImageVisible (state);
    }
}
