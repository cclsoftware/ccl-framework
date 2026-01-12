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
// Filename    : ccl/platform/android/gui/java/SystemTimer.java
// Description : System Timer
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.os.Handler;
import android.os.Looper;

import androidx.annotation.Keep;

//************************************************************************************************
// SystemTimer
//************************************************************************************************

@Keep
public class SystemTimer implements Runnable
{
	private final long nativeTimerPtr;
	private final long delay;
	private final Handler handler;

	public SystemTimer (long nativeTimerPtr, long delay)
	{
		this.nativeTimerPtr = nativeTimerPtr;
		this.delay = delay;

		handler = new Handler (Looper.getMainLooper ());

		// start the initial runnable task
		handler.postDelayed (this, delay);
	}

	@Override
    public void run()
	{
		onTimerNative (nativeTimerPtr);

		// repeat the same runnable code block again
		handler.postDelayed (this, delay);
    }

	public void stop ()
	{
		handler.removeCallbacks (this);
	}

	public static native void onTimerNative (long nativePtr);
}	
