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
// Filename    : ccl/platform/shared/host/platformfileselectorbase.cpp
// Description : Platform File Selector
//
//************************************************************************************************

#include "ccl/platform/shared/host/platformfileselectorbase.h"

#include "ccl/gui/gui.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"

#include "ccl/public/cclversion.h"

namespace CCL {
namespace PlatformIntegration {
	
//************************************************************************************************
// FileFilter
//************************************************************************************************

struct FileFilter
{
	MutableCString description;
	MutableCString extensions;

	FileFilter (StringRef description = nullptr, StringRef extensions = nullptr)
	: description (description, Text::kUTF8),
	  extensions (extensions, Text::kUTF8)
	{}

	FileFilter (const FileType& fileType)
	: description (fileType.getDescription (), Text::kUTF8)
	{
		addExtension (fileType.getExtension ());
	}

	void addExtension (StringRef ext)
	{
		if(!extensions.isEmpty ())
			extensions.append (";");
		extensions.append ("*.").append (ext);
	}
	
	bool operator == (const FileFilter& other) const
	{
		return description == other.description;
	}
};

} // namespace PlatformIntegration
} // namspace CCL
	
//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileSelector")
	XSTRING (AllFiles, "All Files")
	XSTRING (AllSupportedFiles, "All Supported Files")
END_XSTRINGS

using namespace CCL;
using namespace PlatformIntegration;

//************************************************************************************************
// LinuxFileSelector
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlatformFileSelectorBase, NativeFileSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformFileSelectorBase::PlatformFileSelectorBase ()
: platformSelector (CCLGUI_PACKAGE_ID),
  terminated (false)
{
	platformSelector.load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformFileSelectorBase::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	if(platformSelector == nullptr)
		return false;
	
	terminated = false;
	
	AutoPtr<IAsyncOperation> operation = runPlatformSelectorAsync (type, title, filterIndex, window);
	if(!operation.isValid ())
		return false;
	
	GUI.runModalLoop (nullptr, terminated);
	
	return !paths.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PlatformFileSelectorBase::runPlatformSelectorAsync (int type, StringRef _title, int filterIndex, IWindow* window)
{
	if(platformSelector == nullptr)
		return nullptr;
	
	if(type != kSaveFile)
	{
		FileFilter filter (XSTR (AllSupportedFiles));
		ForEach (filters, Boxed::FileType, fileType)
			filter.addExtension (fileType->getExtension ());
		EndFor
		platformSelector->addFilter (filter.description, filter.extensions);
	}
	
	Vector<FileFilter> uniqueFilters;
	ForEach (filters, Boxed::FileType, fileType)
		FileFilter uniqueFilter (*fileType);
		int index = uniqueFilters.index (uniqueFilter);
		if(index != -1)
			uniqueFilters[index].addExtension (fileType->getExtension ());
		else
			uniqueFilters.add (uniqueFilter);
	EndFor
	
	VectorForEach (uniqueFilters, FileFilter, filter)
		platformSelector->addFilter (filter.description, filter.extensions);
	EndFor
	if(type != kSaveFile)
	{
		FileFilter filter (XSTR (AllFiles), "*");
		platformSelector->addFilter (filter.description, filter.extensions);
	}
	
	int mode = type == kSaveFile ? PlatformIntegration::IPlatformFileSelector::kSave : PlatformIntegration::IPlatformFileSelector::kOpen;
	int fileMode = type == kOpenMultipleFiles ? PlatformIntegration::IPlatformFileSelector::kMultipleFiles : PlatformIntegration::IPlatformFileSelector::kFile;
	
	MutableCString title (_title, Text::kUTF8);
	
	MutableCString defaultExtension;
	if(getFilter ())
		defaultExtension = getFilter ()->getExtension ();
	
	MutableCString initialFolder (UrlDisplayString (getInitialFolder ()), Text::kUTF8);
	MutableCString initialFileName (getInitialFileName (), Text::kUTF8);
	
	if(!platformSelector->open (*this, mode, fileMode, title, defaultExtension, initialFolder, initialFileName))
		return nullptr;
	
	operation = NEW AsyncOperation;
	operation->setState (IAsyncOperation::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFileSelectorBase::addResult (CStringPtr path)
{
	AutoPtr<Url> result = NEW Url;
	if(result->fromDisplayString (String (Text::kUTF8, path), IUrl::kFile))
		paths.add (result.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFileSelectorBase::opened (void* nativeWindowHandle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFileSelectorBase::closed (int result)
{
	if(operation)
	{
		operation->setResult (!paths.isEmpty ());
		operation->setStateDeferred (IAsyncOperation::kCompleted);
	}
	terminated = true;
	operation.release ();
}

//************************************************************************************************
// PlatformFolderSelectorBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlatformFolderSelectorBase, NativeFolderSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformFolderSelectorBase::PlatformFolderSelectorBase ()
: platformSelector (CCLGUI_PACKAGE_ID),
  terminated (false)
{
	platformSelector.load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlatformFolderSelectorBase::runPlatformSelector (StringRef title, IWindow* window)
{
	if(platformSelector == nullptr)
		return false;
	
	terminated = false;
	
	AutoPtr<IAsyncOperation> operation = runPlatformSelectorAsync (title, window);
	if(!operation.isValid ())
		return false;
	
	GUI.runModalLoop (nullptr, terminated);
	
	return path.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PlatformFolderSelectorBase::runPlatformSelectorAsync (StringRef _title, IWindow* window)
{
	if(platformSelector == nullptr)
		return nullptr;
	
	int mode = PlatformIntegration::IPlatformFileSelector::kOpen;
	int fileMode = PlatformIntegration::IPlatformFileSelector::kDirectory;
	
	MutableCString title (_title, Text::kUTF8);
	
	MutableCString initialFolder (UrlDisplayString (getInitialPath ()), Text::kUTF8);
	
	if(!platformSelector->open (*this, mode, fileMode, title, "", initialFolder, ""))
		return nullptr;
	
	operation = NEW AsyncOperation;
	operation->setState (IAsyncOperation::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFolderSelectorBase::addResult (CStringPtr path)
{
	Url result;
	if(result.fromDisplayString (String (Text::kUTF8, path), IUrl::kFolder))
		setPath (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFolderSelectorBase::opened (void* nativeWindowHandle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformFolderSelectorBase::closed (int result)
{
	if(operation)
	{
		operation->setResult (!getPath ().isEmpty ());
		operation->setStateDeferred (IAsyncOperation::kCompleted);
	}
	terminated = true;
	operation.release ();
}
