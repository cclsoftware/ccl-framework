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
// Filename    : ccl/gui/dialogs/fileselector.h
// Description : File Selector
//
//************************************************************************************************

#ifndef _ccl_fileselector_h
#define _ccl_fileselector_h

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/ifileselector.h"

namespace CCL {

class Url;
class View;

//************************************************************************************************
// FileSelector
//************************************************************************************************

class FileSelector: public Object,
					public IFileSelector
{
public:
	DECLARE_CLASS (FileSelector, Object)
	DECLARE_PROPERTY_NAMES (FileSelector)
	DECLARE_METHOD_NAMES (FileSelector)

	FileSelector ();
	~FileSelector ();

	IUnknown* getHook () const;
	View* createCustomView ();

	UrlRef getInitialFolder () const;
	StringRef getInitialFileName () const;

	// IFileSelector
	void CCL_API addFilter (const FileType& type) override;
	int CCL_API countFilters () const override;
	const FileType* CCL_API getFilter (int index = 0) const override;
	void CCL_API setHook (IUnknown* hook) override;
	void CCL_API setFolder (UrlRef path) override;
	void CCL_API setFileName (StringRef path) override;
	tbool CCL_API run (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) override;
	IAsyncOperation* CCL_API runAsync (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) override;
	int  CCL_API countPaths () const override;
	IUrl* CCL_API getPath (int index = 0) const override;
	int CCL_API getSaveBehavior () const override;
	void CCL_API setSaveContent (UrlRef url) override;

	CLASS_INTERFACE (IFileSelector, Object)

protected:
	ObjectArray filters;
	ObjectArray paths;
	IUnknown* hook;
	View* customView;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	mutable Url initialFolder;
	String initialFileName;
};

//************************************************************************************************
// NativeFileSelector
//************************************************************************************************

class NativeFileSelector: public FileSelector
{
public:
	DECLARE_CLASS (NativeFileSelector, FileSelector)

	static NativeFileSelector* create ();

	PROPERTY_VARIABLE (int, selectedType, SelectedType)

	// FileSelector
	tbool CCL_API run (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) override;
	IAsyncOperation* CCL_API runAsync (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) override;

protected:
	NativeFileSelector ();

	// platform-specific:
	virtual bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window);
	virtual IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window);
};

//************************************************************************************************
// NativeFolderSelector
//************************************************************************************************

class NativeFolderSelector: public Object,
							public IFolderSelector
{
public:
	DECLARE_CLASS (NativeFolderSelector, Object)
	DECLARE_METHOD_NAMES (NativeFolderSelector)

	Url getInitialPath () const;

	// IFolderSelector
	void CCL_API setPath (UrlRef path) override;
	UrlRef CCL_API getPath () const override;
	tbool CCL_API run (StringRef title = nullptr, IWindow* window = nullptr) override;
	IAsyncOperation* CCL_API runAsync (StringRef title = nullptr, IWindow* window = nullptr) override;

	CLASS_INTERFACE (IFolderSelector, Object)

protected:
	AutoPtr<Url> path;

	NativeFolderSelector ();

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// platform-specific:
	virtual bool runPlatformSelector (StringRef title, IWindow* window);
	virtual IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window);
};

} // namespace CCL

#endif // _ccl_fileselector_h
