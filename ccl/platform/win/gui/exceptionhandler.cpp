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
// Filename    : ccl/platform/win/gui/exceptionhandler.cpp
// Description : Win32 Exception Handler
//
//************************************************************************************************

#define USE_TIMESTAMP	1
#define USE_CRASHSIGNAL	1

#include "ccl/platform/win/gui/exceptionhandler.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/logfile.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ierrorhandler.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include <Dbghelp.h>

#pragma comment (lib, "Dbghelp.lib")

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Win32")
	XSTRING (UnhandledException, "An unexpected error occurred in this application or in a plug-in.\nA problem report has been created at:\n\n%(1)")
END_XSTRINGS

//************************************************************************************************
// ExceptionHandler
//************************************************************************************************

static void makeTimestamp (uchar* ptr)
{
	DateTime now;
	System::GetSystem ().getLocalTime (now);
	const Date& date = now.getDate ();
	const Time& time = now.getTime ();

	::swprintf (ptr, 100, L"_%04d%02d%02d_%02d%02d%02d%03d.dmp",
				date.getYear (), date.getMonth (), date.getDay (),
				time.getHour (), time.getMinute (), time.getSecond (), time.getMilliseconds ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_UNMANAGED_SINGLETON (ExceptionHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExceptionHandler* ExceptionHandler::theInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

LONG WINAPI ExceptionHandler::handleGUIException (LPEXCEPTION_POINTERS exceptionInfo)
{
	handleException (exceptionInfo);

	if(::GetCurrentThreadId () == theInstance->mainThreadId)
		Alert::error (theInstance->messageText);

	return EXCEPTION_EXECUTE_HANDLER;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LONG WINAPI ExceptionHandler::handleException (LPEXCEPTION_POINTERS exceptionInfo)
{
	theInstance->createMiniDump (exceptionInfo);

	System::GetSafetyManager ().reportException (exceptionInfo, theInstance->nativeDumpPath);

	if(theInstance->previousFilter)
		return theInstance->previousFilter (exceptionInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExceptionHandler::test ()
{
	TRY_MESSAGE
	{
		int div = 0;
		int i = 100 / div;
		i++;
	}
	EXCEPT_MESSAGE
	{}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExceptionHandler::ExceptionHandler ()
: timestampPtr (nullptr),
  previousFilter (nullptr),
  signalSource (Signals::kErrorHandler),
  logBuffer (*NEW LogBuffer),
  mainThreadId (0),
  savedDumpsValid (false)
{
	ASSERT (theInstance == nullptr)
	theInstance = this;

	logBuffer.setTitle ("Previous Crashes:");

#if USE_CRASHSIGNAL
	signalSource.signal (Message (kChanged)); // force atom creation

	static const int kReportBufferSize = 100 * 1024; // 100 KB
	appStream.allocateMemory (kReportBufferSize, true);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExceptionHandler::~ExceptionHandler ()
{
	ASSERT (theInstance == this)
	theInstance = nullptr;

	delete &logBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExceptionHandler::install ()
{
	mainThreadId = ::GetCurrentThreadId ();

	ASSERT (previousFilter == nullptr)
	previousFilter = ::SetUnhandledExceptionFilter (handleException);
	//::AddVectoredExceptionHandler (1, handleException);
	
	//catchExceptionsInTimers (true);

	const IExecutableImage& mainImage = System::GetExecutableLoader ().getMainImage ();

	String version;
	String vendor;
	const IAttributeList* metaInfo = mainImage.getMetaInfo ();
	if(metaInfo)
	{
		PackageInfo packageInfo (*metaInfo);
		packageInfo.toXml (xmlStream);
		version = packageInfo.getString (Meta::kPackageVersion);
		vendor = packageInfo.getString (Meta::kPackageVendor);
	}

	Url executablePath;
	mainImage.getPath (executablePath);
	executablePath.getName (dumpFilePrefix, false);
	if(!version.isEmpty ())
	{
		version.replace (CCLSTR ("."), CCLSTR ("_"));
		dumpFilePrefix << "_" << version;
	}

	dumpFilePrefix << "_" << CCL_PLATFORM_STRING;
	
	String fileName (dumpFilePrefix);

	System::GetSystem ().getLocation (miniDumpPath, System::kUserSettingsFolder);

	if(!vendor.isEmpty ())
		miniDumpPath.descend (LegalFileName (vendor), IUrl::kFolder);

	messageText.appendFormat (XSTR (UnhandledException), UrlDisplayString (miniDumpPath));

#if USE_TIMESTAMP
	uchar timestamp[100];
	makeTimestamp (timestamp);
	fileName.append (timestamp);
#else
	fileName.append (".dmp");
#endif

	miniDumpPath.descend (fileName);

	miniDumpPath.toNativePath (nativeDumpPath, nativeDumpPath.size ());

#if USE_TIMESTAMP
	timestampPtr = nativeDumpPath.path + String (nativeDumpPath).index (timestamp);
#endif

	//test ();
	//test ();
	//test ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExceptionHandler::uninstall ()
{
	if(previousFilter)
	{
		::SetUnhandledExceptionFilter (previousFilter);
		//::RemoveVectoredExceptionHandler (handleException);
	}
	previousFilter = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExceptionHandler::createMiniDump (LPEXCEPTION_POINTERS exceptionInfo)
{
	DWORD threadId = ::GetCurrentThreadId ();
	bool mainCrashed = threadId == mainThreadId;

#if USE_TIMESTAMP
	// TODO: Modifying the time stamp is not thread-safe, but we get a file name anyway.
	makeTimestamp (timestampPtr);
#endif

	HANDLE hFile = ::CreateFile (nativeDumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(hFile == INVALID_HANDLE_VALUE)
		return false;

#if USE_CRASHSIGNAL
	if(mainCrashed)
	{
		appStream.seek (0, IStream::kSeekSet);
		appStream.setBytesWritten (0);
		signalSource.signal (Message (Signals::kCrashReported, static_cast<IStream*> (&appStream)));

		#if USE_TIMESTAMP
		if(!logBuffer.isEmpty ())
			logBuffer.dump (appStream);

		// add path of logfile, will be written to next log
		logBuffer.print (String (nativeDumpPath));
		#endif
	}
#endif

	HANDLE hProcess = ::GetCurrentProcess ();
	DWORD processId = ::GetCurrentProcessId ();

	MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};
	dumpInfo.ThreadId = threadId;
	dumpInfo.ExceptionPointers = exceptionInfo;
	dumpInfo.ClientPointers = TRUE;

	MINIDUMP_USER_STREAM userStreams[10] = {0};
	userStreams[0].Type = CommentStreamA; // UTF8???
	userStreams[0].BufferSize = xmlStream.getBytesWritten ();
	userStreams[0].Buffer = xmlStream.getMemoryAddress ();

	MINIDUMP_USER_STREAM_INFORMATION userInfo = {0};
	userInfo.UserStreamArray = userStreams;
	userInfo.UserStreamCount = 1;

#if USE_CRASHSIGNAL
	if(mainCrashed && appStream.getBytesWritten () > 0)
	{
		userStreams[1].Type = CommentStreamA; // UTF8???
		userStreams[1].BufferSize = appStream.getBytesWritten ();
		userStreams[1].Buffer = appStream.getMemoryAddress ();
		userInfo.UserStreamCount++;
	}
#endif

	BOOL result = ::MiniDumpWriteDump (hProcess, processId, hFile, MiniDumpNormal, &dumpInfo, &userInfo, nullptr);

	::CloseHandle (hFile);

	savedDumpsValid = false;

	return result != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExceptionHandler::catchExceptionsInTimers (bool catchExceptions)
{
	// By default, Windows will enclose its calls to TimerProc with an exception handler that consumes and discards all exceptions.

	BOOL swallowExceptions = !catchExceptions;
	::SetUserObjectInformation (::GetCurrentProcess (), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &swallowExceptions, sizeof(swallowExceptions));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ExceptionHandler::scanDumps ()
{
	if(savedDumpsValid)
		return;

	savedDumps.removeAll ();

	Url dumpFolder (miniDumpPath);
	dumpFolder.ascend ();
	
	FileInfo info;
	int64 now = UnixTime::getTime ();
	ForEachFile (System::GetFileSystem ().newIterator (dumpFolder, IFileIterator::kFiles), file)
		System::GetFileSystem ().getFileInfo (info, *file);
		String fileName;
		file->getName (fileName);
		if(!fileName.startsWith (dumpFilePrefix))
			continue;
		if(UnixTime::fromLocal (info.modifiedTime) > now - 7 * DateTime::kSecondsInDay)
			savedDumps.add (*file);
		if(savedDumps.count () >= 10)
			break;
	EndFor

	savedDumpsValid = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ExceptionHandler::countDiagnosticData () const
{
	ExceptionHandler* This = const_cast<ExceptionHandler*> (this);
	This->scanDumps ();

	return savedDumps.count ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExceptionHandler::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index >= 0 && index < savedDumps.count ())
	{
		description.categoryFlags = DiagnosticDescription::kErrorInformation;
		savedDumps[index].getName (description.fileName);
		description.subFolder = "Dumps";
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* CCL_API ExceptionHandler::createDiagnosticData (int index)
{
	if(index >= 0 && index < savedDumps.count ())
		return System::GetFileSystem ().openStream (savedDumps[index]);
	return nullptr;
}
