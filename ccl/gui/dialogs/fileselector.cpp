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
// Filename    : ccl/gui/dialogs/fileselector.cpp
// Description : File Selector
//
//************************************************************************************************

#include "ccl/gui/dialogs/fileselector.h"
#include "ccl/gui/windows/desktop.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/kernel.h"

#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// FileSelector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileSelector, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSelector::FileSelector ()
: hook (nullptr),
  customView (nullptr)
{
	paths.objectCleanup ();
	filters.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSelector::~FileSelector ()
{
	setHook (nullptr);
	if(customView)
		customView->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileSelector::addFilter (const FileType& type)
{
	filters.add (NEW Boxed::FileType (type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileSelector::countFilters () const
{
	return filters.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API FileSelector::getFilter (int index) const
{
	Boxed::FileType* ft = (Boxed::FileType*)filters.at (index);
	return static_cast<FileType*> (ft);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileSelector::setHook (IUnknown* _hook)
{
	take_shared<IUnknown> (hook, _hook);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FileSelector::getHook () const
{
	return hook;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FileSelector::createCustomView ()
{
	if(customView == nullptr && hook != nullptr)
	{
		UnknownPtr<IViewFactory> viewFactory (getHook ());
		customView = unknown_cast<View> (viewFactory->createView ("FileSelectorView", Variant (asUnknown ()), CCL::Rect ()));
	}
	if(customView)
		customView->retain ();
	return customView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileSelector::setFolder (UrlRef path)
{
	initialFolder = path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileSelector::setFileName (StringRef name)
{
	initialFileName = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSelector::run (int type, StringRef title, int filterIndex, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API FileSelector::runAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileSelector::countPaths () const
{
	return paths.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl* CCL_API FileSelector::getPath (int index) const
{
	Url* url = (Url*)paths.at (index);
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef FileSelector::getInitialFolder () const
{
	if(!initialFolder.isEmpty ())
	{
		// ascend until folder exists
		while(!System::GetFileSystem ().fileExists (initialFolder))
			if(!initialFolder.ascend ())
				break;
	}
	return initialFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef FileSelector::getInitialFileName () const
{
	return initialFileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileSelector::getSaveBehavior () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileSelector::setSaveContent (UrlRef url)
{
	CCL_NOT_IMPL ("FileSelector::setSaveContent")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (FileSelector)
	DEFINE_PROPERTY_NAME ("fileName")
END_PROPERTY_NAMES (FileSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSelector::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "fileName")
	{
		var = initialFileName;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSelector::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "fileName")
	{
		setFileName (var.asString ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (FileSelector)
	DEFINE_METHOD_ARGS ("addFilter", "fileType")
	DEFINE_METHOD_ARGR ("runOpen", "[title]", "bool")
	DEFINE_METHOD_ARGR ("runSave", "[title]", "bool")
	DEFINE_METHOD_ARGR ("setFileName", "string", "void")
	DEFINE_METHOD_ARGR ("setFolder", "Url", "void")
	DEFINE_METHOD_ARGR ("countPaths", "", "int")
	DEFINE_METHOD_ARGR ("getPath", "index", "Url")
END_METHOD_NAMES (FileSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSelector::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addFilter")
	{
		Boxed::FileType fileType;
		fileType.fromVariant (msg[0]);
		if(fileType.isValid ())
			addFilter (fileType);
		return true;
	}
	else if(msg == "runOpen" || msg == "runSave")
	{
		int type = msg == "runSave" ? kSaveFile : kOpenFile;
		String title;
		if(msg.getArgCount () > 0)
			title = msg[0].asString ();
		returnValue = run (type, title);
		return true;
	}
	else if(msg == "setFileName")
	{
		String fileName;
		if(msg.getArgCount () > 0)
			fileName = msg[0].asString ();
		setFileName (fileName);
		return true;
	}
	else if(msg == "setFolder")
	{
		UnknownPtr<IUrl> folder (msg[0]);
		if(folder)
		{
			Url resolved (*folder);
			if(folder->getProtocol () == CCLSTR ("local"))
				System::GetSystem ().resolveLocation (resolved, *folder);
			setFolder (resolved);
		}
		return true;
	}
	else if(msg == "countPaths")
	{
		returnValue = countPaths ();
		return true;
	}
	else if(msg == "getPath")
	{
		int index = msg.getArgCount () > 0 ? msg[0].asInt () : 0;
		Url* url = (Url*)paths.at (index);
		returnValue.takeShared (ccl_as_unknown (url));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// NativeFileSelector
//************************************************************************************************

DEFINE_CLASS (NativeFileSelector, FileSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeFileSelector* NativeFileSelector::create ()
{
	// create derived platform specific class via class registry
	Object* object = Kernel::instance ().getClassRegistry ().createObject (ClassID::FileSelector);
	return ccl_cast<NativeFileSelector> (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeFileSelector::NativeFileSelector ()
: selectedType (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFileSelector::run (int type, StringRef title, int filterIndex, IWindow* window)
{
	if(!window)
		window = Desktop.getDialogParentWindow ();

	paths.removeAll (); // remove old result

	return runPlatformSelector (type, title, filterIndex, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API NativeFileSelector::runAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	if(!window)
		window = Desktop.getDialogParentWindow ();

	paths.removeAll (); // remove old result

	return runPlatformSelectorAsync (type, title, filterIndex, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* NativeFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	return nullptr;
}

//************************************************************************************************
// NativeFolderSelector
//************************************************************************************************

DEFINE_CLASS (NativeFolderSelector, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeFolderSelector::NativeFolderSelector ()
: path (NEW Url)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NativeFolderSelector::setPath (UrlRef _path)
{
	path->assign (_path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NativeFolderSelector::getPath () const
{
	ASSERT (path != nullptr)
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url NativeFolderSelector::getInitialPath () const
{
	Url initialFolder (getPath ());
	if(!initialFolder.isEmpty ())
	{
		// ascend until folder exists
		while(!System::GetFileSystem ().fileExists (initialFolder))
			if(!initialFolder.ascend ())
				break;
	}
	return initialFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFolderSelector::run (StringRef title, IWindow* window)
{
	if(!window)
		window = Desktop.getDialogParentWindow ();

	return runPlatformSelector (title, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API NativeFolderSelector::runAsync (StringRef title, IWindow* window)
{
	if(!window)
		window = Desktop.getDialogParentWindow ();

	return runPlatformSelectorAsync (title, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* NativeFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (NativeFolderSelector)
	DEFINE_METHOD_ARGR ("run", "[title]", "bool")
	DEFINE_METHOD_ARGR ("getPath", "", "Url")
END_METHOD_NAMES (NativeFolderSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeFolderSelector::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "run")
	{
		String title;
		if(msg.getArgCount () > 0)
			title = msg[0].asString ();
		returnValue = run (title);
		return true;
	}
	else if(msg == "getPath")
	{
		returnValue.takeShared (static_cast<IUrl*> (path));
		return true;
	}
	return false;
}
