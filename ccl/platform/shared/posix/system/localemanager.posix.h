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
// Filename    : ccl/platform/shared/posix/system/localemanager.posix.h
// Description : POSIX Locale Manager
//
//************************************************************************************************

#ifndef _ccl_localemanager_posix_h
#define _ccl_localemanager_posix_h

#include "ccl/system/localization/localemanager.h"

namespace CCL {

//************************************************************************************************
// PosixLocaleManager
//************************************************************************************************

class PosixLocaleManager: public LocaleManager
{
public:
	DECLARE_STRINGID_MEMBER (kUserLanguage)
	DECLARE_STRINGID_MEMBER (kLanguagePack)
	
protected:
	static const String kSettingsFileName;

	mutable MutableCString userLanguage;
	mutable String languagePack;

	static StringID getPrimaryLanguageCode (CStringPtr langid);
	static StringID getSubLanguageCode (CStringPtr langid);

	void storeSettings () const;
	void restoreSettings () const;

	// LocaleManager
	bool getNativeUserLanguage (MutableCString& language) const override;
	void setNativeUserLanguage (StringID language) override;
	void setNativeLanguagePack (StringRef pathString) override;
	bool getNativeLanguagePack (String& pathString) const override;
};

} // namespace CCL

#endif // _ccl_localemanager_posix_h
