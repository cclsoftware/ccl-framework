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
// Filename    : ccl/platform/win/gui/documentviewer.win.cpp
// Description : PDF Document Viewer (Win32)
//
//************************************************************************************************

/*
	Adobe Acrobat SDK Help
	http://livedocs.adobe.com/acrobat_sdk/9/Acrobat9_HTMLHelp/wwhelp/wwhimpl/js/html/wwhelp.htm?&accessible=true
	(see "Acrobat Interapplication Communication" -> "DDE Messages")

	Alternatively, see http://partners.adobe.com/public/developer/en/acrobat/PDFOpenParameters.pdf
*/

#include "ccl/gui/help/documentviewer.h"

#include "ccl/base/storage/url.h"
#include "ccl/public/base/buffer.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"

#include "ccl/platform/win/cclwindows.h"

#include <shlwapi.h>
#include <Psapi.h>

#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Version.lib")

namespace CCL {
namespace Win32 {

enum PDFType { kReader, kAcrobat, kOtherApp };

//************************************************************************************************
// SimplePDFViewer
//************************************************************************************************

class SimplePDFViewer: public DocumentViewer
{
public:
	SimplePDFViewer (UrlRef readerPath);

	// DocumentViewer
	tbool CCL_API isInstalled ();
	tbool CCL_API canOpenDocument (UrlRef document) const;
	tbool CCL_API openDocument (UrlRef document, StringRef nameDest);
	tbool CCL_API closeAllDocuments ();

protected:
	Url readerPath;

	String makeOpenParameters (StringRef args, UrlRef document);
	bool execute (StringRef parameters);
};

//************************************************************************************************
// AcrobatPDFViewer
//************************************************************************************************

class AcrobatPDFViewer: public DocumentViewer
{
public:
	AcrobatPDFViewer (UrlRef readerPath, StringRef serverName);

	static bool getServerName (String& serverName, LPCWSTR readerPath, PDFType type);

	// DocumentViewer
	tbool CCL_API isInstalled ();
	tbool CCL_API canOpenDocument (UrlRef document) const;
	tbool CCL_API openDocument (UrlRef document, StringRef nameDest);
	tbool CCL_API closeAllDocuments ();

protected:
	Url readerPath;
	String serverName;
	DWORD processId;
	DWORD instanceId;
	HCONV hConversation;

	bool doAll (StringRef command);

