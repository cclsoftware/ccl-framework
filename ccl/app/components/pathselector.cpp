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
// Filename    : ccl/app/components/pathselector.cpp
// Description : Path Selector
//
//************************************************************************************************

#include "ccl/app/components/pathselector.h"

#include "ccl/app/params.h"
#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/base/message.h"
#include "ccl/base/objectconverter.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// PathListModel
//************************************************************************************************

class PathListModel: public Object,
					 public AbstractItemModel
{
public:
	CLASS_INTERFACE (IItemModel, Object)

	PathListModel ();
	~PathListModel ();

	bool checkAddPath (UrlRef path);

	PathList* getPathList () const;
	void setPathList (PathList* pathList);

	PROPERTY_VARIABLE (int, selectedIndex, SelectedIndex)

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	PathList* pathList;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PathList")
	XSTRING (SelectPath, "Select")
	XSTRING (SelectDefaultPath, "Select Default")
	XSTRING (ClearHistory, "Clear History")
	XSTRING (AskAddRootPath, "Searching the root folder of a volume might be very slow.\n\nDo you really want to add %(1)?")
END_XSTRINGS

//************************************************************************************************
// PathList
//************************************************************************************************

DEFINE_CLASS (PathList, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PathList::PathList ()
{
	paths.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::isEmpty () const
{
	return paths.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PathList::getNumPaths () const
{
	return paths.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* PathList::getPath (int index) const
{
	return (Url*)paths.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PathList::newIterator () const
{
	return paths.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::contains (UrlRef _path) const
{
	Url path (_path);
	return paths.contains (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::containsSubPath (UrlRef _path) const
{
	for(auto sub : iterate_as<Url> (paths))
		if(sub->contains (_path))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::addPath (UrlRef _path)
{
	Url* path = NEW Url (_path);
	if(!paths.contains (*path))
	{
		paths.add (path);
		signal (Message (kChanged));
		return true;
	}
	else
	{
		path->release ();
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::removePath (UrlRef path)
{
	Url* url = paths.findIf<Url> ([&] (const Url& u) { return u == path; });
	if(url)
	{
		paths.remove (url);
		url->release ();
		signal (Message (kChanged));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::removeAt (int index)
{
	Url* path = (Url*)paths.at (index);
	if(path)
	{
		paths.remove (path);
		path->release ();
		signal (Message (kChanged));
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathList::removeAll ()
{
	if(!paths.isEmpty ())
	{
		paths.removeAll ();
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::load (const Storage& storage)
{
	storage.getAttributes ().unqueue (paths, nullptr , ccl_typeid<Url> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathList::save (const Storage& storage) const
{
	storage.getAttributes ().queue (nullptr, paths, Attributes::kTemp);
	return true;
}

//************************************************************************************************
// PathListComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PathListComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

PathListComponent::PathListComponent (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("PathList") : name),
  listModel (NEW PathListModel)
{
	paramList.addParam (CSTR ("addPath"), kAddPath);
	paramList.addParam (CSTR ("removePath"), kRemovePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathListComponent::~PathListComponent ()
{
	listModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathListComponent::setPathList (PathList* pathList)
{
	listModel->setPathList (pathList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PathListComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "pathList")
		return ccl_as_unknown (listModel);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kAddPath :
		{
			AutoPtr<IFolderSelector> selector = ccl_new<IFolderSelector> (ClassID::FolderSelector);
			ASSERT (selector != nullptr)
			if(selector->run () && listModel->checkAddPath (selector->getPath ()))
			{
				PathList* pathList = listModel->getPathList ();
				if(pathList)
					pathList->addPath (selector->getPath ());
			}
		}
		break;

	case kRemovePath :
		{
			int index = listModel->getSelectedIndex ();
			PathList* pathList = listModel->getPathList ();
			if(index != -1 && pathList)
				pathList->removeAt (index);
		}
		break;
	}
	return true;
}

//************************************************************************************************
// PathListModel
//************************************************************************************************

PathListModel::PathListModel ()
: pathList (nullptr),
  selectedIndex (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathListModel::~PathListModel ()
{
	setPathList (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathListModel::checkAddPath (UrlRef path)
{
	if(path.isRootPath ())
		return Alert::ask (String ().appendFormat (XSTR (AskAddRootPath), UrlDisplayString (path, Url::kStringDisplayPath))) == Alert::kYes;
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathList* PathListModel::getPathList () const
{
	return pathList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathListModel::setPathList (PathList* _pathList)
{
	share_and_observe (this, pathList, _pathList);
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PathListModel::notify (ISubject* subject, MessageRef msg)
{
	if(subject == pathList && msg == kChanged)
		signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PathListModel::countFlatItems ()
{
	return pathList ? pathList->getNumPaths () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::getItemTitle (String& title, ItemIndexRef index)
{
	const Url* path = pathList ? pathList->getPath (index.getIndex ()) : nullptr;
	if(!path)
		return false;

	path->toDisplayString (title, Url::kStringDisplayPath);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::canRemoveItem (ItemIndexRef index)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::removeItem (ItemIndexRef index)
{
	bool result = false;
	if(pathList)
		result = pathList->removeAt (index.getIndex ());
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	ForEachUnknown (data, unknown)
		UnknownPtr<IUrl> path (unknown);
		if(path)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	bool result = false;
	if(pathList)
		ForEachUnknown (data, unknown)
			UnknownPtr<IUrl> path (unknown);
			if(path)
			{
				Url path2 (*path);
				if(path2.isFile ())
					path2.ascend ();

				if(checkAddPath (path2))
				{
					pathList->addPath (path2);
					result = true;
				}
			}
		EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathListModel::onItemFocused (ItemIndexRef index)
{
	selectedIndex = index.getIndex ();
	return true;
}

//************************************************************************************************
// PathSelector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PathSelector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

PathSelector::PathSelector (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("PathSelector") : name),
  path (*NEW Url)
{
	paramList.addString (CSTR ("pathString"), kPathString);
	paramList.addParam (CSTR ("selectPath"), kSelectPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathSelector::~PathSelector ()
{
	path.release ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelector::setPath (UrlRef path)
{
	this->path = path;
	paramList.byTag (kPathString)->fromString (UrlDisplayString (path, Url::kStringDisplayPath));
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url& PathSelector::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelector::enable (bool state)
{
	paramList.byTag (kSelectPath)->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathSelector::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kSelectPath :
		runSelector ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PathSelector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "runSelector")
		runSelector (false);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelector::runSelector (bool deferred)
{
	if(deferred)
		(NEW Message ("runSelector"))->post (this);

	else
	{
		AutoPtr<IFolderSelector> selector = ccl_new<IFolderSelector> (ClassID::FolderSelector);
		ASSERT (selector != nullptr)
		selector->setPath (getPath ());

		SharedPtr<PathSelector> This (this);
		Promise promise (selector->runAsync ());
		promise.then ([This, selector](IAsyncOperation& operation)
		{
			if(operation.getResult ().asBool ())
			{
				This->setPath (selector->getPath ());
			}
		});	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PathSelector::getObject (StringID name, UIDRef classID)
{
	if(name == "DataTarget")
		return (IDataTarget*) this;

	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoPtr<IUrl> PathSelector::toFolderUrl (IUnknown* unk) const
{
	AutoPtr<IUrl> result;
	UnknownPtr<IUrl> iUrl (unk);
	if(iUrl)
		result = iUrl.detach ();
	else
		result = ObjectConverter::toInterface<IUrl> (unk);

	if(result && result->isFolder ())
		return result;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathSelector::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	if(paramList.byTag (kSelectPath)->isEnabled ())
	{
		ForEachUnknown (data, unk)
			AutoPtr<IUrl> url (toFolderUrl (unk));
			if(url)
				return true;
		EndFor
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathSelector::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(paramList.byTag (kSelectPath)->isEnabled ())
	{
		ForEachUnknown (data, unk)
			AutoPtr<IUrl> url (toFolderUrl (unk));
			if(url)
			{
				setPath (*url);
				return true;
			}
		EndFor
	}
	return false;
}

//************************************************************************************************
// PathSelectorWithHistory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PathSelectorWithHistory, PathSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

PathSelectorWithHistory::PathSelectorWithHistory (StringRef name)
: PathSelector (name), 
  clearHistorySupported (false),
  defaultPath (nullptr)
{
	paramList.addMenu (CSTR ("pathHistory"), kPathHistory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathSelectorWithHistory::~PathSelectorWithHistory ()
{
	safe_release (defaultPath);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelectorWithHistory::setDefaultPath (UrlRef url)
{
	if(url.isEmpty ())
		safe_release (defaultPath);	
	else
	{
		if(defaultPath == nullptr)
			defaultPath = NEW Url (url);
		else
			defaultPath->assign (url);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathSelectorWithHistory::isDefaultPathSelected () const
{
	if(defaultPath != nullptr)
		return getPath ().isEqualUrl (*defaultPath);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelectorWithHistory::setPath (UrlRef path)
{
	SuperClass::setPath (path);

	// select in history
	int index = addUrl (path);

	MenuParam* history = unknown_cast<MenuParam> (paramList.byTag (kPathHistory));
	ASSERT (history != nullptr)
	history->setValue (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelectorWithHistory::enable (bool state)
{
	SuperClass::enable (state);
	paramList.byTag (kPathHistory)->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PathSelectorWithHistory::addUrl (UrlRef url, StringRef title)
{
	MenuParam* history = unknown_cast<MenuParam> (paramList.byTag (kPathHistory));
	ASSERT (history != nullptr)

	// add if not yet in history
	AutoPtr<Url> titleUrl = NEW UrlWithTitle (url, title);
	int index = history->getObjectIndex (*titleUrl);
	if(index == -1)
	{
		int insertIndex = -1;
		if(defaultPath && url.isEqualUrl (*defaultPath, false))
			insertIndex = 0;
		
		history->appendObject (titleUrl.detach (), insertIndex);
		index = insertIndex < 0 ? history->getMax ().asInt () : insertIndex;

		// select if it is the first path
		if(index == 0)
			PathSelectorWithHistory::setPath (url); // do not call derived class here
	}
	return index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PathSelectorWithHistory::addVolumes (int typeMask)
{
	int count = 0;
	ForEachFile (System::GetFileSystem ().newIterator (Url ("file:///")), path)
		VolumeInfo info;
		info.type = INativeFileSystem::kSuppressSlowVolumeInfo; // suppress details for remote drives, etc.
		if(System::GetFileSystem ().getVolumeInfo (info, *path))
			if(typeMask & (1<<info.type))
			{
				String label = PathClassifier::getVolumeLabel (*path, info);
				addUrl (*path, label);
				count++;
			}
	EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PathSelectorWithHistory::selectAt (int index)
{
	paramList.byTag (kPathHistory)->setValue (index, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathSelectorWithHistory::paramChanged (IParameter* param)
{
	if(param->getTag () == kPathHistory)
	{
		MenuParam* history = unknown_cast<MenuParam> (param);
		ASSERT (history != nullptr)

		Url* path = history->getObject<Url> (history->getValue ());
		ASSERT (path != nullptr)
		setPath (*path);
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PathSelectorWithHistory::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IMenu> menu (msg[0]);
		ASSERT (menu != nullptr)
		if(menu->countItems () > 0)
			menu->addSeparatorItem ();
		menu->addCommandItem (CommandWithTitle (CSTR ("Path"), CSTR ("Select"),  XSTR (SelectPath)), this, true);
		if(defaultPath)
			menu->addCommandItem (XSTR (SelectDefaultPath), CSTR ("Path"), CSTR ("Select Default"), this);
		
		if(clearHistorySupported)
		{
			menu->addSeparatorItem ();
			menu->addCommandItem (XSTR (ClearHistory), CSTR ("History"), CSTR ("Clear"), this);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PathSelectorWithHistory::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Path")
	{
		if(msg.name == "Select")
		{
			if(!msg.checkOnly ())
			{
				runSelector (true); // must defer on IOS until menu is closed
			}
			return true;
		}
		else if(msg.name == "Select Default")
		{
			if(msg.checkOnly ())
			{
				if(defaultPath != nullptr)
					return isDefaultPathSelected () == false;
				return false;
			}

			setPath (*defaultPath);
			return true;
		}
		return false;
	}
	else if(msg.category == "History" && msg.name == "Clear")
	{
		MenuParam* history = unknown_cast<MenuParam> (paramList.byTag (kPathHistory));

		if(msg.checkOnly ())
			return history->getObjectCount () > 1;

		Url previousPath (getPath ());

		history->removeAll ();

		if(previousPath.isEmpty () == false)
			addUrl (previousPath);

		return true;
	}
	else
		return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathSelectorWithHistory::storeHistory (Attributes& a, bool includeDefaultPath) const
{
	a.remove ("history");

	if(MenuParam* history = unknown_cast<MenuParam> (paramList.byTag (kPathHistory)))
	{
		int count = history->getObjectCount ();
		for(int i = 0; i < count; i++)
		{
			if(Url* path = history->getObject<Url> (i))
			{
				if(includeDefaultPath == false && defaultPath && path->isEqualUrl (*defaultPath))
					continue;
				
				a.queue ("history", path, Attributes::kTemp);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathSelectorWithHistory::restoreHistory (Attributes& a)
{
	MenuParam* history = unknown_cast<MenuParam> (paramList.byTag (kPathHistory));
	history->removeAll ();
	Url currentPath = getPath ();

	while(AutoPtr<Url> url = a.unqueueObject<Url> ("history"))
	{
		if(System::GetFileSystem ().fileExists (*url))
			addUrl (*url);
	}

	if(currentPath.isEmpty () == false)
	{
		if(history->isEmpty ())
			addUrl (currentPath);
		else if(getPath () != currentPath)
			PathSelectorWithHistory::setPath (currentPath);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathSelectorWithHistory::storeSettings (StringRef settingsID) const
{
	Attributes& a = Settings::instance ().getAttributes (settingsID);
	a.removeAll ();
	return storeHistory (a, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathSelectorWithHistory::restoreSettings (StringRef settingsID) 
{
	Attributes& a = Settings::instance ().getAttributes (settingsID);
	return restoreHistory (a);
}
