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
// Filename    : docbrowser.h
// Description : Documentation Browser
//
//************************************************************************************************

#ifndef _docbrowser_h
#define _docbrowser_h

#include "ccl/app/component.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {
class ClassModelBrowser;
class ElementInspector; }

namespace Spy {

class ClassModelDocument;
class DocumentationEditor;

//************************************************************************************************
// DocumentationBrowser
//************************************************************************************************

class DocumentationBrowser: public CCL::Component
{
public:
	DECLARE_CLASS (DocumentationBrowser, Component)

	DocumentationBrowser ();
	~DocumentationBrowser ();

	CCL::ClassModelBrowser* getClassBrowser () const;

	// Component
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;
	CCL::tbool CCL_API paramChanged (CCL::IParameter* param) override;

protected:
	CCL::ClassModelBrowser* classBrowser;
	CCL::ElementInspector* elementInspector;
	DocumentationEditor* documentationEditor;
	CCL::ObjectArray classModels;
	bool scanDone;

	void scanModels ();
	void saveModels ();
	void scanCode ();

	// IObject
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::ClassModelBrowser* DocumentationBrowser::getClassBrowser () const
{ return classBrowser; }

} // namespace Spy

#endif // _docbrowser_h
