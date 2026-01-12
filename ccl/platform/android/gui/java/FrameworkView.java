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
// Filename    : ccl/platform/android/gui/java/FrameworkView.java
// Description : Framework View
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.os.*;
import android.view.*;
import android.view.accessibility.*;
import android.graphics.*;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

//************************************************************************************************
// HoverEventFilter
//************************************************************************************************

class HoverEventFilter implements Runnable
{
	private final Handler handler = new Handler (Looper.getMainLooper ());

	private final View view;
	private MotionEvent event;
	private long lastActionUp = 0;

	public HoverEventFilter (View view)
	{
		this.view = view;
	}

	public void onHoverEvent (MotionEvent event)
	{
		// when using a pen, Android sends a hover exit event before a touch down
		// and a hover enter event after a touch up; this interferes with the touch
		// handling in CCL, so these events need to be filtered
		
		// discard hover enter events occurring within 10ms of a touch up event
		if(event.getActionMasked () == MotionEvent.ACTION_HOVER_ENTER && event.getEventTime () - lastActionUp <= 10)
			return;

		// delay hover exit events by 10ms and cancel in onTouchEvent if a touch
		// down is registered during this time
		if(event.getActionMasked () == MotionEvent.ACTION_HOVER_EXIT)
			delayEvent (event, 10);
		else
			view.onTouchEvent (event);
	}

	public void onTouchEvent (MotionEvent event)
	{
		int actionIndex = event.getActionIndex ();
		int toolType = event.getToolType (actionIndex);

		if(toolType == MotionEvent.TOOL_TYPE_STYLUS || toolType == MotionEvent.TOOL_TYPE_ERASER)
		{
			// cancel pending hover exit event upon receiving a touch down event
			if(event.getActionMasked () == MotionEvent.ACTION_DOWN)
				cancelDelayedEvent ();

			// store time of last touch up event
			if(event.getActionMasked () == MotionEvent.ACTION_UP)
				lastActionUp = SystemClock.uptimeMillis ();
		}
	}
	
	protected void delayEvent (MotionEvent event, int delay)
	{
		this.event = MotionEvent.obtain (event);

		handler.postDelayed (this, delay);
	}

	protected void cancelDelayedEvent ()
	{
		handler.removeCallbacks (this);
	}

	@Override
	public void run ()
	{
		view.onTouchEvent (event);
	}

	public void finish ()
	{
		cancelDelayedEvent ();
	}
}

//************************************************************************************************
// FrameworkView
//************************************************************************************************

@Keep
public class FrameworkView extends ViewGroup
{
	private final FrameworkGraphics graphics;
	private final HoverEventFilter eventFilter;

	private long nativeViewPtr;
	private int numLayers = 0;
	private int numSprites = 0;

	private final Point screenPos = new Point ();
	private PointF mousePos = new PointF ();

	private final AccessibilityManager accessibilityManager;
	private FrameworkAccessibilityNodeProvider accessibilityNodeProvider;

	private enum ViewLane { kLayer, kSprite, kWindow } ///< lanes of subViews, bottom to top

	public class FrameworkAccessibilityNodeProvider extends AccessibilityNodeProvider
	{
		private int hoveredViewId = HOST_VIEW_ID;

		@Override
		public AccessibilityNodeInfo createAccessibilityNodeInfo (int virtualViewId)
		{
			AccessibilityNodeInfo info;
			if(virtualViewId == HOST_VIEW_ID)
				info = AccessibilityNodeInfo.obtain (FrameworkView.this);
			else
				info = AccessibilityNodeInfo.obtain (FrameworkView.this, virtualViewId);

			onInitializeAccessibilityNodeInfo (info);
			fillAccessibilityNodeInfoNative (nativeViewPtr, virtualViewId, info);

			if(info.isClickable ())
				info.addAction (AccessibilityNodeInfo.AccessibilityAction.ACTION_CLICK);

			if(info.isFocusable ())
			{
				if(info.isFocused ())
					info.addAction (AccessibilityNodeInfo.AccessibilityAction.ACTION_CLEAR_FOCUS);
				else
					info.addAction (AccessibilityNodeInfo.AccessibilityAction.ACTION_FOCUS);
			}

			return info;
		}

