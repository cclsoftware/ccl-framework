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
// Filename    : ccl/public/gui/framework/icolorscheme.h
// Description : Color Scheme Interfaces
//
//************************************************************************************************

#ifndef _ccl_icolorscheme_h
#define _ccl_icolorscheme_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/color.h"

namespace CCL {

class FileType;
class MutableCString;
interface IStorable;
interface IAttributeList;
interface IColorSchemeImporter;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Color schemes singleton [IColorSchemes]. */
	DEFINE_CID (ColorSchemes, 0xe7e2611a, 0xdd45, 0x4fe1, 0xaf, 0x76, 0x6f, 0x78, 0x0, 0x28, 0x80, 0x1e)
}

//************************************************************************************************
// IColorScheme
//************************************************************************************************

interface IColorScheme: IUnknown
{
	/** Get color scheme name. */
	virtual StringID CCL_API getName () const = 0;

	/** Get level with given identifier (hue, saturation, etc.). */
	virtual float CCL_API getLevel (StringID id) const = 0;

	enum UpdateMode { kDetect = -1, kIgnore, kForce };

	/** Set level with given identifier. */
	virtual void CCL_API setLevel (StringID id, float value, int updateMode = kDetect) = 0;

	/** Get current color by name. */
	virtual Color CCL_API getColor (StringID name, ColorRef defaultColor = Colors::kBlack) const = 0;

	/** Configure given default level. */
	virtual void CCL_API setDefaultLevel (StringID id, float value) = 0;

	/** Reset all levels to default. */
	virtual void CCL_API resetToDefaults () = 0;
	
	DECLARE_STRINGID_MEMBER (kPersistentPrefix)
	
	// Levels
	DECLARE_STRINGID_MEMBER (kHueLevel)
	DECLARE_STRINGID_MEMBER (kSaturationLevel)
	DECLARE_STRINGID_MEMBER (kLuminanceLevel)
	DECLARE_STRINGID_MEMBER (kContrastLevel)
	DECLARE_STRINGID_MEMBER (kColorInversion)
	DECLARE_STRINGID_MEMBER (kMainSchemeDependent)	///< dependency on main scheme (1 or 0)
	
	DECLARE_IID (IColorScheme)
};

DEFINE_IID (IColorScheme, 0x6ba6f275, 0x52c4, 0x4be6, 0x85, 0x5b, 0x2a, 0xd1, 0x60, 0x74, 0x57, 0xa5)
DEFINE_STRINGID_MEMBER (IColorScheme, kPersistentPrefix, "GUI.ColorSchemes.")
DEFINE_STRINGID_MEMBER (IColorScheme, kHueLevel, "hueLevel")
DEFINE_STRINGID_MEMBER (IColorScheme, kSaturationLevel, "saturationLevel")
DEFINE_STRINGID_MEMBER (IColorScheme, kLuminanceLevel, "luminanceLevel")
DEFINE_STRINGID_MEMBER (IColorScheme, kContrastLevel, "contrastLevel")
DEFINE_STRINGID_MEMBER (IColorScheme, kColorInversion, "colorInversion")
DEFINE_STRINGID_MEMBER (IColorScheme, kMainSchemeDependent, "mainSchemeDependent")

//************************************************************************************************
// IColorSchemes
//************************************************************************************************

interface IColorSchemes: IUnknown
{
	virtual IColorScheme* CCL_API getScheme (StringID name, tbool create = false) = 0;

	virtual const FileType& CCL_API getSchemeFileType () const = 0;

	virtual IStorable* CCL_API createSchemeFile (IColorScheme* schemes[], int count, 
												 IColorSchemeImporter* importer = nullptr, 
												 int revision = 0) = 0;

	DECLARE_IID (IColorSchemes)
};

DEFINE_IID (IColorSchemes, 0x6cb3efef, 0xc27, 0x492b, 0xa7, 0xc4, 0x3d, 0x8b, 0x54, 0x65, 0x8c, 0x36)

//************************************************************************************************
// IColorSchemeImporter
//************************************************************************************************

interface IColorSchemeImporter: IUnknown
{
	/** Adjust color scheme from older revision. */
	virtual void CCL_API adjustScheme (StringID name, IAttributeList& a, int revision) = 0;

	DECLARE_IID (IColorSchemeImporter)
};

DEFINE_IID (IColorSchemeImporter, 0x96fe73f8, 0x6af2, 0x497b, 0x93, 0x3a, 0xa, 0xd3, 0x8f, 0x2f, 0x95, 0xf4)

} // namespace CCL

#endif // _ccl_icolorscheme_h
