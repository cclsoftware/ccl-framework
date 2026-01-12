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
// Filename    : localetest.cpp
// Description : Locale Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/text/language.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocaleinfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/collections/iunknownlist.h"

using namespace CCL;

enum DaysOfWeek
{
	kSunday, kMonday, kTuesday, kWednesday, kThursday, kFriday, kSaturday
};

//************************************************************************************************
// LocaleTest
//************************************************************************************************

CCL_TEST (LocaleTest, TestDayOfWeek)
{
	const ILocaleInfo& info = System::GetLocaleManager ().getCurrentLocale ();

	CCL_TEST_ASSERT (info.getDayOfWeek (Date (2008, 1, 1)) == kTuesday);
	CCL_TEST_ASSERT (info.getDayOfWeek (Date (2008, 3, 8)) == kSaturday);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (LocaleTest, TestDateTime)
{
	const ILocaleInfo* info = nullptr;
	CCL_TEST_ASSERT ((info = System::GetLocaleManager ().getLocale (LanguageCode::German)) != nullptr);
	if(info == nullptr)
		return;

	DateTime now;
	System::GetSystem ().getLocalTime (now);
	
	String shortDate;
	info->printDate (shortDate, now.getDate ());
	Logging::debug (shortDate);

	String friendlyDate;
	info->printDate (friendlyDate, now.getDate (), ILocaleInfo::kFriendlyDate);
	Logging::debug (friendlyDate);

	String timeString;
	info->printTime (timeString, now.getTime ());
	Logging::debug (timeString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (LocaleTest, TestTranslationTable)
{
	AutoPtr<ITranslationTable> t = System::CreateTranslationTable ();
	t->addVariable ("APPNAME", "Test One");

	const char* scopeName = "Menu";
	const char* keyString = "$APPNAME Website";
	const char* textUTF8 = "$APPNAME网站";

	String text;
	text.appendCString (Text::kUTF8, textUTF8);
	t->addString (scopeName, keyString, text);

	String result;
	t->getString (result, scopeName, keyString);
	Logging::debug ("Result %(1)", result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (LocaleTest, TestGeographicRegions)
{
	ILocaleManager& localeManager (System::GetLocaleManager ());
	Logging::debug ("System ISO: %(1)", String (localeManager.getSystemRegion ()));
	Logging::debug ("\n");

	IterForEachUnknown (localeManager.createGeographicRegionIterator (), unk)
		UnknownPtr<IGeographicRegion> region (unk);
		if(region.isValid ())
		{
			Logging::debug ("ISO: %(1)", region->getISO2Code ());
			Logging::debug ("Eng: %(1)", region->getEnglishName ());
			Logging::debug ("Native: %(1)", region->getNativeName ());
			Logging::debug ("Local: %(1)", region->getLocalizedName ());
			Logging::debug ("\n");
		}
	EndFor
}
