//************************************************************************************************
//
// CCL Spy
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
// Filename    : viewclasses.cpp
// Description : Special view classes
//
//************************************************************************************************

#include "viewclass.h"
#include "viewproperty.h"
#include "shadowview.h"

#include "ccl/base/trigger.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iusercontrol.h"
#include "ccl/public/gui/framework/iview3d.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

namespace Spy {

//************************************************************************************************
// BoxLayoutView
//************************************************************************************************

struct BoxLayoutView: public ViewClass
{
	BoxLayoutView (ViewClass* baseClass)
	: ViewClass ("BoxLayoutView", baseClass)
	{}

	ViewClass& getExactClass (IView* view) override
	{
		return baseClass->getExactClass (view);
	}
};

//************************************************************************************************
// LayoutView
//************************************************************************************************

struct AnchorLayoutView: public ViewClass
{
	AnchorLayoutView ()
	: ViewClass ("AnchorLayoutView")
	{
		ViewClassRegistry::instance ().addClass (classHorizontal = NEW ViewClass (TAG_HORIZONTAL, this));
		ViewClassRegistry::instance ().addClass (classVertical = NEW ViewClass (TAG_VERTICAL, this));
		ViewClassRegistry::instance ().addClass (classTable = NEW ViewClass (TAG_TABLE, this));
		ViewClassRegistry::instance ().addClass (NEW BoxLayoutView (this));

		addProperty (NEW ObjectProperty ("Spacing", ATTR_SPACING));
		addProperty (NEW ObjectProperty ("Margin", ATTR_MARGIN));

		classTable->addProperty (NEW ObjectProperty ("Rows", ATTR_ROWS));
		classTable->addProperty (NEW ObjectProperty ("Columns", ATTR_COLUMNS));
		classTable->addProperty (NEW ObjectProperty ("Cellratio", ATTR_CELLRATIO));
		classTable->addProperty (NEW ObjectProperty ("Mincellratio", ATTR_MINCELLRATIO));
	}

	ViewClass& getExactClass (IView* view) override
	{
		ViewBox vb (view);
		Variant layout;
		if(vb.getAttribute (layout, ATTR_LAYOUTCLASS))
		{
			MutableCString layoutClass (layout.asString ());
			if(layoutClass == LAYOUTCLASS_BOX)
			{
				if(ViewBox (view).getStyle ().isCommonStyle (Styles::kVertical))
					return *classVertical;
				else
					return *classHorizontal;
			}
			else if(layoutClass == LAYOUTCLASS_TABLE)
				return *classTable;

			if(ViewClass* c = ViewClassRegistry::instance ().lookupClass (layoutClass))
				return *c;

			return *ViewClassRegistry::instance ().newClass (layoutClass, this);
		}
		return *this;
	}
private:
	ViewClass* classHorizontal;
	ViewClass* classVertical;
	ViewClass* classTable;
};

//************************************************************************************************
// LayoutView
//************************************************************************************************

struct LayoutView: public ViewClass
{
	LayoutView ()
	: ViewClass ("LayoutView")
	{
		ViewClassRegistry::instance ().addClass (classAnchorLayout = NEW AnchorLayoutView ());
		ViewClassRegistry::instance ().addClass (classFlexbox = NEW ViewClass (TAG_FLEXBOX, this));
		
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXDIRECTION));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXWRAP));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXJUSTIFY));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXALIGN));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXPADDING));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXPADDINGTOP));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXPADDINGRIGHT));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXPADDINGBOTTOM));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXPADDINGLEFT));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXGAP));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXGAPROW));
		classFlexbox->addProperty (NEW FlexContainerProperty (ATTR_FLEXGAPCOLUMN));
	}

	ViewClass& getExactClass (IView* view) override
	{
		ViewBox vb (view);
		Variant layout;
		if(vb.getAttribute (layout, ATTR_LAYOUTCLASS))
		{
			MutableCString layoutClass (layout.asString ());
			if(layoutClass == LAYOUTCLASS_FLEXBOX)
				return *classFlexbox;
		}
		
		return *this;
	}
private:
	ViewClass* classAnchorLayout;
	ViewClass* classFlexbox;
};

//************************************************************************************************
// UserControl(Host)
//************************************************************************************************

struct UserControl: public ViewClass
{
	UserControl ()
	: ViewClass ("UserControlHost")
	{}

	ViewClass& getExactClass (IView* view) override
	{
		UnknownPtr<IUserControlHost> host (view);
		if(host)
		{
			if(IUserControl* control = host->getUserControl ())
			{
				UnknownPtr<IObject> object (control);
				if(object)
				{
					CString className (object->getTypeInfo ().getClassName ());
					if(ViewClass* c = ViewClassRegistry::instance ().lookupClass (className))
					{
						if(className == "ShadowView")
							return c->getExactClass (view);
						return *c;
					}

					return *ViewClassRegistry::instance ().newClass (className, this);
				}
			}
		}
		return *this;
	}
};

