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
// Filename    : core/platform/android/java/Logger.java
// Description : Logger
//
//************************************************************************************************

package dev.ccl.core;

import androidx.annotation.Keep;

//************************************************************************************************
// Logger
//************************************************************************************************

@Keep
public class Logger
{
	// logcat output
	public static void log (String tag, String msg)						{ android.util.Log.d (tag, msg); }
	public static void log (String msg)									{ log (kDefaultTag, msg); }

	public static void logError (String tag, String msg)				{ android.util.Log.e (tag, msg); }
	public static void logError (String tag, String msg, Throwable tr)	{ android.util.Log.e (tag, msg, tr); }
	public static void logError (String msg)							{ logError (kDefaultTag, msg); }
	public static void logError (String msg, Throwable tr)				{ logError (kDefaultTag, msg, tr); }

	private static final String kDefaultTag = "CCL";
}
