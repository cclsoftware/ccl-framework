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
// Filename    : ccl/gui/help/helpinfobuilder.h
// Description : Help Info Builder
//
//************************************************************************************************

#ifndef _ccl_helpinfobuilder_h
#define _ccl_helpinfobuilder_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/ipresentable.h"

namespace CCL {

//************************************************************************************************
// HelpInfoBuilder
//************************************************************************************************

class HelpInfoBuilder: public Object,
					   public IHelpInfoBuilder,
					   public IPresentable
{
public:
	DECLARE_CLASS (HelpInfoBuilder, Object)
	DECLARE_METHOD_NAMES (HelpInfoBuilder)

	HelpInfoBuilder ();
	~HelpInfoBuilder ();

	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_VARIABLE (uint32, ignoreModifiers, IgnoreModifiers)

	class OptionItem: public Object
	{
	public:
		OptionItem ();

		enum Flags { kActive = 1<<0 };

		PROPERTY_VARIABLE (int, flags, Flags)
		PROPERTY_FLAG (flags, kActive, isActive)
		PROPERTY_VARIABLE (uint32, modifiers, Modifiers)
		PROPERTY_SHARED_AUTO (IImage, icon, Icon)
		PROPERTY_STRING (text, Text)
	};

	// IHelpInfoBuilder
	void CCL_API setAttribute (AttrID id, VariantRef value) override;
	void CCL_API addOption (uint32 modifiers, IImage* icon, StringRef text) override;
	void CCL_API addOption (uint32 modifiers, StringID iconName, StringRef text) override;
	void CCL_API setActiveOption (uint32 modifiers) override;

	// IPresentable
	IImage* CCL_API createImage (const Point& size, const IVisualStyle& style) override;
	IView* CCL_API createView (const Rect& size, const IVisualStyle& style) override;
	String CCL_API createText () override;

	CLASS_INTERFACE2 (IHelpInfoBuilder, IPresentable, Object)

protected:
	ObjectArray options;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// HelpInfoCollection
//************************************************************************************************

class HelpInfoCollection: public Object,
						  public IHelpInfoCollection
{
public:
	DECLARE_CLASS (HelpInfoCollection, Object)

	HelpInfoCollection ();

	// IHelpInfoCollection
	IHelpInfoBuilder* CCL_API getInfo (StringID id) const override;
	tresult CCL_API addInfo (StringID id, IHelpInfoBuilder* helpInfo) override;

	CLASS_INTERFACE (IHelpInfoCollection, Object)

protected:
	class Item: public Object
	{
	public:
		PROPERTY_MUTABLE_CSTRING (id, ID)
		PROPERTY_SHARED_AUTO (HelpInfoBuilder, helpInfo, HelpInfo)
	};

	ObjectArray items;
};

} // namespace CCL

#endif // _ccl_controlhelp_h
