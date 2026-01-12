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
// Filename    : ccl/platform/android/gui/java/FrameworkActivity.java
// Description : Framework Activity
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.core.CurrentContext;
import dev.ccl.core.Logger;

import android.app.Activity;
import android.app.AlertDialog;
import android.view.*;
import android.graphics.*;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.bluetooth.*;
import android.Manifest;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.*;
import android.database.Cursor;
import android.provider.ContactsContract;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.net.wifi.WifiManager;
import android.net.Uri;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;
import androidx.core.graphics.Insets;
import androidx.core.view.WindowInsetsCompat;
import androidx.documentfile.provider.DocumentFile;

import java.util.ArrayList;
import java.io.*;
import java.net.*;

//************************************************************************************************
// ActivityCallbackItem
//************************************************************************************************

class ActivityCallbackItem
{
	public FrameworkActivity.ActivityCallback callback;
	public int requestCode;
	public int hashCode;

	public ActivityCallbackItem (int requestCode, FrameworkActivity.ActivityCallback callback)
	{
		this.callback = callback;
		this.requestCode = requestCode;
		this.hashCode = System.identityHashCode (this);
	}
}

//************************************************************************************************
// ApplicationView
//************************************************************************************************

class ApplicationView extends FrameworkView
{
	public ApplicationView (Context context)
	{
		super (context);
	}

	@Override
	protected void onSizeChanged (int w, int h, int oldw, int oldh)
	{
		super.onSizeChanged (w, h, oldw, oldh);
		
		// notify dialogs about the application size change
		FrameworkDialog.applicationSizeChanged ();
	}
}

//************************************************************************************************
// FrameworkActivity
//************************************************************************************************

@Keep
public class FrameworkActivity extends Activity
{
	@Keep
	public static class ActivityCallback
	{
		public void onActivityResult (int requestCode, boolean success, Intent resultData) {}
	}

	@Keep
	public static abstract class RequestPermissionsResultHandler
	{
		private static int nextRequestCode = 0x0FFFFFFF;
		private final int requestCode;

		public RequestPermissionsResultHandler () { requestCode = nextRequestCode++; }
		public int getRequestCode () { return requestCode; }

		public abstract void onRequestPermissionsResult (String[] permissions, int[] grantResults);
	}

	@Keep
	public static class FileInfo
	{
		public long fileSize;
		public long modifiedTime;

		public FileInfo (long fileSize, long modifiedTime)
		{
			this.fileSize = fileSize;
			this.modifiedTime = modifiedTime;
		}
	}

	private ApplicationView view;
	private String appName;

	private Intent savedIntent;
	private WifiManager.MulticastLock multicastLock = null;
	private int systemUiFlags = 0;

	private boolean foregroundActivity = false;
	private boolean awaitingActivityResult = false;
	private boolean processRestartRequested = false;

	private final ArrayList<ActivityCallbackItem> activityCallbacks = new ArrayList<> ();
	private final ArrayList<RequestPermissionsResultHandler> requestPermissionsResultHandlers = new ArrayList<> ();

	private static final int kPermisssionRequestCode = 1;
	private static final int kPermisssionRequestCodeRestart = 2;

	private static FrameworkActivity frameworkActivity;
	private static String mainModuleID;

	private static boolean loadFailed = false;
	private static boolean launchErrorReported = false;

	private static final boolean DEBUG_LOG = false;

	public static FrameworkActivity getActivity ()
	{
		return frameworkActivity;
	}

	public void setAppName (String appName)
	{
		this.appName = appName;
	}

	public static String getMainModuleID ()
	{
		return mainModuleID;
	}

	public String getNativeLibraryDir ()
	{
		return getApplicationInfo ().nativeLibraryDir;
	}

