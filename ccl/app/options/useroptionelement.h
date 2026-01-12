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
// Filename    : ccl/app/options/useroptionelement.h
// Description : User Option Element
//
//************************************************************************************************

#ifndef _ccl_useroptionelement_h
#define _ccl_useroptionelement_h

#include "ccl/base/storage/configuration.h"
#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// UserOptionElement
//************************************************************************************************

class UserOptionElement: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (UserOptionElement, Object)

	UserOptionElement (IParameter* editParam); ///< takes ownership!

	PROPERTY_SHARED_AUTO (IParameter, editParam, EditParam)
	PROPERTY_SHARED_AUTO (IParameter, labelParam, LabelParam)
	void setLabel (StringRef label);

	virtual void init () = 0;

	virtual bool needsApply () const = 0;

	virtual void apply () = 0;

protected:
	void getEditValue (Variant& value) const;
	void setEditValue (VariantRef value);
};

//************************************************************************************************
// ConfigurationElement
//************************************************************************************************

class ConfigurationElement: public UserOptionElement
{
public:
	DECLARE_CLASS_ABSTRACT (ConfigurationElement, UserOptionElement)

	ConfigurationElement (StringID section, StringID key, IParameter* editParam);

	PROPERTY_MUTABLE_CSTRING (section, Section)
	PROPERTY_MUTABLE_CSTRING (key, Key)

	/** Function called when value is applied. */
	typedef void (*ApplyCallback) ();
	PROPERTY_VARIABLE (ApplyCallback, applyCallback, ApplyCallback)

	PROPERTY_BOOL (needsRedraw, RedrawNeeded)

	// UserOptionElement
	void init () override;
	bool needsApply () const override;
	void apply () override;

protected:
	void getCurrentValue (Variant& value) const;
	void setCurrentValue (VariantRef value);

	virtual Configuration::IRegistry& getRegistry () const;
};

//************************************************************************************************
// FrameworkOptionElement
//************************************************************************************************

class FrameworkOptionElement: public ConfigurationElement
{
public:
	DECLARE_CLASS_ABSTRACT (FrameworkOptionElement, ConfigurationElement)

	FrameworkOptionElement (StringID section, StringID key, IParameter* editParam);

protected:
	// ConfigurationElement
	Configuration::IRegistry& getRegistry () const override;
};

} // namespace CCL

#endif // _ccl_useroptionelement_h
