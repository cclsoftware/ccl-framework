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
// Filename    : ccl/app/paramalias.h
// Description : Parameter Alias
//
//************************************************************************************************

#ifndef _ccl_paramalias_h
#define _ccl_paramalias_h

#include "ccl/app/params.h"

#include "ccl/public/base/iformatter.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

//************************************************************************************************
// AliasParam
//************************************************************************************************

class AliasParam: public Object,
				  public IParameter,
				  public IAliasParameter
{
public:
	DECLARE_CLASS (AliasParam, Object)
	DECLARE_METHOD_NAMES (AliasParam)
	DECLARE_PROPERTY_NAMES (AliasParam)
	static void initMethodNames ();

	AliasParam (StringID name = nullptr);
	~AliasParam ();

	PROPERTY_VARIABLE (int, paramType, ParamType) ///< assign explicit parameter type (optional)
	PROPERTY_BOOL (hasExplicitIdentity, HasExplicitIdentity) ///< alias has its own parameter identity
	PROPERTY_BOOL (hasExplicitFormatter, HasExplicitFormatter) ///< alias has its own formatter

	// IAliasParameter
	void CCL_API setOriginal (IParameter* p) override;
	tbool CCL_API hasOriginal () const override;

	// IParameter (specific to alias)
	StringID CCL_API getName () const override;
	void CCL_API setName (StringID name) override;
	void CCL_API connect (IParamObserver* controller, int tag) override;
	int CCL_API getTag () const override;
	IUnknown* CCL_API getController () const override;

	// IParameter (delegated to original)
	IParameter* CCL_API getOriginal () override;
	IUnknown* CCL_API createIdentity () override;
	int CCL_API getType () const override;
	tbool CCL_API isEnabled () const override;
	void CCL_API enable (tbool state) override;
	tbool CCL_API getState (int mask) const override;
	void CCL_API setState (int mask, tbool state) override;
	int CCL_API getVisualState () const override;
	void CCL_API setVisualState (int state) override;
	void CCL_API performUpdate () override;
	void CCL_API beginEdit () override;
	void CCL_API endEdit () override;
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update = false) override;
	Variant	CCL_API getMin () const override;
	Variant	CCL_API getMax () const override;
	void CCL_API setMin (VariantRef min) override;
	void CCL_API setMax (VariantRef max) override;
	Variant CCL_API getDefaultValue () const override;
	void CCL_API setDefaultValue (VariantRef value) override;
	Variant CCL_API boundValue (VariantRef value) const override;
	tbool CCL_API canIncrement () const override;
	int CCL_API getPrecision () const override;
	tbool CCL_API setPrecision (int precision) override;
	void CCL_API increment () override;
	void CCL_API decrement () override;
	void CCL_API takeValue (const IParameter& param, tbool update = false) override;
	float CCL_API getNormalized () const override;
	void CCL_API setNormalized (float value, tbool update = false) override;
	float CCL_API getValueNormalized (VariantRef value) const override;
	Variant CCL_API getValuePlain (float valueNormalized) const override;
	IParamCurve* CCL_API getCurve () const override;
	void CCL_API setCurve (IParamCurve* curve) override;
	IFormatter* CCL_API getFormatter () const override;
	void CCL_API setFormatter (IFormatter* formatter) override;
	void CCL_API getString (String& string, VariantRef value) const override;
	void CCL_API toString (String& string) const override;
	void CCL_API fromString (StringRef string, tbool update = false) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	static Vector<MetaClass::MethodDefinition> aliasMethodNames;

	int tag;
	int flags;
	MutableCString name;
	IParameter* param;
	ISubject* paramSubject;
	SharedPtr<IFormatter> ownFormatter;
	IParamObserver* controller;

	PROPERTY_FLAG (flags, kStorable, hasStorableFlag)
	PROPERTY_FLAG (flags, kPublic, hasPublicFlag)
	PROPERTY_FLAG (flags, kFeedback, hasFeedbackFlag)

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// MultiParamProxyHandler
//************************************************************************************************

class MultiParamProxyHandler: public Object,
	                          public IParamObserver
{
public:
	DECLARE_CLASS (MultiParamProxyHandler, Object)

	enum Type ///< if sources are different, proxy value is set to...
	{
		kMinValue,	///< minimum of all sources (for toggle = all must be set)
		kMaxValue,	///< maximum of all sources (for toggle = any must be set)
		kReset
	};

	MultiParamProxyHandler (Type type = kMinValue, IParameter* proxy = nullptr, bool notifyOriginalController = false);
	~MultiParamProxyHandler ();

	/** proxy will be connected to 'this'. Original controller will be called (IParamObserver) when notifyOriginalController is true*/
	void setProxyParam (IParameter* proxy, bool notifyOriginalController = false);
	IParameter* getProxyParam () const;
	void setValue (VariantRef value, bool update = false);
	void syncSources (bool update);

	void addSourceParameter (IParameter* source);
	void removeSourceParameter (IParameter* source);
	void removeAllSources (bool triggerSync = true);
	int countSources () const;
	IParameter* getSource (int index) const;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IParamObserver, Object)

private:
	SharedPtr <IParameter> proxy;
	Vector <SharedPtr <IParameter> > sources;
	Type type;
	IParamObserver* originalController;

	DECLARE_STRINGID_MEMBER (kSyncProxyParameter)

	void syncProxyParameter ();
	void triggerProxyParameterSync ();
};

} // namespace CCL

#endif // _ccl_paramalias_h
