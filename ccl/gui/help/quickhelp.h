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
// Filename    : ccl/gui/help/quickhelp.cpp
// Description : Quick Help
//
//************************************************************************************************

#ifndef _ccl_quickhelp_h
#define _ccl_quickhelp_h

#include "ccl/gui/views/view.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/collections/hashmap.h"

namespace CCL {

//************************************************************************************************
// QuickHelp
//************************************************************************************************

class QuickHelp: public Object,
				 public IPresentable,
				 public IContextMenuHandler
{
public:
	DECLARE_CLASS (QuickHelp, Object)
	
	QuickHelp ();

	static bool showsHelpIds ();

	bool loadMadcapFile (UrlRef stream);
	View* findView (Point& pos);
	bool setByKey (StringRef key);
	 
	// IPresentable
	IImage* CCL_API createImage (const Point& size, const IVisualStyle& style) override;
	IView* CCL_API createView (const Rect& size, const IVisualStyle& style) override;
	String CCL_API createText () override;

	// IContextMenuHandler
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

	CLASS_INTERFACE2 (IPresentable, IContextMenuHandler, Object)
	
private:
	struct Data
	{
		String title;
		String text;
		
		Data (const Data* other = nullptr)
		{
			if(other)
			{
				title = other->title;
				text = other->text;
			}
		}
		
		void clear () {title.empty (); text.empty ();}
	};

	HashMap<String, Data> table;
	static int hashData (const String& key, int size);

	Point currentPosition;
	View* currentView;
	String currentKey;	
	Data current;
	StringList recentKeys;
	int64 recentKeyStart;

	static constexpr int kMaxRecentKeys = 5;

	void updateRecentKey (StringRef key);
};

} // namespace CCL

#endif // _ccl_quickhelp_h
