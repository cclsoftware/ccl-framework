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
// Filename    : ccl/app/browser/browsernode.h
// Description : Browser Node
//
//************************************************************************************************

#ifndef _ccl_browsernode_h
#define _ccl_browsernode_h

#include "ccl/app/controls/treeviewnode.h"
#include "ccl/app/components/filerenamer.h"

#include "ccl/public/app/ibrowser.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/graphics/textformat.h"
#include "ccl/public/gui/graphics/brush.h"

namespace CCL {

class Browser;
class NodeRemover;
class NodeSorter;
class NodeSorterProvider;
class SortFolderNode;
class DragHandler;

interface ISearchProvider;
interface IMenu;
interface IMenuItem;

//************************************************************************************************
// BrowserNode macros
//************************************************************************************************

#define DELEGATE_IBROWSERNODE_METHODS(Interface, Parent) \
CCL::CString CCL_API getNodeType () const override { return Parent::getNodeType (); } \
CCL::StringRef CCL_API getNodeTitle () const override { return Parent::getNodeTitle (); } \
CCL::tbool CCL_API isNodeType (CCL::StringID type) const override { if(type == #Interface) return true; return Parent::isNodeType (type); }

//************************************************************************************************
// BrowserNode
//************************************************************************************************

class BrowserNode: public TreeViewNode,
				   public IBrowserNode,
				   public ICommandHandler
{
public:
	DECLARE_CLASS (BrowserNode, TreeViewNode)
	DECLARE_METHOD_NAMES (BrowserNode)

	BrowserNode (StringRef title = nullptr, BrowserNode* parent = nullptr);
	~BrowserNode ();

	virtual Browser* getBrowser () const;

	PROPERTY_POINTER (BrowserNode, parent, Parent)

	virtual void setTranslatedTitle (const LocalString& localString) { ASSERT (0) }

	bool hasAncestor (BrowserNode* ancestor) const;
	const BrowserNode* getAncestorNode (MetaClassRef classID) const;	///< get ancestor of given class
	template<class T> T* getAncestorNode () const;
	const BrowserNode* getAncestorNodeWithInterface (UIDRef iid) const; ///< get ancestor with given interface
	template<class T> T* getAncestorNodeWithInterface () const;

	virtual bool getUniqueName (MutableCString& name); ///< must not contain a '/'
	virtual bool drawIconOverlay (const IItemModel::DrawInfo& info) { return false; }
	virtual bool onRefresh (); ///< return true if subNodes must be discarded
	virtual void onNodeRemoved (BrowserNode* node); ///< called when browser has removed a subnode
	virtual bool onOpen (bool deferred = false); ///< called on double click or enter/return key
	virtual bool onEdit (StringID id, const IItemModel::EditInfo& info);
	virtual bool performRemoval (NodeRemover& remover);
	virtual bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes);
	virtual tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes);
	virtual ISearchProvider* getSearchProvider ();

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// TreeViewNode
	int compare (const Object& obj) const override;

	CLASS_INTERFACE2 (IBrowserNode, ICommandHandler, TreeViewNode)

protected:
	IImage* getThemeIcon (StringID name);

	// IBrowserNode
	CString CCL_API getNodeType () const override;
	tbool CCL_API isNodeType (StringID type) const override;
	StringRef CCL_API getNodeTitle () const override { return getTitle (); }

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// helper for implementing drawDetail
	bool drawDetailString (const IItemModel::DrawInfo& info, StringRef string, AlignmentRef alignment = Alignment ());
	bool drawDetailString (const IItemModel::DrawInfo& info, StringRef string, BrushRef textBrush, AlignmentRef alignment = Alignment ());
	bool drawDetailLines (const IItemModel::DrawInfo& info, String strings[], int count, BrushRef textBrush, AlignmentRef alignment = Alignment ());
};

//************************************************************************************************
// FolderNode
//************************************************************************************************

class FolderNode: public BrowserNode
{
public:
	DECLARE_CLASS (FolderNode, BrowserNode)

	FolderNode (StringRef title = nullptr, BrowserNode* parent = nullptr);
	~FolderNode ();

	void add (BrowserNode* node);
	void addSorted (BrowserNode* node);
	void insertAt (int index, BrowserNode* node);
	bool remove (BrowserNode* node);
	void removeAll ();

	FolderNode* getFolder (StringRef title);
	int countNodes () const;
	BrowserNode* getNodeAt (int index) const;
	int getNodeIndex (BrowserNode* node);
	IUnknown* findNode (const IRecognizer* recognizer) const;
	template<class T> T* findNode (const IRecognizer* recognizer) const;

