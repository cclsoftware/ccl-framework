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
// Filename    : viewclass.h
// Description :
//
//************************************************************************************************

#ifndef _viewclass_h
#define _viewclass_h

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/singleton.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {
interface IView;
interface IAttributeList; }

namespace Spy {

class ViewProperty;
class PropertyList;

//************************************************************************************************
// ViewClass
//************************************************************************************************

class ViewClass: public CCL::Object
{
public:
	ViewClass (CCL::StringID className = nullptr, ViewClass* baseClass = nullptr);

	PROPERTY_MUTABLE_CSTRING (className, ClassName)

	virtual ViewClass& getExactClass (CCL::IView* view);
	virtual bool isBaseClassOf (CCL::IView* view);
	virtual CCL::StringID getSkinElementName () const;

	CCL::IImage* getIcon  ();
	ViewProperty* getProperty (CCL::StringID name);
	CCL::Iterator* getProperties ();
	void getProperties (PropertyList& propertyList, CCL::IView* view);

	ViewProperty& addProperty (ViewProperty* p);

protected:
	friend class ViewClassRegistry;
	ViewClass* baseClass;

private:
	CCL::ObjectArray properties;
	CCL::SharedPtr<CCL::IImage> icon;
	bool initialized;

	void init ();
};

//************************************************************************************************
// ViewClassRegistry
//************************************************************************************************

class ViewClassRegistry: public CCL::Object,
						 public CCL::Singleton<ViewClassRegistry>
{
public:
	ViewClass& getClass (CCL::IView* view);
	ViewClass* lookupClass (CCL::StringID className);
	ViewClass* findBaseClass (CCL::IView* view);
	CCL::Iterator* newIterator ();

	ViewClass* addClass (ViewClass* c);
	ViewClass* newClass (CCL::StringID className, ViewClass* baseClass = nullptr);

	ViewClassRegistry ();

private:
	ViewClass* rootClass;
	CCL::AutoPtr<ViewClass> noViewClass;
	CCL::ObjectList classes;
};

//************************************************************************************************
// BaseClassWithInterface
//************************************************************************************************

template<class IFace>
struct BaseClassWithInterface: public ViewClass
{
	BaseClassWithInterface (CCL::StringID className = nullptr, ViewClass* baseClass = nullptr)
	: ViewClass (className, baseClass)
	{}

	bool isBaseClassOf (CCL::IView* view) override
	{
		return CCL::UnknownPtr<IFace> (view).isValid ();
	}
};

//************************************************************************************************
// ViewClassWithSkinName
//************************************************************************************************

struct ViewClassWithSkinName: public ViewClass
{
	ViewClassWithSkinName (CCL::StringID className = nullptr, CCL::StringID skinElementName = nullptr, ViewClass* baseClass = nullptr)
	: ViewClass ("VariantView", baseClass),
	  skinElementName (skinElementName)
	{}

	CCL::StringID getSkinElementName () const override { return skinElementName; }

	CCL::MutableCString skinElementName;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::Iterator* ViewClass::getProperties () { return properties.newIterator (); }

inline CCL::Iterator* ViewClassRegistry::newIterator () { return classes.newIterator (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Spy

#endif // _viewclass_h
