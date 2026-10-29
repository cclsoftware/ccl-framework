//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/android/network/asyncdnsresolver.android.cpp
// Description : Android DNS server discovery for c-ares
//
//************************************************************************************************

#include "ccl/platform/android/interfaces/iandroidsystem.h"
#include "ccl/platform/android/interfaces/iframeworkactivity.h"
#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/base/initterm.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"

#include "ares.h"

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AsyncDNSResolver
/** Since Android 8, c-ares can no longer read the DNS servers from the net.dns# system properties
	and falls back to 127.0.0.1. It needs the JVM and the ConnectivityManager instead, which
	requires the android.permission.ACCESS_NETWORK_STATE permission in the application manifest. */
//************************************************************************************************

CCL_KERNEL_INIT_LEVEL (AsyncDNSResolver, kFrameworkLevelFirst)
{
	JniAccessor jni;
	JavaVM* vm = nullptr;
	if(jni->GetJavaVM (&vm) != JNI_OK)
		return true;

	ares_library_init_jvm (vm);

	UnknownPtr<IAndroidSystem> androidSystem (&System::GetSystem ());
	IFrameworkActivity* activity = androidSystem ? androidSystem->getNativeActivity () : nullptr;
	if(!activity)
	{
		CCL_WARN ("AsyncDNSResolver: no activity, DNS servers can't be determined\n", 0)
		return true;
	}

	// use the application context, the ConnectivityManager outlives the activity
	LocalRef context (jni, Context.getApplicationContext (activity->getJObject ()));
	LocalStringRef serviceName (jni, jobject_cast<jstring> (jni.getStaticField (Context, "CONNECTIVITY_SERVICE", "Ljava/lang/String;")));
	LocalRef connectivityManager (jni, Context.getSystemService (context, serviceName));

	if(!connectivityManager || ares_library_init_android (connectivityManager) != ARES_SUCCESS)
		CCL_WARN ("AsyncDNSResolver: ConnectivityManager not available, DNS servers can't be determined\n", 0)

	return true;
}