	const ObjectArray& getContent () const { return content; }

	// BrowserNode
	bool isFolder () const override { return true; }
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	void onNodeRemoved (BrowserNode* node) override;

protected:
	ObjectArray content;
};

//************************************************************************************************
// FlatFolderNode
/** A folder node that "skips" it's direct child nodes, delivering their children as it's childs. */
//************************************************************************************************

class FlatFolderNode: public FolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (FlatFolderNode, FolderNode)

	FlatFolderNode (StringRef title = nullptr, BrowserNode* parent = nullptr);

	// FolderNode
	bool getSubNodes (Container& children, NodeFlags flags) override;
};

//************************************************************************************************
// SortedNode
//************************************************************************************************

class SortedNode: public FolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (SortedNode, FolderNode)

	SortedNode (StringRef title = nullptr, BrowserNode* parent = nullptr);
	~SortedNode ();

	PROPERTY_MUTABLE_CSTRING (folderBackground, FolderBackground) // custom background id for created child folders

	void setSorterProvider (NodeSorterProvider* provider);
	void setSorter (NodeSorter* sorter);
	NodeSorter* getSorter ();

	FolderNode* addSorted (BrowserNode* node); ///< returns new parent folder of node

	FolderNode* addSubFolders (StringRef path);
	void addSubFolders (IUnknownIterator& pathIterator);

	SortFolderNode* findSortFolderNode (StringRef path) const;

	virtual void build () = 0;

	// FolderNode
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	bool onRefresh () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	/// insert/remove nodes into/from the folder hierarchy and the browser
	static bool insertNode (SortedNode* sortedNode, BrowserNode* node, Browser* browser); ///< node might be destroyed when this returns false!
	static void removeNode (SortedNode* sortedNode, BrowserNode* node, Browser* browser);

protected:
	bool buildDone;
	NodeSorter* sorter;
	NodeSorterProvider* sorterProvider;

	virtual SortFolderNode* newFolder (StringRef title);
	virtual bool canRemoveParentFolder (FolderNode* parentFolder) const;

	BrowserNode* removeSorted (BrowserNode* node);	///< returns the removed node, which might be a parent folder of node; caller takes ownership
};

//************************************************************************************************
// SortFolderNode
/** FolderNode created by SortedNode to build the sorting structure. */
//************************************************************************************************

class SortFolderNode: public FolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (SortFolderNode, FolderNode)

	SortFolderNode (StringRef title = nullptr);

	virtual StringRef getSortName () const;

	// FolderNode
	int compare (const Object& obj) const override;
	void onNodeRemoved (BrowserNode* node) override;
	StringID getCustomBackground () const override;
};

//************************************************************************************************
// Browsable::CustomSortFolderNode
//************************************************************************************************

class CustomSortFolderNode: public SortFolderNode,
							public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (CustomSortFolderNode, SortFolderNode)

	CustomSortFolderNode (StringRef title);

	void getSortPath (String& path) const;
	String getSortPath () const;

	static BrowserNode* setFocusNode (BrowserNode& baseNode, StringRef sortPath);
	static bool askNewFolder (String& newPath, BrowserNode* focusNode, MetaClassRef sortFolderClass);

	bool acceptMovedFolder (CustomSortFolderNode* movedFolder) const;	
	bool prepareMoveIntoFolder (String& oldPath, String& newPath, StringRef targetSortPath);

	// SortFolderNode
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool performRemoval (NodeRemover& remover) override;
	int compare (const Object& obj) const override;
	StringRef getSortName () const override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE (IDataTarget, SortFolderNode)

private:
	String sortName;

	// to be implemented by derived class
	virtual Renamer* createFolderRenamer ();
	virtual bool createNewFolder (bool checkOnly);
	virtual bool removeFolders (NodeRemover& remover, Container& folderNodes);
	virtual DragHandler* createDragHandler (IView* targetView);
};

//************************************************************************************************
// RootNode
//************************************************************************************************

class RootNode: public FolderNode
{
public:
	DECLARE_CLASS (RootNode, FolderNode)

	RootNode (Browser* browser = nullptr, StringRef title = nullptr);
	~RootNode ();

	// BrowserNode
	Browser* getBrowser () const override { return browser; }
	bool getUniqueName (MutableCString& name) override;

protected:
	Browser* browser;
};

//************************************************************************************************
// Browsable::TranslatedNode
/** Mixin class for nodes with a translated title.
    Stores the untranslated key of the title to build a unique name independent from the current language. */
//************************************************************************************************

