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
// Filename    : ccl/app/browser/browsernode.cpp
// Description : Browser Node
//
//************************************************************************************************

#include "ccl/app/browser/browsernode.h"
#include "ccl/app/browser/browser.h"
#include "ccl/app/browser/nodesorter.h"

#include "ccl/app/utilities/fileoperations.h"
#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/sortfolderlist.h"

#include "ccl/app/controls/draghandler.h"

#include "ccl/base/kernel.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/text/stringbuilder.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Browser")
	XSTRING (DoYouWantToRemoveThisFolder, "Do you want to remove this folder?")
	XSTRING (DoYouWantToRemoveTheseFolders, "Do you want to remove these folders?")
	XSTRING (Rename, "Rename")
	XSTRING (Favorites, "Favorites")
	XSTRING (Favorite, "Favorite")
	XSTRING (AddToFavorites, "Add to Favorites")
END_XSTRINGS

StringRef BrowserStrings::strFavorites ()		{ return XSTR (Favorites); }
StringRef BrowserStrings::strFavorite ()		{ return XSTR (Favorite); }
StringRef BrowserStrings::strAddToFavorites ()	{ return XSTR (AddToFavorites); }

//************************************************************************************************
// BrowserNode
//************************************************************************************************

DEFINE_CLASS (BrowserNode, TreeViewNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode::BrowserNode (StringRef title, BrowserNode* parent)
: TreeViewNode (title),
  parent (parent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode::~BrowserNode ()
{
	CCL_PRINTF ("~BrowserNode %x (%s)\n", (int64)this, MutableCString (title).str ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString CCL_API BrowserNode::getNodeType () const
{
	return CString (myClass ().getPersistentName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::isNodeType (StringID typeName) const
{
	if(typeName == "IBrowserNode")
		return true;

	const MetaClass* type = Kernel::instance ().getClassRegistry ().findType (typeName);
	return type ? canCast (*type) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BrowserNode::compare (const Object& obj) const
{
	if(const BrowserNode* node = ccl_cast<BrowserNode> (&obj))
	{
		// folder nodes before others
		bool meFolder = isClass (ccl_typeid<FolderNode> ());
		bool heFolder = node->isClass (ccl_typeid<FolderNode> ()) || node->isClass (ccl_typeid<SortFolderNode> ());
		if(meFolder != heFolder)
			return meFolder ? -1 : 1;

		return compareTitle (*node);
	}
	return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* BrowserNode::getThemeIcon (StringID name)
{
	Browser* browser = getBrowser ();
	ITheme* theme = browser ? browser->getTheme () : nullptr;
	ASSERT (theme)

	IImage* icon = theme ? theme->getImage (name) : nullptr;
	if(icon == nullptr)
	{
		ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();
		if(theme2 && theme2 != theme)
			icon = theme2->getImage (name);
	}

	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Browser* BrowserNode::getBrowser () const
{
	return parent ? parent->getBrowser () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::getUniqueName (MutableCString& name)
{
	name.empty ();
	name.append (title, Text::kUTF8);
	return !name.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::hasAncestor (BrowserNode* ancestor) const
{
	// check if ancestor is a (grand)parent of this
	const BrowserNode* parent = this;
	while((parent = parent->getParent ()))
		if(parent == ancestor)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const BrowserNode* BrowserNode::getAncestorNode (MetaClassRef classID) const
{
	const BrowserNode* parent = this;
	while(parent = parent->getParent ())
		if(parent->canCast (classID))
			return parent;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const BrowserNode* BrowserNode::getAncestorNodeWithInterface (UIDRef iid) const
{
	BrowserNode* parent = ccl_const_cast (this);
	while(parent = parent->getParent ())
	{
		IUnknown* iface = nullptr;
		parent->queryInterface (iid, (void**)&iface);
		if(iface)
		{
			iface->release ();
			return parent;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::drawDetailString (const IItemModel::DrawInfo& info, StringRef string, AlignmentRef alignment)
{
	info.graphics.drawString (info.rect, string, info.style.font, info.style.textBrush, alignment);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::drawDetailString (const IItemModel::DrawInfo& info, StringRef string, BrushRef textBrush, AlignmentRef alignment)
{
	info.graphics.drawString (info.rect, string, info.style.font, textBrush, alignment);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::drawDetailLines (const IItemModel::DrawInfo& info, String strings[], int count, BrushRef textBrush, AlignmentRef alignment)
{
	if(count <= 0)
		return true;

	Rect cellRect (info.rect);
	cellRect.contract (2);

	const IVisualStyle& style = ViewBox (info.view).getVisualStyle ();
	Font detailFont = style.getFont ("detailFont");

	int lineHeight = cellRect.getHeight () / count;
	for(int line = 0; line < count; line++)
	{
		Rect rect (0, 0, cellRect.getWidth (), lineHeight);
		rect.offset (cellRect.left, cellRect.top + line * lineHeight);
		info.graphics.drawString (rect, strings[line], detailFont, textBrush, alignment);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::onRefresh ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserNode::onNodeRemoved (BrowserNode* node)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::performRemoval (NodeRemover& remover)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::onOpen (bool deferred)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::onEdit (StringID id, const IItemModel::EditInfo& info)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* BrowserNode::getSearchProvider ()
{
	if(parent)
		return parent->getSearchProvider ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BrowserNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::checkCommandCategory (CStringRef category) const
{
	// don't need to overwrite this method if we always return true
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::interpretCommand (const CommandMsg& msg)
{
	return interpretCommand (msg, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "nodeType")
	{
		String type (getNodeType ());
		var = type;
		var.share ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::setProperty (MemberID propertyId, const Variant& var)
{
	tbool result = SuperClass::setProperty (propertyId, var);
	if(result)
	{
		// TEST: refresh node display when icon or title changed
		if(propertyId == kTitleProperty || propertyId == kIconProperty)
			if(Browser* browser = getBrowser ())
				browser->redrawNode (this);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (BrowserNode)
	DEFINE_METHOD_NAME ("isNodeType")
END_METHOD_NAMES (BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserNode::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "isNodeType")
	{
		MutableCString type (msg[0].asString ());
		returnValue = isNodeType (type);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// FolderNode
//************************************************************************************************

DEFINE_CLASS (FolderNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderNode::FolderNode (StringRef title, BrowserNode* parent)
: BrowserNode (title, parent)
{
	content.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderNode::~FolderNode ()
{
	for(auto node : iterate_as<BrowserNode> (content))
		node->setParent (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderNode::add (BrowserNode* node)
{
	ASSERT (node != nullptr && node->getParent () == nullptr)
	node->setParent (this);
	content.add (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderNode::addSorted (BrowserNode* node)
{
	ASSERT (node != nullptr && node->getParent () == nullptr)
	node->setParent (this);
	content.addSorted (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderNode::insertAt (int index, BrowserNode* node)
{
	ASSERT (node != nullptr && node->getParent () == nullptr)
	node->setParent (this);
	if(!content.insertAt (index, node))
		content.add (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderNode::remove (BrowserNode* node)
{
	ASSERT (node != nullptr && node->getParent () == this)
	if(content.remove (node))
	{
		node->setParent (nullptr);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderNode::removeAll ()
{
	content.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderNode::onNodeRemoved (BrowserNode* node)
{
	if(remove (node))
		node->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderNode::getSubNodes (Container& children, NodeFlags flags)
{
	if(flags.wantAll ())
		children.add (content, Container::kShare);
	else
	{
		ArrayForEach (content, BrowserNode, node)
			if(flags.shouldAdd (node->isFolder ()))
				children.add (return_shared (node));
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderNode* FolderNode::getFolder (StringRef title)
{
	ForEach (content, BrowserNode, node)
		FolderNode* folder = ccl_cast<FolderNode> (node);
			if(folder && folder->getTitle ().compare (title, false) == Text::kEqual)
				return folder;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FolderNode::findNode (const IRecognizer* recognizer) const
{
	ForEach (content, BrowserNode, node)
		if(recognizer->recognize (node->asUnknown ()))
			return node->asUnknown ();
		else if(FolderNode* folder = ccl_cast<FolderNode> (node))
			if(IUnknown* found = folder->findNode (recognizer))
				return found;
	EndFor
	return nullptr;
}

//************************************************************************************************
// FlatFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FlatFolderNode, FolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

FlatFolderNode::FlatFolderNode (StringRef title, BrowserNode* parent)
: FolderNode (title, parent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlatFolderNode::getSubNodes (Container& children, NodeFlags flags)
{
	// skip our child nodes, let them add their childs
	ArrayForEach (content, BrowserNode, node)
		node->getSubNodes (children, flags);
	EndFor
	return true;
}

//************************************************************************************************
// SortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SortFolderNode, FolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode::SortFolderNode (StringRef title)
: FolderNode (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SortFolderNode::getSortName () const
{
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SortFolderNode::compare (const Object& obj) const
{
	const SortFolderNode* otherSortFolder = ccl_cast<SortFolderNode> (&obj);
	if(otherSortFolder)
		return compareTitle (*otherSortFolder);
	else
		return -obj.compare (*this); // ask the other node, we don't know what nodes should be before or after us
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortFolderNode::onNodeRemoved (BrowserNode* node)
{
	if(remove (node))
	{
		node->release ();

		// remove empty parent folders
		if(content.isEmpty ())
			if(SortedNode* sortedNode = getAncestorNode<SortedNode> ())
				if(Browser* browser = sortedNode->getBrowser ())
					SortedNode::removeNode (sortedNode, this, browser);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID SortFolderNode::getCustomBackground () const
{
	if(SortedNode* sortedNode = getAncestorNode<SortedNode> ())
		return sortedNode->getFolderBackground ();

	return SuperClass::getCustomBackground ();
}

//************************************************************************************************
// Browsable::CustomSortFolderNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CustomSortFolderNode, SortFolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomSortFolderNode::CustomSortFolderNode (StringRef title)
: SortFolderNode (title),
  sortName (title) // keep original (unbranded) title (segment from sort path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CustomSortFolderNode::getSortName () const
{
	return sortName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomSortFolderNode::getSortPath (String& path) const
{
	if(auto parentSortNode = ccl_cast<CustomSortFolderNode> (getParent ()))
	{
		parentSortNode->getSortPath (path);
		path << Url::strPathChar;
	}
	path << sortName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CustomSortFolderNode::getSortPath () const
{
	String path;
	getSortPath (path);
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CustomSortFolderNode::compare (const Object& obj) const
{
	// sort folder nodes before leaf nodes
	auto otherFolder = ccl_cast<CustomSortFolderNode> (&obj);
	if(!otherFolder)
		return -1;

	return SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomSortFolderNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("New Folder"), FileStrings::NewFolder ()), nullptr, true);
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Browser"), CSTR ("Rename File"), FileStrings::RenameFolder ()), nullptr, true); // avoid conflict with global File/Rename command
	contextMenu.addCommandItem (CommandWithTitle (CSTR ("Edit"), CSTR ("Delete"), FileStrings::DeleteFolder ()), nullptr, true);
	return kResultFalse; // (continue)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::interpretCommand (const CommandMsg& msg, const Container* selectedNodes)
{
	if(msg.category == "Browser")
	{
		if(msg.name == "New Folder")
		{
			return createNewFolder (msg.checkOnly ());
		}
		else if(msg.name == "Rename File")
		{
			if(!msg.checkOnly ())
			{
				AutoPtr<Renamer> renamer (createFolderRenamer ());
				if(renamer)
					renamer->runDialog (XSTR (Rename));
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer* CustomSortFolderNode::createFolderRenamer ()
{
	CCL_NOT_IMPL ("CustomSortFolderNode::createRenamer")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::createNewFolder (bool checkOnly)
{
	CCL_NOT_IMPL ("CustomSortFolderNode::createNewFolder")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::removeFolders (NodeRemover& remover, Container& folderNodes)
{
	CCL_NOT_IMPL ("CustomSortFolderNode::removeFolders")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandler* CustomSortFolderNode::createDragHandler (IView* targetView)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* CustomSortFolderNode::setFocusNode (BrowserNode& baseNode, StringRef sortPath)
{
	if(Browser* browser = baseNode.getBrowser ())
	{
		MutableCString browserPath;
		browser->makePath (browserPath, &baseNode);
		browserPath += '/';
		browserPath += sortPath;

		BrowserNode* newFolderNode = browser->findNode (browserPath, true, true);
		browser->setFocusNode (newFolderNode);
		return newFolderNode;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::askNewFolder (String& newPath, BrowserNode* focusNode, MetaClassRef sortFolderClass)
{
	auto parentFolder = ccl_cast<CustomSortFolderNode> (focusNode);
	if(!parentFolder && focusNode)
		parentFolder = ccl_cast<CustomSortFolderNode> (focusNode->getParent ());

	String folderName;
	if(DialogBox ()->askForString (folderName, CSTR ("Name"), FileStrings::NewFolderTitle (), CCLSTR ("NewFolder")) && !folderName.isEmpty ())
	{
		if(parentFolder && parentFolder->canCast (sortFolderClass)) // ignore (parent) folders beyond our own hierarchy
			parentFolder->getSortPath (newPath);

		if(!newPath.isEmpty ())
			newPath << Url::strPathChar;
		newPath << folderName;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::acceptMovedFolder (CustomSortFolderNode* movedFolder) const
{
	// can't move a folder into itself (prevent "Malkovich Malkovich Malkovich")
	if(movedFolder == this)
		return false;

	// can't move into direct parent folder (already there)
	if(movedFolder && movedFolder->getParent () == this)
		return false;

	// can't move any parent into a sub folder
	if(hasAncestor (movedFolder))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::prepareMoveIntoFolder (String& oldPath, String& newPath, StringRef targetSortPath)
{
	getSortPath (oldPath);

	if(oldPath == targetSortPath)
		return false;

	// can't move folder into a (deep) child
	if(targetSortPath.startsWith (String (oldPath) << Url::strPathChar))
		return false;

	int index = oldPath.lastIndex (Url::strPathChar);
	String title (index >= 0 ? oldPath.subString (index + 1) : oldPath);

	newPath = targetSortPath;
	if(!newPath.isEmpty ())
		newPath << Url::strPathChar;
	newPath << title;

	return newPath != oldPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomSortFolderNode::performRemoval (NodeRemover& remover)
{
	bool checkOnly = remover.isCheckOnly ();
	bool result = false;

	// collect nodes that can be removed
	ObjectList folderNodes;

	ForEach (remover, BrowserNode, node)
		if(auto* folderNode = ccl_cast<CustomSortFolderNode> (node))
		{
			if(checkOnly)
				return true;
			else
				folderNodes.add (folderNode);
		}
	EndFor

	if(!checkOnly && !folderNodes.isEmpty ())
	{
		result = true;

		// ask user if folder should be removed...
		String text;
		text << ((folderNodes.count () == 1) ? XSTR (DoYouWantToRemoveThisFolder) : XSTR (DoYouWantToRemoveTheseFolders)) << "\n\n";

		StringBuilder listWriter (text);

		for(auto node :iterate_as<CustomSortFolderNode> (folderNodes))
		{
			listWriter.addItem (node->getTitle ());
			if(listWriter.isLimitReached ())
				break;
		}

		remover.setRemoveDeferred (true);

		if(Alert::ask (text) == Alert::kYes)
			removeFolders (remover, folderNodes);
		else
		{
			// keep them
			for(auto node :iterate_as<CustomSortFolderNode> (folderNodes))
				remover.keepNode (node);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomSortFolderNode::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	AutoPtr<DragHandler> dragHandler (createDragHandler (targetView));
	if(dragHandler && dragHandler->prepare (data, session))
	{
		if(session)
			session->setDragHandler (dragHandler);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomSortFolderNode::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	CCL_NOT_IMPL ("CustomSortFolderNode::insertData")
	return false;
}

//************************************************************************************************
// SortFolderRenamerBase
//************************************************************************************************

SortFolderRenamerBase::SortFolderRenamerBase (CustomSortFolderNode& node)
: Renamer (node.getTitle ()),
  node (node)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderRenamerBase::performRename (StringRef newName)
{
	ASSERT (!newName.isEmpty ())

	String oldPath;
	node.getSortPath (oldPath);

	Browser::ExpandState expandState;

	Browser* browser = node.getBrowser ();
	MutableCString browserPath;
	if(browser)
	{
		browser->makePath (browserPath, &node);
		int index = browserPath.lastIndex ('/');
		if(index > 0)
			browserPath.truncate (index + 1);
		browserPath += newName;

		expandState.store (*browser, node);
	}

	if(!renameFolderInternal (oldPath, newName))
		return false;

	if(browser)
	{
		BrowserNode* newFolderNode = browser->findNode (browserPath, true, true);

		// apply expand state to new node
		if(newFolderNode)
			expandState.restore (*browser, *newFolderNode);

		browser->setFocusNode (newFolderNode);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFolderRenamerBase::doesAlreadyExist (StringRef newName)
{
	String path;
	node.getSortPath (path);
						
	String newPath (SortFolderList::getParentFolder (path));
	if(!newPath.isEmpty ())
		newPath << Url::strPathChar;
	newPath << newName;

	return hasSortFolderInternal (newPath);
}

//************************************************************************************************
// SortedNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SortedNode, FolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortedNode::insertNode (SortedNode* sortedNode, BrowserNode* node, Browser* browser)
{
	if(!sortedNode->buildDone)
	{
		// can't add only that single node when subNodes were never created
		node->release ();
		return false;
	}

	// insert into the FolderNode hierarchy
	FolderNode* parentFolder = sortedNode->addSorted (node);

	// insert into Browser
	if(browser)
	{
		int index = parentFolder->getNodeIndex (node);

		node->retain ();
		bool result = browser->insertNode (parentFolder, node, index);

		if(sortedNode->countNodes () > 0)
			sortedNode->buildDone = true; // was reset in onRefresh

		return result;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortedNode::removeNode (SortedNode* sortedNode, BrowserNode* node, Browser* browser)
{
	// remove the node & empty parent folders
	if(BrowserNode* removedNode = sortedNode->removeSorted (node))
	{
		if(browser)
			browser->removeNode (removedNode);

		removedNode->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortedNode::SortedNode (StringRef title, BrowserNode* parent)
: FolderNode (title, parent),
  sorter (nullptr),
  sorterProvider (nullptr),
  buildDone (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortedNode::~SortedNode ()
{
	if(sorter)
		sorter->release ();

	share_and_observe (this, sorterProvider, (NodeSorterProvider*)nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortedNode::setSorterProvider (NodeSorterProvider* provider)
{
	share_and_observe (this, sorterProvider, provider);

	setSorter (sorterProvider ? sorterProvider->getSorter () : nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortedNode::setSorter (NodeSorter* _sorter)
{
	if(sorter != _sorter)
	{
		take_shared (sorter, _sorter);

		buildDone = false;

		Browser* browser = getBrowser ();
		if(browser)
			browser->refreshNode (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SortedNode::notify (ISubject* subject, MessageRef msg)
{
	if(sorterProvider && subject == sorterProvider && msg == kChanged)
		setSorter (sorterProvider->getSorter ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* SortedNode::newFolder (StringRef title)
{
	return NEW SortFolderNode (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortedNode::canRemoveParentFolder (FolderNode* parentFolder) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderNode* SortedNode::addSubFolders (StringRef path)
{
	FolderNode* current = this;

	ForEachStringToken (path, sorter->getPathDelimiters (), token)
		FolderNode* subFolder = current->getFolder (token);
		if(!subFolder)
		{
			subFolder = newFolder (token);
			current->addSorted (subFolder);	// note: not virtual, hence no recursion :-)
		}
		current = subFolder;
	EndFor

	return current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SortedNode::addSubFolders (IUnknownIterator& pathIterator)
{
	while(IUnknown* unk = pathIterator.nextUnknown ())
	{
		auto path = unknown_cast<Boxed::String> (unk);
		ASSERT (path)
		if(!path)
			continue;

		int index = path->lastIndex (Url::strPathChar);
		String title (index >= 0 ? path->subString (index + 1) : *path);
		if(!title.isEmpty ())
			addSubFolders (*path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderNode* SortedNode::addSorted (BrowserNode* node)
{
	FolderNode* current = this;

	String path;
	if(sorter && sorter->getSortPath (path, node))
		current = addSubFolders (path);

	current->addSorted (node);
	return current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* SortedNode::removeSorted (BrowserNode* node)
{
	if(FolderNode* parentFolder = ccl_cast<FolderNode> (node->getParent ()))
	{
		BrowserNode* toRemove = node;
		bool checkParent = parentFolder != this;
		while(checkParent)
		{
			checkParent = false;

			// parent must be removed if it only contains the node to remove
			const ObjectArray& content = parentFolder->getContent ();
			if(content.count () == 1 && content.at (0) == toRemove)
			{
				auto grandParentFolder = ccl_cast<FolderNode> (parentFolder->getParent ());
				if(grandParentFolder && canRemoveParentFolder (parentFolder))
				{
					toRemove->setParent (nullptr);
					toRemove = parentFolder;
					parentFolder = grandParentFolder;
					checkParent = parentFolder != this;
				}
			}
		}

		if(parentFolder->remove (toRemove))
			return toRemove;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderNode* SortedNode::findSortFolderNode (StringRef path) const
{
	const FolderNode* currentParent = this;
	SortFolderNode* currentSortFolder = nullptr;

	auto findSubFolderInCurrent = [&] (StringRef sortName)
	{
		SortFolderNode* found = nullptr;
		for(auto n : currentParent->getContent ())
		{
			auto sortFolder = ccl_cast<SortFolderNode> (n);
			if(sortFolder && sortFolder->getSortName () == sortName)
			{
				found = sortFolder;
				break;
			}
		}
		return found;
	};

	ForEachStringToken (path, sorter->getPathDelimiters (), segment)
		currentParent = currentSortFolder = findSubFolderInCurrent (segment);
		if(!currentParent)
			break;
	EndFor

	return currentSortFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortedNode::getSubNodes (Container& children, NodeFlags flags)
{
	if(!buildDone)
	{
		removeAll ();
		build ();
		buildDone = true;
	}

	return SuperClass::getSubNodes (children, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SortedNode::onRefresh ()
{
	if(buildDone)
	{
		// rebuild immediately (e.g. FolderNode::findNode must not deliver old nodes anymore)
		removeAll ();
		build ();
	}
	return true;
}

//************************************************************************************************
// RootNode
//************************************************************************************************

DEFINE_CLASS (RootNode, FolderNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

RootNode::RootNode (Browser* browser, StringRef title)
: FolderNode (title),
  browser (browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootNode::~RootNode ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootNode::getUniqueName (MutableCString& name)
{
	name = CSTR ("root");
	return true;
}

//************************************************************************************************
// SeparatorNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SeparatorNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

int SeparatorNode::compare (const Object& obj) const
{
	return -1; // stay before any nodes that are added later
}

//////////////////////////////////////////////////////////////////////////////////

bool SeparatorNode::drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment)
{
	if(id == nullptr)
	{
		Point p1 (0, (info.rect.top + info.rect.bottom) / 2);
		Point p2 (info.view->getSize ().getWidth (), p1.y);

		MutableCString colorName (customBackground);
		colorName += ".linecolor";
		Pen separatorPen (info.view->getVisualStyle ().getColor (colorName, Colors::kGray));
		info.graphics.drawLine (p1, p2, separatorPen);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////

StringID SeparatorNode::getCustomBackground () const
{
	return customBackground;
}

//////////////////////////////////////////////////////////////////////////////////

void SeparatorNode::setCustomBackground (StringID id)
{
	customBackground = id;
}

//************************************************************************************************
// MoveToFolderMenuBuilder
//************************************************************************************************

MoveToFolderMenuBuilder::MoveToFolderMenuBuilder (BrowserNode* nodeToMove)
: nodeToMove (nodeToMove),
  oldParentNode (nullptr)
{
	if(nodeToMove)
		oldParentNode = nodeToMove->getParent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MoveToFolderMenuBuilder::appendSubMenu (IMenu& parentMenu, FolderNode& baseNode)
{
	AutoPtr<IMenu> subMenu (parentMenu.createMenu ());
	subMenu->setMenuAttribute (IMenu::kMenuTitle, FileStrings::MoveToFolder ());

	traverseFolders (*subMenu, baseNode, false);
	parentMenu.addMenu (subMenu.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MoveToFolderMenuBuilder::traverseFolders (IMenu& parentMenu, FolderNode& parentFolder, bool createSubMenu)
{
	IMenuItem* parentFolderItem = addMoveToFolderCommand (parentMenu, parentFolder, createSubMenu);

	AutoPtr<IMenu> targetMenu;
	if(createSubMenu)
		targetMenu = parentMenu.createMenu ();
	else
	{
		parentMenu.addSeparatorItem ();
		targetMenu.share (&parentMenu);
	}

	for(auto node : parentFolder.getContent ())
		if(auto folderNode = ccl_cast<FolderNode> (node))
			if(handlesFolder (folderNode))
				traverseFolders (*targetMenu, *folderNode, true);

	if(createSubMenu && targetMenu->countItems () > 0)
	{
		// add as split menu to parent folder item if possible, or normal subMenu otherwise
		if(parentFolderItem && targetMenu->isExtendedMenu ())
			parentFolderItem->setItemAttribute (IMenuItem::kSplitMenu, static_cast<IMenu*> (targetMenu));
		else
			parentMenu.addMenu (targetMenu.detach ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MoveToFolderMenuBuilder::handlesFolder (FolderNode* folderNode) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenuItem* MoveToFolderMenuBuilder::addMoveToFolderCommand (IMenu& subMenu, FolderNode& targetFolderNode, bool withIcon)
{
	String title (targetFolderNode.getTitle ());
	bool enabled = &targetFolderNode != oldParentNode && &targetFolderNode != nodeToMove;

	AutoPtr<ICommandHandler> handler;
	if(enabled)
		handler = createCommandHandler (targetFolderNode);

	IMenuItem* menuItem = subMenu.addCommandItem (title, CSTR ("File"), CSTR ("Move to Folder"), handler);
	menuItem->setItemAttribute (IMenuItem::kItemEnabled, enabled);
	if(withIcon)
		menuItem->setItemAttribute (IMenuItem::kItemIcon, FileIcons::instance ().getDefaultFolderIcon ());

	return menuItem;
}