	public static void loadNativeLibraries (String moduleID)
	{
		try
		{
			// main module must be loaded first, otherwise cclsystem won't find its exported CCLAndroidMain function
			System.loadLibrary (moduleID);

			if(mainModuleID == null)
			{
				mainModuleID = moduleID;

				// explicitely load native framework modules that require JNI access
				// (they are already loaded via module link dependencies, but this will call their JNI_OnLoad)
				System.loadLibrary ("cclgui");
				System.loadLibrary ("ccltext");
				System.loadLibrary ("cclsystem");
				
				// CCL Net and CCL Security are optional
				try { System.loadLibrary ("cclnet"); } catch (UnsatisfiedLinkError e) {}
				try { System.loadLibrary ("cclsecurity"); } catch (UnsatisfiedLinkError e) {}
			}
		}
		catch (UnsatisfiedLinkError e)
		{
			loadFailed = true;
		}
	}

	public static String getResourceString (String name)
	{
		Resources resources = frameworkActivity.getResources ();
		String packageName = frameworkActivity.getPackageName ();

		return resources.getString (resources.getIdentifier (name, "string", packageName));
	}

	public void reportLaunchError (String message)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder (frameworkActivity);

		builder.setTitle (getResourceString ("launch_failed"));
		builder.setMessage (message);
		builder.setNeutralButton (android.R.string.ok, null);
		builder.setOnDismissListener (new DialogInterface.OnDismissListener ()
		{
			public void onDismiss (DialogInterface dialog)
			{ 
				frameworkActivity.finishAndRemoveTask ();
				Process.killProcess (Process.myPid ());
			}
		});
		builder.setIcon (android.R.drawable.ic_dialog_alert);
		builder.show ();

