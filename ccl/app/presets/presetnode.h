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
// Filename    : ccl/app/presets/presetnode.h
// Description : Preset Node
//
//************************************************************************************************

#ifndef _ccl_presetnode_h
#define _ccl_presetnode_h

#include "ccl/app/browser/filesystemnodes.h"
#include "ccl/app/browser/nodesorter.h"

#include "ccl/app/components/searchprovider.h"

#include "ccl/public/app/ipreset.h"

namespace CCL {

interface IClassDescription;
interface IFileIcons;
interface IMenu;
class PresetMetaAttributes;

namespace Browsable {

interface IPresetContainerNode;

//************************************************************************************************
// PresetNode
//************************************************************************************************

class PresetNode: public FileNode
{
public:
	DECLARE_CLASS (PresetNode, FileNode)

	PresetNode (IPreset* preset = nullptr, BrowserNode* parent = nullptr, bool isSubPreset = false);

	PROPERTY_SHARED_AUTO (IPreset, preset, Preset)
	PROPERTY_BOOL (isSubPreset, IsSubPreset)

	bool isDefaultPreset () const;
	bool supportsFavorites () const;

	static bool getSelectedPresets (UnknownList& presets, Browser* browser);

	// BrowserNode
	IImage* getIcon () override;
	StringID getCustomBackground () const override;
	IUnknown* createDragObject () override;
	bool performRemoval (NodeRemover& remover) override;
	int compare (const Object& obj) const override;
	bool drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment) override;
	bool onOpen (bool deferred = true) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	bool isFolder () const override;
	bool hasSubNodes () const override;
	bool canAutoExpand () const override;
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;
	bool onRefresh () override;

	DECLARE_STRINGID_MEMBER (kFavorite)
	DECLARE_STRINGID_MEMBER (kEditSelection)

private:
	bool onRenamePreset (CommandMsg args, const Container* selectedNodes);
};

//************************************************************************************************
// PresetNodeSorter
/** Sorts presets by subfolder. */
//************************************************************************************************

struct PresetNodeSorter: public NodeSorter
{
	DECLARE_CLASS_ABSTRACT (PresetNodeSorter, NodeSorter)

	// NodeSorter
	bool getSortPath (String& path, const BrowserNode* node) override;

	// find a preset node in a hierarchy starting at baseNode
	static PresetNode* findPresetNode (BrowserNode& baseNode, UrlRef presetUrl, IAttributeList* metaInfo, bool createNodes = false);
};

//************************************************************************************************
// PresetSortFolderNode
//************************************************************************************************

class PresetSortFolderNode: public CustomSortFolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (PresetSortFolderNode, CustomSortFolderNode)

	PresetSortFolderNode (StringRef title);

	IAttributeList* getPresetMetaInfo () const;
	bool determineFileSystemUrl (Url& folder) const;
	bool hasWritablePreset () const;

	static bool sortNodesIntoFolder (const IUnknownList& presets, StringRef sortPath, IPresetContainerNode* containerNode);

private:
	class FolderRenamer;

	// CustomSortFolderNode
	Renamer* createFolderRenamer () override;
	bool createNewFolder (bool checkOnly) override;
	bool removeFolders (NodeRemover& remover, Container& folderNodes) override;
	int compare (const Object& obj) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
};

//************************************************************************************************
// PresetFavoritesNode
//************************************************************************************************

class PresetFavoritesNode: public SortedNode,
						   public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (PresetFavoritesNode, SortedNode)

	PresetFavoritesNode (IAttributeList* metaInfo);

	const IAttributeList* getMetaInfo () const;

	tbool sortNodesIntoFolder (const IUnknownList& items, IDragSession* session, StringRef sortPath);
	bool onNewFolder (BrowserNode* focusNode, bool checkOnly);
	bool onMoveToFolder (CmdArgs args, VariantRef data);
	bool onMoveToNewFolder (CmdArgs args, VariantRef data);
	void appendMoveToFolderMenu (IMenu& menu, PresetNode& presetNode);

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
	SharedPtr<IAttributeList> metaInfo;

	class Sorter;

	String createNewFolder (BrowserNode* focusNode);
};

//************************************************************************************************
// PresetFavoritesSortFolderNode
//************************************************************************************************

class PresetFavoritesSortFolderNode: public CustomSortFolderNode
{
public:
	DECLARE_CLASS_ABSTRACT (PresetFavoritesSortFolderNode, CustomSortFolderNode)

	PresetFavoritesSortFolderNode (StringRef title);

private:
	// CustomSortFolderNode
	Renamer* createFolderRenamer () override;
	bool createNewFolder (bool checkOnly) override;
	bool removeFolders (NodeRemover& remover, Container& folderNodes) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	class FolderRenamer;
};

//************************************************************************************************
// PresetNodesBuilder
//************************************************************************************************

class PresetNodesBuilder: public Object
{
public:
	PresetNodesBuilder (IAttributeList* metaInfo);
	PresetNodesBuilder (const IClassDescription& description);
	~PresetNodesBuilder ();

	DECLARE_STRINGID_MEMBER (kPresetsChanged)

	PROPERTY_BOOL (forceAlways, ForceAlways)
	PROPERTY_STRING (subCategoryFilter, SubCategoryFilter)

