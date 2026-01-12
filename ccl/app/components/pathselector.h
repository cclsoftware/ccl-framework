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
// Filename    : ccl/app/components/pathselector.h
// Description : Path Selector
//
//************************************************************************************************

#ifndef _ccl_pathselector_h
#define _ccl_pathselector_h

#include "ccl/app/component.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/public/gui/idatatarget.h"

namespace CCL {

class PathListModel;

//************************************************************************************************
// PathList
//************************************************************************************************

class PathList: public Object
{
public:
	DECLARE_CLASS (PathList, Object)

	PathList ();

	bool isEmpty () const;
	int getNumPaths () const;
	const Url* getPath (int index) const;
	Iterator* newIterator () const;
	bool contains (UrlRef path) const;
	bool containsSubPath (UrlRef path) const;

	bool addPath (UrlRef path);
	bool removePath (UrlRef path);
	bool removeAt (int index);
	void removeAll ();

	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray paths;
};

//************************************************************************************************
// PathListComponent
//************************************************************************************************

class PathListComponent: public Component
{
public:
	DECLARE_CLASS (PathListComponent, Component)

	PathListComponent (StringRef name = nullptr);
	~PathListComponent ();

	void setPathList (PathList* pathList);

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	enum Tags
	{
		kAddPath = 100,
		kRemovePath
	};

	PathListModel* listModel;
};

//************************************************************************************************
// PathSelector
//************************************************************************************************

class PathSelector: public Component,
					public IDataTarget
{
public:
	DECLARE_CLASS (PathSelector, Component)

	PathSelector (StringRef name = nullptr);
	~PathSelector ();

	virtual void setPath (UrlRef path);
	const Url& getPath () const;

	virtual void enable (bool state);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE (IDataTarget, Component)

protected:
	enum Tags
	{
		kPathString = 100,
		kSelectPath,
		kLastPathTag
	};

	Url& path;

	void runSelector (bool deferred = false);
	AutoPtr<IUrl> toFolderUrl (IUnknown* unk) const;
};

//************************************************************************************************
// PathSelectorWithHistory
//************************************************************************************************

class PathSelectorWithHistory: public PathSelector
{
public:
	DECLARE_CLASS (PathSelectorWithHistory, PathSelector)

	PathSelectorWithHistory (StringRef name = nullptr);
	~PathSelectorWithHistory ();

	PROPERTY_BOOL (clearHistorySupported, ClearHistorySupported)
	void setDefaultPath (UrlRef url);
	bool isDefaultPathSelected () const;

	int addUrl (UrlRef url, StringRef title = nullptr);
	int addVolumes (int typeMask);
	void selectAt (int index);	

	bool storeHistory (Attributes& a, bool includeDefaultPath) const;
	bool restoreHistory (Attributes& a);
	bool storeSettings (StringRef settingsID) const;
	bool restoreSettings (StringRef settingsID);

	// PathSelector
	void setPath (UrlRef path) override;
	void enable (bool state) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
	
protected:
	enum Tags
	{
		kPathHistory = kLastPathTag+1
	};

	Url* defaultPath;
};

} // namespace CCL

#endif // _ccl_pathselector_h
