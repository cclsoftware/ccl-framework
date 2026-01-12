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
// Filename    : ccl/app/browser/filedraghandler.h
// Description : Drag handler for moving / copying files to another directory node
//
//************************************************************************************************

#ifndef _ccl_filedraghandler_h
#define _ccl_filedraghandler_h

#include "ccl/app/controls/draghandler.h"
#include "ccl/app/components/pathselector.h"
#include "ccl/app/browser/browsernode.h"

#include "ccl/base/storage/url.h"

namespace CCL {
namespace Browsable {

class FileNode;

//************************************************************************************************
// Browsable::DragHandlerBase
//************************************************************************************************

class DragHandlerBase: public DragHandler,
					   public IItemDragVerifier
{
public:
	DECLARE_CLASS_ABSTRACT (DragHandlerBase, DragHandler)

	DragHandlerBase (IView* view, Browser* browser);

	virtual bool setTargetNode (BrowserNode* node);

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override;

	CLASS_INTERFACE (IItemDragVerifier, DragHandler)

protected:
	UnknownPtr<IItemView> itemView;
	Browser* browser;
	SharedPtr<BrowserNode> targetNode;
	int flags;

	PROPERTY_FLAG (flags, 1<<0, canTryParentFolders)

	void describeTransferToFolder (String& header, int& dropResult, StringRef oldFolder, StringRef targetFolder, bool isMove = true, bool ignoreSameFolder = false);
	void describeTransferToFavoriteFolder (String& header, int& dropResult, StringRef oldFolder, StringRef targetFolder, bool isAlreadyFavorite, bool ignoreSameFolder = false);
};

//************************************************************************************************
// Browsable::FileDraghandlerBase
//************************************************************************************************

class FileDraghandlerBase: public DragHandlerBase
{
public:
	DECLARE_CLASS_ABSTRACT (FileDraghandlerBase, DragHandlerBase)

	FileDraghandlerBase (IView* view, Browser* browser);

	// DraghandlerBase
	bool setTargetNode (BrowserNode* node) override;
	tbool CCL_API dragOver (const DragEvent& event) override;

protected:
	Url destFolder;

	String makeTitleWithDestFolder (StringRef pattern); ///< pattern string with 1 argument for dest folder name

	virtual void setDestFolder (UrlRef folder);
	virtual bool checkTargetNode (Url& path, BrowserNode* node);
	virtual bool checkDataTarget (IDataTarget* dataTarget, IDragSession* session);
	virtual void notifyTargetNode (Iterator* droppedFiles);
};

//************************************************************************************************
// Browsable::FileDraghandler
//************************************************************************************************

class FileDraghandler: public FileDraghandlerBase
{
public:
	DECLARE_CLASS_ABSTRACT (FileDraghandler, FileDraghandlerBase)

	FileDraghandler (IView* view, Browser* browser);

	// FileDraghandlerBase
	bool checkTargetNode (Url& path, BrowserNode* node) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
	bool getHelp (CCL::IHelpInfoBuilder& helpInfo) override;

private:
	PathList forbiddenTargetFolders;		///< can't drag into these folders
	PathList forbiddenTargetFoldersDeep;	///< can't drag into childs of these folders
	bool canMove;
	bool isPreferCopy;
};

} // namespace Browsable
} // namespace CCL

#endif // _ccl_filedraghandler_h