	bool startProcess ();
	bool isProcessStarted ();
	bool beginConversation ();
	bool doTransaction (StringID command);
	bool doTransaction (StringRef command);
	void endConversation ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static HDDEDATA CALLBACK CCLAcrobatCallback (UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2,
											 HDDEDATA hdata, ULONG_PTR dwData1, ULONG_PTR dwData2)
{
	return NULL;
}

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

namespace CCL {

//************************************************************************************************
// WindowsDocumentViewerFactory
//************************************************************************************************

class WindowsDocumentViewerFactory: public DocumentViewerFactory
{
public:
	IDocumentViewer* createPDFViewer ();
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (WindowsDocumentViewerFactory, kFrameworkLevelFirst)
{
	static WindowsDocumentViewerFactory theFactory;
	DocumentViewer::setFactory (&theFactory);
	return true;
}

//************************************************************************************************
// WindowsDocumentViewerFactory
//************************************************************************************************

IDocumentViewer* WindowsDocumentViewerFactory::createPDFViewer ()
{
	WCHAR buffer[MAX_PATH] = {0};
	DWORD charCount = MAX_PATH;
	HRESULT result = E_FAIL;
	PDFType pdfType = kReader;
	String serverName;

	// 1) try Adobe Reader
	result = ::AssocQueryString (ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE, L"AcroRd32.exe", nullptr, buffer, &charCount);

	// 2) try Adobe Acrobat
	if(FAILED (result))
	{
		pdfType = kAcrobat;
		result = ::AssocQueryString (ASSOCF_OPEN_BYEXENAME, ASSOCSTR_EXECUTABLE, L"Acrobat.exe", nullptr, buffer, &charCount);
	}

	// 3) any PDF Application
	if(FAILED (result))
	{
		pdfType = kOtherApp;
		result = ::AssocQueryString (0, ASSOCSTR_EXECUTABLE, L".pdf", L"open", buffer, &charCount);
	}

	Url readerPath;
	if(SUCCEEDED (result))
	{
		readerPath.fromNativePath (buffer);

		// determine DDE server name for Adobe Reader X and above
		if(pdfType != kOtherApp)
			AcrobatPDFViewer::getServerName (serverName, buffer, pdfType);
	}
	else
	{
		CCL_WARN ("Help Viewer not Found", 0)
	}

	if(pdfType != kOtherApp)
		return NEW AcrobatPDFViewer (readerPath, serverName);
	else
		return NEW SimplePDFViewer (readerPath);
}

//************************************************************************************************
// SimplePDFViewer
//************************************************************************************************

SimplePDFViewer::SimplePDFViewer (UrlRef readerPath)
: readerPath (readerPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePDFViewer::isInstalled ()
{
	return !readerPath.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePDFViewer::canOpenDocument (UrlRef document) const
{
	return document.getFileType () == FileTypes::Pdf ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePDFViewer::openDocument (UrlRef document, StringRef nameDest)
{
	if(nameDest.isEmpty () == false)
		return execute (makeOpenParameters (String () << "nameddest=" << nameDest, document));
	else
		return execute (makeOpenParameters (String::kEmpty, document));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePDFViewer::closeAllDocuments ()
{
	// not implemented!
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SimplePDFViewer::makeOpenParameters (StringRef args, UrlRef document)
{
	String parameters;

	// does not work as expected
	//if(!args.isEmpty ())
	//	parameters << "/A \"" << args << "=OpenActions\" ";

	parameters << "\"" << UrlDisplayString (document) << "\"";
	return parameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SimplePDFViewer::execute (StringRef parameters)
{
	IWindow* parent = System::GetDesktop ().getDialogParentWindow ();
	HWND hwnd = parent ? (HWND)parent->getSystemWindow () : 0;

	NativePath nativePath (readerPath);
	HINSTANCE result = ::ShellExecute (hwnd, L"open", nativePath, StringChars (parameters), 0, SW_SHOW);
	return (int64)result > 32;
}

//************************************************************************************************
// AcrobatPDFViewer
//************************************************************************************************

bool AcrobatPDFViewer::getServerName (String& serverName, LPCWSTR readerPath, PDFType type)
{
	serverName = "acroview";

	DWORD unused = 0;
	DWORD dataSize = ::GetFileVersionInfoSize (readerPath, &unused);
	if(dataSize <= 0)
		return false;

	Buffer data;
	data.resize (dataSize);
	::GetFileVersionInfo (readerPath, unused, dataSize, data.getAddress ());

	void* ptr = 0;
	UINT length = 0;
	::VerQueryValue (data, L"\\", &ptr, &length);
	if(!ptr)
		return false;

	VS_FIXEDFILEINFO* fixedFileInfo = (VS_FIXEDFILEINFO*)ptr;
	WORD majorVersion = HIWORD (fixedFileInfo->dwFileVersionMS);
	if(majorVersion >= 10)
	{
		if(type == kReader)
			serverName << "R" << majorVersion;
		else
			serverName << "A" << majorVersion;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AcrobatPDFViewer::AcrobatPDFViewer (UrlRef readerPath, StringRef serverName)
: readerPath (readerPath),
  serverName (serverName),
  processId (0),
  instanceId (0),
  hConversation (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IDocumentViewer methods
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AcrobatPDFViewer::isInstalled ()
{
	return !readerPath.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AcrobatPDFViewer::canOpenDocument (UrlRef document) const
{
	return document.getFileType () == FileTypes::Pdf ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AcrobatPDFViewer::openDocument (UrlRef document, StringRef nameDest)
{
	UrlDisplayString fileName (document);

	String command;
	command << "[DocOpen(\"" << fileName << "\")]";

	bool result = doAll (command);

	if(nameDest.isEmpty () == false)
	{
		command.empty ();
		command << "[DocGoToNameDest(\"" << fileName << "\",\"" << nameDest << "\")]";
		result |= doAll (command);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AcrobatPDFViewer::closeAllDocuments ()
{
	if(!isProcessStarted ())
		return true;

	bool result = doAll ("[CloseAllDocs()]");
	#if (0 && DEBUG)
	::Sleep (1000);
	#endif
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Internal Methods
//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::doAll (StringRef command)
{
	if(!isProcessStarted ())
	{
		if(!startProcess ())
			return false;
	}

	if(!beginConversation ())
	{
		endConversation (); // cleanup
		return false;
	}

	doTransaction (command);

	endConversation ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::startProcess ()
{
	ASSERT (processId == 0)

	STARTUPINFO startupInfo = {0};
	startupInfo.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION processInfo = {0};

	NativePath nativePath (readerPath);
	BOOL result = ::CreateProcess (nativePath, nullptr, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupInfo, &processInfo);
	if(result)
	{
		processId = processInfo.dwProcessId;

		::CloseHandle (processInfo.hProcess);
		::CloseHandle (processInfo.hThread);
	}
	return result != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::isProcessStarted ()
{
	if(processId == 0)
		return false;

	bool started = false;
	HANDLE hProcess = ::OpenProcess (PROCESS_QUERY_INFORMATION/*|PROCESS_VM_READ*/, FALSE, processId);
	if(hProcess)
	{
		WCHAR name[MAX_PATH] = {};
		::GetModuleFileNameEx (hProcess, NULL, name, MAX_PATH);

		Url path;
		path.fromNativePath (name);
		started = path.isEqualUrl (readerPath) != 0;

		::CloseHandle (hProcess);
	}
	#if DEBUG
	else
	{
		int error = ::GetLastError ();
		error = error;
	}
	#endif

	if(started == false)
		processId = 0; // id is used by other process (user closed our instance)

	return started;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::beginConversation ()
{
	ASSERT (instanceId == 0)
	ASSERT (hConversation == 0)

	UINT result = ::DdeInitialize (&instanceId, CCLAcrobatCallback, APPCMD_CLIENTONLY, 0);
	if(result != DMLERR_NO_ERROR)
		return false;

	HSZ serverName = ::DdeCreateStringHandle (instanceId, StringChars (this->serverName), 0);
	HSZ topicName = ::DdeCreateStringHandle (instanceId, L"control", 0);

	const DWORD kMaxTimeout = 3000;
	const DWORD kSleepInterval = 200;

	DWORD timeout = 0;
	do
	{
		hConversation = ::DdeConnect (instanceId, serverName, topicName, nullptr);
		if(hConversation || (timeout >= kMaxTimeout))
			break;

		::Sleep (kSleepInterval);
		timeout += kSleepInterval;
	} while(true);

	::DdeFreeStringHandle (instanceId, serverName);
	::DdeFreeStringHandle (instanceId, topicName);

	return hConversation != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AcrobatPDFViewer::endConversation ()
{
	if(hConversation)
	{
		BOOL result = ::DdeDisconnect (hConversation);
		ASSERT (result == TRUE)
		hConversation = 0;
	}

	if(instanceId)
	{
		BOOL result = ::DdeUninitialize (instanceId);
		ASSERT (result == TRUE)
		instanceId = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::doTransaction (StringID command)
{
	ASSERT (hConversation != 0)

	const char* data = command.str ();
	DWORD size = command.length () + 1;

	DWORD result = 0;
	::DdeClientTransaction ((BYTE*)data, size, hConversation, NULL, CF_TEXT, XTYP_EXECUTE, 1000, &result);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AcrobatPDFViewer::doTransaction (StringRef command)
{
	ASSERT (hConversation != 0)

	StringChars chars (command);
	const uchar* data = chars;
	DWORD size = (command.length () + 1) * 2;

	DWORD result = 0;
	::DdeClientTransaction ((BYTE*)data, size, hConversation, NULL, CF_UNICODETEXT, XTYP_EXECUTE, 1000, &result);

	return true;
}
