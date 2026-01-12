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
// Filename    : ccl/gui/theme/usertheme.h
// Description : UserTheme class
//
//************************************************************************************************

#ifndef _ccl_usertheme_h
#define _ccl_usertheme_h

#include "ccl/gui/theme/theme.h"

namespace CCL {

class FileType;
class SkinWizard;
interface ITranslationTable;

//************************************************************************************************
// UserTheme
//************************************************************************************************

class UserTheme: public Theme
{
public:
	DECLARE_CLASS (UserTheme, Theme)

	UserTheme (StringID themeID = nullptr, ITranslationTable* table = nullptr, ModuleRef module = nullptr);
	~UserTheme ();

	static const FileType& getFileType ();

	ModuleRef getModuleReference () const;
	bool load (UrlRef path);
	bool reload (bool keepImages = false);

	// Theme
	StringID CCL_API getThemeID () const override;
	IUnknown* CCL_API getResource (StringID name) override;
	IGradient* CCL_API getGradient (StringID name) override;
	IImage* CCL_API getImage (StringID name) override;
	IView* CCL_API createView (StringID name, IUnknown* controller, IAttributeList* arguments = nullptr) override;
	void getVariables (IAttributeList& list) const override;
	void setZoomFactor (float factor) override;
	float getZoomFactor () const override;

	CLASS_INTERFACES (Theme)

protected:
	SkinWizard* skinWizard;
};

} // namespace CCL

#endif // _ccl_usertheme_h
