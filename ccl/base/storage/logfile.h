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
// Filename    : ccl/base/storage/logfile.h
// Description : Log File
//
//************************************************************************************************

#ifndef _ccl_logfile_h
#define _ccl_logfile_h

#include "ccl/base/storage/textfile.h"

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/linkablelist.h"

#include "ccl/public/system/alerttypes.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// LogEvent
//************************************************************************************************

class LogEvent: public Object,
				public Alert::Event
{
public:
	DECLARE_CLASS (LogEvent, Object)

	LogEvent (String message = nullptr, Alert::AlertType type = Alert::kInformation);
	LogEvent (const Alert::Event& e);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// LogEventList
//************************************************************************************************

class LogEventList: public StorableObject
{
public:
	DECLARE_CLASS (LogEventList, StorableObject)

	LogEventList ();

	ObjectArray& getEvents () { return events; }
	const ObjectArray& getEvents () const { return events; }

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray events;
};

//************************************************************************************************
// LogFile
//************************************************************************************************

class LogFile: public TextFile,
			   public Alert::IReporter
{
public:
	DECLARE_CLASS (LogFile, TextFile)
	
	LogFile ();
	LogFile (UrlRef path);
	LogFile (StringRef fileName, bool platformSpecific);

	PROPERTY_BOOL (lowLevelEventsOnly, LowLevelEventsOnly)

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;
	
	CLASS_INTERFACE (IReporter, Object)

protected:
	static const int kDefaultEventFormat;

	Threading::CriticalSection lock;
	int eventFormat;
};

//************************************************************************************************
// LogBuffer
// Buffer with a limited number of c-string entries
//************************************************************************************************

class LogBuffer: public Unknown,
				 public Alert::IReporter
{
public:
	LogBuffer (int maxEntries = 30);

	PROPERTY_MUTABLE_CSTRING (title, Title)

	void print (StringRef text);
	void print (CStringRef text);

	void dump (IStream& stream) const;
	bool isEmpty () const;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity _minSeverity, int _eventFormat) override;

	CLASS_INTERFACE (IReporter, Unknown)

private:
	Threading::CriticalSection lock;
	LinkableList entries;
	int maxEntries;
	int eventFormat;

	struct Entry: public Linkable, public MutableCString {};
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void LogBuffer::print (StringRef text)
{ print (MutableCString (text, Text::kUTF8)); }

inline bool LogBuffer::isEmpty () const
{ return entries.isEmpty (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_logfile_h
