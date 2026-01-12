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
// Filename    : ccl/app/browser/pluginnodes.h
// Description : Plug-in Nodes
//
//************************************************************************************************

#ifndef _ccl_pluginnodes_h
#define _ccl_pluginnodes_h

#include "ccl/app/presets/presetnode.h"

namespace CCL {
namespace Browsable {

class PlugInClassNode;
class PlugInPresetNode;

//************************************************************************************************
// Browsable::PlugInCategoryNode
//************************************************************************************************

class PlugInCategoryNode: public SortedNode,
						  public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInCategoryNode, SortedNode)

	PlugInCategoryNode (StringRef category1, StringRef title, bool presetNode = false);
	PlugInCategoryNode (StringRef category1, StringRef subCategory1, StringRef title, bool presetNode = false);
	~PlugInCategoryNode ();

	static const int kUserFolderSorterTag = 'usrf';

	PROPERTY_STRING (category1, Category1)
	PROPERTY_STRING (subCategory1, SubCategory1)
	PROPERTY_BOOL (presetNode, PresetNode)
	PROPERTY_SHARED_AUTO (IObjectFilter, classFilter, ClassFilter)

	PROPERTY_STRING (category2, Category2)

	PROPERTY_FLAG (flags, 1<<0, isEditMode)
	PROPERTY_FLAG (flags, 1<<1, canEditPresentation)
	PROPERTY_FLAG (flags, 1<<2, hasFavoritesFolder)
	PROPERTY_FLAG (flags, 1<<3, hasRecentFolder)
	PROPERTY_FLAG (flags, 1<<4, hasPresetFavoritesFolder)

	virtual void onPluginNodeReady (PlugInPresetNode& pluginNode) {}

	// SortedNode
	void build () override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;
	bool getUniqueName (MutableCString& name) override;
	StringID getCustomBackground () const override;
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	SortFolderNode* newFolder (StringRef title) override;
	bool canRemoveParentFolder (FolderNode* parentFolder) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	static PlugInClassNode* findRegularPluginClassNode (UIDRef classID, FolderNode& parentFolder);
	static PlugInClassNode* findPluginClassNode (UIDRef classID, FolderNode& parentFolder, IObjectFilter* folderFilter);

	bool isSortByUserFolder () const;
	tbool sortNodesIntoFolder (const IUnknownList& data, IDragSession* session, StringRef sortPath);
	bool onNewFolder (BrowserNode* node, bool checkOnly);

	static void signalPresentationChanged (bool deferred = false);

	static void setBranding (IBrowserNodeBranding* branding);

	CLASS_INTERFACE (IDataTarget, SortedNode)

	bool matches (const IClassDescription& description, bool checkHiddenState) const;

protected:
	int flags;
	static IBrowserNodeBranding* branding;

	void init ();
	bool matchesFilter (const IClassDescription& description, bool checkHiddenState) const;	/// internal use, doesn't check category
	bool matchesFilter (const IClassDescription& description) const;						/// internal use, doesn't check category

	friend class PlugInFavoritesNode;
	friend class RecentPlugInsNode;
	friend class PlugInClassNode;
	virtual BrowserNode* createSubNode (const IClassDescription& description);
};

//************************************************************************************************
// Browsable::PlugInClassNode
//************************************************************************************************

class PlugInClassNode: public SortedNode,
					   public IClassNode
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInClassNode, SortedNode)

	PlugInClassNode (const IClassDescription& description);
	~PlugInClassNode ();

	const IClassDescription& CCL_API getClassDescription () const override; ///< [IClassNode]

	PROPERTY_FLAG (flags, 1<<0, isEditMode)
	PROPERTY_FLAG (flags, 1<<1, canEditPresentation)
	PROPERTY_FLAG (flags, 1<<2, dragAsPreset)
	static constexpr int kLastFlag = 2;

	// SortedNode
	bool hasSubNodes () const override;
	bool getUniqueName (MutableCString& name) override;
	int compare (const Object& obj) const override;
	IUnknown* createDragObject () override;
	void build () override {}
	StringID getCustomBackground () const override;
	bool drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment) override;
	bool onEdit (StringID id, const IItemModel::EditInfo& info) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;

	DECLARE_STRINGID_MEMBER (kVisible)
	DECLARE_STRINGID_MEMBER (kFavorite)

	CLASS_INTERFACE (IClassNode, SortedNode)

