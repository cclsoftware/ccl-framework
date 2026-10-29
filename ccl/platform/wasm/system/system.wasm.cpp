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
// Filename    : ccl/platform/wasm/system/system.wasm.cpp
// Description : WebAssembly CCL System implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/system.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/platform/shared/posix/system/system.posix.h"

#include <uuid/uuid.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include <emscripten/heap.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (double, getHeapSizeLimit, (),
{
	if(typeof performance !== 'undefined' && performance.memory)
		return performance.memory.jsHeapSizeLimit;
	return 0;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (double, getUsedHeapSize, (),
{
	if(typeof performance !== 'undefined' && performance.memory)
		return performance.memory.usedJSHeapSize;
	return 0;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (int, getHardwareConcurrency, (),
{
	if(typeof navigator !== 'undefined' && navigator.hardwareConcurrency)
		return navigator.hardwareConcurrency;
	return 1;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, getBrowserName, (char* cString, int cStringSize),
{
	let name = "";

	const uaData = navigator.userAgentData;
	const brands = uaData ? uaData.brands : null;
	if(Array.isArray (brands))
	{
		const brand = brands.find (b => !/not.*brand/i.test (b.brand));
		if(brand)
			name = brand.brand;
	}

	if(!name)
	{
		const userAgent = navigator.userAgent || "WebAssembly";
		name = userAgent.split (' ')[0];
	}

	if(uaData && uaData.platform)
		name += " on " + uaData.platform;

	stringToUTF8 (name, cString, cStringSize);
	return true;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, getUserAgentName, (char* cString, int cStringSize),
{
	const userAgent = navigator.userAgent || "WebAssembly";
	stringToUTF8 (userAgent, cString, cStringSize);
	return true;
});

} // namespace Wasm

//************************************************************************************************
// WasmSystemInformation
//************************************************************************************************

class WasmSystemInformation: public SystemInformation
{
public:
	WasmSystemInformation ();

	// ISystemInformation
	void CCL_API getLocalTime (DateTime& dateTime) const override;
	void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const override;
	void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const override;
	void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const override;
	int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const override;
	int64 CCL_API getSecureComputerTime () const override;
	void CCL_API getComputerName (String& name, int flags = 0) const override;
	void CCL_API getUserName (String& name, int flags = 0) const override;
	int CCL_API getNumberOfCPUs () const override;
	int CCL_API getNumberOfCores () const override;
	void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const override;
	void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const override;
	IAsyncOperation* CCL_API searchApplications (StringRef filter) const override;
	void CCL_API terminate () override;
	tbool CCL_API isProcessSandboxed () const override;

protected:
	String getDeviceIdentifier () const;
	
	// SystemInformation
	bool getNativeLocation (IUrl& url, System::FolderType type) const override;
};

//************************************************************************************************
// WasmImage
//************************************************************************************************

class WasmImage: public ExecutableImage
{
public:
	WasmImage (ModuleRef nativeRef, bool isLoaded);
	~WasmImage ();

	// ExecutableImage
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	tbool CCL_API getPath (IUrl& url) const override;

private:
	void unload ();
};

//************************************************************************************************
// WasmExecutableLoader
//************************************************************************************************

class WasmExecutableLoader: public ExecutableLoader
{
public:
	// IExecutableLoader
	tresult CCL_API loadImage (IExecutableImage*& image, UrlRef path) override;
	IExecutableImage* CCL_API createImage (ModuleRef module) override;
	tresult CCL_API execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
							 int flags = 0, IUnknown* context = nullptr) override;
	tresult CCL_API relaunch (ArgsRef args) override;
	tresult CCL_API terminate (Threading::ProcessID processId) override;
	tresult CCL_API getExecutablePath (IUrl& path, Threading::ProcessID processId) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetMainModuleRef) ()
{
	ModuleRef mainModule = ::dlopen (nullptr, RTLD_LAZY | RTLD_NOLOAD);
	return mainModule;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (LockMemory) (tbool state, void* address, int size)
{
	if(state)
		::mlock (address, size);
	else
		::munlock (address, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API System::CCL_ISOLATED (CreateUID) (UIDBytes& uid)
{
	uuid_t buffer;
	uuid_generate (buffer);
	
	UID tmp;
	tmp.fromBuffer (buffer);
	
	uid = tmp;
	
	return kResultOk;
}

//************************************************************************************************
// LinuxSystemInformation
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemInformation, WasmSystemInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmSystemInformation::WasmSystemInformation ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::terminate ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmSystemInformation::isProcessSandboxed () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmSystemInformation::getNativeLocation (IUrl& path, System::FolderType folderType) const
{
	bool succeeded = false;
	
	switch(folderType)
	{
	case System::kSystemFolder : CCL_FALLTHROUGH
	case System::kAppSupportFolder :
		path.fromPOSIXPath ("/", Url::kFolder);
		succeeded = true;
		break;

    case System::kAppPluginsFolder :
		succeeded = getNativeLocation (path, System::kAppSupportFolder);
		if(succeeded)
			path.descend (CCLSTR ("Plugins"), Url::kFolder);
		break;

	case System::kTempFolder :
		path.fromPOSIXPath ("/tmp", Url::kFolder);
		succeeded = true;
		break;

	case System::kSharedDataFolder : CCL_FALLTHROUGH
	case System::kSharedSettingsFolder : CCL_FALLTHROUGH
	case System::kSharedSupportFolder : CCL_FALLTHROUGH
	case System::kUserAppSupportFolder :
		path.fromPOSIXPath ("/userdata", Url::kFolder);
		succeeded = true;
		break;

	case System::kUserSettingsFolder : CCL_FALLTHROUGH
	case System::kUserPreferencesFolder :
		succeeded = getNativeLocation (path, System::kUserAppSupportFolder);
		if(succeeded)
			path.descend ("Settings", IUrl::kFolder);
		break;

	case System::kUserDownloadsFolder :
		succeeded = getNativeLocation (path, System::kUserAppSupportFolder);
		if(succeeded)
			path.descend ("Downloads", IUrl::kFolder);
		break;

	case System::kUserDocumentFolder :
		succeeded = getNativeLocation (path, System::kUserAppSupportFolder);
		if(succeeded)
			path.descend ("Documents", IUrl::kFolder);
		break;
	}

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::getLocalTime (DateTime& dateTime) const
{
	PosixTimeConversion::getLocalTime (dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	PosixTimeConversion::convertLocalTimeToUTC (utc, localTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	PosixTimeConversion::convertUTCToLocalTime (localTime, utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	PosixTimeConversion::convertUnixTimeToUTC (utc, unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WasmSystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	return PosixTimeConversion::convertUTCToUnixTime (utc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WasmSystemInformation::getSecureComputerTime () const
{
	return ::time (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::getComputerName (String& name, int flags) const
{
	char buffer[CORE_STRING_STACK_MAX] = "";
	if(getBrowserName (buffer, ARRAY_COUNT (buffer)))
		name.appendCString (Text::kUTF8, buffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::getUserName (String& name, int flags) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WasmSystemInformation::getNumberOfCPUs () const
{
	return getNumberOfCores ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WasmSystemInformation::getNumberOfCores () const
{
	return getHardwareConcurrency ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::getMemoryInfo (System::MemoryInfo& memoryInfo) const
{
	uint64 heapLimit = uint64(getHeapSizeLimit ());
	if(heapLimit > 0)
	{
		memoryInfo.physicalRAMSize = heapLimit;
		memoryInfo.processMemoryTotal = heapLimit;

		uint64 usedHeap = uint64(getUsedHeapSize ());
		if(usedHeap > 0 && usedHeap <= heapLimit)
			memoryInfo.processMemoryAvailable = heapLimit - usedHeap;
		else
			memoryInfo.processMemoryAvailable = heapLimit;
	}
	else
	{
		uint64 wasmMemorySize = uint64(emscripten_get_heap_max ());
		memoryInfo.physicalRAMSize = wasmMemorySize;
		memoryInfo.processMemoryTotal = wasmMemorySize;
		memoryInfo.processMemoryAvailable = wasmMemorySize;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmSystemInformation::getComputerInfo (IAttributeList& attributes, int flags) const
{
	char buffer[CORE_STRING_STACK_MAX] = "";
	if(getUserAgentName (buffer, ARRAY_COUNT (buffer)))
		attributes.setAttribute (System::kDeviceModel, buffer);
	attributes.setAttribute (System::kDeviceIdentifier, getDeviceIdentifier ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String WasmSystemInformation::getDeviceIdentifier () const
{
	Url path;
	if(!getNativeLocation (path, System::kUserSettingsFolder))
		return String::kEmpty;
	path.descend ("deviceId", IUrl::kFile);

	String deviceId = TextUtils::loadRawString (path);
	if(deviceId.isEmpty ())
	{
		UID uid;
		uid.generate ();
		uid.toString (deviceId);

		TextFile file (path);
		if(file.isValid ())
			file->writeString (deviceId);
	}
	return deviceId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API WasmSystemInformation::searchApplications (StringRef filter) const
{
	return nullptr;
}

//************************************************************************************************
// WasmExecutableLoader
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (ExecutableLoader, WasmExecutableLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmExecutableLoader::loadImage (IExecutableImage*& image, UrlRef path)
{
	MutableCString systemPath (UrlDisplayString (path), Text::kSystemEncoding);
	void* handle = ::dlopen (systemPath.str (), RTLD_NOW);
	if(handle)
	{
		WasmImage* lib = NEW WasmImage (handle, true);
		image = lib;
		return kResultOk;
	}
	else
		CCL_WARN ("Module could not be loaded: %s\n", ::dlerror ());

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* CCL_API WasmExecutableLoader::createImage (ModuleRef module)
{
	return NEW WasmImage (module, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmExecutableLoader::execute (Threading::ProcessID& processId, UrlRef path, ArgsRef args,
												  int flags, IUnknown* context)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmExecutableLoader::relaunch (ArgsRef args)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmExecutableLoader::terminate (Threading::ProcessID processId)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmExecutableLoader::getExecutablePath (IUrl& path, Threading::ProcessID processId)
{
	return kResultNotImplemented;
}

//************************************************************************************************
// WasmImage
//************************************************************************************************

WasmImage::WasmImage (ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmImage::~WasmImage ()
{
	if(isLoaded && nativeRef)
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmImage::unload ()
{
	if(nativeRef)
		::dlclose (nativeRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API WasmImage::getFunctionPointer (CStringPtr name) const
{
	void* result = nullptr;
	if(nativeRef)
	{
		result = ::dlsym (nativeRef, name);
		if(!result)
			CCL_PRINTF ("Function pointer not found in library: %s\n", ::dlerror ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmImage::getPath (IUrl& url) const
{
	return false;
}
