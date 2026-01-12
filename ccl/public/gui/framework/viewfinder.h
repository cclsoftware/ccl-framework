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
// Filename    : ccl/public/gui/framework/viewfinder.h
// Description : View Finder
//
//************************************************************************************************

#ifndef _ccl_viewfinder_h
#define _ccl_viewfinder_h

#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//************************************************************************************************
// ViewFinder
/** Finds views covered behind a given skipView. */
//************************************************************************************************

class ViewFinder
{
public:
	ViewFinder (IView* skipView)
	: skipView (skipView)
	{}

	IView* findNextView (IView* parent, const Point& where)
	{
		ForEachChildViewReverse (parent, v)
			Point where2 (where);
			where2.offset (-v->getSize ().left, -v->getSize ().top);

			Rect client;
			if(v->getVisibleClient (client) && client.pointInside (where2))
			{
				IView* result = findNextView (v, where2);
				if(result)
					return result;

				if(skipView)
				{
					skippedViews.append (v);

					if(v == skipView)
						skipView = nullptr; // want the next matching view

					continue; // ignore this view
				}

				// ignore ancestors of already skipped views
				if(isAncestorOfSkipped (v))
					continue;

				return v;
			}
		EndFor
		return nullptr;
	}

private:
	IView* skipView;
	LinkedList<IView*> skippedViews;

	bool isAncestor (IView* ancestor, IView* child)
	{
		if(IView* parent = child->getParentView ())
			return parent == ancestor || isAncestor (ancestor, parent);

		return false;
	}

	bool isAncestorOfSkipped (IView* view)
	{
		// ignore ancestors of already skipped views
		ListForEach (skippedViews, IView*, skipped)
			if(isAncestor (view, skipped))
				return true;
		EndFor
		return false;
	}
};

} // namespace CCL

#endif // _ccl_viewfinder_h
