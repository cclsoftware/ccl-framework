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
// Filename    : ccl/platform/win/system/registry.h
// Description : Windows Registry Access
//
//************************************************************************************************

#ifndef _ccl_registry_h
#define _ccl_registry_h

#include "ccl/public/storage/iurl.h"

namespace CCL {
namespace Registry {

class KeyIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Registry macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachRegistryKey(keyIterator, key) \
{ CCL::Registry::KeyIterator* __iter = keyIterator; \
  CCL::Deleter<CCL::Registry::KeyIterator> __del (__iter); \
  if(__iter) while(__iter->next ()) { \
  CCL::String key = __iter->currentKey ();

//////////////////////////////////////////////////////////////////////////////////////////////////
// Registry Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

enum RegKey
{
	kKeyClassesRoot,		///< HKEY_CLASSES_ROOT
	kKeyCurrentUser,		///< HKEY_CURRENT_USER
	kKeyLocalMachine,		///< HKEY_LOCAL_MACHINE
	kNumRootKeys
};

enum RegAccess
{
	kAccessDefault,		///< default
	kAccess64Bit,		///< KEY_WOW64_64KEY
	kAccess32Bit,  	    ///< KEY_WOW64_32KEY
	kNumAccessModes
};


/** Compose value name for current application. */
String AppValueName ();

//************************************************************************************************
// Accessor
/** Registry accessor */
//************************************************************************************************

class Accessor: public Unknown
{
public:
	Accessor (RegKey rootKey = kKeyCurrentUser, StringRef basePath = nullptr, RegAccess access = kAccessDefault);

	KeyIterator* newKeyIterator (StringRef subPath = nullptr) const;

	bool readString (String& string, StringRef subPath = nullptr, StringRef name = nullptr) const;
	bool writeString (StringRef string, StringRef subPath = nullptr, StringRef name = nullptr);

	bool readDWORD (uint32& value, StringRef subPath = nullptr, StringRef name = nullptr) const;
	bool writeDWORD (uint32 value, StringRef subPath = nullptr, StringRef name = nullptr);

protected:
	RegKey rootKey;
	String basePath;
	RegAccess access;

	String& makeFullPath (String& dst, StringRef subPath) const;
};

//************************************************************************************************
// IniAccessor
/** .INI file accessor */
//************************************************************************************************

class IniAccessor: public Unknown
{
public:
	IniAccessor (UrlRef iniPath);

	/**	NOTE: There doesn't seem to be a place in the registry which can be read/written by
		all non-privileged users, therefore we use a simple .ini file for this purpose. */
	static IniAccessor& getSharedInstance ();

	bool readString (String& string, StringRef section, StringRef keyName) const;
	bool writeString (StringRef section, StringRef keyName, StringRef string);

protected:
	NativePath iniPath;
};

//************************************************************************************************
// KeyIterator
/** Registry key iterator */
//************************************************************************************************

class KeyIterator: public Unknown
{
public:
	~KeyIterator ();

	bool next ();
	String currentKey () const;

protected:
	friend class Accessor;
	KeyIterator (void* hKey);

	void* hKey;
	int index;
	enum Constants { kMaxKeyLength = 2048 };
	uchar keyName[kMaxKeyLength];
};

} // namespace Registry
} // namespace CCL

#endif // _ccl_registry_h