		launchErrorReported = true;
	}

    /** Called when the activity is first created. */
    @Override
    public void onCreate (Bundle savedInstanceState)
    {
		frameworkActivity = this;

		CurrentContext.set (this);

		getWindow ().setBackgroundDrawable (null);

		super.onCreate (savedInstanceState);

		requestWindowFeature (Window.FEATURE_NO_TITLE); // remove titlebar

		// make statusbar transparent
		getWindow ().setStatusBarColor (Color.TRANSPARENT);
		getWindow ().setFlags (WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS, WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

		view = new ApplicationView (this);
		setContentView (view);

		if(appName != null)
			view.setContentDescription (appName);

		if(!loadFailed)
		{
			if(onCreateNative (savedInstanceState, view.getNativeViewPtr ()))
				onNewIntent (getIntent ());
			else if(!launchErrorReported)
				reportLaunchError (getResourceString ("init_failed_message"));
		}
		else
			reportLaunchError (getResourceString ("load_failed_message"));
    }

    @Override
	public void onNewIntent (Intent intent)
	{
		setIntent (intent);

		// check file access permissions when trying to open file:// URIs
		String scheme = intent.getScheme ();
		if(scheme != null && scheme.equals ("file"))
		{
			this.savedIntent = getIntent ();
			if(!checkAndRequestPermissions (new String[] { Manifest.permission.READ_EXTERNAL_STORAGE }, kPermisssionRequestCodeRestart))
				setIntent (new Intent ());
		}

		onNewIntentNative ();
	}

    @Override
	public void onRestart ()
	{
		onRestartNative ();
		super.onRestart ();
	}

    @Override
	public void onResume ()
	{
		acquireMulticastLock ();

		onResumeNative ();

		super.onResume ();

		foregroundActivity = true;

		// defer applying system visibility flags to avoid jumping of our "launch" drawable when the available area grows
		final Handler handler = new Handler (Looper.getMainLooper ());
		handler.postDelayed (new Runnable ()
		{
			@Override
			public void run()
			{
				refreshSystemUiVisibility ();
			}
		}, 100);
	}

    @Override
	public void onPause ()
	{
		releaseMulticastLock ();

		foregroundActivity = false;

		onPauseNative ();
		super.onPause ();
	}

    @Override
	public void onStop ()
	{
		onStopNative (awaitingActivityResult);
		super.onStop ();
	}

	/** Perform final cleanup before the activity is destroyed. */
    @Override
	public void onDestroy ()
	{
		// first destroy view (-> close window)
		view.destruct ();
		view = null;

		onDestroyNative ();

		frameworkActivity = null;

		super.onDestroy ();
		
		if(processRestartRequested)
			System.exit (0);
	}

	@Override
	public void onSaveInstanceState (@NonNull Bundle outState)
	{
		onSaveInstanceStateNative (outState, awaitingActivityResult);
		super.onSaveInstanceState (outState);
	}

	@Override
	public void onRestoreInstanceState (@NonNull Bundle savedInstanceState)
	{
		onRestoreInstanceStateNative (savedInstanceState);
		super.onRestoreInstanceState (savedInstanceState);
	}

	/** Enable holding a multicast lock during "running" state. To be called only by Apps that need it. */
	protected void enableMulticastLock ()
	{
		if(multicastLock == null)
		{
			WifiManager wifiManager = (WifiManager) getApplicationContext ().getSystemService (Context.WIFI_SERVICE);
			if(wifiManager != null)
				multicastLock = wifiManager.createMulticastLock ("CCL");
		}
	}

	private void acquireMulticastLock ()
	{
		if(multicastLock != null)
		{
			multicastLock.setReferenceCounted (true);
			multicastLock.acquire ();
		}
	}

	private void releaseMulticastLock ()
	{
		if(multicastLock != null)
			multicastLock.release ();
	}

	public AssetManager getAssets ()
	{
		return getResources ().getAssets ();
	}

	@Override
	public void onConfigurationChanged (@NonNull Configuration newConfig)
	{
		super.onConfigurationChanged (newConfig);
		
		onConfigurationChangedNative ();

		refreshSystemUiVisibility ();
	}

	public void setKeepScreenOn (boolean state)
	{
		if(state)
			getWindow ().addFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		else
			getWindow ().clearFlags (WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}

	public void setAllowedInterfaceOrientations (int orientations)
	{
		int requested = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

		if(orientations == GuiConstants.kOrientationFlagLandscapeAllowed)
			requested = ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE;
		else if(orientations == GuiConstants.kOrientationFlagPortraitAllowed)
			requested = ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT;

		setRequestedOrientation (requested);
	}

	public Point getDisplaySize ()
	{
		Point size = new Point ();
		getWindowManager ().getDefaultDisplay ().getRealSize (size);
		return size;
	}

	public Rect getRectOnScreen ()
	{
		if(view == null)
			return null;

		Rect screenRect = view.getRectOnScreen ();
		if(screenRect.isEmpty())
			getWindowManager ().getDefaultDisplay ().getRectSize (screenRect);
		return screenRect;
	}

	public Rect getRectInWindow ()
	{
		if(view == null)
			return null;

		Rect windowRect = view.getRectInWindow ();
		if(windowRect.isEmpty())
			getWindowManager ().getDefaultDisplay ().getRectSize (windowRect);
		return windowRect;
	}

	private int getResourceDimension (String name)
	{
		int resourceId = getResources ().getIdentifier (name, "dimen", "android");
		return resourceId > 0 ? getResources ().getDimensionPixelSize (resourceId) : 0;
	}

	public int getStatusBarHeight ()
	{
		if((getSystemUiVisibility () & View.SYSTEM_UI_FLAG_FULLSCREEN) != 0)
			return 0;

		return getResourceDimension ("status_bar_height");
	}

	public boolean isStatusBarVisible ()
	{
		if(view == null || !view.isAttachedToWindow ())
			return true;

		WindowInsets insets = view.getRootWindowInsets ();
		return insets.getStableInsetTop () != 0;
	}

	public Rect getInsets ()
	{
		if(view == null || !view.isAttachedToWindow ())
			return new Rect ();

		int insetTypes = WindowInsetsCompat.Type.displayCutout ();
		if((getSystemUiVisibility () & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
			insetTypes |= WindowInsetsCompat.Type.statusBars ();
		if((getSystemUiVisibility () & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0)
			insetTypes |= WindowInsetsCompat.Type.navigationBars ();

		WindowInsetsCompat windowInsets = WindowInsetsCompat.toWindowInsetsCompat (view.getRootWindowInsets (), view);
		Insets insets = windowInsets.getInsets (insetTypes);

		return new Rect (insets.left, insets.top, insets.right, insets.bottom);
	}

	public int getDensity ()
	{
		return getResources ().getConfiguration ().densityDpi;
	}

	public int getOrientation ()
	{
		return getResources ().getConfiguration ().orientation;
	}

	public int getScreenSize ()
	{
		return getResources ().getConfiguration ().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
	}

	public void setLightStatusBar (boolean lightStatusBar)
	{
		Window window = getWindow ();
        int systemUiVisibility = window.getDecorView ().getSystemUiVisibility ();

		if(lightStatusBar)
			systemUiVisibility |= View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
		else
			systemUiVisibility &= ~View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;

        window.getDecorView ().setSystemUiVisibility (systemUiVisibility);
	}

	public void setSystemUiVisibility (boolean hideStatusBar, boolean hideNavBar)
	{
		if(view == null)
			return;

		// always layout as with hidden status bar
		systemUiFlags = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
		view.setSystemUiVisibility (systemUiFlags);

		// hide flags will be applied later in onResume
		if(hideStatusBar)
			systemUiFlags |= (View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
		if(hideNavBar)
			systemUiFlags |= (View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}

	public void refreshSystemUiVisibility ()
	{
		if(view != null)
			view.refreshSystemUiVisibility ();
	}

	public int getSystemUiVisibility ()
	{
		return systemUiFlags;
	}

	public boolean isForegroundActivity ()
	{
		return foregroundActivity;
	}

	public boolean openApplicationSettings ()
	{
		try
		{
			Intent intent = new Intent (Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
			intent.setData (Uri.fromParts ("package", getPackageName (), null));
			startActivity (intent);
		}
		catch (Exception e)
		{
			return false;
		}

		return true;
	}

	public boolean openUrl (String url)
	{
		try
		{
			Intent intent = new Intent (Intent.ACTION_VIEW);
			intent.setData (Uri.parse (url));
			startActivity (intent);
		}
		catch (Exception e)
		{
			return false;
		}

		return true;
	}

	public long getPackageInstallTime ()
	{
		long installTime = System.currentTimeMillis ();

		try
		{
			installTime = getPackageManager ().getPackageInfo (getPackageName(), 0).firstInstallTime;
		}
		catch (Exception e)
		{}

		return installTime / 1000;
	}

	public long getPackageUpdateTime ()
	{
		long updateTime = System.currentTimeMillis ();

		try
		{
			updateTime = getPackageManager ().getPackageInfo (getPackageName(), 0).lastUpdateTime;
		}
		catch (Exception e)
		{}

		return updateTime / 1000;
	}

	public boolean checkPermission (String permission)
	{
		if(checkCallingOrSelfPermission (permission) == PackageManager.PERMISSION_GRANTED)
			return true;

		Logger.logError ("missing permission: " + permission);
		return false;
	}

	public boolean checkAndRequestPermissions (String[] permissions)
	{
		return checkAndRequestPermissions (permissions, kPermisssionRequestCode);
	}

	public boolean checkAndRequestPermissions (String[] permissions, int requestCode)
	{
		ArrayList<String> pending = new ArrayList<> ();
		for(String permission : permissions)
			if(checkSelfPermission (permission) != PackageManager.PERMISSION_GRANTED)
				pending.add (permission);

		if(pending.isEmpty ())
			return true;

		// request permissions: shows dialog, will invoke callback onRequestPermissionsResult
		requestPermissions (pending.toArray (new String[0]), requestCode);
		return false;
	}

	public void checkAndRequestPermissions (String[] permissions, RequestPermissionsResultHandler resultHandler)
	{
		requestPermissionsResultHandlers.add (resultHandler);

		if(checkAndRequestPermissions (permissions, resultHandler.getRequestCode ()))
		{
			int[] grantResults = new int[permissions.length];
			for(int i = 0; i < grantResults.length; i++)
				grantResults[i] = PackageManager.PERMISSION_GRANTED;

			resultHandler.onRequestPermissionsResult (permissions, grantResults);
			requestPermissionsResultHandlers.remove (resultHandler);
		}
	}

	@Override
	public void onRequestPermissionsResult (int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
	{
		for(int i = 0; i < permissions.length; i++)
		{
			if(DEBUG_LOG)
				Logger.log ("onRequestPermissionsResult: " + requestCode + ", " + permissions[i] + " Result: " + grantResults[i]);

			if(grantResults[i] != PackageManager.PERMISSION_GRANTED)
				Logger.logError ("missing permission: " + permissions[i]);
		}

		if(requestCode == kPermisssionRequestCodeRestart)
		{
			boolean allGranted = true;
			for(int grantResult : grantResults)
				if(grantResult != PackageManager.PERMISSION_GRANTED)
					allGranted = false;

			if(permissions.length > 0 && allGranted)
			{
				Intent intent = new Intent (this, this.getClass ());

				intent.setAction (savedIntent.getAction ());
				intent.setData (savedIntent.getData ());
				intent.setFlags (Intent.FLAG_ACTIVITY_NO_ANIMATION | Intent.FLAG_ACTIVITY_FORWARD_RESULT);

				startActivity (intent);
			}
		}
		else
		{
			for(RequestPermissionsResultHandler resultHandler : requestPermissionsResultHandlers)
			{
				if(resultHandler.getRequestCode () == requestCode)
				{
					resultHandler.onRequestPermissionsResult (permissions, grantResults);
					requestPermissionsResultHandlers.remove (resultHandler);
					break;
				}
			}
		}

		super.onRequestPermissionsResult (requestCode, permissions, grantResults);
	}

	public String getComputerName ()
	{
		String computerName = null;

		if((Build.VERSION.SDK_INT <= 30 && checkPermission (Manifest.permission.BLUETOOTH)) ||
		   (Build.VERSION.SDK_INT >= 31 && checkPermission ("android.permission.BLUETOOTH_CONNECT")))
		{
			BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
			if(adapter != null)
			{
				String adapterName = adapter.getName ();
				if(adapterName != null)
					computerName = adapterName;
			}
		}

		if(computerName == null)
		{
			String manufacturer = Build.MANUFACTURER;
			String model = Build.MODEL;
			if(model.startsWith (manufacturer))
				computerName = model;
			else
				computerName = manufacturer + " " + model;
		}
		return computerName;
	}

	public String getUserName ()
	{
		String userName = "";
		if(checkPermission (Manifest.permission.READ_CONTACTS))
		{
			try
			{
				Cursor cursor = getContentResolver ().query (ContactsContract.Profile.CONTENT_URI, null, null, null, null);
				if(cursor != null)
				{
					if(cursor.moveToFirst ())
						userName = cursor.getString (cursor.getColumnIndex (ContactsContract.Profile.DISPLAY_NAME));

					cursor.close ();
				}
			}
			catch (Exception e)
			{}
		}
		return userName;
	}

	public int getSDKVersion ()
	{
		return Build.VERSION.SDK_INT;
	}

	public String getDeviceID ()
	{
		return Secure.getString (getContentResolver(), Secure.ANDROID_ID);
	}

	public ParcelFileDescriptor openContentFile (String uriString, String mode)
	{
		Uri uri = getContentFileUri (uriString, mode.contains ("w"));
		if(uri == null)
			return null;

		try
		{
			ParcelFileDescriptor descriptor = getContentResolver ().openFileDescriptor (uri, mode);
			if(descriptor != null)
			{
				// persist permissions across restarts
				try
				{
					getContentResolver ().takePersistableUriPermission (uri, Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
					// todo: when to releasePersistableUriPermission?
				}
				catch (SecurityException e)
				{}

				return descriptor;
			}
		}
		catch (Exception e)
		{
			Logger.logError ("openContentFile: " + e.getMessage () + ", " + uriString);
		}

		return null;
	}

	public boolean contentFileExists (String uriString)
	{
		Uri uri = getContentFileUri (uriString, false);
		if(uri == null)
			return false;

		DocumentFile sourceFile = DocumentFile.fromSingleUri (this, uri);
		return sourceFile.exists ();
	}

	public FileInfo getContentFileInfo (String uriString)
	{
		Uri uri = getContentFileUri (uriString, false);
		if(uri == null)
			return null;
		
		DocumentFile sourceFile = DocumentFile.fromSingleUri (this, uri);
		return new FileInfo (sourceFile.length (), sourceFile.lastModified () / 1000);
	}
	
	public String getContentFileDisplayName (String uriString)
	{
		Uri uri = getContentFileUri (uriString, false);
		if(uri == null)
			return "";
			
		DocumentFile sourceFile = DocumentFile.fromSingleUri (this, uri);
		return sourceFile.getName ();
	}

	public Uri getContentFileUri (String uriString, boolean create)
	{
		final String kContentScheme = "content";
		final String kFileScheme = "file";

		Uri uri = Uri.parse (uriString);
		if(uri != null && uri.getScheme () == null)
			return Uri.parse (kFileScheme.concat ("://").concat (uriString));

		if(uri == null || !uri.getScheme ().equals (kContentScheme))
			return uri;

		DocumentFile file = DocumentFile.fromSingleUri (this, uri);
		if(file.exists ())
			return uri;

		try
		{
			// try opening the passed URI first
			ParcelFileDescriptor descriptor = getContentResolver ().openFileDescriptor (uri, create ? "rw" : "r");
			descriptor.close ();
		}
		catch (IllegalArgumentException | FileNotFoundException e)
		{
			// if opening the URI failed, check if this is a tree URI with an appended file name
			// by listing the files in this folder, then return the corresponding document URI
			Uri parent = Uri.parse (uriString.substring (0, uriString.lastIndexOf ("/")));
			String displayName = uriString.substring (uriString.lastIndexOf ("/") + 1);
			boolean exists = false;

			try
			{
				ContentResolver contentResolver = getContentResolver ();
				Uri children = DocumentsContract.buildChildDocumentsUriUsingTree (parent, DocumentsContract.getTreeDocumentId (parent));

				Cursor cursor = contentResolver.query (children, new String[] { DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME }, null, null, null);
				if(cursor != null)
				{
					while(!exists && cursor.moveToNext ())
					{
						String docId = cursor.getString (0);
						String name = cursor.getString (1);
						if(name.equals (displayName))
						{
							uri = DocumentsContract.buildDocumentUriUsingTree (parent, docId);
							exists = true;
						}
					}

					cursor.close ();
				}

				// create the document if requested and it doesn't exist yet
				if(!exists && create)
					uri = DocumentsContract.createDocument (getContentResolver (), parent, "*/*", displayName);

				return uri;
			}
			catch (Exception x)
			{}
		}
		catch (Exception e)
		{}

		return uri;
	}

	public void relaunchActivity ()
	{
		Context context = getApplicationContext ();

		// use PackageManager to get package information
		PackageManager packageManager = context.getPackageManager ();
		if(packageManager == null)
			return;

		// find the intent that launches this activity
		Intent launchIntent = packageManager.getLaunchIntentForPackage (context.getPackageName ());
		if(launchIntent == null)
			return;

		// create a restart activity from the launch intent and start it
		Intent restartIntent = Intent.makeRestartActivityTask (launchIntent.getComponent ());
		if(restartIntent == null)
			return;

		processRestartRequested = true;

		context.startActivity (restartIntent);
	}

	private static final int FILE_SELECTOR_REQUEST_OPEN = 100;
	private static final int FILE_SELECTOR_REQUEST_CREATE = 101;
	private static final int FOLDER_SELECTOR_REQUEST = 102;
	private static final int SHARE_FILE_REQUEST = 103;
	private static final int SHARE_TEXT_REQUEST = 104;

	public boolean isMimeTypeSupported (String mimeType)
	{
		return android.webkit.MimeTypeMap.getSingleton ().hasMimeType (mimeType);
	}

	public boolean startActivityWithRequestCode (Intent intent, int requestCode)
	{
		try
		{
			awaitingActivityResult = true;
			startActivityForResult (intent, requestCode);
		}
		catch (Exception e)
		{
			awaitingActivityResult = false;
			return false;
		}

		return true;
	}

	public boolean runFileSelector (boolean create, String mimeType, String initialFileName)
	{
		Intent intent = new Intent (create ? Intent.ACTION_CREATE_DOCUMENT : Intent.ACTION_OPEN_DOCUMENT);
		intent.addCategory (Intent.CATEGORY_OPENABLE); // only show results that can be "opened", such as a file
		if(DEBUG_LOG)
			Logger.log ("mimeType: " + mimeType);
		
		if(mimeType.isEmpty ())
			intent.setType ("*/*");
		else
			intent.setTypeAndNormalize (mimeType);

		if(!initialFileName.isEmpty ())
			intent.putExtra (Intent.EXTRA_TITLE, initialFileName);

		return startActivityWithRequestCode (intent, create ? FILE_SELECTOR_REQUEST_CREATE : FILE_SELECTOR_REQUEST_OPEN);
	}

	public boolean runFolderSelector (String initialFolderName)
	{
		Intent intent = new Intent (Intent.ACTION_OPEN_DOCUMENT_TREE);
		intent.addCategory (Intent.CATEGORY_DEFAULT);
		intent.addFlags (Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION | Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

		if(!initialFolderName.isEmpty ())
			intent.putExtra (Intent.EXTRA_TITLE, initialFolderName);

		return startActivityWithRequestCode (intent, FOLDER_SELECTOR_REQUEST);
	}
	
	public boolean runFileSharing (String uriString, String mimeType)
	{
		Uri uri = Uri.parse (uriString);
		if(uri == null)
			return false;

		try
		{
			File file = new File (new URI (uriString.replace (" ", "%20")));
			Uri contentUri = FileProvider.getUriForFile (this, getPackageName (), file);

			Intent intent = new Intent (Intent.ACTION_SEND);
			intent.putExtra (Intent.EXTRA_STREAM, contentUri);
			intent.setFlags (Intent.FLAG_GRANT_READ_URI_PERMISSION);

			if(mimeType.isEmpty ())
				intent.setType ("*/*");
			else
				intent.setTypeAndNormalize (mimeType);

			return startActivityWithRequestCode (intent, SHARE_FILE_REQUEST);
		}
		catch (Exception e)
		{}
		
		return false;
	}
	
	public boolean runTextSharing (String text)
	{
		try
		{
			Intent intent = new Intent (Intent.ACTION_SEND);
			intent.putExtra (Intent.EXTRA_TEXT, text);
			intent.setType ("text/plain");
			return startActivityWithRequestCode (intent, SHARE_TEXT_REQUEST);
		}
		catch (Exception e)
		{}
		
		return false;
	}

	public boolean startActivityWithCallback (Intent intent, int requestCode, ActivityCallback callback)
	{
		ActivityCallbackItem item = new ActivityCallbackItem (requestCode, callback);
		activityCallbacks.add (item);

		return startActivityWithRequestCode (intent, item.hashCode);
	}

	ActivityCallbackItem findActivityCallbackItem (int hashCode)
	{
		for(ActivityCallbackItem item : activityCallbacks)
			if(item.hashCode == hashCode)
				return item;
		return null;
	}

	@Override
	public void onActivityResult (int requestCode, int resultCode, Intent resultData)
	{
		awaitingActivityResult = false;

		if(requestCode == FILE_SELECTOR_REQUEST_OPEN || requestCode == FILE_SELECTOR_REQUEST_CREATE)
		{
			if(resultCode == Activity.RESULT_OK && resultData != null)
			{
				Uri uri = resultData.getData ();
				if(uri != null)
				{
					// test (returned null)
					//Uri uri2 = getContentResolver ().canonicalize (uri);

					String uriString = uri.toString ();
					if(DEBUG_LOG)
					{
						Logger.log ("File Selector: " + uriString);
						Logger.log ("  mimeType: " + getContentResolver ().getType (uri));
					}

					onFileSelectorResult (uriString, getContentFileDisplayName (uriString));
				}
			}
			else
				onFileSelectorResult (null, null);
		}
		else if(requestCode == FOLDER_SELECTOR_REQUEST)
		{
			if(resultCode == Activity.RESULT_OK && resultData != null)
			{
				Uri uri = resultData.getData ();
				if(uri != null)
				{
					getContentResolver ().takePersistableUriPermission (uri, Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

					uri = DocumentsContract.buildDocumentUriUsingTree(uri, DocumentsContract.getTreeDocumentId(uri));

					String uriString = uri.toString ();
					onFolderSelectorResult (uriString, null);
				}
			}
			else
				onFolderSelectorResult (null, null);
		}
		else if(requestCode == SHARE_FILE_REQUEST || requestCode == SHARE_TEXT_REQUEST)
		{
			onSharingFinished ();
		}
		else
		{
			ActivityCallbackItem item = findActivityCallbackItem (requestCode);
			if(item != null)
			{
				item.callback.onActivityResult (item.requestCode, resultCode == Activity.RESULT_OK, resultData);
				activityCallbacks.remove (item);
			}
		}
	}

	private int getCharacterCodeForKeyEvent (KeyEvent event)
	{
		int character = event.getUnicodeChar ();
		if(character == 0 && (event.isCtrlPressed () || event.isAltPressed ()))
			character = event.getUnicodeChar (0);

		return character;
	}

	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event)
	{
		//Logger.log ("onKeyDown: keyCode: " + keyCode + "  char: " + String.format ("%x", event.getUnicodeChar ()) + "  displayLabel: " + event.getDisplayLabel () + "  modifiers: " + event.getModifiers () + (event.getRepeatCount () > 0 ? " Repeat" : ""));

		if(!(getCurrentFocus () instanceof TextControl))
			if(onKeyEventNative (true, keyCode, getCharacterCodeForKeyEvent (event), event.getModifiers (), event.getRepeatCount () > 0))
				return true;

		return super.onKeyDown (keyCode, event);
	}

	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event)
	{
		if(!(getCurrentFocus () instanceof TextControl))
			onKeyEventNative (false, keyCode, getCharacterCodeForKeyEvent (event), event.getModifiers (), event.getRepeatCount () > 0);

		return super.onKeyUp (keyCode, event);
	}

	@Override
	public void onBackPressed ()
	{
		if(onBackPressedNative ())
			return;

		super.onBackPressed ();
	}

	public static native void onFolderSelectorResult (String uri, String displayName);
	public static native void onFileSelectorResult (String uri, String displayName);
	public static native void onSharingFinished ();

	public native boolean onCreateNative (Bundle savedInstanceState, long nativeViewPtr);
	public native void onNewIntentNative ();

	public native void onRestartNative ();
	public native void onResumeNative ();
	public native void onPauseNative ();
	public native void onStopNative (boolean keepDialogsOpen);
	public native void onDestroyNative ();

	public native void onSaveInstanceStateNative (Bundle outState, boolean keepDialogsOpen);
	public native void onRestoreInstanceStateNative (Bundle savedInstanceState);

	public native void onConfigurationChangedNative ();

	public native boolean onKeyEventNative (boolean isDown, int keyCode, int character, int modifiers, boolean isRepeat);
	public native boolean onBackPressedNative ();
}
