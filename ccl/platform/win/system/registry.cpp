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
// Filename    : ccl/platform/win/system/registry.cpp
// Description : Windows Registry Access
//
//************************************************************************************************

#include "ccl/platform/win/system/registry.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
namespace Registry {

//////////////////////////////////////////////////////////////////////////////////////////////////

static HKEY theRootKeys[kNumRootKeys] =
{
	HKEY_CLASSES_ROOT,	// kKeyClassesRoot
	HKEY_CURRENT_USER,	// kKeyCurrentUser
	HKEY_LOCAL_MACHINE	// kKeyLocalMachine
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static REGSAM theAccessFlags[kNumAccessModes] =
{
	0,	
	KEY_WOW64_64KEY,	// kAccess64Bit
	KEY_WOW64_32KEY	    // kAccess32Bit
};

static const String strBackslash = CCLSTR ("\\");

//////////////////////////////////////////////////////////////////////////////////////////////////

String AppValueName ()
{
	Url path;
	System::GetExecutableLoader ().getMainImage ().getPath (path);
	return UrlDisplayString (path).toLowercase ();
}

//************************************************************************************************
// Accessor
//************************************************************************************************

Accessor::Accessor (RegKey rootKey, StringRef _basePath, RegAccess _access)
: rootKey (rootKey),
  basePath (_basePath),
  access (_access)
{
	if(!basePath.isEmpty () && basePath.lastChar () != '\\')
		basePath.append (strBackslash);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& Accessor::makeFullPath (String& dst, StringRef subPath) const
{
	dst.empty ();
	dst.append (basePath);
	if(!subPath.isEmpty ())
	{
		dst.append (subPath);
		dst.append (strBackslash); // do we need this?
	}
	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyIterator* Accessor::newKeyIterator (StringRef subPath) const
{
	String path;
	makeFullPath (path, subPath);

	HKEY hKey = nullptr;
	
	if(::RegOpenKeyExW (theRootKeys[rootKey], StringChars (path), 0, KEY_ENUMERATE_SUB_KEYS | theAccessFlags [access], &hKey) == ERROR_SUCCESS)	
		return NEW KeyIterator (hKey);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Accessor::readString (String& string, StringRef subPath, StringRef name) const
{
	String path;
	makeFullPath (path, subPath);

	HKEY hKey = nullptr;
	if(::RegOpenKeyExW (theRootKeys[rootKey], StringChars (path), 0, KEY_QUERY_VALUE | theAccessFlags [access], &hKey) == ERROR_SUCCESS)
	{
		DWORD type = 0;
		uchar buffer[4096];
		DWORD size = 4096 * sizeof(uchar);

		LONG result = ::RegQueryValueExW (hKey, StringChars (name), nullptr, &type, (LPBYTE)buffer, &size);
		::RegCloseKey (hKey);

		if(result == ERROR_SUCCESS && type == REG_SZ)
		{
			string.assign (buffer);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Accessor::writeString (StringRef string, StringRef subPath, StringRef name)
{
	String path;
	makeFullPath (path, subPath);

	HKEY hKey = nullptr;
	if(::RegCreateKeyExW (theRootKeys[rootKey], StringChars (path), 0, nullptr/*class*/, 0/*options*/, 
						 KEY_ALL_ACCESS | theAccessFlags [access], nullptr/*security*/, &hKey, nullptr/*disposition*/) == ERROR_SUCCESS)
	{
		StringChars chars (string);
		const BYTE* data = (const BYTE*)(const uchar*)chars;
		DWORD size = (string.length () + 1) * sizeof(uchar);

		LONG result = ::RegSetValueExW (hKey, StringChars (name), 0, REG_SZ, data, size);
		::RegCloseKey (hKey);

		if(result == ERROR_SUCCESS)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Accessor::readDWORD (uint32& value, StringRef subPath, StringRef name) const
{
	String path;
	makeFullPath (path, subPath);

	HKEY hKey = nullptr;
	if(::RegOpenKeyExW (theRootKeys[rootKey], StringChars (path), 0, KEY_QUERY_VALUE | theAccessFlags [access], &hKey) == ERROR_SUCCESS)
	{
		DWORD type = 0;
		DWORD buffer[1] = {0};
		DWORD size = sizeof(buffer);

		LONG result = ::RegQueryValueExW (hKey, StringChars (name), nullptr, &type, (LPBYTE)buffer, &size);
		::RegCloseKey (hKey);

		if(result == ERROR_SUCCESS && type == REG_DWORD)
		{
			value = buffer[0];
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Accessor::writeDWORD (uint32 value, StringRef subPath, StringRef name)
{
	String path;
	makeFullPath (path, subPath);

	HKEY hKey = nullptr;
	if(::RegCreateKeyExW (theRootKeys[rootKey], StringChars (path), 0, nullptr/*class*/, 0/*options*/, 
						 KEY_ALL_ACCESS | theAccessFlags [access], nullptr/*security*/, &hKey, nullptr/*disposition*/) == ERROR_SUCCESS)
	{
		const BYTE* data = (const BYTE*)&value;
		DWORD size = sizeof(value);

		LONG result = ::RegSetValueExW (hKey, StringChars (name), 0, REG_DWORD, data, size);
		::RegCloseKey (hKey);

		if(result == ERROR_SUCCESS)
			return true;
	}
	return false;
}

//************************************************************************************************
// IniAccessor
//************************************************************************************************

IniAccessor& IniAccessor::getSharedInstance ()
{
	static AutoPtr<IniAccessor> theInstance;
	if(!theInstance.isValid ())
	{
		Url iniPath;
		System::GetSystem ().getLocation (iniPath, System::kSharedSettingsFolder);
		iniPath.descend (CCL_SETTINGS_NAME, Url::kFolder);
		
		// ensure that folder exists
		if(!System::GetFileSystem ().fileExists (iniPath))
			System::GetFileSystem ().createFolder (iniPath);

		iniPath.descend ("SharedRegistry.ini");
		theInstance = NEW IniAccessor (iniPath);
	}
	return *theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IniAccessor::IniAccessor (UrlRef iniPath)
: iniPath (iniPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IniAccessor::readString (String& string, StringRef section, StringRef keyName) const
{
	const DWORD kBufferSize = 4096;
	WCHAR buffer[kBufferSize];
	DWORD result = ::GetPrivateProfileStringW (StringChars (section), StringChars (keyName), nullptr, buffer, kBufferSize, iniPath);
	if(result > 0)
	{
		string.append (buffer, result);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IniAccessor::writeString (StringRef section, StringRef keyName, StringRef string)
{
	return ::WritePrivateProfileStringW (StringChars (section), StringChars (keyName), StringChars (string), iniPath) != 0;
}

//************************************************************************************************
// KeyIterator
//************************************************************************************************

KeyIterator::KeyIterator (void* hKey)
: hKey (hKey),
  index (0)
{
	::wcscpy (keyName, L"");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyIterator::~KeyIterator ()
{
	::RegCloseKey ((HKEY)hKey);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String KeyIterator::currentKey () const
{
	return String (keyName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KeyIterator::next ()
{
	FILETIME unused;
	DWORD length = kMaxKeyLength;
	LONG result = ::RegEnumKeyExW ((HKEY)hKey, index++, keyName, &length, nullptr, nullptr, nullptr, &unused);
	return result == ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL
} // namespace Registry
