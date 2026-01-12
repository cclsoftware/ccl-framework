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
// Filename    : ccl/platform/android/gui/window.android.cpp
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/gui/androidview.h"
#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"

#include "ccl/platform/android/graphics/androidbitmap.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/controls/textbox.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/graphics/dpiscale.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.FrameworkDialog
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkDialog, CCLGUI_CLASS_PREFIX "FrameworkDialog")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject)
	DECLARE_JNI_METHOD (int64, getNativeViewPtr)
	DECLARE_JNI_METHOD (void, show, int, int, int, int, bool)
	DECLARE_JNI_METHOD (void, setSize, int, int, int, int)
	DECLARE_JNI_METHOD (jobject, getSize)

	// inherited from DialogFragment
	DECLARE_JNI_METHOD (void, dismiss)
END_DECLARE_JNI_CLASS (FrameworkDialog)

DEFINE_JNI_CLASS (FrameworkDialog)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Landroid/content/Context;)V")
	DEFINE_JNI_METHOD (getNativeViewPtr, "()J")
	DEFINE_JNI_METHOD (show, "(IIIIZ)V")
	DEFINE_JNI_METHOD (setSize, "(IIII)V")
	DEFINE_JNI_METHOD (getSize, "()Landroid/graphics/Rect;")

	// inherited from DialogFragment
	DEFINE_JNI_METHOD (dismiss, "()V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkDialog, onDismissNative, JniIntPtr nativeViewPtr)
{
	FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr);
	if(!nativeView)
		return;

	if(Dialog* dialog = ccl_cast<Dialog> (nativeView->getWindow ()))
	{
		if(IWindow* popup = Desktop.getTopWindow (kPopupLayer))
			if(dialog->isChild (unknown_cast<PopupSelectorWindow> (popup)))
				popup->close ();

		dialog->onActivate (false);

		if(!dialog->isInCloseEvent ())
			dialog->onClose ();

		if(AsyncOperation* dialogOperation = dialog->getDialogOperation ())
		{
			dialogOperation->setResult (dialog->getDialogResult ());
			dialogOperation->setState (AsyncOperation::kCompleted);
		}

		dialog->setInDestroyEvent (true);
		dialog->removed (0);
		dialog->onDestroy ();

		Desktop.removeWindow (dialog);

		dialog->setInCloseEvent (false);
	}
}

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

void PopupSelectorWindow::onActivate (bool state)
{
	SuperClass::onActivate (state);
}

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	if(!activity->isForegroundActivity ())
	{
		// cannot show dialogs while in the background
		AsyncOperation* operation = AsyncOperation::createFailed ();
		setDialogOperation (operation);
		return operation;
	}

	JniAccessor jni;
	dialog.assign (jni, jni.newObject (FrameworkDialog, FrameworkDialog.construct, activity->getJObject ()));
	
	// connect to the FrameworkView created by the java FrameworkDialog
	if(FrameworkView* frameworkView = FrameworkView::fromIntPtr (FrameworkDialog.getNativeViewPtr (dialog)))
		frameworkView->initWithWindow (this);

	Rect dialogSize = getSize ();
	bool isSheetStyle = style.isCustomStyle (Styles::kWindowBehaviorSheetStyle);
	if(isSheetStyle && popupSizeInfo.parent)
		dialogSize.setSize (Rect (popupSizeInfo.sizeLimits).getRightBottom ());

	PixelPoint size (dialogSize.getSize (), getContentScaleFactor ());
	PixelPoint pos (dialogSize.getLeftTop (), getContentScaleFactor ());

	bool isCentered = !style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) || isSheetStyle;

	FrameworkDialog.show (dialog, pos.x, pos.y, size.x, size.y, isCentered);

	static_cast<Dialog*> (this)->initFocusView ();

	AsyncOperation* operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);
	setDialogOperation (operation);
	return operation;
}

//************************************************************************************************
// AndroidDialog
//************************************************************************************************