protected:
	IClassDescription* description;
	int flags;

	// IClassNode
	DELEGATE_IBROWSERNODE_METHODS (IClassNode, SortedNode)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// Browsable::PlugInPresetNode
//************************************************************************************************

class PlugInPresetNode: public PlugInClassNode,
						public IPresetContainerNode,
						public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInPresetNode, PlugInClassNode)

	PlugInPresetNode (const IClassDescription& description);
	~PlugInPresetNode ();

	void hasPresetFavoritesFolder (bool state);

	void onPresetCreated (IPreset& preset);
	void onPresetRemoved (IPreset& preset);

	// PlugInClassNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override;
	bool onRefresh () override;
	void build () override;
	SortFolderNode* newFolder (StringRef title) override;
	bool canRemoveParentFolder (FolderNode* parentFolder) const override;
	bool drawIconOverlay (const IItemModel::DrawInfo& info) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;

	// IPresetContainerNode
	IAttributeList* getPresetMetaInfo () const override;
	String getPresetClassKey () const override;
	bool supportsFavorites () const override;
	PresetNodesBuilder& getPresetNodesBuilder () override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE2 (IPresetContainerNode, IDataTarget, PlugInClassNode)

private:
	PresetNodesBuilder builder;
};

//************************************************************************************************
// Browsable::PlugInSortFolderNode
//************************************************************************************************

class PlugInSortFolderNode: public CustomSortFolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInSortFolderNode, CustomSortFolderNode)

	PlugInSortFolderNode (StringRef title);

private:
	// CustomSortFolderNode
	Renamer* createFolderRenamer () override;
	bool createNewFolder (bool checkOnly) override;
	bool removeFolders (NodeRemover& remover, Container& folderNodes) override;
	DragHandler* createDragHandler (IView* targetView) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
};

//************************************************************************************************
// Browsable::PlugInFavoritesNode
//************************************************************************************************

class PlugInFavoritesNode: public SortedNode,
						   public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInFavoritesNode, SortedNode)

	PlugInFavoritesNode ();

	tbool sortNodesIntoFolder (const IUnknownList& items, IDragSession* session, StringRef sortPath);
	bool onNewFolder (BrowserNode* focusNode, bool checkOnly);

	// SortedNode
	bool isFolder () const override { return true; }
	int compare (const Object& obj) const override;
	void build () override;
	SortFolderNode* newFolder (StringRef title) override;
	bool canRemoveParentFolder (FolderNode* parentFolder) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE (IDataTarget, SortedNode)

private:
	class Sorter;

	bool canEditPresentation () const;
};

//************************************************************************************************
// Browsable::FavoritesSortFolderNode
//************************************************************************************************

class FavoritesSortFolderNode: public CustomSortFolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (FavoritesSortFolderNode, CustomSortFolderNode)

	FavoritesSortFolderNode (StringRef title);

private:
	// CustomSortFolderNode
	Renamer* createFolderRenamer () override;
	bool createNewFolder (bool checkOnly) override;
	bool removeFolders (NodeRemover& remover, Container& folderNodes) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
};

//************************************************************************************************
// Browsable::RecentPlugInsNode
//************************************************************************************************

class RecentPlugInsNode: public BrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (RecentPlugInsNode, BrowserNode)

	RecentPlugInsNode ();

	// BrowserNode
	bool isFolder () const override { return true; }
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	int compare (const Object& obj) const override;
};

} // namespace Browsable

//************************************************************************************************
// PluginSearchProvider
//************************************************************************************************

class PluginSearchProvider: public SearchProvider
{
public:
	PluginSearchProvider (StringRef category, IObjectFilter* classFilter = nullptr);

	PROPERTY_STRING (resultCategory, ResultCategory)

	// ISearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUrlFilter* getSearchResultFilter () const override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;

private:
	AutoPtr<IUrlFilter> hiddenPluginsFilter;
};

//************************************************************************************************
// HiddenPluginsFilter
//************************************************************************************************

class HiddenPluginsFilter: public UrlFilter
{
public:
	HiddenPluginsFilter (IParameter* bypassParam = nullptr, IObjectFilter* classFilter = nullptr);

	// IUrlFilter
	tbool CCL_API matches (UrlRef url) const override;

private:
	SharedPtr<IParameter> bypassParam;
	SharedPtr<IObjectFilter> classFilter;
};

} // namespace CCL

#endif // _ccl_pluginnodes_h
