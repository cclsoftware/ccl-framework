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
// Filename    : ccl/app/editing/tools/toolcollection.cpp
// Description : Tool Collection
//
//************************************************************************************************

#include "ccl/app/editing/tools/toolcollection.h"
#include "ccl/app/editing/tools/toolbar.h"
#include "ccl/app/editing/tools/edittool.h"
#include "ccl/app/editing/tools/toolconfig.h"

#include "ccl/base/message.h"

#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ToolCollection
//************************************************************************************************

DEFINE_CLASS (ToolCollection, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolCollection::ToolCollection ()
: toolset (nullptr),
  toolBar (nullptr)
{
	tools.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolCollection::~ToolCollection ()
{
	if(toolset)
		ccl_release (toolset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ToolBar* ToolCollection::getToolBar () const
{
	return toolBar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::setToolBar (ToolBar* tb)
{
	SharedPtr<ToolCollection> lifeGuard (this); // prevent heap use-after-free. (toolBar->removeToolCollection (this) releases this)

	if(toolBar)
		toolBar->removeToolCollection (this);

	toolBar = tb;

	if(toolBar)
		toolBar->addToolCollection (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::setActiveTool (EditTool* tool)
{
	if(toolBar)
		toolBar->setActiveTool (tool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* ToolCollection::getActiveTool ()
{
	ASSERT (toolBar || tools.count () <= 1) // (need a ToolBar to select tools)

	EditTool* tool = toolBar ? toolBar->getActiveTool (*this) : nullptr;
	if(!tool)
		tool = getTool (0);
	return tool;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::addTool (EditTool* tool)
{
	tools.add (tool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::removeTool (EditTool* tool)
{
	tools.remove (tool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::addTools (INativeToolSet& toolset)
{
	IterForEach (toolset.getTools (), EditTool, tool)
		addTool (return_shared (tool));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::configureTools (StringRef toolsetName)
{
	ASSERT (toolset == nullptr)
	if(toolset == nullptr)
	{
		toolset = ccl_new<IToolSet> (toolsetName);
		ASSERT (toolset != nullptr)
		if(toolset)
		{
			UnknownPtr<INativeToolSet> nativeToolSet (toolset);
			if(nativeToolSet)
			{
				addTools (*nativeToolSet);
			}
			else
			{
				for(int i = 0, count = toolset->countConfigurations (); i < count; i++)
				{
					AutoPtr<IToolConfiguration> config = toolset->createConfiguration (i);
					ASSERT (config != nullptr)
					if(config)
						addTool (NEW ConfigTool (config));
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolCollection::isEmpty () const
{
	return tools.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ToolCollection::countTools () const
{
	return tools.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* ToolCollection::getTool (int index) const
{
	return (EditTool*)tools.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTool* ToolCollection::findTool (StringID name) const
{
	ForEach (tools, EditTool, t)
		if(t->getName () == name)
			return t;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ToolCollection::getToolIndex (const EditTool* tool) const
{
	return tools.index (tool);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ToolCollection::newIterator () const
{
	return tools.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::onToolChanged ()
{
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ToolCollection::onToolModeChanged (StringID mode)
{
	if(EditTool* tool = getActiveTool ())
	{
		tool->setActiveMode (mode);
		signal (Message (kChanged));
	}
}
