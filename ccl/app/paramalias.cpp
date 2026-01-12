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
// Filename    : ccl/app/paramalias.cpp
// Description : Parameter Alias
//
//************************************************************************************************

#include "ccl/app/paramalias.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (AliasParam, kFrameworkLevelFirst)
{
	AliasParam::initMethodNames ();
	return true;
}

//************************************************************************************************
// AliasParam
//************************************************************************************************

DEFINE_CLASS (AliasParam, Object)
DEFINE_CLASS_NAMESPACE (AliasParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (AliasParam, 0x249d8e02, 0xe1d6, 0x43e0, 0x8b, 0xe5, 0xa6, 0xc7, 0xdb, 0x57, 0xe0, 0xce)
Vector<MetaClass::MethodDefinition> AliasParam::aliasMethodNames;

//////////////////////////////////////////////////////////////////////////////////////////////////

AliasParam::AliasParam (StringID name)
: tag (0),
  flags (0),
  name (name),
  param (nullptr),
  paramSubject (nullptr),
  controller (nullptr),
  paramType (-1),
  hasExplicitIdentity (false),
  hasExplicitFormatter (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AliasParam::~AliasParam ()
{
	hasFeedbackFlag (false); // don't query for IObserver in dtor!
	setOriginal (nullptr);

	signal (Message (kDestroyed));
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AliasParam::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IParameter)
	QUERY_INTERFACE (IAliasParameter)

	Object::queryInterface (iid, ptr);
	if(*ptr == nullptr && param)
		param->queryInterface (iid, ptr);

	return *ptr == nullptr ? kResultNoInterface : kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setOriginal (IParameter* p)
{
	ASSERT (p != this)
	if(p == this)
		return;

	if(p != param)
	{
		if(paramSubject)
			paramSubject->removeObserver (this);

		param = p;
		paramSubject = UnknownPtr<ISubject> (p);

		if(paramSubject)
			paramSubject->addObserver (this);

		if(hasFeedbackFlag ())
			if(UnknownPtr<IObserver> observer = controller)
				observer->notify (this, Message (kChanged, kOriginalChanged));

		signal (Message (kChanged, kOriginalChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::hasOriginal () const
{
	return param != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API AliasParam::getOriginal ()
{
	return param ? param->getOriginal () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API AliasParam::createIdentity ()
{
	if(isHasExplicitIdentity ())
		return Parameter::createIdentity (this);
	else
		return param ? param->createIdentity () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::notify (ISubject* subject, MessageRef msg)
{
	if(paramSubject && subject == paramSubject)
	{
		if(msg == kDestroyed)
		{
			setOriginal (nullptr);
			signal (Message (kOriginalDestroyed));
		}
		else
		{
			if(hasFeedbackFlag ())
				if(UnknownPtr<IObserver> observer = controller)
					observer->notify (this, msg);

			signal (msg);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AliasParam::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setName (StringID name)
{
	this->name = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::connect (IParamObserver* controller, int tag)
{
	this->tag = tag;
	this->controller = controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AliasParam::getTag () const
{
	return tag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API AliasParam::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AliasParam::getType () const
{
	return paramType != -1 ? paramType : param ? param->getType () : kToggle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::isEnabled () const
{
	return param && param->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::enable (tbool state)
{
	if(param)
		param->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::getState (int mask) const
{
	if(mask == kStorable)
		return hasStorableFlag ();
	if(mask == kPublic)
		return hasPublicFlag ();
	if(mask == kFeedback)
		return hasFeedbackFlag ();

	if(param)
		return param->getState (mask);
	else
	{
		if(mask == kOutOfRange) // alias is out of range without original
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setState (int mask, tbool state)
{
	if(mask == kStorable)
		hasStorableFlag (state != 0);
	else if(mask == kPublic)
		hasPublicFlag (state != 0);
	else if(mask == kFeedback)
		hasFeedbackFlag (state != 0);
	else if(param)
		param->setState (mask, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AliasParam::getVisualState () const
{
	return param ? param->getVisualState () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setVisualState (int state)
{
	if(param)
		param->setVisualState (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::performUpdate ()
{
	if(param)
		param->performUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::beginEdit ()
{
	if(param)
		param->beginEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::endEdit ()
{
	if(param)
		param->endEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API AliasParam::getValue () const
{
	if(param)
		return param->getValue ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setValue (VariantRef value, tbool update)
{
	if(param)
		param->setValue (value, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::takeValue (const IParameter& _param, tbool update)
{
	if(param)
		param->takeValue (_param, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API AliasParam::getMin () const
{
	if(param)
		return param->getMin ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API AliasParam::getMax () const
{
	if(param)
		return param->getMax ();
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setMin (VariantRef min)
{
	if(param)
		param->setMin (min);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setMax (VariantRef max)
{
	if(param)
		param->setMax (max);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API AliasParam::getDefaultValue () const
{
	if(param)
		return param->getDefaultValue ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setDefaultValue (VariantRef value)
{
	if(param)
		param->setDefaultValue (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API AliasParam::boundValue (VariantRef value) const
{
	if(param)
		return param->boundValue (value);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::canIncrement () const
{
	return param ? param->canIncrement () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AliasParam::getPrecision () const
{
	return param ? param->getPrecision () : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::setPrecision (int precision)
{
	return param ? param->setPrecision (precision) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::increment ()
{
	if(param)
		param->increment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::decrement ()
{
	if(param)
		param->decrement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API AliasParam::getNormalized () const
{
	return param ? param->getNormalized () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setNormalized (float value, tbool update)
{
	if(param)
		param->setNormalized (value, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API AliasParam::getValueNormalized (VariantRef value) const
{
	return param ? param->getValueNormalized (value) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API AliasParam::getValuePlain (float valueNormalized) const
{
	return param ? param->getValuePlain (valueNormalized) : Variant ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamCurve* CCL_API AliasParam::getCurve () const
{
	return param ? param->getCurve () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setCurve (IParamCurve* curve)
{
	if(param)
		param->setCurve (curve);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* CCL_API AliasParam::getFormatter () const
{
	if(isHasExplicitFormatter ())
		return ownFormatter;
	else
		return param ? param->getFormatter () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::setFormatter (IFormatter* formatter)
{
	if(isHasExplicitFormatter ())
	{
		ownFormatter = formatter;
		deferChanged ();
	}
	else if(param)
		param->setFormatter (formatter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::getString (String& string, VariantRef value) const
{
	if(param)
	{
		if(isHasExplicitFormatter () && ownFormatter)
		{
			if(ownFormatter->isNormalized ())
				ownFormatter->printString (string, getValueNormalized (value));
			else
				ownFormatter->printString (string, value);		
		}
		else
			param->getString (string, value);
	}
	else
		string.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::toString (String& string) const
{
	if(param)
	{
		if(isHasExplicitFormatter () && ownFormatter)
			getString (string, getValue ());	
		else
			param->toString (string);		
	}
	else
		string.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AliasParam::fromString (StringRef string, tbool update)
{
	if(param)
	{
		if(isHasExplicitFormatter () && ownFormatter)
		{
			Variant v;
			if(ownFormatter->isNormalized ())
			{
				if(ownFormatter->scanString (v, string))
					setNormalized (v, update);
			}
			else
			{
				if(ownFormatter->scanString (v, string))
					setValue (v, update);
			}		
		}
		else
			param->fromString (string, update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AliasParam::initMethodNames ()
{
	if(aliasMethodNames.isEmpty ())
	{
		auto addMethodsOf = [] (const ITypeInfo& paramClass)
		{
			if(const ITypeInfo::MethodDefinition* methodNames = paramClass.getMethodNames ())
				for(int i = 0; methodNames[i].name != nullptr; i++)
					aliasMethodNames.add (methodNames[i]);
		};

		// collect method definitions of Parameter classes
		addMethodsOf (ccl_typeid<AliasParam> ()); // our own table (below)
		addMethodsOf (ccl_typeid<Parameter> ());
		addMethodsOf (ccl_typeid<ListParam> ());
		addMethodsOf (ccl_typeid<ImageProvider> ());

		MetaClass::MethodDefinition end = { nullptr, nullptr, nullptr };
		aliasMethodNames.add (end);

		MetaClass::MethodNamesModifier (__class, aliasMethodNames);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// Keep in sync with Parameter properties.
BEGIN_PROPERTY_NAMES (AliasParam)
	DEFINE_PROPERTY_TYPE ("value", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("default", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("type", ITypeInfo::kInt)
	DEFINE_PROPERTY_TYPE ("min", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("max", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("name", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("string", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("enabled", ITypeInfo::kBool)
	DEFINE_PROPERTY_TYPE ("signalAlways", ITypeInfo::kBool)
	DEFINE_PROPERTY_TYPE ("reverse", ITypeInfo::kBool)
END_PROPERTY_NAMES (AliasParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::getProperty (Variant& var, MemberID propertyId) const
{
	UnknownPtr<IObject> paramObj (param);
	return paramObj ? paramObj->getProperty (var, propertyId) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::setProperty (MemberID propertyId, const Variant& var)
{
	UnknownPtr<IObject> paramObj (param);
	return paramObj ? paramObj->setProperty (propertyId, var) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (AliasParam)
	DEFINE_METHOD_ARGS ("setOriginal", "original: Parameter")
	DEFINE_METHOD_ARGR ("hasOriginal", "", "bool")
	DEFINE_METHOD_ARGS ("setFeedbackNeeded", "needed: bool")
END_METHOD_NAMES (AliasParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AliasParam::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setOriginal")
	{
		setOriginal (UnknownPtr<IParameter> (msg[0]));
		return true;
	}
	else if(msg == "hasOriginal")
	{
		returnValue = hasOriginal ();
		return true;
	}
	else if(msg == "setFeedbackNeeded")
	{
		setFeedbackNeeded (msg[0].asBool ());
		return true;
	}

	UnknownPtr<IObject> paramObj (param);
	if(paramObj && paramObj->invokeMethod (returnValue, msg))
		return true;

	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// MultiParamProxyHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiParamProxyHandler, Object)
DEFINE_STRINGID_MEMBER_ (MultiParamProxyHandler, kSyncProxyParameter, "syncProxyParameter")

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiParamProxyHandler::MultiParamProxyHandler (Type _type, IParameter* _representative, bool notifyOriginalController)
: type (_type),
  originalController (nullptr)
{
	setProxyParam (_representative, notifyOriginalController);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiParamProxyHandler::~MultiParamProxyHandler ()
{
	removeAllSources (false); // don't trigger sync from dtor
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::setProxyParam (IParameter* newProxyParameter, bool notifyOriginalController)
{
	proxy = newProxyParameter;
	if(proxy && notifyOriginalController)
	{
		if(originalController == nullptr)
		{
			originalController = UnknownPtr<IParamObserver> (proxy->getController ());
			ASSERT (originalController != this)
			if(originalController == this)
				originalController = nullptr;
		}
	}
	else
		originalController = nullptr;

	if(proxy)
		proxy->connect (this, proxy->getTag ()); // keep original tag!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* MultiParamProxyHandler::getProxyParam () const
{
	return proxy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::setValue (VariantRef value, bool update)
{
	if(proxy)
	{
		proxy->setValue (value); // no update here
		syncSources (update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::triggerProxyParameterSync ()
{
	(NEW Message (kSyncProxyParameter))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::addSourceParameter (IParameter* source)
{
	sources.add (source);
	ISubject::addObserver (source, this);
	triggerProxyParameterSync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::removeSourceParameter (IParameter* source)
{
	ISubject::removeObserver (source, this);
	sources.remove (source);
	triggerProxyParameterSync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::removeAllSources (bool triggerSync)
{
	for(int i = 0, count = countSources (); i < count; i++)
		ISubject::removeObserver (getSource (i), this);
	sources.removeAll ();
	if(triggerSync)
		triggerProxyParameterSync ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MultiParamProxyHandler::countSources () const
{
	return sources.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* MultiParamProxyHandler::getSource (int index) const
{
	return sources.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiParamProxyHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kSyncProxyParameter)
	{
		syncProxyParameter ();
	}
	else if(msg == kChanged)
	{
		if(UnknownPtr<IParameter> param = subject)
			triggerProxyParameterSync ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MultiParamProxyHandler::paramChanged (IParameter* param)
{
	if(param == proxy)
	{
		if(originalController)
			originalController->paramChanged (param);

		syncSources (true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiParamProxyHandler::paramEdit (IParameter* param, tbool begin)
{
	if(originalController)
		originalController->paramEdit (param, begin);

	if(param == proxy)
	{
		for(int i = 0, count = countSources (); i < count; i++)
		{
			IParameter* sourceParam = getSource (i);
			if(sourceParam->isEnabled ())
			{
				if(begin)
					sourceParam->beginEdit ();
				else
					sourceParam->endEdit ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::syncProxyParameter ()
{
	if(proxy)
	{
		int sourceCount = countSources ();

		bool anyEnabled = false;
		for(auto& s : sources)
			if(s->isEnabled ())
			{
				anyEnabled = true;
				break;
			}
		proxy->enable (anyEnabled);

		if(sourceCount > 0)
		{
			Variant value;
			for(int i = 0; i < sourceCount; i++)
			{
				Variant sourceValue = getSource (i)->getValue ();
				if(type == kReset)
				{
					if(sourceValue > getSource (i)->getMin ())
					{
						value = true;
						break;
					}
				}
				else
				{
					if(i == 0)
						value = sourceValue;
					else
					{
						if(type == kMinValue)
							value = ccl_min (value, sourceValue);
						else if(type == kMaxValue)
							value = ccl_max (value, sourceValue);
					}
				}
			}

			proxy->setValue (value);
			if(type == kReset)
				proxy->enable (value.asBool ());
		}
		else
			proxy->setValue (proxy->getMin ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiParamProxyHandler::syncSources (bool update)
{
	if(proxy)
	{
		Variant value = proxy->getValue ();
		for(int i = 0, count = countSources (); i < count; i++)
		{
			IParameter* sourceParam = getSource (i);
			if(sourceParam->isEnabled ())
			{
				if(type == kReset)
					value = sourceParam->getMin ();

				if(sourceParam->getValue () != value)
					sourceParam->setValue (value, update);
				else if(update && sourceParam->isSignalAlways ())
					sourceParam->performUpdate ();
			}
		}
	}
}
