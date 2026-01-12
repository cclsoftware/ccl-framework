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
// Filename    : ccl/app/browser/nodesorter.cpp
// Description : Browser Node Sorter
//
//************************************************************************************************

#include "ccl/app/browser/nodesorter.h"
#include "ccl/app/browser/browser.h"

#include "ccl/app/params.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// NodeSorter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NodeSorter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NodeSorter::NodeSorter ()
: tag (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NodeSorter::toString (String& string, int flags) const
{
	string = title;
	return true;
}

//************************************************************************************************
// NodeSorterProvider
//************************************************************************************************

NodeSorterProvider::NodeSorterProvider ()
: sorter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeSorterProvider::setSorter (NodeSorter* _sorter)
{
	if(sorter != _sorter)
	{
		sorter = _sorter;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SorterTags
	{
		kSortBy = 100
	};
}

//************************************************************************************************
// NodeSorterComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NodeSorterComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

NodeSorterComponent::NodeSorterComponent ()
: Component (CCLSTR ("Sorter"))
{
	sortList = unknown_cast<ListParam> (paramList.addList (CSTR ("sortBy"), Tag::kSortBy));
	sortList->setStorable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeSorterComponent::addSorter (NodeSorter* sorter, StringRef title, int tag)
{
	sortList->appendObject (sorter);
	sorter->setTitle (title);
	sorter->setTag (tag);

	if(sorterProvider.getSorter () == nullptr)
		sorterProvider.setSorter (sorter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeSorterComponent::removeSorter (int tag)
{
	int index = getSorterIndex (tag);
	if(index >= 0)
		sortList->removeAt (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int NodeSorterComponent::getSorterIndex (int tag) const
{
	int max = sortList->getMax ().asInt ();
	
	for(int i = 0; i <= max; i++)
		if(NodeSorter* sorter = sortList->getObject<NodeSorter> (i))
			if(sorter->getTag () == tag)
				return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeSorterComponent::selectSorterByTag (int tag)
{
	int index = getSorterIndex (tag);
	if(index >= 0)
		sortList->setValue (index, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NodeSorterComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kSortBy)
	{
		// store browser state in old sorter
		Browser* browser = getParentNode<Browser> ();
		if(browser && sorterProvider.getSorter ())
			sorterProvider.getSorter ()->setBrowserState (browser->createSnapshot ());

		NodeSorter* sorter = unknown_cast<NodeSorter> (sortList->getSelectedValue ());
		sorterProvider.setSorter (sorter);

		// restore browser state from new sorter
		if(browser && sorter && sorter->getBrowserState ())
			browser->restoreSnapshot (sorter->getBrowserState ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NodeSorterComponent::save (const Storage& storage) const
{
	return paramList.save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NodeSorterComponent::load (const Storage& storage)
{
	return paramList.load (storage);
}
