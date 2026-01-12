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
// Filename    : ccl/public/gui/framework/ithememanager.h
// Description : Theme Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ithememanager_h
#define _ccl_ithememanager_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

class FileType;
interface ITheme;
interface ITranslationTable;

//************************************************************************************************
// IThemeManager
/** The theme manager maintains a list of loaded themes. 
	\ingroup gui_skin */
//************************************************************************************************

interface IThemeManager: IUnknown
{
	/** Get theme file type. */
	virtual const FileType& CCL_API getThemeFileType () const = 0;

	/** Load theme from package file. */
	virtual tresult CCL_API loadTheme (ITheme*& theme, UrlRef path, StringID themeID, ITranslationTable* table = nullptr, ModuleRef module = nullptr) = 0;

	/** Get theme by identifier. */
	virtual ITheme* CCL_API getTheme (StringID themeID) const = 0;

	/** Get theme by module. */
	virtual ITheme* CCL_API getModuleTheme (ModuleRef module) const = 0;

	/** Get application theme (main module), can be null. */
	virtual ITheme* CCL_API getApplicationTheme () const = 0;

	/** Reload theme. */
	virtual tresult CCL_API reloadTheme (ITheme* theme, tbool keepImages = false) = 0;

	/** Unload theme. */
	virtual tresult CCL_API unloadTheme (ITheme* theme) = 0;

	/** Reload *all* themes. */
	virtual tresult CCL_API reloadAll (tbool keepImages = false) = 0;

	/** Add global location where package files can be found for import. */
	virtual tresult CCL_API addSearchLocation (UrlRef folder) = 0;

	static const String kThemeProtocol; ///< theme protocol identifier

	DECLARE_IID (IThemeManager)
};

DEFINE_IID (IThemeManager, 0x8ea2c465, 0xfa3b, 0x4fe8, 0x98, 0x52, 0xa5, 0x68, 0x15, 0xa3, 0x19, 0xbd)

} // namespace CCL

#endif // _ccl_ithememanager_h
