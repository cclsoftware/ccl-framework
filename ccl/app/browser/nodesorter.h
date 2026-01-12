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
// Filename    : ccl/app/browser/nodesorter.h
// Description : Browser Node Sorter
//
//************************************************************************************************

#ifndef _ccl_nodesorter_h
#define _ccl_nodesorter_h

#include "ccl/app/component.h"

namespace CCL {

class BrowserNode;
class ListParam;

//************************************************************************************************
// NodeSorter
/** Abstract base class for providing a sort path for a browser node. */
//************************************************************************************************

class NodeSorter: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (NodeSorter, Object)
	NodeSorter ();

	/** Get a sort path for the node. */
	virtual bool getSortPath (String& path, const BrowserNode* node) = 0;

	/** Get one or more delimiter chars that are used to break the path into folders. */
	virtual StringRef getPathDelimiters () const { return CCLSTR ("/"); }

	PROPERTY_STRING (title, Title)
	PROPERTY_VARIABLE (int, tag, Tag)
	PROPERTY_AUTO_POINTER (Object, browserState, BrowserState)

	// Object
	bool toString (String& string, int flags) const override;
};

//************************************************************************************************
// NodeSorterFlat
/** Provides no paths, nodes will be arranged in a flat list. */
//************************************************************************************************

class NodeSorterFlat: public NodeSorter
{
public:
	bool getSortPath (String& path, const BrowserNode* node) override { return false; }
};

//************************************************************************************************
// NodeSorterProvider
/** Provides a NodeSorter. Signals kChanged when the sorter gets replaced. */
//************************************************************************************************

class NodeSorterProvider: public Object
{
public:
	NodeSorterProvider ();

	NodeSorter* getSorter () const;
	void setSorter (NodeSorter* sorter);

private:
	NodeSorter* sorter;
};

//************************************************************************************************
// NodeSorterComponent
/** Manages selecting a sorter from a list. */
//************************************************************************************************

class NodeSorterComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (NodeSorterComponent, Component)

	NodeSorterComponent ();

	void addSorter (NodeSorter* sorter, StringRef title, int tag = -1);
	void removeSorter (int tag);
	void selectSorterByTag (int tag);

	NodeSorterProvider& getSorterProvider ();

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	bool save (const Storage& storage) const override;
	bool load (const Storage& storage) override;

private:
	NodeSorterProvider sorterProvider;
	ListParam* sortList;

protected:
	int getSorterIndex (int tag) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline NodeSorter* NodeSorterProvider::getSorter () const
{ return sorter; }

inline NodeSorterProvider& NodeSorterComponent::getSorterProvider ()
{ return sorterProvider; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_nodesorter_h