AndroidDialog::AndroidDialog (const Rect& size, StyleRef style, StringRef title)
: AndroidWindow (size, style, title),
  popupSizeInfo (Point ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidDialog::setWindowSize (Rect& size)
{
	if(dialog && frameworkView && !frameworkView->isResizing ())
	{
		PixelRect pixelSize (size, getContentScaleFactor ());

		FrameworkDialog.setSize (dialog, pixelSize.left, pixelSize.top, pixelSize.getWidth (), pixelSize.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidDialog::updateSize ()
{
	if(!dialog)
		return;

	JniAccessor jni;
	JniObject jrect (jni, FrameworkDialog.getSize (dialog));

	Rect rect;
	FrameworkGraphics::toCCLRect (rect, jni, jrect);
	DpiScale::toCoordRect (rect, getContentScaleFactor ());

	View::setSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidDialog::close ()
{
	if(isInCloseEvent ())
		return true;

	if(onClose ())
	{
		setInCloseEvent (true);

		// sometimes Android fails to call onDismiss() and
		// onDestroy() on the dialog after the dismiss() call; this
		// makes sure the window is still removed from Desktop
		Desktop.removeWindow (this);

		FrameworkDialog.dismiss (dialog);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidDialog::scrollClient (RectRef rect, PointRef delta)
{
	// bypass offscreen scrolling, this somehow seems to slow down redraw in dialogs
	Window::scrollClient (rect, delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidDialog::setSizeInfo (const PopupSizeInfo& sizeInfo)
{
	popupSizeInfo = sizeInfo;
}

//************************************************************************************************
// AndroidWindow
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AndroidWindow, Window)

/////////////////////////////////////////////////////////////////////////////////////////////////

AndroidWindow::AndroidWindow (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  frameworkView (0),
  ownsFrameworkView (false),
  isTranslucent (false)
{
	if(!FrameworkView::isOffscreenEnabled ())
		setCollectUpdates (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidWindow::~AndroidWindow ()
{
	destruct ();

	if(ownsFrameworkView && frameworkView)
		delete frameworkView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidWindow::isAppWindow () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::makeNativePopupWindow (IWindow* parent)
{
	ASSERT (!frameworkView)
	frameworkView = FrameworkView::createWithContext (FrameworkActivity::getCurrentActivity ()->getJObject ());
	if(frameworkView)
	{
		ownsFrameworkView = true;
		frameworkView->initWithWindow (this);

		CCL_PRINTF ("AndroidWindow::makeNativePopupWindow %x (%x): %d, %d (%d x %d)\n", frameworkView, frameworkView->getJObject (), size.left, size.top, size.getWidth (), size.getHeight ())

		PixelRect pixelSize (size, getContentScaleFactor ());

		FrameworkViewClass.setSize (*frameworkView, pixelSize.left, pixelSize.top, pixelSize.getWidth (), pixelSize.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::adjustSizeToScreen (FrameworkView& parentView)
{
	Window* parent = parentView.getWindow ();
	if(!parent)
		return;

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();
	int statusBarHeight = DpiScale::pixelToCoord (activity->getStatusBarHeight (), getContentScaleFactor ());
	int edgeMargin = statusBarHeight / 2;

	bool isSheetStyle = getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle);
	Rect rect (isSheetStyle ? initialSize : getSize ());
	Rect parentSize (parent->getSize ());

	Rect availableSize (parentSize);
	availableSize.contract (edgeMargin);
	if(parentSize.top == 0)
		availableSize.top = statusBarHeight;

	// limit width and height to parent view size
	if(rect.getWidth () > availableSize.getWidth ())
		rect.setWidth (availableSize.getWidth ());

	if(rect.getHeight () > availableSize.getHeight ())
		rect.setHeight (availableSize.getHeight ());

	// center window if outside of parent
	if(rect.left < availableSize.left || rect.right > availableSize.right)
		rect.centerH (availableSize);

	if(rect.top < availableSize.top || rect.bottom > availableSize.bottom)
		rect.centerV (availableSize);

	// always center sheet style windows (mimic behavior of iOS sheets)
	if(isSheetStyle)
		rect.center (availableSize);

	setSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::showWindow (bool state)
{
	CCL_PRINT ("AndroidWindow::showWindow")
	ASSERT (frameworkView != FrameworkActivity::getCurrentActivity ()->getContentView ())

	FrameworkView* parentView = FrameworkActivity::getCurrentActivity ()->getContentView ();
	if(parentView && frameworkView)
	{
		initialSize = getSize ();
		adjustSizeToScreen (*parentView);

		if(state)
			parentView->addView (frameworkView);
		else
			parentView->removeView (frameworkView);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidWindow::close ()
{
	ASSERT (frameworkView != FrameworkActivity::getCurrentActivity ()->getContentView ())
	if(!frameworkView || isInCloseEvent () || isInDestroyEvent ())
		return false;

	CCL_PRINTF ("AndroidWindow::close: frameworkView %x (%x)\n", frameworkView, frameworkView->getJObject ())

	if(onClose ())
	{
		setInCloseEvent (true);
		setInDestroyEvent (true);
		
		onDestroy ();

		if(frameworkView && frameworkView->getParentView ())
		{
			frameworkView->getParentView ()->removeView (frameworkView);

			ASSERT (ownsFrameworkView)
			if(ownsFrameworkView)
				FrameworkViewClass.destruct (*frameworkView);

			frameworkView = nullptr;
		}

		release ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::setWindowSize (Rect& size)
{
	if(frameworkView && !frameworkView->isResizing () && !isAppWindow ())
	{
		CCL_PRINTF ("AndroidWindow::setWindowSize: %d, %d (%d x %d)\n", size.left, size.top, size.getWidth (), size.getHeight ())
		ASSERT (frameworkView != FrameworkActivity::getCurrentActivity ()->getContentView ())

		PixelRect pixelSize (size, getContentScaleFactor ());

		FrameworkViewClass.setSize (*frameworkView, pixelSize.left, pixelSize.top, pixelSize.getWidth (), pixelSize.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidWindow::moveWindow (PointRef pos)
{
	// prevent moving app window and sheet-style windows
	if(isAppWindow () || getStyle ().isCustomStyle (Styles::kWindowBehaviorSheetStyle))
		return;

	Rect size (getSize ());
	size.moveTo (pos);
	setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::updateSize ()
{
	if(!frameworkView || style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		return;

	JniAccessor jni;
	JniObject jrect (jni, FrameworkViewClass.getRectOnScreen (*frameworkView));

	Rect rect;
	FrameworkGraphics::toCCLRect (rect, jni, jrect);
	DpiScale::toCoordRect (rect, getContentScaleFactor ());

	View::setSize (rect);

	// inform render target
	if(NativeWindowRenderTarget* t = getRenderTarget ())
		t->onSize ();

	// adjust child windows to new size
	for(FrameworkView* child : frameworkView->getChildren ())
		child->getWindow ()->adjustSizeToScreen (*frameworkView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API AndroidWindow::clientToScreen (Point& pos) const
{
	Point origin;
	screenToClient (origin);
	pos -= origin;
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API AndroidWindow::screenToClient (Point& pos) const
{
	pos -= size.getLeftTop ();
	return pos;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidWindow::center ()
{
	ASSERT (frameworkView != FrameworkActivity::getCurrentActivity ()->getContentView ())

	FrameworkView* parentView = FrameworkActivity::getCurrentActivity ()->getContentView ();
	if(parentView && parentView != frameworkView)
	{
		Rect appSize;
		parentView->getSize (appSize);

		Rect size (getSize ());
		size.center (appSize);

		setSize (size);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::updateBackgroundColor ()
{
	isTranslucent = shouldBeTranslucent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidWindow::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == IWindow::kStatusBarStyle)
	{
		FrameworkActivity::getCurrentActivity ()->setLightStatusBar (var.asInt () == IWindow::kDarkContent);
		return true;
	}

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float AndroidWindow::getContentScaleFactor () const
{
	return frameworkView ? frameworkView->getContentScaleFactor () : 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidWindow::isAttached ()
{
	return frameworkView != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidWindow::invalidate (RectRef rect)
{
	if(isInDestroyEvent ())
		return;

	if(frameworkView)
		frameworkView->invalidate (rect);

	if(NativeWindowRenderTarget* target = getRenderTarget ())
		if(IMutableRegion* region = target->getInvalidateRegion ())
			region->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidWindow::redraw ()
{
	// nothing here, suppress ASSERT in SuperClass
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidWindow::draw (const UpdateRgn& updateRgn)
{
	// store update region bounds in current graphics for optimizations
	if(FrameworkGraphics* graphics = graphicsDevice ? ccl_cast<FrameworkGraphics> (graphicsDevice->getNativeDevice ()) : 0)
	{
		if(isTranslucent)
			graphics->clearRect (updateRgn.bounds);

		graphics->beginDraw (updateRgn.bounds);
	}

	CCL_PRINTF ("AndroidWindow::draw: x: (%d, %d)  y: (%d, %d)\n", updateRgn.bounds.left, updateRgn.bounds.right, updateRgn.bounds.top, updateRgn.bounds.bottom)
	Window::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidWindow::scrollClient (RectRef rect, PointRef delta)
{
	CCL_PRINTF ("scrollClient: x: (%d, %d) y: (%d, %d) (%d x %d) by (%d, %d)\n", rect.left, rect.right, rect.top, rect.bottom, rect.getWidth (), rect.getHeight (), delta.x, delta.y)

	AndroidBitmap* offscreen = frameworkView ? frameworkView->getOffscreen () : 0;
	if(offscreen && (delta.x * delta.y == 0))
	{
		Rect sourceRect (rect);

		// check if a part of the source rect is dirty (don't scroll dirty pixels)
		Rect dirty (frameworkView->getDirtyRegion ().getBoundingBox ());
		if(dirty.bound (sourceRect))
		{
			CCL_PRINTF ("DIRTY part of scroll rect: x: %d, %d,  y: %d, %d\n", dirty.left, dirty.right, dirty.top, dirty.bottom)

			Rect inv (sourceRect);
			if(delta.x != 0)
			{
				if(delta.x < 0)
				{
					ASSERT (dirty.left < sourceRect.right) // result of bound ()
					if(dirty.left < sourceRect.right)
					{
						// invalidate the part that whe won't scroll (in target coords)
						inv.left = dirty.left + delta.x;

						// exclude dirty part from scroll source rect
						sourceRect.right = dirty.left;
					}
				}
				else if(delta.x > 0)
				{
					ASSERT (dirty.right > sourceRect.left) // result of bound ()
					if(dirty.right > sourceRect.left)
					{
						// invalidate the part that whe won't scroll (in target coords)
						inv.right = dirty.right + delta.x;

						// exclude dirty part from scroll source rect
						sourceRect.left = dirty.right;
					}
				}
			}
			else
			{
				if(delta.y < 0)
				{
					ASSERT (dirty.top < sourceRect.bottom) // result of bound ()
					if(dirty.top < sourceRect.bottom)
					{
						// invalidate the part that whe won't scroll (in target coords)
						inv.top = dirty.top + delta.y;

						// exclude dirty part from scroll source rect
						sourceRect.bottom = dirty.top;
					}
				}
				else if(delta.y > 0)
				{
					ASSERT (dirty.bottom > sourceRect.top) // result of bound ()
					if(dirty.bottom > sourceRect.top)
					{
						// invalidate the part that whe won't scroll (in target coords)
						inv.bottom = dirty.bottom + delta.y;

						// exclude dirty part from scroll source rect
						sourceRect.top = dirty.bottom;
					}
				}
			}
			invalidate (inv);

			if(sourceRect.isEmpty ())
			{
				Window::scrollClient (rect, delta); // (just invalidate)
				return;
			}
		}
		
		Rect finishRect (sourceRect);
		Point finishDelta (delta);

		// exclude source coords outside of offscreen (scrollPixelRect would grab pixels from opposite edge)
		// invalidate (more) target pixels instead
		Point darkSource (sourceRect.right - (getWidth () - 1), sourceRect.bottom - (getHeight () - 1));
		if(darkSource.x > 0)
		{
			sourceRect.right -= darkSource.x;
			if(finishDelta.x)
				finishDelta.x -= darkSource.x;
		}
		if(darkSource.y > 0)
		{
			sourceRect.bottom -= darkSource.y;
			if(finishDelta.y)
				finishDelta.y -= darkSource.y;
		}

		float scaleFactor = frameworkView->getContentScaleFactor ();
		PixelRect pixelRect (sourceRect, scaleFactor);
		PixelPoint pixelDelta (delta, scaleFactor);

		// handle fractional scaling
		bool fractionalScaling = DpiScale::isIntAligned (scaleFactor) == false;
		if(fractionalScaling)
		{
			PixelRectF rectF (sourceRect, scaleFactor);
			PixelPointF deltaF (delta, scaleFactor);
			if(rectF.isPixelAligned () == false || deltaF.isPixelAligned () == false)
			{
				// cannot scroll fractional pixels
				Rect r = Rect (sourceRect).offset (delta).join (sourceRect);
				invalidate (r);
				return;
			}
		}

		// scroll rect and invalidate edges
		offscreen->scrollPixelRect (pixelRect, pixelDelta);
		finishScroll (finishRect, finishDelta);

		// invalidate scrolled area without adding it to the dirty region to ensure
		// it will be included in the clipping area for the next redraw request
		Rect r = Rect (finishRect).offset (delta).join (finishRect);
		frameworkView->invalidate (r, false);
	}
	else
		Window::scrollClient (rect, delta);

	// inform render target
	if(NativeWindowRenderTarget* target = getRenderTarget ())
	{
		target->onScroll (rect, delta);
		finishScroll (rect, delta);
	}
}
