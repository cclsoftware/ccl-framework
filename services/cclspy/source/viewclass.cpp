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
// Filename    : viewclass.cpp
// Description : View Class
//
//************************************************************************************************

#include "viewclass.h"
#include "viewproperty.h"
#include "objectinfo.h"

#include "ccl/app/component.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;
using namespace Spy;

//************************************************************************************************
// ViewRootClass ("View")
//************************************************************************************************

class ViewRootClass: public ViewClass
{
public:
	ViewRootClass () : ViewClass ("View")
	{
		addProperty (NEW SourceCodeProperty).setName ("Source code");
		addProperty (NEW ViewAttributeProperty<IView::kTitle>).setName ("Title");
		addProperty (NEW ViewAttributeProperty<IView::kTooltip>).setName ("Tooltip");
		addProperty (NEW SizeProperty).setName ("Size");
		addProperty (NEW SizeLimitsProperty).setName ("SizeLimits");
		addProperty (NEW SizeModeProperty).setName ("SizeMode");
		addProperty (NEW StyleFlagsProperty).setName ("Options");
		addProperty (NEW VisualStyleProperty).setName ("Style");
		addProperty (NEW ViewAttributeProperty<IView::kName>).setName ("Name");
		addProperty (NEW ViewAttributeProperty<IView::kLayerBackingEnabled>).setName ("LayerBacking");
		addProperty (NEW ZoomFactorProperty).setName ("Zoom Factor");
		addProperty (NEW ControllerPathProperty).setName ("Controller");
		addProperty (NEW ObjectProperty ("Helpid", IView::kHelpId));
		
		addProperty (NEW FlexItemProperty (ATTR_FLEXGROW));
		addProperty (NEW FlexItemProperty (ATTR_FLEXSHRINK));
		addProperty (NEW FlexItemProperty (ATTR_FLEXBASIS));
		addProperty (NEW FlexItemProperty (ATTR_FLEXALIGNSELF));
		addProperty (NEW FlexItemProperty (ATTR_FLEXMARGIN));
		addProperty (NEW FlexItemProperty (ATTR_FLEXMARGINTOP));
		addProperty (NEW FlexItemProperty (ATTR_FLEXMARGINRIGHT));
		addProperty (NEW FlexItemProperty (ATTR_FLEXMARGINBOTTOM));
		addProperty (NEW FlexItemProperty (ATTR_FLEXMARGINLEFT));
		addProperty (NEW FlexItemProperty (ATTR_FLEXINSET));
		addProperty (NEW FlexItemProperty (ATTR_FLEXINSETTOP));
		addProperty (NEW FlexItemProperty (ATTR_FLEXINSETRIGHT));
		addProperty (NEW FlexItemProperty (ATTR_FLEXINSETBOTTOM));
		addProperty (NEW FlexItemProperty (ATTR_FLEXINSETLEFT));
		addProperty (NEW FlexItemProperty (ATTR_FLEXPOSITIONTYPE));
		addProperty (NEW FlexItemProperty (ATTR_FLEXSIZEMODE));
	}
};

//************************************************************************************************
// ViewClassRegistry
//************************************************************************************************

DEFINE_SINGLETON (ViewClassRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClassRegistry::ViewClassRegistry ()
: rootClass (NEW ViewRootClass),
  noViewClass (NEW ViewClass)
{
	noViewClass->setClassName ("UnknownView");

	classes.objectCleanup ();
	classes.add (rootClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass* ViewClassRegistry::addClass (ViewClass* c)
{
	if(c->baseClass == nullptr)
		c->baseClass = rootClass;
	classes.add (c);
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass* ViewClassRegistry::newClass (StringID className, ViewClass* baseClass)
{
	ViewClass* viewClass = NEW ViewClass (className, baseClass);
	addClass (viewClass);
	return viewClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass* ViewClassRegistry::lookupClass (StringID className)
{
	ForEach (classes, ViewClass, c)
		if(c->getClassName () == className)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass* ViewClassRegistry::findBaseClass (IView* view)
{
	ForEach (classes, ViewClass, c)
		if(c->isBaseClassOf (view))
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass& ViewClassRegistry::getClass (IView* view)
{
	UnknownPtr<IObject> object (view);
	if(object)
	{
		CString className (object->getTypeInfo ().getClassName ());
		if(ViewClass* c = lookupClass (className))
			return c->getExactClass (view);

		if(ViewClass* baseClass = findBaseClass (view))
			return *newClass (className, &baseClass->getExactClass (view));

		return *newClass (className, rootClass);
	}
	return *noViewClass;
}

//************************************************************************************************
// ViewClass
//************************************************************************************************

ViewClass::ViewClass (StringID className, ViewClass* baseClass)
: className (className),
  baseClass (baseClass),
  icon (nullptr),
  initialized (false)
{
	properties.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewProperty& ViewClass::addProperty (ViewProperty* p)
{
	properties.add (p);
	return *p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewProperty* ViewClass::getProperty (StringID name)
{
	ArrayForEachFast (properties, ViewProperty, p)
		if(p->getName () == name)
			return p;
	EndFor

	if(baseClass)
		return baseClass->getProperty (name);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewClass::getProperties (PropertyList& propertyList, IView* view)
{
	if(baseClass)
		baseClass->getProperties (propertyList, view);

	ArrayForEachFast (properties, ViewProperty, p)
		Variant value;
		if(p->getValue (value, view))
			propertyList.setProperty (p->getName (), value, p);
	EndFor

	propertyList.setProperty ("Class", String (getClassName ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass& ViewClass::getExactClass (IView* view)
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewClass::isBaseClassOf (IView* view)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ViewClass::getSkinElementName () const
{
	return getClassName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewClass::init ()
{
	if(!initialized)
	{
		initialized = true;

		MutableCString iconName	("icon:");
		iconName.append (className);
		icon = RootComponent::instance ().getTheme ()->getImage (iconName);

		if(!icon && baseClass)
			icon = baseClass->getIcon ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ViewClass::getIcon  ()
{
	init ();
	return icon;
}
