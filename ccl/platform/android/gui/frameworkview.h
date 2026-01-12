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
// Filename    : ccl/platform/android/gui/frameworkview.h
// Description : Framework View (native)
//
//************************************************************************************************

#ifndef _ccl_android_frameworkview_h
#define _ccl_android_frameworkview_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/platform/android/graphics/frameworkgraphics.h"

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/graphics/rect.h"

#include "core/public/gui/corerectlist.h"

namespace CCL {

class AndroidWindow;
interface IGraphics;

namespace Android {

//************************************************************************************************
// dev.ccl.FrameworkView
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkViewClass, CCLGUI_CLASS_PREFIX "FrameworkView")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject)
	DECLARE_JNI_METHOD (void, destruct)
	DECLARE_JNI_METHOD (int64, getNativeViewPtr)
	DECLARE_JNI_METHOD (jobject, getRectOnScreen)
	DECLARE_JNI_METHOD (void, setSize, int, int, int, int)
	DECLARE_JNI_METHOD (void, accessibilityContentChanged, int)
END_DECLARE_JNI_CLASS (FrameworkViewClass)

class FrameworkGraphics;

//************************************************************************************************
// FrameworkView
//************************************************************************************************

class FrameworkView: public Object,
					 public JniObject,
					 public JniCast<FrameworkView>
{
public:
	FrameworkView (JNIEnv* jni, jobject object, jobject graphics);
	~FrameworkView ();

	static FrameworkView* createWithContext (jobject context); ///< creates a new FrameworkView including the java object

	static bool isOffscreenEnabled ();

	void createApplicationView ();
	void initWithWindow (AndroidWindow* window);

	CCL::Rect& getSize (CCL::Rect& size) const;
	void invalidate (RectRef rect, bool addToDirtyRegion = true);
	float getContentScaleFactor () const;
	AndroidWindow* getWindow () const;
	AndroidBitmap* getOffscreen ();
	FrameworkGraphics* createOffscreenDevice ();
	const Core::RectList<32>& getDirtyRegion () const;
	bool isResizing () const;

	FrameworkView* getParentView () const;
	bool addView (FrameworkView* child);
	bool removeView (FrameworkView* child);
	const LinkedList<FrameworkView*>& getChildren () const { return children; }

	void accessibilityContentChanged (int virtualViewId);

	// called by Java class:
	void onSizeChanged (PointRef sizeInPixel);
	void redraw ();

	void onTouchEvent (int actionCode, int actionId, int toolType, int buttonState, int metaState, jintArray pointerIds, jfloatArray pointerCoords, float pressure, float orientation, int source);
	void onMouseEvent (int actionCode, int buttonState, int metaState, float posX, float posY, float hscroll, float vscroll);

	void fillAccessibilityNodeInfo (int virtualViewId, jobject info) const;
	int getVirtualViewAt (PointRef pos) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	FrameworkGraphics* graphics;
	FrameworkView* parentView;
	AutoPtr<AndroidWindow> window;
	LinkedList<FrameworkView*> children;
	Core::RectList<32> dirtyRegion;
	float contentScaleFactor;
	bool resizing;

	MouseEvent mouseDownEvent;
	bool mouseDownEventSent;

	void sendMouseDownEvent (bool dragged);

	struct OffscreenState: public Unknown
	{
		AutoPtr<AndroidBitmap> bitmap;
		AutoPtr<FrameworkGraphics> graphics;
		FrameworkGraphics::ScaleHelper* scaler;

		OffscreenState ();
		~OffscreenState ();
	
		void init (PointRef sizeInPixel, float contentScaleFactor);
	} offscreen;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AndroidBitmap* FrameworkView::getOffscreen ()
{ return offscreen.bitmap; }

inline const Core::RectList<32>& FrameworkView::getDirtyRegion () const
{ return dirtyRegion; }

inline bool FrameworkView::isResizing () const
{ return resizing; }

inline FrameworkView* FrameworkView::getParentView () const
{ return parentView; }

} // namespace Android
} // namespace CCL

#endif // _ccl_android_frameworkview_h
