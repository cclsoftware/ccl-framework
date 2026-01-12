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
// Filename    : ccl/app/browser/filedraghandler.cpp
// Description : Drag handler for moving / copying files to another directory node
//
//************************************************************************************************

#include "ccl/app/browser/filedraghandler.h"
#include "ccl/app/browser/browser.h"
#include "ccl/app/browser/filesystemnodes.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/utilities/pathclassifier.h"
#include "ccl/app/utilities/fileoperations.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Browsable;

//************************************************************************************************
// Browsable::DragHandlerBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DragHandlerBase, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandlerBase::DragHandlerBase (IView* view, Browser* browser)
: DragHandler (view),
  browser (browser),
  itemView (view),
  targetNode (nullptr),
  flags (0)
{
	if(itemView && browser)
		setChildDragHandler (itemView->createDragHandler (IItemView::kCanDragOnItem, this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragHandlerBase::setTargetNode (BrowserNode* node)
{
	targetNode = node;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandlerBase::verifyTargetItem (ItemIndex& item, int& relation)
{
	targetNode = nullptr;

	if(BrowserNode* dragNode = browser->resolveNode (*itemView, item))
	{
		if(setTargetNode (dragNode))
			return true;

		if(canTryParentFolders ())
		{
			// find a target directory node upwards
			BrowserNode* node = dragNode;
			while(node = node->getParent ())
			{
				if(setTargetNode (node))
				{
					item = ItemIndex (node->asUnknown ()); // todo: only works in treeView

					if(node == browser->getTreeRoot ())
						relation = IItemViewDragHandler::kFullView;

					return true;
				}
			}
		}
	}
	else if(browser->isListMode ())
	{
		// in list mode we set the parent node if no other node can be resolved
		if(BrowserNode* listParent = browser->getListParentNode ())
		{
			if(setTargetNode (listParent))
			{
				item = ItemIndex (listParent->asUnknown ());
				relation = IItemViewDragHandler::kFullView;
				return true;
			}
		}
	}
	else if(BrowserNode* treeRoot = browser->getTreeRoot ())
	{
		// not on a node: try current root node
		if(setTargetNode (treeRoot))
		{
			item = ItemIndex ();
			relation = IItemViewDragHandler::kFullView;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandlerBase::describeTransferToFolder (String& header, int& dropResult, StringRef oldFolder, StringRef targetFolder, bool isMove, bool ignoreSameFolder)
{
	// move /copy to another folder
	dropResult = isMove ? IDragSession::kDropMove : IDragSession::kDropCopyReal;

	if(targetFolder == oldFolder)
	{
		if(!ignoreSameFolder)
			header = isMove ? FileStrings::Move () : FileStrings::Copy (); // already in target folder, indicate that we're about to move
	}
	else
	{
		if(targetFolder.isEmpty ())
			header = isMove ? FileStrings::MoveToRoot () : FileStrings::CopyToRoot ();
		else
		{
			Variant args[] = { targetFolder };
			header.appendFormat (isMove ? FileStrings::MoveTo () : FileStrings::CopyTo (), args, 1);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandlerBase::describeTransferToFavoriteFolder (String& header, int& dropResult, StringRef oldFolder, StringRef targetFolder, bool isAlreadyFavorite, bool ignoreSameFolder)
{
	if(isAlreadyFavorite)
	{
		// move to another favorites folder
		dropResult = IDragSession::kDropMove;

		if(targetFolder == oldFolder)
		{
			if(!ignoreSameFolder)
				header = FileStrings::Move (); // already in target folder, indicate that we're about to move
		}
		else
		{
			String pathString (BrowserStrings::strFavorites ());
			if(!targetFolder.isEmpty ())
				pathString << Url::strPathChar << targetFolder;

			Variant args[] = { pathString };
			header.appendFormat (FileStrings::MoveTo (), args, 1);
		}
	}
	else
	{
		dropResult = IDragSession::kDropCopyShared;
		header = BrowserStrings::strAddToFavorites ();
		if(!targetFolder.isEmpty ())
			header << " \"" << targetFolder << "\"";
	}
}

//************************************************************************************************
// Browsable::FileDraghandlerBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileDraghandlerBase, DragHandlerBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileDraghandlerBase::FileDraghandlerBase (IView* view, Browser* browser)
: DragHandlerBase (view, browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileDraghandlerBase::makeTitleWithDestFolder (StringRef pattern)
{
	String fileName;
	destFolder.getName (fileName);

	String text;
	Variant args[] = { fileName };
	text.appendFormat (pattern, args, 1);
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileDraghandlerBase::setDestFolder (UrlRef folder)
{
	destFolder = folder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDraghandlerBase::setTargetNode (BrowserNode* node)
{
	Url path;
	if(checkTargetNode (path, node))
	{
		targetNode = node;
		setDestFolder (path);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDraghandlerBase::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	UnknownPtr<IDataTarget> dataTarget (ccl_as_unknown (targetNode));
	if(dataTarget)
		if(checkDataTarget (dataTarget, &event.session) == false)
			targetNode = nullptr;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDraghandlerBase::checkTargetNode (Url& path, BrowserNode* node)
{
	if(DirectoryNode* dirNode = ccl_cast<DirectoryNode> (node))
		if(dirNode->getTargetLocation (path))
			if(path.isFolder ())
				if(System::GetFileSystem ().isLocalFile (path) && !System::GetFileSystem ().isWriteProtected (path))
					return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDraghandlerBase::checkDataTarget (IDataTarget* dataTarget, IDragSession* session)
{
	ASSERT (dataTarget && session)
	if(dataTarget)
		return dataTarget->canInsertData (data, session);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileDraghandlerBase::notifyTargetNode (Iterator* droppedFiles)
{
	UnknownList data;
	IterForEach (droppedFiles, Url, path)
		data.add (ccl_as_unknown (NEW Url (*path)));
	EndFor

	UnknownPtr<IDataTarget> dataTarget (ccl_as_unknown (targetNode));
	ASSERT (dataTarget.isValid ())
	if(dataTarget)
		dataTarget->insertData (data);
}

//************************************************************************************************
// Browsable::FileDraghandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileDraghandler, FileDraghandlerBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileDraghandler::FileDraghandler (IView* view, Browser* browser)
: FileDraghandlerBase (view, browser),
  canMove (true),
  isPreferCopy (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDraghandler::checkTargetNode (Url& path, BrowserNode* node)
{
	if(!SuperClass::checkTargetNode (path, node))
		return false;

	// check forbidden target folders
	ForEach (forbiddenTargetFolders, Url, url)
		if(*url == path)
			return false;
	EndFor

	ForEach (forbiddenTargetFoldersDeep, Url, url)
		if(url->contains (path))
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FileDraghandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	UnknownPtr<IUrl> url (&item);
	if(url && (url->isNativePath () || PathClassifier::needsExtraction (*url)))
	{
		if(canMove && System::GetFileSystem ().isWriteProtected (*url))
			canMove = false;

		url->retain ();
		return url;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileDraghandler::finishPrepare ()
{
	spriteBuilder.addHeader (nullptr);

	ForEachUnknown (data, unk)
		UnknownPtr<IUrl> url (unk);
		if(url)
		{
			// can't drag into same parent folder
			Url parentFolder (*url);
			if(parentFolder.ascend ())
				forbiddenTargetFolders.addPath (parentFolder);

			forbiddenTargetFolders.addPath (*url);     // can't drag folder into itself
			forbiddenTargetFoldersDeep.addPath (*url); // can't drag folder into childs of itself

			// sprite
			AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*url));
			String fileName;
			url->getName (fileName, true);
			spriteBuilder.addItem (icon, fileName);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDraghandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	SuperClass::getHelp (helpInfo);

	if(canMove)
	{
		if(isPreferCopy)
		{
			helpInfo.addOption (0, nullptr, FileStrings::Copy ());
			helpInfo.addOption (kCopyRealModifer, nullptr, FileStrings::Move ());
			helpInfo.addOption (kCopySharedModifer, nullptr, FileStrings::Move ());
		}
		else
		{
			helpInfo.addOption (0, nullptr, FileStrings::Move ());
			helpInfo.addOption (kCopyRealModifer, nullptr, FileStrings::Copy ());
			helpInfo.addOption (kCopySharedModifer, nullptr, FileStrings::Copy ());
		}
	}
	else
		helpInfo.addOption (0, nullptr, FileStrings::Copy ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDraghandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	int result = IDragSession::kDropNone;
	String header;

	if(targetNode)
	{
		bool isCopy = true;
		if(canMove)
		{
			isCopy = event.keys.isSet (kCopyRealModifer|kCopySharedModifer);

			// toggle default from move to copy when dragging to other volume
			bool preferCopy = false;
			UnknownPtr<IUrl> firstSource (data.getFirst ());
			if(firstSource)
			{
				// a derived DirectoryNode can customize the decision
				if(auto directoryNode = ccl_cast<DirectoryNode> (targetNode))
					preferCopy = directoryNode->shouldCopyByDefault (*firstSource);
				else
					preferCopy = !PathClassifier::isSameVolume (destFolder, *firstSource);
			}

			if(preferCopy)
				isCopy = !isCopy;

			if(preferCopy != isPreferCopy)
			{
				isPreferCopy = preferCopy;
				updateHelp ();
			}
		}

		if(isCopy)
		{
			result = IDragSession::kDropCopyReal;
			header = makeTitleWithDestFolder (FileStrings::CopyTo ());
		}
		else
		{
			result = IDragSession::kDropMove;
			header = makeTitleWithDestFolder (FileStrings::MoveTo ());
		}
	}

	event.session.setResult (result);
	if(sprite)
		spriteBuilder.replaceItemText (*sprite, 0, header);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDraghandler::afterDrop (const DragEvent& event)
{
	SuperClass::afterDrop (event);

	if(event.session.getResult () != IDragSession::kDropNone)
	{
		bool isMove = event.session.getResult () == IDragSession::kDropMove;

		FileTransferOperation batchOperation (isMove ? FileTransferOperation::kMove : FileTransferOperation::kCopy);
		batchOperation.setDestFolder (destFolder);

		ForEachUnknown (data, unk)
			UnknownPtr<IUrl> url (unk);
			if(url)
				batchOperation.addFile (*url);
		EndFor

		bool result = batchOperation.run (isMove ? FileStrings::MovingFiles (): FileStrings::CopyingFiles ());

		// notify target node
		if(result && targetNode)
		{
			ObjectArray destPaths;
			ForEach (batchOperation, BatchOperation::Task, task)
				destPaths.add (const_cast<Url*> (&task->getDestPath ()));
			EndFor
			notifyTargetNode (destPaths.newIterator ());

			// select first destination file
			if(auto firstTask = ccl_cast<BatchOperation::Task> (batchOperation.getTasks ().getFirst ()))
				if(BrowserNode* firstDestNode = browser->findNodeWithUrl (firstTask->getDestPath ()))
					browser->setFocusNode (firstDestNode);
		}
	}
	return true;
}
