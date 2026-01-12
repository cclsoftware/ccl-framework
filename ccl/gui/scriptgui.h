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
// Filename    : ccl/gui/scriptgui.h
// Description : Scripting GUI
//
//************************************************************************************************

#ifndef _ccl_scriptgui_h
#define _ccl_scriptgui_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class Theme;
interface ICodeResourceLoaderHook;

//************************************************************************************************
// IScriptComponent
//************************************************************************************************

interface IScriptComponent: IUnknown
{
	virtual void CCL_API construct (IUnknown* outerComponent) = 0;

	DECLARE_IID (IScriptComponent)
};

//************************************************************************************************
// ScriptComponent
//************************************************************************************************

class ScriptComponent: public Object,
					   public AbstractNode,
					   public AbstractController,
					   public IParamObserver,
					   public IScriptComponent
{
public:
	DECLARE_CLASS (ScriptComponent, Object)
	DECLARE_METHOD_NAMES (ScriptComponent)
	
	ScriptComponent (IUnknown* scriptObject = nullptr);
	~ScriptComponent ();

	// IObjectNode
	IObjectNode* CCL_API getRoot () const override;
	int CCL_API countChildren () const override;
	IObjectNode* CCL_API getChild  (int index) const override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	StringRef CCL_API getObjectID () const override;

	// IController
	IParameter* CCL_API findParameter (StringID name) const override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	IObject* scriptObject;
	ObjectArray functions;
	Vector<IObjectNode*> children;

	// IScriptComponent
	void CCL_API construct (IUnknown* outerComponent) override;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ScriptGUIHost
/** Object accessed from script applications via "Host.GUI". */
//************************************************************************************************

class ScriptGUIHost: public Object,
					 public Singleton<ScriptGUIHost>
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptGUIHost, Object)
	DECLARE_METHOD_NAMES (ScriptGUIHost)
	DECLARE_PROPERTY_NAMES (ScriptGUIHost)

	ScriptGUIHost ();
	~ScriptGUIHost ();

	ICodeResourceLoaderHook* getHook ();

	class ConstantList: public Object
	{
	public:
		DECLARE_CLASS (ConstantList, Object)

		DECLARE_STYLEDEF (valueNames)

		// IObject
		tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
		tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	};

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

protected:
	ICodeResourceLoaderHook* loaderHook;
	ConstantList* constantList;

	int runDialog (Theme* theme, StringID form, IUnknown* controller, int buttons);
};

} // namespace CCL

#endif // _ccl_scriptcomponent_h
