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
// Filename    : ccl/app/controls/treeviewnode.cpp
// Description : Tree View Node
//
//************************************************************************************************

#include "ccl/app/controls/treeviewnode.h"

using namespace CCL;

//************************************************************************************************
// TreeViewNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeViewNode, ListViewItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewNode::TreeViewNode (StringRef title)
: ListViewItem (title)
{}

//************************************************************************************************
// TreeViewFolderNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeViewFolderNode, TreeViewNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewFolderNode::TreeViewFolderNode (StringRef title)
: TreeViewNode (title)
{
	content.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewFolderNode::add (TreeViewNode* node)
{
	content.add (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewFolderNode::addSorted (TreeViewNode* node)
{
	content.addSorted (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeViewFolderNode::insertAt (int index, TreeViewNode* node)
{
	return content.insertAt (index, node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeViewFolderNode::remove (TreeViewNode* node)
{
	return content.remove (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewFolderNode::removeAll ()
{
	content.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeViewFolderNode::getSubNodes (Container& children, NodeFlags flags)
{
	if(flags.wantAll ())
		children.add (content, Container::kShare);
	else
	{
		ArrayForEach (content, TreeViewNode, node)
			if(flags.shouldAdd (node->isFolder ()))
				children.add (return_shared (node));
		EndFor
	}
	return true;
}
