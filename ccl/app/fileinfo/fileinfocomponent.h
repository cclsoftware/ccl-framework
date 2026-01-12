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
// Filename    : ccl/app/fileinfo/fileinfocomponent.h
// Description : File Info Component
//
//************************************************************************************************

#ifndef _ccl_fileinfocomponent_h
#define _ccl_fileinfocomponent_h

#include "ccl/app/component.h"
#include "ccl/app/fileinfo/fileinforegistry.h"

namespace CCL {

interface IImageProvider;

//************************************************************************************************
// FileInfoComponent
/** Base class for file info components. */
//************************************************************************************************

class FileInfoComponent: public Component,
						 public IFileInfoComponent
{
public:
	DECLARE_CLASS (FileInfoComponent, Component)

	FileInfoComponent (StringRef name = nullptr, StringID formName = nullptr);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_MUTABLE_CSTRING (skinNamespace, SkinNamespace)
	PROPERTY_BOOL (explicitSkinNamespace, ExplicitSkinNamespace)

	virtual void assignSkinNamespace (StringID skinNamespace);

	// IFileInfoComponent
	tbool CCL_API setFile (UrlRef path) override; ///< override to set parameters
	tbool CCL_API isDefault () override;
	tbool CCL_API setDisplayAttributes (IImage* icon, StringRef title) override;
	tbool CCL_API getFileInfoString (String& result, StringID id) const override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override; ///< tries to create viewName from theme

	static bool isLocal (UrlRef path); ///< check for local file

	CLASS_INTERFACE (IFileInfoComponent, Component)

protected:
	IImageProvider* fileIcon;
};

//************************************************************************************************
// StandardFileInfo
/** Provides standard file information (name, path, size, date).
	Can be used as base class for specialized file info components. */
//************************************************************************************************

class StandardFileInfo: public FileInfoComponent
{
public:
	DECLARE_CLASS (StandardFileInfo, FileInfoComponent)

	StandardFileInfo (StringRef name = CCLSTR ("StandardFileInfo"), StringID formName = "StandardFileInfo");

	// FileInfoComponent
	tbool CCL_API setFile (UrlRef path) override;
	tbool CCL_API setDisplayAttributes (IImage* icon, StringRef title) override;
	tbool CCL_API getFileInfoString (String& result, StringID id) const override;
};

//************************************************************************************************
// TFileInfoFactory
/** Template for a generic FileInfoFactory. The component class is passed as template argument,
	it must implement a static function
		static bool canHandleFile (UrlRef path);
	that tells if the component is suitable for that file. */
//************************************************************************************************

template<class InfoComponent>
class TFileInfoFactory: public FileInfoFactory
{
public:
	// FileInfoFactory
	IFileInfoComponent* CCL_API createComponent (UrlRef path) const override
	{
		if(InfoComponent::canHandleFile (path))
		{
			InfoComponent* component = NEW InfoComponent;
			if(!component->setFile (path))
				safe_release (component);
			return component;
		}
		return nullptr;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTER_FILEINFO(InfoComponent)\
CCL_KERNEL_INIT (InfoComponent##Register) {\
	CCL::FileInfoRegistry::instance ().registerFileInfoFactory (NEW CCL::TFileInfoFactory<InfoComponent>);\
	return true;}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_fileinfocomponent_h
