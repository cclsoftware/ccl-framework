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
// Filename    : ccl/base/storage/logfile.cpp
// Description : Log File
//
//************************************************************************************************

#include "ccl/base/storage/logfile.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// LogEvent
//************************************************************************************************

DEFINE_CLASS (LogEvent, Object)
DEFINE_CLASS_NAMESPACE (LogEvent, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

LogEvent::LogEvent (String message, Alert::AlertType type)
: Alert::Event (message, type)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LogEvent::LogEvent (const Alert::Event& e)
: Alert::Event (e)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LogEvent::load (const Storage& storage)
{
	CCL_NOT_IMPL ("LogEvent::load() not implemented!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LogEvent::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	if(time != DateTime ())
		a.set ("time", Format::PortableDateTime::print (time));

	a.set ("type", type);
	a.set ("message", message);

	if(!moduleName.isEmpty ())
		a.set ("module", moduleName);
	if(!fileName.isEmpty ())
		a.set ("file", fileName);
	if(lineNumber != 0)
		a.set ("line", lineNumber);
	return true;
}

//************************************************************************************************
// LogEventList
//************************************************************************************************

DEFINE_CLASS (LogEventList, StorableObject)
DEFINE_CLASS_NAMESPACE (LogEventList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

LogEventList::LogEventList ()
{
	events.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LogEventList::load (const Storage& storage)
{
	CCL_NOT_IMPL ("LogEventList::load() not implemented!")
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool LogEventList::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.queue (nullptr, events);
	return true;
}

//************************************************************************************************
// LogFile
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LogFile, TextFile)

//////////////////////////////////////////////////////////////////////////////////////////////////

const int LogFile::kDefaultEventFormat = Alert::Event::kWithTime|Alert::Event::kWithAlertType|Alert::Event::kWithModule;

//////////////////////////////////////////////////////////////////////////////////////////////////

LogFile::LogFile ()
: lowLevelEventsOnly (true),
  eventFormat (kDefaultEventFormat)
{
	String fileName;
	Url executablePath;
	if(System::GetExecutableLoader ().getMainImage ().getPath (executablePath))
		executablePath.getName (fileName, false);
	else
		fileName = "app";
	fileName.append (".log");

	Url logFilePath;
	System::GetSystem ().getLocation (logFilePath, System::kAppSettingsFolder);
	logFilePath.descend (fileName);

	create (logFilePath, Text::kUTF8, Text::kSystemLineFormat, ITextStreamer::kFlushLineEnd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LogFile::LogFile (StringRef fileName, bool platformSpecific)
: lowLevelEventsOnly (true),
  eventFormat (kDefaultEventFormat)
{
	Url logFilePath;
	System::GetSystem ().getLocation (logFilePath, 
									  platformSpecific ? System::kAppSettingsPlatformFolder :
									  System::kAppSettingsFolder);
	logFilePath.descend (fileName);

	create (logFilePath, Text::kUTF8, Text::kSystemLineFormat, ITextStreamer::kFlushLineEnd);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LogFile::LogFile (UrlRef path)
: TextFile (path),
  lowLevelEventsOnly (true),
  eventFormat (kDefaultEventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LogFile::reportEvent (const Alert::Event& e)
{
	if(isLowLevelEventsOnly () && !e.isLowLevel ()) // log only events caused by CCL_WARN
		return;

	Threading::ScopedLock scopedLock (lock);
	if(streamer == nullptr)
		return;

	String line (e.format (eventFormat));
	streamer->writeString (line, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LogFile::setReportOptions (Severity minSeverity, int eventFormat)
{}

//************************************************************************************************
// LogBuffer
//************************************************************************************************

LogBuffer::LogBuffer (int maxEntries)
: maxEntries (maxEntries),
  eventFormat (Alert::Event::kWithAlertType|Alert::Event::kWithModule)
{
	entries.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LogBuffer::print (CStringRef text)
{
	Entry* entry = nullptr;
	if(entries.count () >= maxEntries)
		if((entry = (Entry*)entries.removeFirst ())) // remove oldest and reuse it
			entry->empty ();
	
	if(!entry)
		entry = NEW Entry;

	DateTime time;
	System::GetSystem ().getLocalTime (time);

	entry->append (Format::DateTime::print (time, Format::DateTime::kTime));
	entry->append (" ");
	entry->append (text);
	entries.add (entry);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LogBuffer::reportEvent (const Alert::Event& e)
{
	Threading::ScopedLock scopedLock (lock);
	print (e.format (eventFormat));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LogBuffer::setReportOptions (Severity _minSeverity, int _eventFormat)
{
	// minSeverity: not yet supported

	eventFormat = _eventFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LogBuffer::dump (IStream& stream) const
{
	const char newLine = '\n';
	stream.write (title, title.length ());
	stream.write (&newLine, 1);

	ListForEachLinkableFast (entries, Entry, e)
		stream.write (e->str (), e->length ());
		stream.write (&newLine, 1);
	EndFor
}
