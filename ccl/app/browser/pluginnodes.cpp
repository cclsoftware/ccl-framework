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
// Filename    : ccl/app/browser/pluginnodes.cpp
// Description : Plug-in Nodes
//
//************************************************************************************************

#include "ccl/app/browser/pluginnodes.h"
#include "ccl/app/browser/pluginselector.h"
#include "ccl/app/browser/plugindraghander.h"
#include "ccl/app/browser/filesystemnodes.h"
#include "ccl/app/browser/nodesorter.h"
#include "ccl/app/browser/browser.h"

#include "ccl/app/presets/objectpreset.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfile.h"

#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/utilities/fileoperations.h"

#include "ccl/app/components/filerenamer.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"

using namespace CCL;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Browser")
	XSTRING (PlugInCategory, "Plug-ins")
	XSTRING (Recent, "Recent")
	XSTRING (Hide, "Hide")
	XSTRING (Reset, "Reset")
	XSTRING (Revert, "Revert")
	XSTRING (AskReset, "Are you sure you want to reset the plug-in browser to default?")
	XSTRING (AskRevert, "Are you sure you want to revert your last plug-in browser changes?")
	XSTRING (DeleteThumbnail, "Delete Thumbnail")
END_XSTRINGS

//************************************************************************************************
// SwipeNodesMouseHandler
/** Swiping over multiple plugin nodes, e.g. to toggle a property (could be generalized for any BrowserNode, e.g. Browser::swipeNodes) */
//************************************************************************************************