//************************************************************************************************
// ShadowViewClass (placeholder for a foreign view)
//************************************************************************************************

struct ShadowViewClass: public ViewClass
{
	struct NativeSizeProperty: SizeProperty
	{
		bool getValue (Variant& value, IView* view) override
		{
			if(ShadowView* shadowView = ShadowView::cast_IView<ShadowView> (view))
				return assignSize (value, shadowView->getNativeSize ());

			return SizeProperty::getValue (value, view);
		}
	};

	ShadowViewClass ()
	: ViewClass ("ShadowView", &ViewClassRegistry::instance ().getClass (nullptr)) // base class "noView"
	{
		addProperty (NEW UserControlObjectProperty ("Source code", "source"));
		addProperty (NEW UserControlObjectProperty ("name", "name"));
		addProperty (NEW NativeSizeProperty).setName ("Size");

		//addProperty (NEW UserControlObjectProperty ("style.backColor", Core::Portable::ViewAttributes::kBackColor));
		//addProperty (NEW UserControlObjectProperty ("style.foreColor", Core::Portable::ViewAttributes::kForeColor));
		// ...
	}

	ViewClass& getExactClass (IView* view) override
	{
		UnknownPtr<IUserControlHost> host (view);
		IUserControl* control = host ? host->getUserControl () : nullptr;
		UnknownPtr<IObject> object (control);

		MutableCString className (Property (object, "Class").get ().asString ());
		className += " "; // to distinguish from CCL classes with same name

		// create new subClass with real className
		if(ViewClass* c = ViewClassRegistry::instance ().lookupClass (className))
			return *c;

		return *ViewClassRegistry::instance ().newClass (className, this);
	}
};

//************************************************************************************************
// Form
//************************************************************************************************

struct Form: public BaseClassWithInterface<IForm>
{
	Form ()
	{
		setClassName ("Form");
		addProperty (NEW FormNameProperty).setName ("FormName");
	}
};

//************************************************************************************************
// Control
//************************************************************************************************

struct Control: public ViewClass
{
	Control (ViewClass* baseClass)
	: ViewClass ("Control", baseClass)
	{
		addProperty (NEW ParamNameProperty).setName ("Parameter");
		addProperty (NEW ParamValueProperty).setName ("Value");
	}
};

//************************************************************************************************
// CommandParamControl
//************************************************************************************************

struct CommandParamControl: public ViewClass
{
	CommandParamControl (ViewClass* baseClass)
	: ViewClass ("Control", baseClass)
	{
		addProperty (NEW ParamCommandProperty).setName ("Value");
	}
};

//************************************************************************************************
// ControlBase
//************************************************************************************************

struct ControlBase: public BaseClassWithInterface<IControl>
{
	ControlBase (CCL::StringID className = nullptr, ViewClass* baseClass = nullptr)
	: BaseClassWithInterface<IControl> (className, baseClass)
	{
		// internal classes (not registered)
		controlClass = NEW Control (this);
		commandParamControl = NEW CommandParamControl (this);
	}

	ViewClass& getExactClass (IView* view) override
	{
		UnknownPtr<IControl> control (view);
		if(control && UnknownPtr<ICommandParameter> (control->getParameter ()).isValid ())
			return *commandParamControl;

		return *controlClass;
	}

	AutoPtr<ViewClass> controlClass;
	AutoPtr<ViewClass> commandParamControl;
};

//************************************************************************************************
// Scene3DViewClass
//************************************************************************************************

struct Scene3DViewClass: public BaseClassWithInterface<ISceneView3D>
{
	Scene3DViewClass ()
	{
		setClassName ("SceneView3D");
		addProperty (NEW SceneNode3DProperty).setName ("Scene");
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT (ViewClasses)
{
	ControlBase* controlBase = nullptr;
	ViewClassRegistry::instance ().addClass (NEW Form);
	ViewClassRegistry::instance ().addClass (NEW LayoutView);
	ViewClassRegistry::instance ().addClass (NEW ShadowViewClass);
	ViewClassRegistry::instance ().addClass (NEW UserControl);
	ViewClassRegistry::instance ().addClass (controlBase = NEW ControlBase);
	ViewClassRegistry::instance ().addClass (NEW ViewClassWithSkinName ("VariantView", "Variant", controlBase->controlClass));
	ViewClassRegistry::instance ().addClass (NEW ViewClassWithSkinName ("HelpInfoView", "HelpInfo")); 
	ViewClassRegistry::instance ().addClass (NEW BaseClassWithInterface<IWindowBase>)->setClassName ("WindowBase");
	ViewClassRegistry::instance ().addClass (NEW BaseClassWithInterface<IScrollView>)->setClassName ("ScrollView");
	ViewClassRegistry::instance ().addClass (NEW Scene3DViewClass);
	return true;
}

} // namespace Spy
