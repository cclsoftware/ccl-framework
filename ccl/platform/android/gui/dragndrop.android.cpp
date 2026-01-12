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
// Filename    : ccl/platform/android/gui/dragndrop.android.cpp
// Description : Android Drag-and-Drop
//
//************************************************************************************************

#include "ccl/gui/system/dragndrop.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/gui.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {

//************************************************************************************************
// AndroidDragSession
//************************************************************************************************

class AndroidDragSession: public DragSession
{
public:
	DECLARE_CLASS (AndroidDragSession, DragSession)

	AndroidDragSession (IUnknown* source = 0, int inputDevice = kMouseInput);
	~AndroidDragSession ();

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
	return NEW AndroidDragSession (source, inputDevice);
}

//************************************************************************************************
// AndroidDragSession
//************************************************************************************************

DEFINE_CLASS (AndroidDragSession, DragSession)
DEFINE_CLASS_UID (AndroidDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidDragSession::AndroidDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  dragSprite (0),
  dragGuard (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidDragSession::~AndroidDragSession ()
{
	delete dragGuard;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API AndroidDragSession::dragAsync ()
{
    Point where;
    GUI.getMousePosition (where);

    // find window
    IWindow* parentWindow = 0;
    UnknownPtr<IView> view = source;
    if(view)
        parentWindow = view->getIWindow ();
    if(parentWindow == 0)
        parentWindow = Desktop.getDialogParentWindow ();
    if(parentWindow == 0)
        parentWindow = Desktop.getApplicationWindow ();
    if(parentWindow == 0)
        parentWindow = Desktop.findWindow (where);

    Window* window = unknown_cast<Window> (parentWindow);
	if(window)
    {
        GUI.hideTooltip ();

		ASSERT (!dragGuard)
        dragGuard = NEW DragGuard (*this);

        ASSERT (dragSprite == 0)
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

void AndroidDragSession::onDragFinished (const DragEvent& event)
{
    if(dragSprite)
    {
        dragSprite->hide ();
        dragSprite->release ();
        dragSprite = 0;
    }

	if(dragOperation)
	{
		dragOperation->setResult (getResult ());
		dragOperation->setState (AsyncOperation::kCompleted);
	}

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidDragSession::showNativeDragImage (bool state)
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