	PROPERTY_FLAG (flags, 1 << 0, hasFavoritesFolder)

	IAttributeList* getMetaInfo () const;
	String getClassKey () const;

	bool hasPresets (const BrowserNode& node) const;
	void buildNodes (SortedNode& parentFolder);
	void resetPresets ();

	bool hasPresetsPending () const { return presetsPending; }
	void drawPresetsPending (const IItemModel::DrawInfo& info);

	static PresetNode* findPresetNode (IPreset& preset, SortedNode& parentFolder);
	bool onNewPresetFolder (IPresetContainerNode& containerNode, BrowserNode* focusNode, bool checkOnly);
	bool hasSortFolder (StringRef sortPath) const;

	void onPresetCreated (IPreset& preset, SortedNode& parentFolder);
   	void onPresetRemoved (IPreset& preset, SortedNode& parentFolder);
	void onPresetSubFoldersChanged (MessageRef msg, SortedNode& baseNode);
	void onPresetFavoritesChanged (StringRef classKey, SortedNode& baseNode, StringRef folderPath);

	void appendMoveToFolderMenu (IMenu& menu, IPresetContainerNode& containerNode, PresetNode& presetNode);
	bool onMoveToFolder (CmdArgs args, VariantRef data);
	bool onMoveToNewFolder (CmdArgs args, VariantRef data);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	AutoPtr<IAttributeList> metaInfo;
	AutoPtr<IUnknownList> presets;
	bool presetsPending;
	bool resetSuspended;
	int flags;

	class FolderRenamer;

	bool shouldForcePresets (const BrowserNode& node) const;
	void getPresets (bool force);
	void cancelPresets (bool destructing = false);
	void checkAddPreset (IPreset& preset);
	void filterPresets ();
	bool isFiltered (IPreset& preset) const;

	String createNewPresetFolder (IPresetContainerNode& containerNode, BrowserNode* focusNode);

	static void getPresetsHiddenBy (IUnknownList& hiddenPresets, IPreset& preset);
	static PresetNode* findPresetNodeDeep (UrlRef presetUrl, FolderNode& parentFolder);
};

//************************************************************************************************
// IPresetContainerNode
//************************************************************************************************

interface IPresetContainerNode: public IUnknown
{
	virtual IAttributeList* getPresetMetaInfo () const = 0;
	virtual String getPresetClassKey () const = 0;
	virtual bool supportsFavorites () const = 0;
	virtual PresetNodesBuilder& getPresetNodesBuilder () = 0;

	DECLARE_IID (IPresetContainerNode)
};

//************************************************************************************************
// PresetContainerNode
/** Creates child nodes for presets that match the given metaInfo. */
//************************************************************************************************

class PresetContainerNode: public SortedNode,
						   public IPresetContainerNode,
						   public IDataTarget
{
public:
	DECLARE_CLASS_ABSTRACT (PresetContainerNode, SortedNode)

	PresetContainerNode (IAttributeList* metaInfo, StringRef title, BrowserNode* parent = nullptr);
	~PresetContainerNode ();
	
	PresetNodesBuilder& getBuilder ();

	void hasFavoritesFolder (bool state);

	// SortedNode
	bool hasSubNodes () const override;
	bool onRefresh () override;
	void build () override;
	SortFolderNode* newFolder (StringRef title) override;
	bool canRemoveParentFolder (FolderNode* parentFolder) const override;
	bool drawIconOverlay (const IItemModel::DrawInfo& info) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	bool interpretCommand (const CommandMsg& msg, const Container* selectedNodes) override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;

	// IPresetContainerNode
	IAttributeList* getPresetMetaInfo () const override;
	String getPresetClassKey () const override;
	bool supportsFavorites () const override;
	PresetNodesBuilder& getPresetNodesBuilder () override { return getBuilder (); }

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	static IRecognizer* createRecognizer (IAttributeList& metaInfo);

	CLASS_INTERFACE2 (IPresetContainerNode, IDataTarget, SortedNode)

protected:
	PresetNodesBuilder builder;

	class Recognizer;

	/// Check if a preset could be inside this node. Checks for matching category by default
	virtual bool handlesPreset (const PresetMetaAttributes& presetAttribs) const;

	virtual void onPresetCreated (IPreset& preset);
	virtual void onPresetRemoved (IPreset& preset);
};

} // namespace Browsable

//************************************************************************************************
// PresetSearchProvider
//************************************************************************************************

class PresetSearchProvider: public SearchProvider
{
public:
	DECLARE_CLASS (PresetSearchProvider, SearchProvider)

	PresetSearchProvider (StringRef category);
	PresetSearchProvider (UIDRef classId);

	PROPERTY_SHARED_POINTER (IUrlFilter, urlFilter, UrlFilter)
	
	// ISearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;
	IUrlFilter* getSearchResultFilter () const override;
	
private:
	PresetSearchProvider () {}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const IAttributeList* Browsable::PresetFavoritesNode::getMetaInfo () const
{ return metaInfo;}

inline Browsable::PresetNodesBuilder& Browsable::PresetContainerNode::getBuilder () 
{ return builder; }

inline IAttributeList* Browsable::PresetNodesBuilder::getMetaInfo () const 
{ return metaInfo; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_presetnode_h
