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
// Filename    : ccl/app/fileinfo/volumeinfocomponent.h
// Description : Volume Info Component
//
//************************************************************************************************

#ifndef _ccl_volumeinfocomponent_h
#define _ccl_volumeinfocomponent_h

#include "ccl/app/fileinfo/fileinfocomponent.h"

#include "ccl/public/system/inativefilesystem.h"

namespace CCL {

//************************************************************************************************
// VolumeInfoComponent
//************************************************************************************************

class VolumeInfoComponent: public StandardFileInfo
{
public:
	DECLARE_CLASS (VolumeInfoComponent, StandardFileInfo)

	VolumeInfoComponent ();
	~VolumeInfoComponent ();

	virtual void update ();

	// FileInfoComponent
	tbool CCL_API setFile (UrlRef path) override;

protected:
	Url* currentPath;
	int volumeType;
	VolumeInfo info;
	bool valid;

	virtual bool accepts (UrlRef path);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// PackageVolumeInfo
//************************************************************************************************

class PackageVolumeInfo: public VolumeInfoComponent
{
public:
	DECLARE_CLASS (PackageVolumeInfo, VolumeInfoComponent)

	PackageVolumeInfo ();

	static void setDefaultVolumeSubType (StringRef defaultType);
	static bool isPackageVolume (UrlRef path, StringRef subType = nullptr);

	PROPERTY_STRING (volumeSubType, VolumeSubType)

protected:
	static String defaultVolumeSubType;

	// VolumeInfoComponent
	bool accepts (UrlRef path) override;
	void update () override;
	tbool CCL_API setDisplayAttributes (IImage* icon, StringRef title) override;
};

} // namespace CCL

#endif // _ccl_volumeinfocomponent_h
