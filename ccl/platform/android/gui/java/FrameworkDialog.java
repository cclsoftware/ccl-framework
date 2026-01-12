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
// Filename    : ccl/platform/android/gui/java/FrameworkDialog.java
// Description : Framework Dialog
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.view.*;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Point;
import android.graphics.Rect;
import android.app.Activity;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.Keep;

import java.util.ArrayList;

//************************************************************************************************
// DialogView
//************************************************************************************************

class DialogView extends FrameworkView
{
	public Point size = new Point ();
	private final FrameworkDialog frameworkDialog;

	public DialogView (Context context, FrameworkDialog dialog)
	{
		super (context);

		this.frameworkDialog = dialog;
	}

	public Window getDialogWindow ()
	{
		Dialog dialog = frameworkDialog.getDialog ();
		return dialog != null ? dialog.getWindow () : null;
	}

	@Override
    protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec)
	{
		setMeasuredDimension (size.x, size.y);
    }
}

//************************************************************************************************
// FrameworkDialog
//************************************************************************************************

@Keep
public class FrameworkDialog extends DialogFragment
{
	private static final ArrayList<FrameworkDialog> activeDialogs = new ArrayList<> ();

	private DialogView view;
	private final Point requestedPos = new Point ();
	private final Point requestedSize = new Point ();
	private final Point adjustedPos = new Point ();
	private final Point adjustedSize = new Point ();
	boolean isCentered = false;
	boolean onDismissCalled = false;
	
	public FrameworkDialog ()
	{
		FrameworkActivity activity = FrameworkActivity.getActivity ();
		if(activity != null)
		    view = new DialogView (activity, this);

		activeDialogs.add (this);
	}

	public FrameworkDialog (Context context)
	{
		view = new DialogView (context, this);

		activeDialogs.add (this);
    }

    @Override
	public void onDestroy ()
	{
		// sometimes onDestroy is called before onDismiss, in which case
		// we need to call onDismissNative before destructing the view
		if(!onDismissCalled)
			onDismissNative (getNativeViewPtr ());

		activeDialogs.remove (this);

		if(view != null)
		{
			view.destruct ();
			view = null;
		}

		super.onDestroy ();
	}

    public long getNativeViewPtr ()
	{
		return view != null ? view.getNativeViewPtr () : 0;
	}

	private void adjustMetrics (int x, int y, int width, int height)
	{
		FrameworkActivity activity = (FrameworkActivity) view.getContext ();
		Point displaySize = activity.getDisplaySize ();
		Rect workArea = activity.getRectInWindow ();
		Rect screenRect = activity.getRectOnScreen ();

		if(workArea == null || screenRect == null)
			return;

		requestedPos.set (x, y);
		requestedSize.set (width, height);

		int statusBarHeight = activity.getStatusBarHeight ();
		boolean isStatusBarVisible = activity.isStatusBarVisible ();

		if(screenRect.left == statusBarHeight)
			workArea.offset (statusBarHeight, 0);

		// limit window size to screen area
		if(isCentered)
		{
			// subtract a small edge area and account for the status bar
			int edgeMargin = statusBarHeight / 2;
			int topMargin = isStatusBarVisible ? statusBarHeight + statusBarHeight / 4 : edgeMargin;

			workArea.left += edgeMargin;
			workArea.top += topMargin;
			workArea.right -= edgeMargin;
			workArea.bottom -= edgeMargin;

			adjustedSize.x = Math.min (requestedSize.x, workArea.width ());
			adjustedSize.y = Math.min (requestedSize.y, workArea.height ());
			
			adjustedPos.x = workArea.left + (workArea.width () - adjustedSize.x) / 2;
			adjustedPos.y = workArea.top + (workArea.height () - adjustedSize.y) / 2;
		}
		else
		{
			adjustedPos.x = workArea.left + requestedPos.x - screenRect.left;
			adjustedPos.y = workArea.top + requestedPos.y - screenRect.top;

			adjustedSize.x = Math.min (requestedSize.x, workArea.right - adjustedPos.x);
			adjustedSize.y = Math.min (requestedSize.y, workArea.bottom - adjustedPos.y);
		}
	}

    public void show (int x, int y, int width, int height, boolean centered)
	{
		isCentered = centered;

		adjustMetrics (x, y, width, height);
		
		view.size = adjustedSize;

		Activity activity = (Activity)view.getContext ();
		show (activity.getFragmentManager(), "dialog");
	}
	
    public void setSize (int x, int y, int width, int height)
	{
		adjustMetrics (x, y, width, height);

		view.size = adjustedSize;
		view.layout (0, 0, view.size.x, view.size.y);

		Window window = view.getDialogWindow ();
		if(window != null)
		{
			WindowManager.LayoutParams params = window.getAttributes ();

			params.x = adjustedPos.x;
			params.y = adjustedPos.y;
			params.width = adjustedSize.x;
			params.height = adjustedSize.y;

			window.setAttributes (params);
		}
	}

	public Rect getSize ()
	{
		int[] location = new int[2];
		view.getLocationOnScreen (location);

		return new Rect (location[0], location[1], location[0] + view.size.x, location[1] + view.size.y);
	}

    @Override
	public View onCreateView (LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
		view.layout (0, 0, view.size.x, view.size.y);

		return view;
	}

	@Override
	public Dialog onCreateDialog (Bundle savedInstanceState)
	{
		Dialog dialog = super.onCreateDialog (savedInstanceState);
		dialog.getWindow ().requestFeature (Window.FEATURE_NO_TITLE);
		dialog.getWindow ().setBackgroundDrawable (new ColorDrawable (0));

		// FLAG_NOT_FOCUSABLE prevents leaving immersive mode
		dialog.getWindow ().setFlags (WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);

		return dialog;
	}
	
	public static void applicationSizeChanged ()
	{
		for (FrameworkDialog dialog : activeDialogs)
			dialog.onApplicationSizeChanged ();
	}

	private void onApplicationSizeChanged ()
	{
		// adjust window size upon possible screen layout/size change
		setSize (requestedPos.x, requestedPos.y, requestedSize.x, requestedSize.y);
	}

	@Override
	public void onStart ()
	{
		super.onStart ();

		Window window = view.getDialogWindow ();
		if(window != null)
		{
			window.setLayout (ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);

			WindowManager.LayoutParams params = window.getAttributes ();

			params.x = adjustedPos.x;
			params.y = adjustedPos.y;
			params.width = adjustedSize.x;
			params.height = adjustedSize.y;
			params.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
			params.gravity = Gravity.LEFT | Gravity.TOP;
			params.horizontalMargin = 0;
			params.verticalMargin = 0;
			params.dimAmount = 0.2f; // dim the outside area less than default

			if(Build.VERSION.SDK_INT >= 30)
				params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;

			window.setAttributes (params);

			// FLAG_NOT_FOCUSABLE prevents the keyboard from appearing, so remove it here
			window.clearFlags (WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
		}
		
		// copy ui visibility flags from application window (stay in immersive mode)
		view.refreshSystemUiVisibility ();
	}

	@Override
	public void onDismiss (DialogInterface dialog)
	{
		onDismissCalled = true;

		if(view != null)
			onDismissNative (getNativeViewPtr ());

		super.onDismiss (dialog);
	}

	public native void onDismissNative (long nativeViewPtr);
}
