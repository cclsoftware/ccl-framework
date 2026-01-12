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
// Filename    : ccl/app/browser/filebrowser.h
// Description : File Browser
//
//************************************************************************************************

#ifndef _ccl_filebrowser_h
#define _ccl_filebrowser_h

#include "ccl/app/browser/browser.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/signalsource.h"

namespace CCL {

namespace Browsable {
class FileNode;
class DirectoryNode; }

class FilePreviewComponent;

//************************************************************************************************
// FileBrowser
//************************************************************************************************

class FileBrowser: public Browser
{
public:
	DECLARE_CLASS (FileBrowser, Browser)

	FileBrowser (StringRef name = nullptr, StringRef title = nullptr, FilePreviewComponent* preview = nullptr);
	~FileBrowser();

	PROPERTY_SHARED_AUTO (IUrlFilter, urlFilter, UrlFilter)

	void addDesktop ();
	void addUserDocumentFolder ();
	void addUserMusicFolder ();
	BrowserNode* addUserContent ();
	void addVolumes ();
	BrowserNode* addPackages (StringRef subType);
	void addLocation (UrlRef path, StringRef title = nullptr);

	FilePreviewComponent* getPreview () { return preview; }
	virtual FileRenamer* createFileRenamer (BrowserNode* node) const;

	bool selectFocusFileNode (UrlRef url, bool selectExclusive = true);

protected:
	FilePreviewComponent* preview;
	AutoSignalSink fileSystemSink;

	class NewFileTabTarget;

	// Browser
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	BrowserNode* findNodeWithUrl (UrlRef url) override;
	void onNodeFocused (BrowserNode* node, bool inList) override;
	bool onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info) override;
	tbool canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView) override;
};

//************************************************************************************************
// FileNodeFinder
/** Finds a child node of a starting Directory node. */
//************************************************************************************************

class FileNodeFinder
{
public:
	FileNodeFinder (Browser& browser);

	Browsable::FileNode* findFileNode (Browsable::DirectoryNode& directoryNode, UrlRef targetPath);
	Browsable::FileNode* findFileNode (Browsable::DirectoryNode& directoryNode, StringID relativePathString);

	PROPERTY_BOOL (acceptAncestor, AcceptAncestor)	///< if node not found, processs deepest found ancestor; default: true
	PROPERTY_BOOL (createNodes, CreateNodes)		///< create ancestor nodes of target node; default: false

protected:
	Browser& browser;
};

} // namespace CCL

#endif // _ccl_filebrowser_h