template<class BaseClass>
class TranslatedNode: public BaseClass
{
public:
	TranslatedNode ();

	void updateTranslatedTitle (); ///< restores the title set in a previous setTranslatedTitle call

	// BrowserNode
	void setTranslatedTitle (const LocalString& localString) override;
	bool getUniqueName (MutableCString& name) override;

private:
	const char* uniqueNodeName;
};

//************************************************************************************************
// Browsable::SeparatorNode
/** Passive node used for decoration, e.g. as separator. */
//************************************************************************************************

class SeparatorNode: public BrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (SeparatorNode, BrowserNode)

	void setCustomBackground (StringID id);

	// BrowserNode
	int compare (const Object& obj) const override;
	bool drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment) override;
	StringID getCustomBackground () const override;

protected:
	MutableCString customBackground;
};

//************************************************************************************************
// SortFolderRenamerBase
//************************************************************************************************

class SortFolderRenamerBase: public Renamer
{
public:
	SortFolderRenamerBase (CustomSortFolderNode& node);

	// Renamer
	bool performRename (StringRef newName) override;
	bool doesAlreadyExist (StringRef newName) override;

protected:
	CustomSortFolderNode& node;

	virtual bool renameFolderInternal (String oldPath, StringRef newName) const = 0;
	virtual bool hasSortFolderInternal (StringRef newPath) const = 0;
};

//************************************************************************************************
// MoveToFolderMenuBuilder
//************************************************************************************************

class MoveToFolderMenuBuilder
{
public:
	MoveToFolderMenuBuilder (BrowserNode* nodeToMove);

	void appendSubMenu (IMenu& parentMenu, FolderNode& baseNode);

protected:
	BrowserNode* nodeToMove;
	BrowserNode* oldParentNode;

	virtual bool handlesFolder (FolderNode* folderNode) const;
	virtual ICommandHandler* createCommandHandler (FolderNode& targetFolderNode) = 0; // create handler for moving to given target folder

	void traverseFolders (IMenu& parentMenu, FolderNode& parentFolder, bool createSubMenu);
	IMenuItem* addMoveToFolderCommand (IMenu& subMenu, FolderNode& folderNode, bool withIcon);
};

//************************************************************************************************
// IBrowserNodeVisitor
//************************************************************************************************

interface IBrowserNodeVisitor
{
	virtual void visitNode (BrowserNode& node) = 0;
};

//************************************************************************************************
// IBrowserNodeBranding
//************************************************************************************************

interface IBrowserNodeBranding
{
	virtual void applyBranding (BrowserNode& parentNode, Container& subNodes) = 0;
};

//************************************************************************************************
// BrowserStrings
//************************************************************************************************

namespace BrowserStrings
{
	StringRef strFavorites ();
	StringRef strFavorite ();
	StringRef strAddToFavorites ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// BrowserNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T> inline T* BrowserNode::getAncestorNode () const
{ return (T*)getAncestorNode (ccl_typeid<T> ()); }

template<class Iface> inline Iface* BrowserNode::getAncestorNodeWithInterface () const
{
	UnknownPtr<Iface> iface;
	if(const BrowserNode* node = getAncestorNodeWithInterface (ccl_iid<Iface> ()))
		iface = ccl_const_cast (node)->asUnknown ();
	return iface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// FolderNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int FolderNode::countNodes () const
{ return content.count (); }

inline BrowserNode* FolderNode::getNodeAt (int index) const
{ return (BrowserNode*)content.at (index); }

inline int FolderNode::getNodeIndex (BrowserNode* node)
{ return content.index (node); }

template<class T> inline T* FolderNode::findNode (const IRecognizer* recognizer) const
{ return unknown_cast<T> (findNode (recognizer)); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// SortedNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline NodeSorter* SortedNode::getSorter ()
{ return sorter; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// TranslatedNode implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Base>
inline TranslatedNode<Base>::TranslatedNode ()
: uniqueNodeName (nullptr) {}

template<class Base>
void TranslatedNode<Base>::setTranslatedTitle (const LocalString& localString)
{
	Base::title = localString;
	uniqueNodeName = localString.getKey ();
}

template<class Base>
void TranslatedNode<Base>::updateTranslatedTitle ()
{
	if(uniqueNodeName)
		Base::title = LocalString (uniqueNodeName);
}

template<class Base>
bool TranslatedNode<Base>::getUniqueName (MutableCString& name)
{
	name = uniqueNodeName ? uniqueNodeName : Base::title;
	name.replace ('/', '\\');
	return !name.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_browsernode_h
