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
// Filename    : ccl/system/localization/localeinfo.h
// Description : Locale Info
//
//************************************************************************************************

#ifndef _ccl_localeinfo_h
#define _ccl_localeinfo_h

#include "ccl/public/system/ilocaleinfo.h"

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringdictionary.h"

namespace CCL {

//************************************************************************************************
// LocaleInfoBase
//************************************************************************************************

class LocaleInfoBase: public StorableObject,
					  public ILocaleInfo
{
public:
	DECLARE_CLASS (LocaleInfoBase, StorableObject)

	// ILocaleInfo
	StringRef CCL_API getTitle () const override;
	int CCL_API getDayOfWeek (const Date& date) const override;
	StringRef CCL_API getWeekdayName (int dayOfWeek) const override;
	StringRef CCL_API getMonthName (int month) const override;
	tresult CCL_API printDate (String& result, const Date& date, int flags = 0) const override;
	tresult CCL_API printTime (String& result, const Time& time, int flags = 0) const override;
	tresult CCL_API printTimeAgo (String& result, const DateTime& localTime, int flags = 0) const override;
	tresult CCL_API printTimeAhead (String& result, const DateTime& localTime, int flags = 0) const override;
	tresult CCL_API printDuration (String& result, double seconds, int flags = 0) const override;
	tresult CCL_API printCurrency (String& result, double value, int flags = 0) const override;
	tresult CCL_API printByteSize (String& result, double byteSize, int flags = 0) const override;
	tresult CCL_API scanDuration (double& seconds, StringRef string, int defaultUnit = kSeconds, int flags = 0) const override;
	tresult CCL_API printBytesPerSecond (String& result, double bytesPerSecond, int flags = 0) const override;

	CLASS_INTERFACE (ILocaleInfo, StorableObject)
};

//************************************************************************************************
// LocaleInfo
//************************************************************************************************

class LocaleInfo: public LocaleInfoBase
{
public:
	DECLARE_CLASS (LocaleInfo, LocaleInfoBase)

	bool isValid () const;
	PROPERTY_MUTABLE_CSTRING (language, Language)

	PROPERTY_STRING (dateFormat, DateFormat)
	PROPERTY_STRING (timeFormat, TimeFormat)
	PROPERTY_STRING (friendlyDateFormat, FriendlyDateFormat)

	// LocaleInfoBase
	StringRef CCL_API getTitle () const override;
	StringRef CCL_API getWeekdayName (int dayOfWeek) const override;
	StringRef CCL_API getMonthName (int month) const override;
	tresult CCL_API printDate (String& result, const Date& date, int flags = 0) const override;
	tresult CCL_API printTime (String& result, const Time& time, int flags = 0) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	String title;
	StringDictionary weekdays;
	StringDictionary months;

	static const String weekdayID[7];
	static const String monthID[12];
};

//************************************************************************************************
// LocaleInfoList
//************************************************************************************************

class LocaleInfoList: public StorableObject
{
public:
	DECLARE_CLASS (LocaleInfoList, Object)

	LocaleInfoList ();

	const ObjectList& getLocales () const;

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectList locales;
};

//************************************************************************************************
// GeographicRegion
//************************************************************************************************

class GeographicRegion: public Object,
						public IGeographicRegion
{
public:
	DECLARE_CLASS (GeographicRegion, Object)

	String englishName;
	String nativeName;
	String localizedName;
	String iso2Code;

	// IGeographicRegion
	StringRef CCL_API getEnglishName () const override;
	StringRef CCL_API getNativeName () const override;
	StringRef CCL_API getLocalizedName () const override;
	StringRef CCL_API getISO2Code () const override;

	// Object
	int compare (const Object& obj) const override;

	CLASS_INTERFACE (IGeographicRegion, Object)
};

//************************************************************************************************
// GeographicRegionList
//************************************************************************************************

class GeographicRegionList: public ObjectArray
{
public:
	GeographicRegionList ();

	bool containsRegion (StringRef englishName) const;
	bool containsRegionISO (StringRef iso2) const;
};

} // namespace CCL

#endif // _ccl_localeinfo_h
