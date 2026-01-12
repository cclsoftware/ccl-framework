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
// Filename    : ccl/app/documents/recentdocuments.h
// Description : Recent Document Management
//
//************************************************************************************************

#ifndef _ccl_recentdocuments_h
#define _ccl_recentdocuments_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/public/collections/unknownlist.h"

#include "ccl/public/gui/framework/iview.h"

namespace CCL {

class Settings;
interface IMenu;

//************************************************************************************************
// RecentDocuments
//************************************************************************************************

class RecentDocuments: public Component
{
public:
	DECLARE_CLASS (RecentDocuments, Component)

	enum Options { kShowFullPathInMenu = 1<<0 };

	RecentDocuments (StringRef name = nullptr, int maxPathCount = 20, int maxMenuEntries = 20, 
					 int options = 0);
	~RecentDocuments ();

	static StringRef getTranslatedTitle ();

	int count () const;
	const Url* at (int index) const;
	bool contains (UrlRef path) const;
	void setRecentPath (UrlRef path);
	bool removeRecentPath (UrlRef path);
	Iterator* newRecentPathsIterator (bool pinnedFirst = false) const; // of Url objects

	bool isPathPinned (UrlRef path) const;
	void setPathPinned (UrlRef path, bool state);
	Iterator* newPinnedPathsIterator () const; // of Url objects

	void relocate (UrlRef oldUrl, UrlRef newUrl); // urls can be folder or single file
	void clearAll ();

	bool hasMenus () const;
	void addMenu (IMenu* menu);
	void removeMenus ();

	void store ();
	void restore ();

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

protected:
	class Saver;

	Settings& settings;
	int maxPathCount;
	int maxMenuEntries;
	int options;
	ObjectArray paths;
	ObjectArray pinnedPaths;
	UnknownList menus;
	ViewPtr frame;
	Saver* saver;
	bool restoredOnce;

	static bool shouldSaveRelative ();
	static bool getRelativeLocation (Url& folder);
	void commitPaths ();
	void changed (bool saveNeeded = true);
	void updateMenus ();
	Url getSettingsPath () const;
	Url makeBackupPath () const;
	int getPathIndex (UrlRef url, ObjectArray& container) const;
};

} // namespace CCL

#endif // _ccl_recentdocuments_h
