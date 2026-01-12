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
// Filename    : ccl/platform/android/gui/java/WebControl.java
// Description : WebControl
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.webkit.*;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.Keep;

//************************************************************************************************
// WebControlWebViewClient 
//************************************************************************************************

class WebControlWebViewClient extends WebViewClient
{
	private final WebControl webControl;
	private final boolean restrictToLocal;

	public WebControlWebViewClient (WebControl webControl, boolean restrictToLocal)
	{
		this.webControl = webControl;
		this.restrictToLocal = restrictToLocal;
	}

	@Override
	public boolean shouldOverrideUrlLoading (WebView view, WebResourceRequest request)
	{	
		if(restrictToLocal)
		{
			// restrict to local pages: open in external app otherwise
			String protocol = request.getUrl ().getScheme ().toLowerCase ();
			if(protocol.isEmpty () || protocol.equals ("file"))
			{
				Context context = FrameworkActivity.getActivity ();
				Intent intent = new Intent (Intent.ACTION_VIEW, request.getUrl ());
				context.startActivity (intent);
				return true;
			}
		}
		return false; // default: let WebView load the page
	}

	@Override
	public void onPageFinished (WebView view, String url)
	{
		webControl.onPageFinishedNative (webControl.getNativeWebControlPtr ());
	}
}

//************************************************************************************************
// WebControl
//************************************************************************************************

@Keep
public class WebControl extends WebView
{
	private final FrameworkView parentView;
	private long nativeWebControlPtr;

	public WebControl (Context context, long nativeWebControlPtr, FrameworkView parentView, boolean restrictToLocal)
	{
		super (context);
		this.parentView = parentView;
		this.nativeWebControlPtr = nativeWebControlPtr;

		WebSettings webSettings = getSettings();
		webSettings.setJavaScriptEnabled (true);

		setWebViewClient (new WebControlWebViewClient (this, restrictToLocal));
    }

	public void show ()
	{
		parentView.addView (this);
	}

	public void remove ()
	{
		nativeWebControlPtr = 0;
		parentView.removeView (this);
	}

	public void setSize (int left, int top, int width, int height)
	{
		layout (left, top, left + width, top + height);
	}

	long getNativeWebControlPtr ()
	{
		return nativeWebControlPtr;
	}

	public native void onPageFinishedNative (long nativeWebControlPtr);
}
