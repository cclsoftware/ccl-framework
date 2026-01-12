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
// Filename    : ccl/app/editing/addins/editaddindescription.h
// Description : Edit Add-in Description
//
//************************************************************************************************

#ifndef _ccl_editaddindescription_h
#define _ccl_editaddindescription_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IClassDescription;
interface IPluginMetaClass;

//************************************************************************************************
// EditAddInDescription
//************************************************************************************************

class EditAddInDescription: public StorableObject
{
public:
	DECLARE_CLASS (EditAddInDescription, StorableObject)

	EditAddInDescription ();

	class Registrar;
	class Category;

	PROPERTY_STRING (themeName, ThemeName)
	PROPERTY_STRING (formName, FormName)
	PROPERTY_STRING (groupName, GroupName)

	// runtime information
	PROPERTY_POINTER (IPluginMetaClass, pluginMetaClass, PluginMetaClass)
	PROPERTY_POINTER (IUnknown, windowClass, WindowClass)

	// StorableObject
	bool load (const Storage& storage) override;
};

//************************************************************************************************
// EditAddInDescription::Category
//************************************************************************************************

class EditAddInDescription::Category: public Object
{
public:
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (windowClassPath, WindowClassPath)
};

//************************************************************************************************
// EditAddInDescription::Registrar
//************************************************************************************************

class EditAddInDescription::Registrar: public Object,
									   public Singleton<Registrar>
{
public:
	Registrar ();
	~Registrar ();

	static const CString kCommandCategory;

	/** Define add-in category (windowClassPath: controller path to EditAddInCollection). */
	void defineCategory (StringRef name, StringRef windowClassPath);

	/** Register view commands for all add-ins. */
	void registerAddInCommands ();

	/** Called by EditAddInCollection. */
	void onAddInsInitialize (bool state);

	static bool isHidden (const IClassDescription& classInfo);

protected:
	ObjectArray categories;
	ObjectArray descriptions;
	int useCount;

	void registerAddInClasses (StringRef subCategory, StringRef windowClassPath);
	void unregisterAll ();
};

} // namespace CCL

#endif // _ccl_editaddindescription_h
