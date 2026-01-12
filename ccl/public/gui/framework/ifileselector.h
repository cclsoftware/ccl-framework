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
// Filename    : ccl/public/gui/framework/ifileselector.h
// Description : File Selector Interface
//
//************************************************************************************************

#ifndef _ccl_ifileselector_h
#define _ccl_ifileselector_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

class FileType;
interface IWindow;
interface IParameter;
interface IFileSelectorCustomize;
interface IAsyncOperation;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (FileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf);
	DEFINE_CID (FolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc);
}

//************************************************************************************************
// IFileSelector
/** File selector interface. 
Created with ClassID::FileSelector
\ingroup gui_dialog */
//************************************************************************************************

interface IFileSelector: IUnknown
{
	/** File selector types. */
	enum Type
	{
		kOpenFile,				///< open file
		kOpenMultipleFiles,		///< open multiple files
		kSaveFile				///< save file
	};

	/** Add file type filter. */
	virtual void CCL_API addFilter (const FileType& type) = 0;

	/** Get number of file type filters. */
	virtual int CCL_API countFilters () const = 0;
	
	/** Get file type filter by index. */
	virtual const FileType* CCL_API getFilter (int index) const = 0;
 
	/** Set hook object (IFileSelectorHook for notifications, IViewFactory for custom view on macOS). */
	virtual void CCL_API setHook (IUnknown* hook) = 0;

	/** Set initial folder. */
	virtual void CCL_API setFolder (UrlRef path) = 0;

	/** Set initial filename. */
	virtual void CCL_API setFileName (StringRef fileName) = 0;

	/** Run file selector. */
	virtual tbool CCL_API run (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) = 0;

	/** Run file selector asynchronously. */
	virtual IAsyncOperation* CCL_API runAsync (int type, StringRef title = nullptr, int filterIndex = 0, IWindow* window = nullptr) = 0;

	/** Get number of paths selected. */
	virtual int CCL_API countPaths () const = 0;

	/** Get path from result list. */
	virtual IUrl* CCL_API getPath (int index = 0) const = 0;

	/** Save behavior. */
	enum SaveBehavior
	{
		kSaveCreatesFile = 1<<0,	///< an empty file might already exist at the path returned in kSaveFile mode
		kSaveNeedsContent = 1<<1	///< the content to be saved must exist as a file beforing running the selector; use setSaveContent ()
	};

	/** Get platform-specific save behavior. */
	virtual int CCL_API getSaveBehavior () const = 0;

	/** Set content to be saved. */
	virtual void CCL_API setSaveContent (UrlRef url) = 0;

	DECLARE_IID (IFileSelector)
};

DEFINE_IID (IFileSelector, 0xabcbbf1b, 0xa5b, 0x4194, 0x94, 0x98, 0x6, 0x49, 0x5e, 0xfd, 0x7e, 0x99)

//************************************************************************************************
// IFileSelectorHook
/** File selector hook interface.
\ingroup gui_dialog */
//************************************************************************************************

interface IFileSelectorHook: IUnknown
{
	/** Selected file has changed. */
	virtual void CCL_API onSelectionChanged (IFileSelector& fs, UrlRef path) = 0;

	/** Selected file type filter has changed. */
	virtual void CCL_API onFilterChanged (IFileSelector& fs, int filterIndex) = 0;

	/** Customize file selector (Windows only). */
	virtual void CCL_API onCustomize (IFileSelectorCustomize& fsc) = 0;

	DECLARE_IID (IFileSelectorHook)
};

DEFINE_IID (IFileSelectorHook, 0x6e0c65a9, 0x4242, 0x4496, 0x9c, 0x37, 0xf5, 0x56, 0x89, 0x58, 0xd2, 0x8c)

//************************************************************************************************
// IFileSelectorCustomize
/** File selector customization interface (Windows only).
\ingroup gui_dialog */
//************************************************************************************************

interface IFileSelectorCustomize: IUnknown
{
	/** Begin group of controls. */
	virtual void CCL_API beginGroup (StringRef title) = 0;

	/** End group of controls. */
	virtual void CCL_API endGroup () = 0;
	
	/** Add text box. */
	virtual void CCL_API addTextBox (IParameter* p) = 0;
	
	/** Add button. */
	virtual void CCL_API addButton (IParameter* p, StringRef title) = 0;
	
	/** Add check box. */
	virtual void CCL_API addCheckBox (IParameter* p, StringRef title) = 0;

	DECLARE_IID (IFileSelectorCustomize)
};

DEFINE_IID (IFileSelectorCustomize, 0xeedba81d, 0xeda5, 0x4db6, 0x87, 0x30, 0x5a, 0x6a, 0x4c, 0xeb, 0xe4, 0x4a)

//************************************************************************************************
// IFolderSelector
/** Folder selector interface. 
\ingroup gui_dialog */
//************************************************************************************************

interface IFolderSelector: IUnknown
{
	/** Set initial path. */
	virtual void CCL_API setPath (UrlRef path) = 0;

	/** Get last selected path. */
	virtual UrlRef CCL_API getPath () const = 0;

	/** Run folder selector. */
	virtual tbool CCL_API run (StringRef title = nullptr, IWindow* window = nullptr) = 0;

	/** Run folder selector asynchronously. */
	virtual IAsyncOperation* CCL_API runAsync (StringRef title = nullptr, IWindow* window = nullptr) = 0;

	DECLARE_IID (IFolderSelector)
};

DEFINE_IID (IFolderSelector, 0x487ec5b4, 0x196b, 0x48f5, 0x86, 0x0, 0xe6, 0x41, 0xeb, 0xa2, 0xb9, 0x21)

} // namespace CCL

#endif // _ccl_ifileselector_h
