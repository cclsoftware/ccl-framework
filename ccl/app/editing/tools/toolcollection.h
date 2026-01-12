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
// Filename    : ccl/app/editing/tools/toolcollection.h
// Description : Tool Collection
//
//************************************************************************************************

#ifndef _ccl_toolcollection_h
#define _ccl_toolcollection_h

#include "ccl/base/collections/objectlist.h"

namespace CCL {

class EditTool;
class ToolBar;
interface IToolSet;
interface INativeToolSet;

//************************************************************************************************
// ToolCollection
/** Collection of editing tools. */
//************************************************************************************************

class ToolCollection: public Object
{
public:
	DECLARE_CLASS (ToolCollection, Object)

	ToolCollection ();
	~ToolCollection ();

	// ToolBar
	void setToolBar (ToolBar* toolBar);
	ToolBar* getToolBar () const;
	void onToolChanged ();
	void onToolModeChanged (StringID mode);

	// Tools
	void addTool (EditTool* tool);
	void removeTool (EditTool* tool);
	void configureTools (StringRef toolsetName);
	void addTools (INativeToolSet& toolset);

	EditTool* getActiveTool ();
	void setActiveTool (EditTool* tool);

	bool isEmpty () const;
	int countTools () const;
	EditTool* getTool (int index) const;
	EditTool* findTool (StringID name) const;
	int getToolIndex (const EditTool* tool) const;
	Iterator* newIterator () const;

protected:
	ObjectList tools;
	IToolSet* toolset;
	ToolBar* toolBar;
};

} // namespace CCL

#endif // _ccl_toolcollection_h
