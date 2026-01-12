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
// Filename    : ccl/platform/win/system/resourceloader.win.cpp
// Description : Windows Resource Loader
//
//************************************************************************************************

#include "ccl/system/virtualfilesystem.h"

#include "ccl/base/objectnode.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/container.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {

//************************************************************************************************
// WindowsResourceFileSystem
//************************************************************************************************

class WindowsResourceFileSystem: public ResourceFileSystem
{
public:
	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode, IUnknown* context) override;
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override;
	tbool CCL_API fileExists (UrlRef url) override;
};

namespace Win32 {

//************************************************************************************************
// Win32::ResourceNaming
//************************************************************************************************

class ResourceNaming
{
public:
	static String fromRawName (StringRef name);
	static String toRawName (StringRef name);

protected:
	struct CharReplacement
	{
		String character;
		String entity;

		CharReplacement (StringRef character, StringRef entity)
		: character (character),
		  entity (entity)
		{}
	};

	static const CharReplacement toReplace[];
};

//************************************************************************************************
// Win32::ResourceEntry
//************************************************************************************************

class ResourceEntry: public ObjectNode
{
public:
	ResourceEntry (StringRef name = nullptr, bool data = false)
	: ObjectNode (name),
	  data (data)
	{}

	PROPERTY_BOOL (data, Data)

	ResourceEntry* findEntry (StringRef name) const
	{ return (ResourceEntry*)findChildNode (name); }

	void addEntry (ResourceEntry* entry)
	{ addChild (entry); }

	ResourceEntry* lookupEntry (StringRef path) const
	{ return (ResourceEntry*)unknown_cast<ObjectNode> (lookupChild (path)); }
};

//************************************************************************************************
// Win32::ResourceList
//************************************************************************************************

class ResourceList
{
public:
	void scan (ModuleRef module);

	void addEntry (StringRef fullName);
	void removeAll ();

	const ResourceEntry* lookup (StringRef path) const;

protected:
	ResourceEntry root;

