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
// Filename    : ccl/app/controls/treeviewnode.h
// Description : Tree View Node
//
//************************************************************************************************

#ifndef _ccl_treeviewnode_h
#define _ccl_treeviewnode_h

#include "ccl/app/controls/listviewitem.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

class Container;

//************************************************************************************************
// TreeViewNode
/** Base class for nodes represented in a TreeView control. */
//************************************************************************************************

class TreeViewNode: public ListViewItem
{
public:
	DECLARE_CLASS (TreeViewNode, ListViewItem)

	TreeViewNode (StringRef title = nullptr);

	virtual bool isFolder () const { return false; }
	virtual bool hasSubNodes () const { return isFolder (); }
	virtual bool canAutoExpand () const { return true; } ///< can the node automatically expand on click?

	struct NodeFlags
	{
		enum Flags
		{
			kFolders = 1<<0, 
			kLeafs = 1<<1, 
			kAll = kFolders|kLeafs
		};

		int flags;

		NodeFlags (int flags = 0): flags (flags) {}

		PROPERTY_FLAG (flags, kFolders, wantFolders)
		PROPERTY_FLAG (flags, kLeafs, wantLeafs)

		bool wantAll () const;
		bool shouldAdd (bool isFolder) const;
	};

	virtual bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) { return false; }
};

//************************************************************************************************
// TreeViewFolderNode
//************************************************************************************************

class TreeViewFolderNode: public TreeViewNode
{
public:
	DECLARE_CLASS (TreeViewFolderNode, TreeViewNode)

	TreeViewFolderNode (StringRef title = nullptr);

	void add (TreeViewNode* node);
	void addSorted (TreeViewNode* node);
	bool insertAt (int index, TreeViewNode* node);
	bool remove (TreeViewNode* node);
	void removeAll ();

	const ObjectArray& getContent () const { return content; }

	// TreeViewNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override { return !content.isEmpty (); }
	bool getSubNodes (Container& children, NodeFlags flags = NodeFlags::kAll) override;

protected:
	ObjectArray content;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// TreeViewNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool TreeViewNode::NodeFlags::wantAll () const 
{ 
	return flags == kAll; 
}
		
inline bool TreeViewNode::NodeFlags::shouldAdd (bool isFolder) const
{
	return isFolder && wantFolders () || !isFolder && wantLeafs (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_treeviewnode_h
