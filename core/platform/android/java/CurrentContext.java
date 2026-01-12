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
// Filename    : core/platform/android/java/CurrentContext.java
// Description : Android Context Management
//
//************************************************************************************************

package dev.ccl.core;

import android.content.Context;

import androidx.annotation.Keep;

//************************************************************************************************
// CurrentContext
//************************************************************************************************

@Keep
public class CurrentContext
{
	private static Context currentContext;

	public static Context get ()
	{
		return currentContext;
	}

	public static void set (Context context)
	{
		currentContext = context;
	}
}
