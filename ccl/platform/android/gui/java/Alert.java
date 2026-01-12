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
// Filename    : ccl/platform/android/gui/java/Alert.java
// Description : AlertBox implementation
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;

import androidx.annotation.Keep;

//************************************************************************************************
// Alert
//************************************************************************************************

@Keep
public class Alert
{
	private long nativeAlertPtr;
	private AlertDialog dialog;

	public Alert (long nativeAlertPtr)
	{
		this.nativeAlertPtr = nativeAlertPtr;
	}
	
	public void run (Context context, String title, String message, String button1, String button2, String button3)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder (context);
		builder.setTitle (title);
		builder.setMessage (message);
		builder.setCancelable (true);
		//builder.setIcon ();

		builder.setPositiveButton (button1, new DialogInterface.OnClickListener ()
		{
			public void onClick (DialogInterface dialog, int id)
			{
				onAlertResult (0);
			}
		});

		builder.setNegativeButton (button2, new DialogInterface.OnClickListener ()
		{
			public void onClick (DialogInterface dialog, int id)
			{
				onAlertResult (1);
			}
		});

		builder.setNeutralButton (button3, new DialogInterface.OnClickListener ()
		{
			public void onClick (DialogInterface dialog, int id)
			{
				onAlertResult (2);
			}
		});

		builder.setOnDismissListener (new DialogInterface.OnDismissListener  ()
		{
			public void onDismiss (DialogInterface dialog)
			{
				onAlertResult (-1);
			}
		});

		dialog = builder.create ();	
		dialog.show ();
	}

	public void onAlertResult (int button)
	{
		if(nativeAlertPtr != 0)
			onAlertResultNative (nativeAlertPtr, button);

		nativeAlertPtr = 0;
	}

	public void dismiss ()
	{
		if(dialog != null)
			dialog.dismiss ();
	}

	public static native void onAlertResultNative (long nativeAlertPtr, int button);
}	
