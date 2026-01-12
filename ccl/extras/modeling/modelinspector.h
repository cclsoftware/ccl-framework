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
// Filename    : ccl/extras/modeling/modelinspector.h
// Description : Class Model Inspector
//
//************************************************************************************************

#ifndef _modelinspector_h
#define _modelinspector_h

#include "ccl/app/component.h"

namespace CCL {

interface IImage;
class UrlWithTitle;

namespace Model {
class Element; 
class ClassRepository; }

class PropertyListModel;
class LinkListModel;
class ElementDocumenter;

//************************************************************************************************
// ElementInspector
//************************************************************************************************

class ElementInspector: public Component
{
public:
	DECLARE_CLASS (ElementInspector, Component)

	ElementInspector ();
	~ElementInspector ();

	PROPERTY_POINTER (IObserver, browser, Browser)

	void setInspectedElement (Model::Element* element, IImage* icon = nullptr);
	Model::Element* getInspectedElement () const { return inspectedElement; }

	void setEnabled (bool state);

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	Model::Element* inspectedElement;
	PropertyListModel* propertyList;
	ElementDocumenter* documenter;
};

//************************************************************************************************
// ElementDocumenter
//************************************************************************************************

class ElementDocumenter: public Component
{
public:
	DECLARE_CLASS (ElementDocumenter, Component)

	ElementDocumenter ();
	~ElementDocumenter ();

	DECLARE_STRINGID_MEMBER (kElementDirty)

	void setEnabled (bool state);
	void setTargetElement (Model::Element* element);
	StringRef getLink (int index) const;
	bool setLink (int index, StringRef link);
	void rebuildLinks ();

	ElementInspector* getInspector () const;

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	Model::Element* targetElement;
	LinkListModel* linkList;
	bool inputEnabled;

	void signalDirty ();
};

} // namespace CCL

#endif // _modelinspector_h
