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
// Filename    : ccl/platform/win/system/nativefilesearcher.win.cpp
// Description : Win32 native file searcher
//
//************************************************************************************************
/*
	ISearchFolderItemFactory (available since Windows Vista)
		http://msdn.microsoft.com/en-us/library/bb775176%28VS.85%29.aspx

	SeachFolder Example
		http://code.msdn.microsoft.com/shellapplication
*/

#define DEBUG_LOG 1

#include "ccl/platform/win/system/nativefilesystem.win.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isearcher.h"

#include "ccl/platform/win/system/cclcom.h"
#include "ccl/platform/win/system/cclcoinit.h"
#include "ccl/public/systemservices.h"

#include <structuredquery.h>
#pragma comment (lib, "structuredquery.lib")

//Note: property keys are defined in propkey.h
//#include <propkey.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32::ShellFileSearcher
//************************************************************************************************

class ShellFileSearcher: public Unknown,
						 public ISearcher
{
public:
	ShellFileSearcher (ISearchFolderItemFactory* searchFolder, StringRef searchTerms);
	~ShellFileSearcher ();

	static ISearcher* createInstance (ISearchDescription& description);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Unknown)

protected:
	ComPtr<ISearchFolderItemFactory> searchFolder;
	String searchTerms;

	tresult CCL_API findInternal (ISearchResultSink& resultSink, IProgressNotify* progress);
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// NativeFileSystem
//************************************************************************************************

ISearcher* CCL_API WindowsNativeFileSystem::createSearcher (ISearchDescription& description)
{
	return Win32::ShellFileSearcher::createInstance (description);
}

//************************************************************************************************
// Win32::ShellFileSearcher
//************************************************************************************************

ISearcher* ShellFileSearcher::createInstance (ISearchDescription& description)
{
	ASSERT (System::IsInMainThread ())

	ComPtr<ISearchFolderItemFactory> searchFolder = com_new<ISearchFolderItemFactory> (CLSID_SearchFolderItemFactory);
	if(!searchFolder)
		return nullptr;

	// prepare scope
	NativePath nativePath (description.getStartPoint ());
	ComPtr<IShellItem> shellItem;
	::SHCreateItemFromParsingName (nativePath, nullptr, __uuidof(IShellItem), shellItem);
	if(!shellItem)
		return nullptr;

	ComPtr<IShellItemArray> scope;
	::SHCreateShellItemArrayFromShellItem (shellItem, __uuidof(IShellItemArray), scope);
	if(!scope)
		return nullptr;

	HRESULT hr = searchFolder->SetScope (scope);
	ASSERT (SUCCEEDED (hr))

	// build condition
	ComPtr<IConditionFactory> conditionFactory = com_new<IConditionFactory> (CLSID_ConditionFactory);
	if(!conditionFactory)
		return nullptr;

	ComPtr<ICondition> condition;
	PropVariant value;
	value.fromString (description.getSearchTerms ());
	LPCWSTR propertyName = L"System.FileName"; // PKEY_FileName
	conditionFactory->MakeLeaf (propertyName, COP_VALUE_CONTAINS, nullptr, &value, nullptr, nullptr, nullptr, FALSE, condition);
	if(!condition)
		return nullptr;

	hr = searchFolder->SetCondition (condition);
	ASSERT (SUCCEEDED (hr))

	return NEW ShellFileSearcher (searchFolder.detach (), description.getSearchTerms ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ShellFileSearcher::ShellFileSearcher (ISearchFolderItemFactory* searchFolder, StringRef searchTerms)
: searchFolder (searchFolder),
  searchTerms (searchTerms)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ShellFileSearcher::~ShellFileSearcher ()
{
	CCL_PRINTLN ("Win32::ShellFileSearcher dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShellFileSearcher::find (ISearchResultSink& resultSink, CCL::IProgressNotify* progress)
{
	if(!System::IsInMainThread ())
		System::CoWinRTInitialize ();

	tresult tr = findInternal (resultSink, progress);

	if(!System::IsInMainThread ())
		System::CoWinRTUninitialize ();

	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ShellFileSearcher::findInternal (ISearchResultSink& resultSink, CCL::IProgressNotify* progress)
{
	// This retrieves an IShellItem of the search. It is a virtual child of the desktop.
	ComPtr<IShellItem> resultFolder;
	HRESULT hr = searchFolder->GetShellItem (__uuidof(IShellItem), resultFolder);
	ASSERT (SUCCEEDED (hr))
	if(!resultFolder)
		return kResultFailed;

	// When the retrieved IShellItem is enumerated, it returns the search results.
	ComPtr<IEnumShellItems> enumerator;
	hr = resultFolder->BindToHandler (nullptr, BHID_StorageEnum, __uuidof(IEnumShellItems), enumerator);
	ASSERT (SUCCEEDED (hr))
	if(!enumerator)
		return kResultFailed;

	while(1)
	{
		ULONG fetched = 0;
		IShellItem* items[1] = {nullptr};
		hr = enumerator->Next (1, items, &fetched);
		ComPtr<IShellItem> current (items[0]);
		if(!current)
			break;

		if(progress && progress->isCanceled ())
			return kResultAborted;

		// Get the item's file system path, if it has one
		WCHAR* nativePath = nullptr;
		hr = current->GetDisplayName (SIGDN_FILESYSPATH, &nativePath);
		if(FAILED (hr))
			continue;

		ComDeleter<WCHAR> nativePathDeleter (nativePath);

		// Get requested set of attributes
		SFGAOF attributes = 0;
		hr = current->GetAttributes (SFGAO_FILESYSTEM|SFGAO_FOLDER, &attributes);
		ASSERT (SUCCEEDED (hr))

		// feed sink
		AutoPtr<Url> path = NEW Url;
		bool isFolder = (attributes & SFGAO_FOLDER) != 0;
		path->fromNativePath (nativePath, isFolder ? Url::kFolder : Url::kFile);

		// only accept if fileName without extension matches searchTerms
		String nameWithout;
		path->getName (nameWithout, false);
		if(nameWithout.contains (searchTerms, false))
			resultSink.addResult (static_cast<IUrl*> (path.detach ()));
	}

	return kResultOk;
}
