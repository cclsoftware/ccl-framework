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
// Filename    : ccl/extras/webfs/webfilenodes.h
// Description : Web File Nodes
//
//************************************************************************************************

#ifndef _ccl_webfilenodes_h
#define _ccl_webfilenodes_h

#include "ccl/extras/webfs/webfileaction.h"

#include "ccl/app/browser/filesystemnodes.h"

#include "ccl/public/extras/iwebfilebrowser.h"

namespace CCL {

interface IDownloadable;

namespace Browsable {

class WebFileNode;
class WebVolumeNode;
class WebDirectoryNode;

//************************************************************************************************
// WebNodesBuilder
//************************************************************************************************

class WebNodesBuilder: public Web::GetDirectoryAction
{
public:
	WebNodesBuilder ();

	void onRefresh (DirectoryNode& node);

	bool getSubNodes (DirectoryNode& node, Container& children, BrowserNode::NodeFlags flags);

	void updateNode (BrowserNode& node);

	void drawStateOverlay (BrowserNode& node, const IItemModel::DrawInfo& info);

protected:
	bool insideNodeUpdate;
};

//************************************************************************************************
// WebNodeCustomizer
//************************************************************************************************

class WebNodeCustomizer
{
public:
	WebNodeCustomizer ();
	
	template <typename Interface> Interface* openHandler (FileNode& node);

	PROPERTY_MUTABLE_CSTRING (backgroundId, BackgroundID)
	StringID getBackgroundID (StringID defaultId) const;

	void customizeFile (WebFileNode& node);
	void customizeSearchResult (FileNode& node);
	void customizeFolder (WebDirectoryNode& node);
	void customizeVolume (WebVolumeNode& node, bool isUpdate = false);
	void uncustomizeVolume (WebVolumeNode& node);

	void refresh ();

protected:
	void customize (FileNode& node, bool isThisNode);
};

//************************************************************************************************
// WebRootNode
//************************************************************************************************

class WebRootNode: public TranslatedDirectoryNode
{
public:
	DECLARE_CLASS (WebRootNode, TranslatedDirectoryNode)

	WebRootNode (BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);
	~WebRootNode ();

	// DirectoryNode
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	ISearchProvider* getSearchProvider () override;
	IUnknown* createDragObject () override;

protected:
	SignalSink& signalSink;

	// DirectoryNode
	IImage* getIcon () override;
	BrowserNode* createNode (Url* forPath) const override;
};

//************************************************************************************************
// IWebNode
//************************************************************************************************

interface IWebNode: IUnknown
{
	virtual void updateCustomization () = 0;

	DECLARE_IID (IWebNode)
};

//************************************************************************************************
// WebVolumeNode
//************************************************************************************************

class WebVolumeNode: public VolumeNode,
					 public Web::IWebFileBrowserHost,
					 public IWebNode
{
public:
	DECLARE_CLASS (WebVolumeNode, VolumeNode)

	WebVolumeNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);
	~WebVolumeNode ();

	// VolumeNode
	bool getTargetLocation (Url& path) const override;
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
	bool getSubNodes (Container& children, NodeFlags flags) override;
	StringID getCustomBackground () const override;
	bool drawIconOverlay (const IItemModel::DrawInfo& info) override;
	bool onRefresh () override;
	bool onOpen (bool deferred = true) override;
	bool onEdit (StringID id, const IItemModel::EditInfo& info) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IWebNode
	void updateCustomization () override;

	CLASS_INTERFACE3 (ISearchProvider, IWebFileBrowserHost, IWebNode, VolumeNode)

protected:
	WebNodesBuilder builder;
	WebNodeCustomizer customizer;

	bool onOpenVolume (bool isEdit);

	// IWebFileBrowserHost
	void CCL_API updateItemInBrowser (UrlRef webfsUrl) override;
	void CCL_API setItemUpdateInProgress (tbool state) override;

	// ISearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;
};

//************************************************************************************************
// WebDirectoryNode
//************************************************************************************************

class WebDirectoryNode: public DirectoryNode,
						public IWebNode
{
public:
	DECLARE_CLASS (WebDirectoryNode, DirectoryNode)

	WebDirectoryNode (Url* path = nullptr, BrowserNode* parent = nullptr, IUrlFilter* urlFilter = nullptr);
	~WebDirectoryNode ();

	// DirectoryNode
	bool getUniqueName (MutableCString& name) override;
	bool isHiddenFile (UrlRef path) const override;
	bool getTargetLocation (Url& path) const override;
	bool shouldCopyByDefault (UrlRef sourcePath) const override;
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;
	ISearchProvider* getSearchProvider () override;
	bool getSubNodes (Container& children, NodeFlags flags) override;
	StringID getCustomBackground () const override;
	bool drawIconOverlay (const IItemModel::DrawInfo& info) override;
	IUnknown* createDragObject () override;
	bool onRefresh () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IWebNode
	void updateCustomization () override;

	CLASS_INTERFACE (IWebNode, DirectoryNode)

protected:
	WebNodesBuilder builder;
	WebNodeCustomizer customizer;

	bool isAcceptedChildPath (IDownloadable& downloadable) const;
};

//************************************************************************************************
// WebFileNode
//************************************************************************************************

class WebFileNode: public FileNode,
				   public IWebNode
{
public:
	DECLARE_CLASS (WebFileNode, FileNode)

	WebFileNode (Url* path = nullptr, BrowserNode* parent = nullptr);

	// FileNode
	bool getUniqueName (MutableCString& name) override;
	StringID getCustomBackground () const override;
	IUnknown* createDragObject () override;

	// IWebNode
	void updateCustomization () override;

	CLASS_INTERFACE (IWebNode, FileNode)

protected:
	WebNodeCustomizer customizer;
};

} // namespace Browsable
} // namespace CCL

#endif // _ccl_webfilenodes_h