	static BOOL CALLBACK enumCallback (HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam);
};

//************************************************************************************************
// Win32::ResourceIterator
//************************************************************************************************

class ResourceIterator: public Object,
						public IFileIterator
{
public:
	ResourceIterator (UrlRef basePath, int mode);

	// IFileIterator
	const IUrl* CCL_API next () override;

	CLASS_INTERFACE (IFileIterator, Object)

protected:
	int mode;
	Url basePath;
	Url current;
	ResourceList resourceList;
	AutoPtr<Iterator> iterator;

	bool filterEntry (const ResourceEntry& entry) const;
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// Win32::ResourceNaming
//************************************************************************************************

const ResourceNaming::CharReplacement ResourceNaming::toReplace[] =
{
	CharReplacement ("@", "&#40;"),
	CharReplacement ("'", "&#27;"),
	CharReplacement (" ", "&#20;")
};

//////////////////////////////////////////////////////////////////////////////////////////////////

String ResourceNaming::fromRawName (StringRef _name)
{
	String name (_name);
	name.toLowercase ();
	for(int i = 0; i < ARRAY_COUNT (toReplace); i++)
		name.replace (toReplace[i].entity, toReplace[i].character);
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ResourceNaming::toRawName (StringRef _name)
{
	String name (_name);
	for(int i = 0; i < ARRAY_COUNT (toReplace); i++)
		name.replace (toReplace[i].character, toReplace[i].entity);
	return name;
}

//************************************************************************************************
// ResourceFileSystem
//************************************************************************************************

ResourceFileSystem& ResourceFileSystem::instance ()
{
	static WindowsResourceFileSystem theInstance;
	return theInstance;
}

//************************************************************************************************
// WindowsResourceFileSystem
//************************************************************************************************

CCL::IStream* CCL_API WindowsResourceFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	ASSERT ((mode & IStream::kCreate) == 0)
	if(mode & IStream::kCreate)
		return nullptr;

	ModuleRef module = System::GetModuleWithIdentifier (url.getHostName ());
	ASSERT (module != nullptr)
	if(!module)
		return nullptr;

	CCL::IStream* stream = nullptr;
	String rawName (ResourceNaming::toRawName (url.getPath ()));
	HRSRC hResource = ::FindResource ((HMODULE)module, StringChars (rawName), RT_RCDATA);
	if(hResource)
	{
		DWORD size = ::SizeofResource ((HMODULE)module, hResource);
		HGLOBAL hGlobal = ::LoadResource ((HMODULE)module, hResource);
		void* address = hGlobal ? ::LockResource (hGlobal) : nullptr;
		if(size > 0 && address)
		{
			// Note: Resource memory remains valid as long as the module is loaded.
			// We copy the memory here, in case the module is released earlier than the stream.
			#if 0
			stream = NEW MemoryStream (address, size);
			#else
			MemoryStream* memStream = NEW MemoryStream;
			memStream->allocateMemory (size);
			memStream->write (address, size);
			memStream->rewind ();
			stream = memStream;
			#endif
		}
	}
	return stream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API WindowsResourceFileSystem::newIterator (UrlRef url, int mode)
{
	return NEW ResourceIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsResourceFileSystem::fileExists (UrlRef url)
{
	if(url.isFolder ())
	{
		ResourceIterator iter (url, IFileIterator::kAll);
		return iter.next () != nullptr;
	}
	else
	{
		ModuleRef module = System::GetModuleWithIdentifier (url.getHostName ());
		ASSERT (module != nullptr)
		if(!module)
			return false;

		String rawName (ResourceNaming::toRawName (url.getPath ()));
		HRSRC hResource = ::FindResource ((HMODULE)module, StringChars (rawName), RT_RCDATA);
		return hResource != nullptr;
	}
}

//************************************************************************************************
// Win32::ResourceIterator
//************************************************************************************************

ResourceIterator::ResourceIterator (UrlRef basePath, int mode)
: mode (mode),
  basePath (basePath)
{
	ModuleRef module = System::GetModuleWithIdentifier (basePath.getHostName ());
	ASSERT (module != nullptr)
	if(!module)
		return;

	resourceList.scan (module);

	String basePathString (basePath.getPath ());
	basePathString.toLowercase ();
	const ResourceEntry* folderEntry = resourceList.lookup (basePathString);
	//ASSERT (folderEntry != 0)
	iterator = folderEntry ? folderEntry->newIterator () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ResourceIterator::filterEntry (const ResourceEntry& entry) const
{
	bool wantFiles = (mode & IFileIterator::kFiles) != 0;
	bool wantFolders = (mode & IFileIterator::kFolders) != 0;
	return (entry.isData () && wantFiles) || (!entry.isData () && wantFolders);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API ResourceIterator::next ()
{
	if(iterator)
	{
		ResourceEntry* entry;
		while((entry = (ResourceEntry*)iterator->next ()) != nullptr)
		{
			if(filterEntry (*entry))
			{
				current.assign (basePath);
				current.descend (entry->getName (), entry->isData () ? Url::kFile : Url::kFolder);
				return &current;
			}
		}
	}
	return nullptr;
}

//************************************************************************************************
// Win32::ResourceList
//************************************************************************************************

BOOL CALLBACK ResourceList::enumCallback (HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	// Note: resource enumeration returns capitalized names only,
	// we use them always in lowercase!
	ResourceList* This = (ResourceList*)lParam;
	String fullName (ResourceNaming::fromRawName (String (lpszName)));
	This->addEntry (fullName);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceList::scan (ModuleRef module)
{
	::EnumResourceNames ((HINSTANCE)module, RT_RCDATA, enumCallback, (LONG_PTR)this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceList::addEntry (StringRef fullName)
{
	ResourceEntry* parent = &root;
	AutoPtr<IStringTokenizer> tokenizer = fullName.tokenize (Url::strPathChar);
	if(tokenizer)
		while(!tokenizer->done ())
		{
			uchar delimiter = 0;
			String s = tokenizer->nextToken (delimiter);
			if(tokenizer->done ())
				parent->addEntry (NEW ResourceEntry (s, true));
			else
			{
				ResourceEntry* entry = parent->findEntry (s);
				if(!entry)
					parent->addEntry (entry = NEW ResourceEntry (s, false));
				parent = entry;
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ResourceEntry* ResourceList::lookup (StringRef path) const
{
	if(path.isEmpty ())
		return &root;
	return root.lookupEntry (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ResourceList::removeAll ()
{
	root.removeAll ();
}