		@Override
		public boolean performAction (int virtualViewId, int action, Bundle arguments)
		{
			if(virtualViewId == HOST_VIEW_ID)
				return performAccessibilityAction (action, arguments);

			switch(action)
			{
			case AccessibilityNodeInfo.ACTION_CLICK :
				sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_VIEW_CLICKED);

				// simulate touch
				long eventTime = SystemClock.uptimeMillis ();
				AccessibilityNodeInfo info = createAccessibilityNodeInfo (virtualViewId);
				Rect bounds = new Rect ();
				info.getBoundsInScreen (bounds);
				MotionEvent event = MotionEvent.obtain (eventTime, eventTime, MotionEvent.ACTION_DOWN, bounds.centerX (), bounds.centerY (), 0);
				onTouchEvent (event);
				event.setAction (MotionEvent.ACTION_UP);
				onTouchEvent (event);

				return true;
			case AccessibilityNodeInfo.ACTION_FOCUS :
				sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_VIEW_FOCUSED);
				return true;
			case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS :
				sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
				return true;
			case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS :
				sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
				return true;
			}

			return false;
		}

		public boolean dispatchHoverEvent (MotionEvent event)
		{
			int action = event.getAction ();
			if(action != MotionEvent.ACTION_HOVER_MOVE && action != MotionEvent.ACTION_HOVER_ENTER && action != MotionEvent.ACTION_HOVER_EXIT)
				return false;

			if(action == MotionEvent.ACTION_HOVER_EXIT)
			{
				if(hoveredViewId != HOST_VIEW_ID)
				{
					updateHoveredView (HOST_VIEW_ID);
					return true;
				}
			}
			else
			{
				int virtualViewId = getVirtualViewAtNative (nativeViewPtr, (int) event.getX (), (int) event.getY ());
				updateHoveredView (virtualViewId);

				if(virtualViewId != HOST_VIEW_ID)
					return true;
			}

			return false;
		}

		private void updateHoveredView (int virtualViewId)
		{
			if(hoveredViewId == virtualViewId)
				return;

			sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
			sendAccessibilityEventForVirtualView (hoveredViewId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);

			hoveredViewId = virtualViewId;
		}
	}

	public FrameworkView (Context context) 
	{
		super (context);

		graphics = new FrameworkGraphics ();
		nativeViewPtr = constructNative (graphics);

		eventFilter = new HoverEventFilter (this);
		accessibilityManager = (AccessibilityManager) context.getSystemService (Context.ACCESSIBILITY_SERVICE);

		setFocusableInTouchMode (true);
		setDefaultFocusHighlightEnabled (false); // disable green frame around focused view

		setLayerType (LAYER_TYPE_HARDWARE, null);
		setWillNotDraw (false);
		setClipChildren (true);
    }
	
	public void destruct ()
	{
		eventFilter.finish ();

		destructNative (nativeViewPtr);
		nativeViewPtr = 0;
	}

	public long getNativeViewPtr ()
	{
		return nativeViewPtr;
	}

	public Rect getRectOnScreen ()
	{
		if(isAttachedToWindow ())
		{
			int[] location = new int[] { -1, -1 };
			getLocationOnScreen (location);
			screenPos.set (location[0], location[1]);
		}

		return new Rect (screenPos.x, screenPos.y, screenPos.x + getWidth (), screenPos.y + getHeight ());
	}

	public Rect getRectInWindow ()
	{
		Point windowPos = new Point ();
		if(isAttachedToWindow ())
		{
			int[] location = new int[] { -1, -1 };
			getLocationInWindow (location);
			windowPos.set (location[0], location[1]);
		}

		return new Rect (windowPos.x, windowPos.y, windowPos.x + getWidth (), windowPos.y + getHeight ());
	}

    public void setSize (int x, int y, int width, int height)
	{
		screenPos.set (x, y);

		// convert position from screen to window coordinates
		FrameworkActivity activity = (FrameworkActivity) getContext ();
		Rect screenRect = activity.getRectOnScreen ();

		x -= screenRect.left;
		y -= screenRect.top;

		layout (x, y, x + width, y + height);
	}

	private ViewLane getSubViewLane (View view)
	{
		// We use subViews to implement various CCL features. Their z-order must be arranged in this order (bottom to top):
		// 1. Layers, 2. Sprites (TransparentWindow), 3. PopupWindow (only als child of app view)
		if(view instanceof GraphicsLayerView)
		{
			if(((GraphicsLayerView)view).isSprite)
				return ViewLane.kSprite;
			else
				return ViewLane.kLayer;
		}
		return ViewLane.kWindow;
	}

	@Override
    public void addView (View view)
	{
		int index = -1;

		switch(getSubViewLane (view))
		{
		case kLayer:
			index = numLayers; // after last layer
			numLayers++;
			break;

		case kSprite:
			index = numLayers + numSprites; // after last sprite
			numSprites++;
			break;

		case kWindow: // after last window (-1)
			break;
		}

		//CCL.log ("addView: layers: " + numLayers + " sprites: " + numSprites);
		super.addView (view, index);
	}

	@Override
    public void removeView (View view)
	{
		switch(getSubViewLane (view))
		{
		case kLayer:
			numLayers--;
			break;

		case kSprite:
			numSprites--;
			break;

		case kWindow:
			break;
		}

		//CCL.log ("removeView: layers: " + numLayers + " sprites: " + numSprites);
		super.removeView (view);
	}

	public void refreshSystemUiVisibility ()
	{
		if(getContext () instanceof FrameworkActivity)
			setSystemUiVisibility (((FrameworkActivity) getContext ()).getSystemUiVisibility ());
	}

	@Override
	protected void onAttachedToWindow ()
	{
		super.onAttachedToWindow ();

		Activity activity = (Activity)getContext ();
		Window window = activity.getWindow ();
		if(window != null)
			window.addFlags (WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED);
	}

	@Override
	protected void onDetachedFromWindow ()
	{
		eventFilter.finish ();

		super.onDetachedFromWindow ();
	}

	@Override
	public boolean hasOverlappingRendering ()
	{
        return false;
    }

	@Override
	protected void onConfigurationChanged (Configuration newConfig)
	{
		// request layout so the native window can update its
		// coordinates in case position on screen changed
		if(!isInLayout ())
			requestLayout ();
	}

	@Override
	protected void onLayout (boolean changed, int l, int t, int r, int b)
	{
		onLayoutNative (nativeViewPtr);

		// don't layout sublayer views
	}

	@Override
	protected void onSizeChanged (int w, int h, int oldw, int oldh)
	{
		super.onSizeChanged (w, h, oldw, oldh);

		onSizeChangedNative (nativeViewPtr, w, h);
	}

	@Override
	public WindowInsets onApplyWindowInsets (WindowInsets insets)
	{
		insets = super.onApplyWindowInsets (insets);
		
		onApplyWindowInsetsNative (nativeViewPtr, insets);

		return insets;
	}

	@Override
    protected void onVisibilityChanged (@NonNull View view, int visibility)
	{
		if(visibility == View.VISIBLE)
			requestFocus ();
	}

	@Override
	public void onWindowFocusChanged (boolean hasWindowFocus)
	{
		if(hasWindowFocus)
			refreshSystemUiVisibility ();
	}

	@Override
    protected void onDraw (@NonNull Canvas canvas) 
	{
		super.onDraw (canvas);
		
		GraphicsLayerView.checkHardwareLayerSupport (canvas);

		graphics.setCanvas (canvas);
		redrawNative (nativeViewPtr);
		graphics.setCanvas (null);
	}

	@Override
	public boolean dispatchHoverEvent (MotionEvent event)
	{
		// attempt to dispatch hover events to accessibility first
		if(accessibilityManager.isEnabled () && accessibilityManager.isTouchExplorationEnabled ())
			if(accessibilityNodeProvider != null && accessibilityNodeProvider.dispatchHoverEvent (event))
				return true;

		return super.dispatchHoverEvent (event);
	}

	@Override
	public boolean onHoverEvent (MotionEvent event)
	{
		// route all hover events through the event filter
		eventFilter.onHoverEvent (event);

		return true;
	}

	@Override
	public boolean onTouchEvent (MotionEvent event)
	{
		onMotionEventInternal (event);

		// notify the event filter of all touch events
		eventFilter.onTouchEvent (event);

		// not sure if required, but we must return true to get following events for a touch
		super.onTouchEvent (event);
		return true;
	}

	@Override
	public boolean onGenericMotionEvent (MotionEvent event)
	{
		int actionIndex = event.getActionIndex ();
		if(event.getToolType (actionIndex) == MotionEvent.TOOL_TYPE_MOUSE)
			onMotionEventInternal (event);

		super.onGenericMotionEvent (event);
		return true;
	}

	// on some Android devices, right mouse button presses are
	// delivered as back key press events, translate those here
	private int getButtonFromKeyEvent (int keyCode, KeyEvent event)
	{
		if(event.getSource() != InputDevice.SOURCE_MOUSE)
			return 0;

		if(keyCode == KeyEvent.KEYCODE_BACK)
			return MotionEvent.BUTTON_SECONDARY;

		return 0;
	}

	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event)
	{
		int button = getButtonFromKeyEvent (keyCode, event);
		if(button == 0)
			return super.onKeyDown (keyCode, event);

		onMouseEventNative (nativeViewPtr, MotionEvent.ACTION_BUTTON_PRESS, button, event.getMetaState (), mousePos.x, mousePos.y, 0, 0);
		return true;
	}

	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event)
	{
		int button = getButtonFromKeyEvent (keyCode, event);
		if(button == 0)
			return super.onKeyUp (keyCode, event);

		onMouseEventNative (nativeViewPtr, MotionEvent.ACTION_BUTTON_RELEASE, button, event.getMetaState (), mousePos.x, mousePos.y, 0, 0);
		return true;
	}

	protected void onMotionEventInternal (MotionEvent event)
	{
		// the changed pointer:
		int actionCode = event.getActionMasked ();
		int actionIndex = event.getActionIndex ();
		int actionId = event.getPointerId (actionIndex);
		int toolType = event.getToolType (actionIndex);
		int buttonState = event.getButtonState ();
		int metaState = event.getMetaState ();
		int source = event.getSource ();

		if(toolType == MotionEvent.TOOL_TYPE_MOUSE)
		{
			float hscroll = event.getAxisValue (MotionEvent.AXIS_HSCROLL);
			float vscroll = event.getAxisValue (MotionEvent.AXIS_VSCROLL);

			mousePos = new PointF (event.getX (0), event.getY (0));

			onMouseEventNative (nativeViewPtr, actionCode, buttonState, metaState, mousePos.x, mousePos.y, hscroll, vscroll);
		}
		else
		{
			// transfer coords for all pointers via one float array with x, y
			int numPointers = event.getPointerCount ();
			int[] pointerIds = new int[numPointers];
			float[] pointerCoords = new float[numPointers * 2];

			int p = 0;
			for(int i = 0; i < numPointers; i++)
			{
				pointerIds[i] = event.getPointerId (i);
				pointerCoords[p++] = event.getX (i);
				pointerCoords[p++] = event.getY (i);
			}

			float pressure = 0;
			float orientation = 0;
			if(toolType == MotionEvent.TOOL_TYPE_STYLUS || toolType == MotionEvent.TOOL_TYPE_ERASER)
			{
				pressure = event.getPressure (actionIndex);
				orientation = event.getOrientation (actionIndex);
			}

			onTouchEventNative (nativeViewPtr, actionCode, actionId, toolType, buttonState, metaState, pointerIds, pointerCoords, pressure, orientation, source);
		}
	}

	@Override
	public AccessibilityNodeProvider getAccessibilityNodeProvider ()
	{
		if(accessibilityNodeProvider == null)
			accessibilityNodeProvider = new FrameworkAccessibilityNodeProvider ();

		return accessibilityNodeProvider;
	}

	public void accessibilityContentChanged (int virtualViewId)
	{
		sendAccessibilityEventForVirtualView (virtualViewId, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);
	}

	public void sendAccessibilityEventForVirtualView (int virtualViewId, int eventType)
	{
		if(!accessibilityManager.isEnabled () || accessibilityNodeProvider == null)
			return;

		AccessibilityEvent event = AccessibilityEvent.obtain (eventType);
		AccessibilityNodeInfo info = accessibilityNodeProvider.createAccessibilityNodeInfo (virtualViewId);

		event.getText ().add (info.getText ());
		event.setContentDescription (info.getContentDescription ());
		event.setSource (FrameworkView.this, virtualViewId);

		requestSendAccessibilityEvent (FrameworkView.this, event);
	}

    public native long constructNative (FrameworkGraphics graphics);
    public native void destructNative (long nativeViewPtr);
	public native void onLayoutNative (long nativeViewPtr);
	public native void onSizeChangedNative (long nativeViewPtr, int width, int height);
	public native void onApplyWindowInsetsNative (long nativeViewPtr, WindowInsets insets);
    public native void redrawNative (long nativeViewPtr);

	public native void onTouchEventNative (long nativeViewPtr, int actionCode, int actionId, int toolType, int buttonState, int metaState, int[] pointerIds, float[] pointerCoords, float pressure, float orientation, int source);
	public native void onMouseEventNative (long nativeViewPtr, int actionCode, int buttonState, int metaState, float posX, float posY, float hscroll, float vscroll);

	public native void fillAccessibilityNodeInfoNative (long nativeViewPtr, int virtualViewId, AccessibilityNodeInfo info);
	public native int getVirtualViewAtNative (long nativeViewPtr, int x, int y);
}
