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
// Filename    : ccl/gui/theme/thememanager.h
// Description : Theme Manager
//
//************************************************************************************************

#ifndef _ccl_thememanager_h
#define _ccl_thememanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/gui/graphics/igraphicscleanup.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/ithememanager.h"

namespace CCL {

class Theme;

//************************************************************************************************
// ThemeManager
//************************************************************************************************

class ThemeManager: public Object,
					public IThemeManager,
					public IGraphicsCleanup,
					public Singleton<ThemeManager>
{
public:
	DECLARE_CLASS_ABSTRACT (ThemeManager, Object)
	DECLARE_METHOD_NAMES (ThemeManager)

	ThemeManager ();
	~ThemeManager ();

	Theme& getDefaultTheme ();	    ///< do we need this???
	Theme& getFrameworkTheme ();	///< internal theme of GUI framework

	void onSystemMetricsChanged ();
	void onSystemColorsChanged ();
	void onSystemFontsChanged ();
	
	// IThemeManager
	const FileType& CCL_API getThemeFileType () const override;
	tresult CCL_API loadTheme (ITheme*& theme, UrlRef path, StringID themeID, ITranslationTable* table = nullptr, ModuleRef module = nullptr) override;
	ITheme* CCL_API getTheme (StringID themeID) const override;
	ITheme* CCL_API getModuleTheme (ModuleRef module) const override;
	ITheme* CCL_API getApplicationTheme () const override;
	tresult CCL_API reloadTheme (ITheme* theme, tbool keepImages = false) override;
	tresult CCL_API unloadTheme (ITheme* theme) override;
	tresult CCL_API reloadAll (tbool keepImages = false) override;
	tresult CCL_API addSearchLocation (UrlRef folder) override;

	CLASS_INTERFACE (IThemeManager, Object)

protected:
	ObjectList themes;
	Theme* defaultTheme;
	Theme* frameworkTheme;
	ITranslationTable* frameworkStrings;

	void updateSystemMetrics ();
	void updateSystemColors ();
	void updateSystemFonts ();
	
	// IGraphicsCleanup
	void cleanupGraphics () override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_thememanager_h