template<typename Lambda>
class SwipeNodesMouseHandler: public Object,
							  public AbstractMouseHandler,
							  public AbstractItemSelection
{
public:
	SwipeNodesMouseHandler (Browser* browser, IItemView* itemView, Lambda visitNode)
	: browser (browser), itemView (itemView), visitNode (visitNode)
	{}

	bool onMove (int moveFlags) override
	{
		if(itemView)
		{
			Rect rect (first.where, current.where);
			rect.normalize ();
			itemView->findItems (rect, *this); ///< calls select ;-)
		}
		return true;
	}
		
	void CCL_API finish (const MouseEvent& event, tbool canceled = false) override
	{
		PlugInCategoryNode::signalPresentationChanged ();
	}

	// AbstractItemSelection
	void CCL_API select (ItemIndexRef index) override
	{
		if(PlugInClassNode* classNode = ccl_cast<PlugInClassNode> (browser->resolveNode (*itemView, index)))
		{
			if(!categoryNode)
				categoryNode = classNode->getAncestorNode<PlugInCategoryNode> ();

			visitNode (classNode);

			if(Browser* browser = classNode->getBrowser ())
				browser->redrawNode (classNode);
		}
	}

	CLASS_INTERFACE (IMouseHandler, Object)

protected:
	Browser* browser;
	IItemView* itemView;
	Lambda visitNode;
	SharedPtr<PlugInCategoryNode> categoryNode;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Lambda>
bool swipeNodes (BrowserNode* startNode, IView* view, const GUIEvent& editEvent, Lambda visitNode)
{
	Browser* browser = startNode ? startNode->getBrowser () : nullptr;
	UnknownPtr<IItemView> itemView (view);
	if(browser && itemView)
		if(auto mouseEvent = editEvent.as<MouseEvent> ())
		{
			IMouseHandler* mouseHandler = NEW SwipeNodesMouseHandler<Lambda> (browser, itemView, visitNode);

			itemView->beginMouseHandler (mouseHandler, *mouseEvent);
			mouseHandler->trigger (*mouseEvent, 0); // initial action
			return true;
		}
	return false;
}

//************************************************************************************************
// PluginSearchProvider
//************************************************************************************************

PluginSearchProvider::PluginSearchProvider (StringRef category, IObjectFilter* classFilter)
: hiddenPluginsFilter (NEW HiddenPluginsFilter (nullptr, classFilter)),
  resultCategory (XSTR (PlugInCategory))
{
	startPoint.setProtocol (CCLSTR ("class"));
	startPoint.setPath (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* PluginSearchProvider::createSearcher (ISearchDescription& description)
{
	return System::GetPlugInManager ().createSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* PluginSearchProvider::getSearchResultFilter () const
{
	return hiddenPluginsFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PluginSearchProvider::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	UnknownPtr<IUrl> url (resultItem);
	if(url)
		if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (*url))
		{
			PlugInClass classInfo (*description);
			if(classInfo.getCategory () == startPoint.getPath ())
			{
				args.presentation.setIcon (classInfo.getIcon ());
				args.presentation.setThumbnail (System::GetPluginSnapshots ().getSnapshot (description->getClassID ()));
				args.presentation.setTitle (classInfo.getTitle ().isEmpty () ? classInfo.getName () : classInfo.getTitle ());

				args.resultCategory = getResultCategory ();
				args.sortString = PlugInSortMethods::getType (*description);
			}
		}
	return nullptr;
}

//************************************************************************************************
// HiddenPluginsFilter
//************************************************************************************************

HiddenPluginsFilter::HiddenPluginsFilter (IParameter* bypassParam, IObjectFilter* classFilter)
: bypassParam (bypassParam),
  classFilter (classFilter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HiddenPluginsFilter::matches (UrlRef url) const
{
	if(!bypassParam || !bypassParam->getValue ().asBool ())
	{
		UID cid;
		if(cid.fromString (url.getHostName ()))
			if(System::GetPluginPresentation ().isHidden (cid))
				return false;
	}

	if(classFilter && !classFilter->matches (ccl_const_cast (System::GetPlugInManager ().getClassDescription (url))))
		return false;

	return true;
}

//************************************************************************************************
// Browsable::PlugInCategoryNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInCategoryNode, SortedNode)
IBrowserNodeBranding* PlugInCategoryNode::branding = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCategoryNode::setBranding (IBrowserNodeBranding* _branding)
{
	branding = _branding;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClassNode* PlugInCategoryNode::findRegularPluginClassNode (UIDRef classID, FolderNode& parentFolder)
{
	AutoPtr<ObjectFilter> folderFilter (ObjectFilter::create ([] (IUnknown* obj)
	{
		auto node = unknown_cast<BrowserNode> (obj);
		return !ccl_cast<PlugInFavoritesNode> (node) && !ccl_cast<RecentPlugInsNode> (node);
	}));

	return findPluginClassNode (classID, parentFolder, folderFilter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClassNode* PlugInCategoryNode::findPluginClassNode (UIDRef classID, FolderNode& parentFolder, IObjectFilter* folderFilter)
{
	ForEach (parentFolder.getContent (), BrowserNode, node)
		if(auto classNode = ccl_cast<PlugInClassNode> (node))
		{
			if(classNode->getClassDescription ().getClassID () == classID)
				return classNode;
		}
		else if(auto folder = ccl_cast<FolderNode> (node))
		{
			// recursion
			if(!folderFilter || folderFilter->matches (node->asUnknown ()))
				if(PlugInClassNode* classNode = findPluginClassNode (classID, *folder, folderFilter))
					return classNode;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryNode::PlugInCategoryNode (StringRef category1, StringRef title, bool presetNode)
: SortedNode (title),
  category1 (category1),
  presetNode (presetNode),
  flags (0)
{
	init ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryNode::PlugInCategoryNode (StringRef category1, StringRef subCategory1, StringRef title, bool presetNode)
: SortedNode (title),
  category1 (category1),
  subCategory1 (subCategory1),
  presetNode (presetNode),
  flags (0)
{
	init ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCategoryNode::init ()
{
	if(isPresetNode ())
		SignalSource::addObserver (Signals::kPresetManager, this);

	SignalSource::addObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryNode::~PlugInCategoryNode ()
{
	if(isPresetNode ())
		SignalSource::removeObserver (Signals::kPresetManager, this);

	SignalSource::removeObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::getUniqueName (MutableCString& name)
{
	name = category1;
	if(!subCategory1.isEmpty ())
	{
		name += ":";
		name += subCategory1;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PlugInCategoryNode::getCustomBackground () const
{
	return CSTR ("plugincategory");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::getSubNodes (Container& children, NodeFlags flags)
{
	bool result = SuperClass::getSubNodes (children, flags);

	if(branding)
		branding->applyBranding (*this, children);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInCategoryNode::notify (ISubject* s, MessageRef msg)
{
	bool presetCreated = (msg == Signals::kPresetCreated);
	if(presetCreated || msg == Signals::kPresetRemoved)
	{
		CCL_PRINTF ("PlugInCategoryNode (%s): %s\n", MutableCString (getBrowser ()->getName ()).str (), msg.getID ().str ())
		UnknownPtr<IPreset> preset (msg[0]);
		if(preset)
		{
			if(IAttributeList* attribs = preset->getMetaInfo ())
			{
				UID classID;
				bool hasClassID = PresetMetaAttributes (*attribs).getClassID (classID);
				if(hasClassID)
				{
					auto updateClassNode = [&] (PlugInPresetNode* classNode)
					{
						if(classNode)
						{
							if(presetCreated)
								classNode->onPresetCreated (*preset);
							else
								classNode->onPresetRemoved (*preset);
						}
					};

					auto updateInExtraFolder = [&] (MetaClassRef metaClass)
					{
						// find folder node of given class in our childs
						if(BrowserNode* folder = findNode<BrowserNode> (AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* unk) { Object* obj = unknown_cast<Object> (unk); return obj && obj->canCast (metaClass); }))))
						{
							// find classNode inside it
							if(Browser* browser = getBrowser ())
								updateClassNode (browser->findNode<PlugInPresetNode> (AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
								{
									PlugInClassNode* classNode = unknown_cast<PlugInClassNode> (obj);
									return classNode && classNode->getClassDescription ().getClassID () == classID;
								})), folder));
						}
					};

					// 1. update in regular sorting structure
					updateClassNode (ccl_cast<PlugInPresetNode> (findRegularPluginClassNode (classID, *this)));

					// 2. update in extra folders
					if(hasFavoritesFolder ())
						updateInExtraFolder (ccl_typeid<PlugInFavoritesNode> ());

					if(hasRecentFolder ())
						updateInExtraFolder (ccl_typeid<RecentPlugInsNode> ());
				}
			}
		}
	}
	else if(msg == Signals::kPresetsRefreshed)
	{
		if(Browser* browser = getBrowser ())
			browser->refreshAll (true);
	}
	else if(msg == Signals::kClassCategoryChanged || msg == Signals::kPluginPresentationChanged)
	{
		BrowserNode* nodeToRefresh = this;
		bool mustRefresh = true;

		if(msg == Signals::kClassCategoryChanged)
		{
			String category (msg[0].asString ());
			mustRefresh = category == category1 || category == category2;
		}
		else if(msg.getArgCount () > 0)
		{
			MutableCString changeType (msg[0].asString ());
			if(changeType == IPluginPresentation::kAttributeChanged)
				mustRefresh = false;
			else if(changeType == IPluginPresentation::kUsageChanged)
			{
				// only if category matches
				String category (msg[1].asString ());
				mustRefresh = category == category1 || category == category2;
				if(mustRefresh)
				{
					// no need to refresh all when only the lastUsage of a plug-in has changed - only refresh the RecentPlugInsNode if necessary
					RecentPlugInsNode* recentNode = nullptr;
					if(hasRecentFolder ())
					{
						AutoPtr<IRecognizer> r = Recognizer::create ([] (IUnknown* obj) { return unknown_cast<RecentPlugInsNode> (obj) != nullptr; });
						recentNode = findNode<RecentPlugInsNode> (r);
					}

					Browser* browser = getBrowser ();
					if(recentNode && browser && browser->wasExpanded (recentNode))
						nodeToRefresh = recentNode;
					else
						mustRefresh = false;
				}
			}
		}

		if(mustRefresh)
		{
			if(Browser* browser = getBrowser ())
			{
				ScopedVar<bool> scope (browser->getRestoringState (), true); // force getPresets immediately (not in background) to avoid loosing preset folder expand states
				browser->refreshNode (nodeToRefresh, true);
			}
		}
	}
	SuperClass::notify (s, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PlugInCategoryNode::createSubNode (const IClassDescription& description)
{
	// no presets in edit mode, except in ListView
	bool withPresets = isPresetNode () && !isEditMode ();
	if(!withPresets)
	{
		Browser* browser = getBrowser ();
		if(browser && browser->isListMode ())
			withPresets = true;
	}

	PlugInClassNode* classNode = nullptr;
	if(withPresets)
	{
		auto node = NEW PlugInPresetNode (description);
		node->hasPresetFavoritesFolder (hasPresetFavoritesFolder ());
		classNode = node;
	}
	else
		classNode = NEW PlugInClassNode (description);

	if(isPresetNode ())
		classNode->dragAsPreset (true); // but drag plugin as preset in edit mode
	
	classNode->canEditPresentation (canEditPresentation ());
	classNode->isEditMode (isEditMode ());
	return classNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::isSortByUserFolder () const
{
	return sorter && sorter->getTag () == kUserFolderSorterTag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* PlugInCategoryNode::newFolder (StringRef title)
{
	if(isSortByUserFolder ())
		return NEW PlugInSortFolderNode (title);

	return SuperClass::newFolder (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	auto sortFolder = ccl_cast<PlugInSortFolderNode> (parentFolder);
	return sortFolder ? System::GetPluginPresentation ().hasSortFolder (getCategory1 (), sortFolder->getSortPath ()) : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::matchesFilter (const IClassDescription& description, bool checkHiddenState) const
{
	if(checkHiddenState)
		if(System::GetPluginPresentation ().isHidden (description.getClassID ()))
			return false;

	return classFilter == nullptr || classFilter->matches (const_cast<IClassDescription*> (&description));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::matchesFilter (const IClassDescription& description) const
{
	bool checkHiddenState = !isEditMode (); // show all plugins in edit mode to allow comeback
	return matchesFilter (description, checkHiddenState);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::matches (const IClassDescription& description, bool checkHiddenState) const
{
	// check filter (and hidden state)
	if(matchesFilter (description, checkHiddenState))
	{
		// check categories
		if(description.getCategory () == category1)
			if(subCategory1.isEmpty () || description.getSubCategory () == subCategory1)
				return true;

		if(!category2.isEmpty () && description.getCategory () == category2)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCategoryNode::build ()
{
	if(hasFavoritesFolder ())
		add (NEW PlugInFavoritesNode);

	if(hasRecentFolder ())
		add (NEW RecentPlugInsNode);

	if(hasFavoritesFolder () || hasRecentFolder ())
	{
		// add separator, but not in icon mode of ListView
		Browser* browser = getBrowser ();
		if(!(browser && browser->isListMode () && browser->getListViewType () == Styles::kListViewIcons))
		{
			SeparatorNode* separator = NEW SeparatorNode;
			separator->setCustomBackground (CSTR ("pluginseparator"));
			add (separator);
		}
	}

	if(subCategory1.isEmpty ())
	{
		ForEachPlugInClass (category1, description)
			if(matchesFilter (description))
				addSorted (createSubNode (description));
		EndFor
	}
	else
	{
		ForEachPlugInClass (category1, description)
			if(description.getSubCategory () == subCategory1 && matchesFilter (description))
				addSorted (createSubNode (description));
		EndFor
	}

	if(!category2.isEmpty ())
	{
		ForEachPlugInClass (category2, description)
			if(matchesFilter (description))
				addSorted (createSubNode (description));
		EndFor
	}

	if(isSortByUserFolder ())
	{
		// get additional (empty) folders
		AutoPtr<IUnknownIterator> iterator (System::GetPluginPresentation ().getSortFolders (category1));
		if(iterator)
			addSubFolders (*iterator);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCategoryNode::signalPresentationChanged (bool deferred)
{
	if(deferred)
		SignalSource (Signals::kPlugIns).deferSignal (NEW Message (Signals::kPluginPresentationChanged));
	else
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PlugInCategoryNode::sortNodesIntoFolder (const IUnknownList& data, IDragSession* session, StringRef sortPath)
{
	DragDataExtractor dataExtractor;
	dataExtractor.construct<PluginDraghandler> (data, session);

	ForEachUnknown (dataExtractor, unk)
		if(PlugInClass* pluginClass = unknown_cast<PlugInClass> (unk))
			System::GetPluginPresentation ().setSortPath (pluginClass->getClassID (), sortPath);
		else if(PlugInSortFolderNode* folderNode = unknown_cast<PlugInSortFolderNode> (unk))
		{
			String oldPath, newPath;
			if(folderNode->prepareMoveIntoFolder (oldPath, newPath, sortPath))
				System::GetPluginPresentation ().moveSortFolder (getCategory1 (), oldPath, newPath);
		}
	EndFor

	signalPresentationChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::onNewFolder (BrowserNode* focusNode, bool checkOnly)
{
	if(!canEditPresentation () || !isSortByUserFolder ())
		return false;

	if(!checkOnly)
	{
		String newPath;
		if(CustomSortFolderNode::askNewFolder (newPath, focusNode, ccl_typeid<PlugInSortFolderNode> ()))
		{
			System::GetPluginPresentation ().addSortFolder (getCategory1 (), newPath);
			signalPresentationChanged ();

			CustomSortFolderNode::setFocusNode (*this, newPath);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInCategoryNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	AutoPtr<PluginDraghandler> dragHandler (NEW PluginDraghandler (targetView, getBrowser ()));
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInCategoryNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return sortNodesIntoFolder (data, session, String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInCategoryNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	if(canEditPresentation ())
	{
		if(isSortByUserFolder ())
		{
			contextMenu.addSeparatorItem ();
			contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
		}

		contextMenu.addSeparatorItem ();
		contextMenu.addCommandItem (XSTR (Reset), CSTR ("Browser"), CSTR ("Reset"), nullptr);
		contextMenu.addCommandItem (XSTR (Revert), CSTR ("Browser"), CSTR ("Revert"), nullptr);
	}
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser")
	{
		if(msg.name == "New Folder")
			return onNewFolder (this, msg.checkOnly ());

		if(msg.name == "Reset")
		{
			if(!msg.checkOnly ())
				if(Alert::ask (XSTR (AskReset), Alert::kYesNo) == Alert::kYes)
				{
					System::GetPluginPresentation ().reset ();
					signalPresentationChanged ();
				}

			return true;
		}
		if(msg.name == "Revert")
		{
			if(!msg.checkOnly ())
				if(Alert::ask (XSTR (AskRevert), Alert::kYesNo) == Alert::kYes)
				{
					System::GetPluginPresentation ().revert ();
					signalPresentationChanged ();
				}
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// Browsable::PlugInSortFolderRenamer
//************************************************************************************************

class PlugInSortFolderRenamer: public SortFolderRenamerBase
{
public:
	PlugInSortFolderRenamer (CustomSortFolderNode& node)
	: SortFolderRenamerBase (node)
	{
		if(auto categoryNode = node.getAncestorNode<PlugInCategoryNode> ())
			category = categoryNode->getCategory1 ();
	}

	// SortFolderRenamerBase
	bool renameFolderInternal (String oldPath, StringRef newName) const override
	{
		System::GetPluginPresentation ().renameSortFolder (category, oldPath, newName);
		PlugInCategoryNode::signalPresentationChanged ();
		return true;
	}

	bool hasSortFolderInternal (StringRef newPath) const override
	{
		return System::GetPluginPresentation ().hasSortFolder (category, newPath) != 0;
	}

protected:
	String category;
};

//************************************************************************************************
// Browsable::PlugInFavoritesFolderRenamer
//************************************************************************************************

class PlugInFavoritesFolderRenamer: public PlugInSortFolderRenamer
{
public:
	using PlugInSortFolderRenamer::PlugInSortFolderRenamer;

	// SortFolderRenamerBase
	bool renameFolderInternal (String oldPath, StringRef newName) const override
	{
		System::GetPluginPresentation ().renameFavoriteFolder (category, oldPath, newName);
		PlugInCategoryNode::signalPresentationChanged ();
		return true;
	}

	bool hasSortFolderInternal (StringRef newPath) const override
	{
		return System::GetPluginPresentation ().hasFavoriteFolder (category, newPath) != 0;
	}
};

//************************************************************************************************
// Browsable::PlugInSortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInSortFolderNode, CustomSortFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSortFolderNode::PlugInSortFolderNode (StringRef title)
: CustomSortFolderNode (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* PlugInSortFolderNode::createFolderRenamer ()
{
	return NEW PlugInSortFolderRenamer (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSortFolderNode::createNewFolder (bool checkOnly)
{
	auto categoryNode = getAncestorNode<PlugInCategoryNode> ();
	return categoryNode ? categoryNode->onNewFolder (this, checkOnly) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSortFolderNode::removeFolders (NodeRemover& remover, Container& folderNodes)
{
	if(PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		for(auto obj : folderNodes)
			if(auto node = ccl_cast<PlugInSortFolderNode> (obj))
			{
				String path;
				node->getSortPath (path);
				System::GetPluginPresentation ().removeSortFolder (categoryNode->getCategory1 (), path);
					
				remover.keepNode (node); // (will be removed from browser via PresetMananger signal)
			}

		PlugInCategoryNode::signalPresentationChanged (true); // deferred, after nodes have been removed
		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandler* PlugInSortFolderNode::createDragHandler (IView* targetView)
{
	return NEW PluginDraghandler (targetView, getBrowser ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSortFolderNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		String sortPath;
		getSortPath (sortPath);
		return categoryNode->sortNodesIntoFolder (data, session, sortPath);
	}
	return false;
}

//************************************************************************************************
// Browsable::PlugInFavoritesNode::Sorter
//************************************************************************************************

class PlugInFavoritesNode::Sorter: public NodeSorter
{
public:
	// NodeSorter
	bool getSortPath (String& path, const BrowserNode* node) override;
};

//************************************************************************************************
// Browsable::PlugInFavoritesNode::Sorter
//************************************************************************************************

bool PlugInFavoritesNode::Sorter::getSortPath (String& path, const BrowserNode* node)
{
	if(auto plugNode = ccl_cast<PlugInClassNode> (node))
	{
		path = System::GetPluginPresentation ().getFavoriteFolder (plugNode->getClassDescription ().getClassID ());
		return true;
	}
	return false;
}

//************************************************************************************************
// Browsable::PlugInFavoritesNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInFavoritesNode, SortedNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInFavoritesNode::PlugInFavoritesNode ()
: SortedNode (BrowserStrings::strFavorites ())
{
	setIcon (RootComponent::instance ().getTheme ()->getImage ("FolderIcon:FavoritesFolder"));
	setSorter (AutoPtr<Sorter> (NEW Sorter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInFavoritesNode::compare (const Object& obj) const
{
	// sort before folders & plugins
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInFavoritesNode::build ()
{
	if(auto categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		ForEachPlugInClass (categoryNode->getCategory1 (), description)
			if(!categoryNode->matchesFilter (description))
				continue;

			if(System::GetPluginPresentation ().isFavorite (description.getClassID ())
				&& (categoryNode->isEditMode () || !System::GetPluginPresentation ().isHidden (description.getClassID ()))) // hidden vs favorite: hide!
			{
				BrowserNode* plugNode = categoryNode->createSubNode (description);
				addSorted (plugNode);
			}
		EndFor

		// get additional (empty) folders
		AutoPtr<IUnknownIterator> iterator (System::GetPluginPresentation ().getFavoriteFolders (categoryNode->getCategory1 ()));
		if(iterator)
			addSubFolders (*iterator);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* PlugInFavoritesNode::newFolder (StringRef title)
{
	return NEW FavoritesSortFolderNode (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInFavoritesNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	auto categoryNode = getAncestorNode<PlugInCategoryNode> ();
	auto sortFolder = ccl_cast<FavoritesSortFolderNode> (parentFolder);
	return (categoryNode && sortFolder) ? !System::GetPluginPresentation ().hasFavoriteFolder (categoryNode->getCategory1 (), sortFolder->getSortPath ()) : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInFavoritesNode::canEditPresentation () const
{
	PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ();
	return categoryNode && categoryNode->canEditPresentation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInFavoritesNode::onNewFolder (BrowserNode* focusNode, bool checkOnly)
{
	if(!checkOnly)
	{
		if(auto categoryNode = getAncestorNode<PlugInCategoryNode> ())
		{
			String newPath;
			if(CustomSortFolderNode::askNewFolder (newPath, focusNode, ccl_typeid<FavoritesSortFolderNode> ()))
			{
				SharedPtr<BrowserNode> holder (this);

				System::GetPluginPresentation ().addFavoriteFolder (categoryNode->getCategory1 (), newPath);
				categoryNode->signalPresentationChanged ();

				CustomSortFolderNode::setFocusNode (*this, newPath);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PlugInFavoritesNode::sortNodesIntoFolder (const IUnknownList& data, IDragSession* session, StringRef sortPath)
{
	if(auto categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		DragDataExtractor dataExtractor;
		dataExtractor.construct<PluginDraghandler> (data, session);

		ForEachUnknown (dataExtractor, unk)
			if(auto pluginClass = unknown_cast<PlugInClass> (unk))
				System::GetPluginPresentation ().setFavorite (pluginClass->getClassID (), true, sortPath);
			else if(auto folderNode = unknown_cast<FavoritesSortFolderNode> (unk))
			{
				String oldPath, newPath;
				if(folderNode->prepareMoveIntoFolder (oldPath, newPath, sortPath))
					System::GetPluginPresentation ().moveFavoriteFolder (categoryNode->getCategory1 (), oldPath, newPath);
			}
		EndFor

		categoryNode->signalPresentationChanged ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInFavoritesNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	AutoPtr<PluginDraghandler> dragHandler (NEW PluginDraghandler (targetView, getBrowser ()));
	if(dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInFavoritesNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return sortNodesIntoFolder (data, session, String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInFavoritesNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	if(canEditPresentation ())
		contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);

	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInFavoritesNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser" && canEditPresentation ())
	{
		if(msg.name == "New Folder")
			return onNewFolder (this, msg.checkOnly ());
	}
	return false;
}

//************************************************************************************************
// Browsable::FavoritesSortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FavoritesSortFolderNode, CustomSortFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

FavoritesSortFolderNode::FavoritesSortFolderNode (StringRef title)
: CustomSortFolderNode (title)
{
	if(IImage* icon = RootComponent::instance ().getTheme ()->getImage ("FolderIcon:FavoritesSortFolder"))
		setIcon (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* FavoritesSortFolderNode::createFolderRenamer ()
{
	return NEW PlugInFavoritesFolderRenamer (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FavoritesSortFolderNode::createNewFolder (bool checkOnly)
{
	auto favoritesNode = getAncestorNode<PlugInFavoritesNode> ();
	return favoritesNode ? favoritesNode->onNewFolder (this, checkOnly) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FavoritesSortFolderNode::removeFolders (NodeRemover& remover, Container& folderNodes)
{
	if(auto categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		for(auto obj : folderNodes)
			if(auto node = ccl_cast<FavoritesSortFolderNode> (obj))
			{
				String path;
				node->getSortPath (path);
				System::GetPluginPresentation ().removeFavoriteFolder (categoryNode->getCategory1 (), path);
					
				remover.removeNode (node);
			}

		PlugInCategoryNode::signalPresentationChanged (true); // deferred, after nodes have been removed
		return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FavoritesSortFolderNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(auto favoritesNode = getAncestorNode<PlugInFavoritesNode> ())
	{
		String sortPath;
		getSortPath (sortPath);
		return favoritesNode->sortNodesIntoFolder (data, session, sortPath);
	}
	return false;
}

//************************************************************************************************
// Browsable::RecentPlugInsNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RecentPlugInsNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

RecentPlugInsNode::RecentPlugInsNode ()
: BrowserNode (XSTR (Recent))
{
	setIcon (RootComponent::instance ().getTheme ()->getImage ("FolderIcon:RecentFolder"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int RecentPlugInsNode::compare (const Object& obj) const
{
	// sort before folders & plugins
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentPlugInsNode::getSubNodes (Container& children, NodeFlags flags)
{
	if(PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ())
	{
		struct UsedPlugin
		{
			const IClassDescription* description;
			int64 lastUsage;

			UsedPlugin (const IClassDescription* description = nullptr, int64 lastUsage = 0)
			: description (description), lastUsage (lastUsage)
			{}

			bool operator > (const UsedPlugin& other) const
			{
				return lastUsage < other.lastUsage;
			}
		};

		enum { kMaxRecentPlugins = 10 };
		FixedSizeVector<UsedPlugin, kMaxRecentPlugins> recent;

		ForEachPlugInClass (categoryNode->getCategory1 (), description)
			if(!categoryNode->matchesFilter (description))
				continue;

			int64 usage = System::GetPluginPresentation ().getLastUsage (description.getClassID ());
			if(usage > 0
				&& (categoryNode->isEditMode () || !System::GetPluginPresentation ().isHidden (description.getClassID ()))) // hidden vs recent: hide!
			{
				if(recent.count () < kMaxRecentPlugins)
					recent.addSorted (UsedPlugin (&description, usage));
				else if(usage > recent.last ().lastUsage) // newer than oldest
				{
					recent.removeLast ();
					recent.addSorted (UsedPlugin (&description, usage));
				}
			}
		EndFor

		VectorForEachFast (recent, UsedPlugin, usedPlugin)
			BrowserNode* child = categoryNode->createSubNode (*usedPlugin.description);
			child->setParent (this);
			children.add (child);
		EndFor
	}
	return true;
}

//************************************************************************************************
// Browsable::PlugInClassNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInClassNode, SortedNode)
DEFINE_STRINGID_MEMBER_ (PlugInClassNode, kVisible, "visible")
DEFINE_STRINGID_MEMBER_ (PlugInClassNode, kFavorite, "favorite")

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClassNode::PlugInClassNode (const IClassDescription& _description)
: description (nullptr),
  flags (0)
{
	_description.clone (description);

	// prefer localized name if available
	String className;
	_description.getLocalizedName (className);
	setTitle (className);

	setIcon (PlugInClass (_description).getIcon ());
	setThumbnail (System::GetPluginSnapshots ().getSnapshot (_description.getClassID ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClassNode::~PlugInClassNode ()
{
	if(description)
		description->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription& CCL_API PlugInClassNode::getClassDescription () const
{
	ASSERT (description != nullptr)
	return *description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClassNode::getUniqueName (MutableCString& name)
{
	UID (description->getClassID ()).toCString (name);
	name += ".";
	name.append (title, Text::kUTF8);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID PlugInClassNode::getCustomBackground () const
{
	return CSTR ("plugin");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClassNode::hasSubNodes () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInClassNode::compare (const Object& obj) const
{
	// sort folder nodes before plugins
	if(ccl_cast<PlugInSortFolderNode> (&obj) || ccl_cast<FavoritesSortFolderNode> (&obj))
		return 1;

	return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PlugInClassNode::createDragObject ()
{
	ASSERT (description != nullptr)
	if(dragAsPreset ())
		return ccl_as_unknown (NEW ObjectPreset (description));
	else
		return return_shared (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClassNode::drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment)
{
	if(id == kVisible)
	{
		if(canEditPresentation ())
		{
			bool isVisible = !System::GetPluginPresentation ().isHidden (description->getClassID ());
			if(IImage* icon = info.view->getVisualStyle ().getImage ("VisibleIcon"))
				ItemModelPainter ().drawButtonImage (info, icon, isVisible);
		}
		return true;
	}
	else if(id == kFavorite)
	{
		bool isFavorite = System::GetPluginPresentation ().isFavorite (description->getClassID ()) != 0;
		if(isFavorite || isEditMode ()) // draw "off" state only in edit mode
			if(IImage* icon = info.view->getVisualStyle ().getImage ("FavoriteIcon"))
				ItemModelPainter ().drawButtonImage (info, icon, isFavorite);
		
		return true;
	}

	if(id == nullptr && System::GetPluginPresentation ().isHidden (description->getClassID ()))
	{
		// draw hidden plugin title disabled (edit mode)
		ItemModelPainter ().drawTitle (info, getTitle (), false);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClassNode::onEdit (StringID id, const IItemModel::EditInfo& info)
{
	if(isEditMode ())
	{
		if(id == kVisible)
		{
			// toggle hidden state
			tbool isHidden = System::GetPluginPresentation ().isHidden (description->getClassID ());

			swipeNodes (this, info.view, info.editEvent, [isHidden] (PlugInClassNode* classNode)
			{
				if(classNode)
					System::GetPluginPresentation ().setHidden (classNode->getClassDescription ().getClassID (), !isHidden);
			});

			return true;
		}
		else if(id == kFavorite)
		{
			// toggle favorite state
			tbool isFavorite = System::GetPluginPresentation ().isFavorite (description->getClassID ());

			swipeNodes (this, info.view, info.editEvent, [isFavorite] (PlugInClassNode* classNode)
			{
				if(classNode)
					System::GetPluginPresentation ().setFavorite (classNode->getClassDescription ().getClassID (), !isFavorite);
			});

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInClassNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	bool canAddFolder = false;
	if(canEditPresentation ())
	{
		contextMenu.addSeparatorItem ();
		contextMenu.addCommandItem (BrowserStrings::strFavorite (), CSTR ("Browser"), CSTR ("Set Favorite"), nullptr);
		contextMenu.addCommandItem (XSTR (Hide), CSTR ("Browser"), CSTR ("Set Visible"), nullptr);

		PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ();
		if(categoryNode && categoryNode->isSortByUserFolder ())
			canAddFolder = true;
	}

	if(!canAddFolder && getAncestorNode<PlugInFavoritesNode> ())
		canAddFolder = true;

	if(canAddFolder)
	{
		contextMenu.addSeparatorItem ();
		contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
	}
	
	// only user thumbnails can be removed
	if(getThumbnail () && System::GetPluginSnapshots ().hasUserSnapshot (description->getClassID ()))
	{
		contextMenu.addSeparatorItem ();
		contextMenu.addCommandItem (XSTR (DeleteThumbnail), CSTR ("Browser"), CSTR ("Delete Thumbnail"), nullptr);
	}
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClassNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser" && canEditPresentation ())
	{
		if(msg.name == "Set Favorite")
		{
			tbool isFavorite = System::GetPluginPresentation ().isFavorite (description->getClassID ());
			if(msg.checkOnly ())
			{
				UnknownPtr<IMenuItem> menuItem (msg.invoker);
				if(menuItem)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, isFavorite);
			}
			else
			{
				Browser::visitEditNodes<PlugInClassNode> (this, selectedNodes, [&] (PlugInClassNode& classNode)
				{
					System::GetPluginPresentation ().setFavorite (classNode.getClassDescription ().getClassID (), !isFavorite);
				});

				PlugInCategoryNode::signalPresentationChanged ();
			}
			return true;
		}
		else if(msg.name == "Set Visible")
		{
			tbool isHidden = System::GetPluginPresentation ().isHidden (description->getClassID ());
			if(msg.checkOnly ())
			{
				UnknownPtr<IMenuItem> menuItem (msg.invoker);
				if(menuItem)
					menuItem->setItemAttribute (IMenuItem::kItemChecked, isHidden);
			}
			else
			{
				Browser::visitEditNodes<PlugInClassNode> (this, selectedNodes, [&] (PlugInClassNode& classNode)
				{
					System::GetPluginPresentation ().setHidden (classNode.getClassDescription ().getClassID (), !isHidden);
				});

				PlugInCategoryNode::signalPresentationChanged ();
			}
			return true;
		}
		else if(msg.name == "New Folder")
		{
			// depending on the context (parent), we must create a favorites folder or sort folder for this plug-in
			if(auto favoritesNode = getAncestorNode<PlugInFavoritesNode> ())
				return favoritesNode->onNewFolder (this, msg.checkOnly ());
			else if(PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ())
				return categoryNode->onNewFolder (this, msg.checkOnly ());
		}
		else if(msg.name == "Delete Thumbnail")
		{
			if(!msg.checkOnly ())
				System::GetPluginSnapshots ().setUserSnapshot (description->getClassID (), nullptr);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInClassNode::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "classDescription")
	{
		var.takeShared (description);
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// Browsable::PlugInPresetNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInPresetNode, PlugInClassNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInPresetNode::PlugInPresetNode (const IClassDescription& description)
: PlugInClassNode (description),
  builder (description)
{
	AutoPtr<PresetNodeSorter> sorter (NEW PresetNodeSorter);
	setSorter (sorter);

	builder.addObserver (this);
	SignalSource::addObserver (Signals::kPresetManager, this);

	setFolderBackground ("presetfolder");
	dragAsPreset (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInPresetNode::~PlugInPresetNode ()
{
	builder.removeObserver (this);
	SignalSource::removeObserver (Signals::kPresetManager, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PlugInPresetNode::getPresetMetaInfo () const
{
	return builder.getMetaInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInPresetNode::onRefresh ()
{
	builder.resetPresets ();
	return SuperClass::onRefresh ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInPresetNode::hasSubNodes () const
{
	return !content.isEmpty () || builder.hasPresets (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInPresetNode::hasPresetFavoritesFolder (bool state)
{
	builder.hasFavoritesFolder (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInPresetNode::supportsFavorites () const
{
	return builder.hasFavoritesFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInPresetNode::getPresetClassKey () const
{
	return builder.getClassKey ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNodesBuilder& PlugInPresetNode::getPresetNodesBuilder ()
{
	return builder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInPresetNode::build ()
{
	builder.buildNodes (*this);
	
	if(!builder.hasPresetsPending ())
		if(PlugInCategoryNode* categoryNode = getAncestorNode<PlugInCategoryNode> ())
			categoryNode->onPluginNodeReady (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* PlugInPresetNode::newFolder (StringRef title)
{
	return NEW PresetSortFolderNode (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInPresetNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	auto sortFolder = ccl_cast<PresetSortFolderNode> (parentFolder);
	return !(sortFolder && builder.hasSortFolder (sortFolder->getSortPath ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInPresetNode::drawIconOverlay (const IItemModel::DrawInfo& info)
{
	if(builder.hasPresetsPending ())
		builder.drawPresetsPending (info);
	else
		SuperClass::drawIconOverlay (info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInPresetNode::onPresetCreated (IPreset& preset)
{
	builder.onPresetCreated (preset, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInPresetNode::onPresetRemoved (IPreset& preset)
{
	builder.onPresetRemoved (preset, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInPresetNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInPresetNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	return PresetSortFolderNode::sortNodesIntoFolder (data, String::kEmpty, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInPresetNode::notify (ISubject* s, MessageRef msg)
{
	if(s == &builder)
	{
		if(msg == PresetNodesBuilder::kPresetsChanged)
		{
			if(Browser* browser = getBrowser ())
				browser->refreshNode (this);
		}
		else if(msg == kChanged)
		{
			if(Browser* browser = getBrowser ())
				browser->redrawNode (this);
		}
	}
	else if(msg == Signals::kPresetSubFolderAdded || msg == Signals::kPresetSubFolderRemoved)
	{
		builder.onPresetSubFoldersChanged (msg, *this);
	}
	else if(msg == Signals::kPresetFavoritesChanged && supportsFavorites ())
	{
		String classId (msg[0].asString ());
		String folderPath (msg.getArgCount () > 1 ? msg[1].asString () : String::kEmpty);
		builder.onPresetFavoritesChanged (classId, *this, folderPath);
	}
	else
		SuperClass::notify (s, msg);
}
