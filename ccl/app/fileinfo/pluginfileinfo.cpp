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
// Filename    : ccl/app/fileinfo/pluginfileinfo.cpp
// Description : Plug-in File Info
//
//************************************************************************************************

#include "ccl/app/fileinfo/pluginfileinfo.h"

#include "ccl/app/utilities/pluginclass.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PlugInFileInfoTags
	{
		kName = 100,
		kCategory,
		kVendor,
		kWebsite,
		kHasWebsite,
		kIsFavorite,
		kDescription,
		kHasDescription
	};
}

//************************************************************************************************
// PlugInFileInfo
//************************************************************************************************

REGISTER_FILEINFO (PlugInFileInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInFileInfo::registerInfo () // force linkage
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInFileInfo::canHandleFile (UrlRef path)
{
	return path.getProtocol () == CCLSTR ("class");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PlugInFileInfo, FileInfoComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInFileInfo::PlugInFileInfo ()
: FileInfoComponent (CCLSTR ("PlugInFileInfo"), CSTR ("PlugInFileInfo")),
  pluginsSignalSink (Signals::kPlugIns)
{
	paramList.addString (CSTR ("name"), Tag::kName);
	paramList.addString (CSTR ("category"), Tag::kCategory);
	paramList.addString (CSTR ("vendor"), Tag::kVendor);
	paramList.addString (CSTR ("website"), Tag::kWebsite);
	paramList.addParam (CSTR ("hasWebsite"), Tag::kHasWebsite);
	paramList.addParam (CSTR ("isFavorite"), Tag::kIsFavorite);
	paramList.addString (CSTR ("description"), Tag::kDescription);
	paramList.addParam (CSTR ("hasDescription"), Tag::kHasDescription);

	pluginsSignalSink.setObserver (this);
	pluginsSignalSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInFileInfo::~PlugInFileInfo ()
{
	pluginsSignalSink.enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInFileInfo::setFile (UrlRef path)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (path);
	if(!description)
		return false;

	String name, category, vendor, website, descr;
	IImage* icon = nullptr;

	PlugInClass plugClass (*description);
	icon = plugClass.getIcon ();
	cid = description->getClassID ();

	// prefer localized strings if available
	description->getLocalizedName (name);
	description->getLocalizedSubCategory (category);
	description->getLocalizedDescription (descr);

	const uchar buffer[4] = {0x20, 0xB7, 0x20, 0}; // middot
	String separator (buffer);
	category.replace (Url::strPathChar, separator);

	vendor = plugClass.getClassVendor ();
	if(vendor.isEmpty ())
		vendor = description->getModuleVersion ().getVendor ();

	website = description->getModuleVersion ().getUrl ();

	fileIcon->setImage (icon);

	paramList.byTag (Tag::kName)->fromString (name);
	paramList.byTag (Tag::kVendor)->fromString (vendor);
	paramList.byTag (Tag::kCategory)->fromString (category);
	paramList.byTag (Tag::kWebsite)->fromString (website);
	paramList.byTag (Tag::kHasWebsite)->setValue (!website.isEmpty ());
	paramList.byTag (Tag::kIsFavorite)->setValue (System::GetPluginPresentation ().isFavorite (cid));
	paramList.byTag (Tag::kDescription)->setValue (descr);
	paramList.byTag (Tag::kHasDescription)->setValue (!descr.isEmpty ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInFileInfo::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kClassCategoryChanged)
	{
		// favorite property might have changed (todo: more fine-grained message)
		paramList.byTag (Tag::kIsFavorite)->setValue (System::GetPluginPresentation ().isFavorite (cid));
	}
}
