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
// Filename    : ccl/platform/android/gui/androidintent.h
// Description : Android Intent
//
//************************************************************************************************

#ifndef _ccl_androidintent_h
#define _ccl_androidintent_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// AndroidIntent
//************************************************************************************************

class AndroidIntent : public JniObject,
					  public Unknown
{
public:
	AndroidIntent (JNIEnv* jni, jobject object);
	~AndroidIntent ();

	CString getAction () const;
	String getDataString () const;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_androidintent_h
