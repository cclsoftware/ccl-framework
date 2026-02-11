//************************************************************************************************
//
// CCL Generator
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
// Filename    : cclgeneratormodel.h
// Description : Generator Tool model
//
//************************************************************************************************

#ifndef _cclgeneratormodel_h
#define _cclgeneratormodel_h

#include "ccl/base/storage/attributes.h"

namespace CCL {

//************************************************************************************************
// MetaFileFormat
//************************************************************************************************

namespace MetaFileFormat
{
	// List attributes
	const CStringPtr kAttrClasses = "classes";
	const CStringPtr kAttrConstants = "constants";
	const CStringPtr kAttrDefinitions = "definitions";
	const CStringPtr kAttrEnumerations = "enums";
	const CStringPtr kAttrEnumerators = "enumerators";
	const CStringPtr kAttrGroups = "groups";

	// Single attributes
	const CStringPtr kAttrBrief = "brief";
	const CStringPtr kAttrDetails = "details";
	const CStringPtr kAttrClass = "class";
	const CStringPtr kAttrType = "type";
	const CStringPtr kAttrName = "name";
	const CStringPtr kAttrValue = "value";
	const CStringPtr kAttrDescription = "description";
	const CStringPtr kAttrExpression = "expression";
	const CStringPtr kAttrAutoValue = "autoValue";
	const CStringPtr kAttrFunction = "function";
	const CStringPtr kAttrArgs = "args";

	// Value 'type'
	const CStringPtr kValueTypeBool = "bool";
	const CStringPtr kValueTypeInt = "int";
	const CStringPtr kValueTypeBigInt = "bigint";
	const CStringPtr kValueTypeFloat = "float";
	const CStringPtr kValueTypeDouble = "double";
	const CStringPtr kValueTypeString = "string";

	// Functions
	const CStringPtr kFunctionIDFourCC = "fourcc"; // calculate four-character code (int)
}

//************************************************************************************************
// LanguageConfigFormat
//************************************************************************************************

namespace LanguageConfigFormat
{
	const CStringPtr kAttrId = "id";

	// Meta type to language specific type.
	const CStringPtr kAttrTypeBool = "boolType";
	const CStringPtr kAttrTypeInt = "intType";
	const CStringPtr kAttrTypeBigInt = "bigIntType";
	const CStringPtr kAttrTypeFloat = "floatType";
	const CStringPtr kAttrTypeDouble = "doubleType";
	const CStringPtr kAttrTypeString = "stringType";

	// Meta value to language specific value.
	const CStringPtr kAttrBoolValueTrue = "boolValueTrue";
	const CStringPtr kAttrBoolValueFalse = "boolValueFalse";
}

//************************************************************************************************
// LanguageConfig
//************************************************************************************************

class LanguageConfig: public Object
{
public:
	DECLARE_CLASS (LanguageConfig, Object)

	static LanguageConfig* createFromAttributes (const Attributes& a);

	LanguageConfig ();

	bool load (const Attributes& a);

	StringID getLanguageID () const;
	String getBoolType () const;
	String getIntType () const;
	String getBigIntType () const;
	String getFloatType () const;
	String getDoubleType () const;
	String getStringType () const;
	String getBoolValueTrue () const;
	String getBoolValueFalse () const;

protected:
	MutableCString languageId;
	Attributes attrs;
};

namespace MetaModel {

class Constant;
class Definition;
class Enumeration;
class Enumerator;
class Group;
class ValueFunction;

//************************************************************************************************
// MetaModel::ModelObject
/** Meta model shared traits. */
//************************************************************************************************

class ModelObject: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ModelObject, Object)

	virtual bool load (const Attributes& a) = 0;
	virtual bool save (Attributes& a) const = 0;
};

//************************************************************************************************
// MetaModel::Root
/** Meta model root object. */
//************************************************************************************************

class Root: public ModelObject
{
public:
	DECLARE_CLASS (Root, ModelObject)

	PROPERTY_STRING (description, Description)

	Root ();

	void add (Constant* constant);
	void add (Definition* definition);
	void add (Enumeration* enumeration);
	void add (Group* group);

	const ObjectArray& getConstants () const;
	const ObjectArray& getDefinitions () const;
	const ObjectArray& getEnums () const;
	const ObjectArray& getGroups () const;

	bool hasData () const;

	// ModelObject
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;

protected:
	ObjectArray definitions; // Optional
	ObjectArray constants; // Optional
	ObjectArray enums; // Optional
	ObjectArray groups; // Optional
};

//************************************************************************************************
// MetaModel::Group
/* Allow grouping of certain elements that would
typically not be grouped via classes. */
//************************************************************************************************

class Group: public Root
{
public:
	DECLARE_CLASS (Group, Root)
	static Group* createFromAttributes (const Attributes& a);

	PROPERTY_MUTABLE_CSTRING (name, Name) // Required

	// Root
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;
};

//************************************************************************************************
// MetaModel::Documented
/** Meta model object documentation properties. */
//************************************************************************************************

class Documented: public ModelObject
{
public:
	DECLARE_CLASS (Documented, ModelObject)

	PROPERTY_STRING (brief, Brief) // Optional
	PROPERTY_STRING (details, Details) // Optional
	PROPERTY_STRING (comment, Comment) // Optional

	// ModelObject
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;
};

//************************************************************************************************
// MetaModel::ValueFunction
//************************************************************************************************

class ValueFunction: public ModelObject
{
public:
	DECLARE_CLASS (ValueFunction, ModelObject)
	static ValueFunction* createFromAttributes (const Attributes& a);

	PROPERTY_STRING (name, Name) // Required, function call ID.

	const Vector<Variant>& getArgs () const;

protected:
	Vector<Variant> args; ///< Function call arguments.

	// ModelObject
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;
};

//************************************************************************************************
// MetaModel::TypedValue
/** Pair a value with a type attribute and an optional 'expression' state.
 The expression state may flag literals that should not imply string data
 type value. Example: a constant value may be of type 'int' but the associated
 value is a string in the format "1<<0". */
//************************************************************************************************

class TypedValue: public Documented
{
public:
	DECLARE_CLASS (TypedValue, Documented)

	PROPERTY_VARIABLE (String, value, Value) // Required
	PROPERTY_MUTABLE_CSTRING (valueType, ValueType) // Optional, default: int
	PROPERTY_BOOL (expression, Expression) // Value is an expression, examples: "log(2)", "1<<0"

	TypedValue ();
	ValueFunction* getValueFunction () const;

	// Documented
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;

protected:
	AutoPtr<ValueFunction> valueFunction;
};

//************************************************************************************************
// MetaModel::Assignment
/** [TYPE] LH -> RH assignment, specifying LH type,
 may imply a statement like #define LH RH or value assignment LH = RH. */
//************************************************************************************************

class Assignment: public TypedValue
{
public:
	DECLARE_CLASS (Assignment, TypedValue)

	PROPERTY_MUTABLE_CSTRING (name, Name) // Required

	// TypedValue
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;
};

//************************************************************************************************
// MetaModel::Definition
/** Meta model object representing a definition statement. */
//************************************************************************************************

class Definition: public Assignment
{
public:
	DECLARE_CLASS (Definition, Assignment)
	static Definition* createFromAttributes (const Attributes& a);
};

//************************************************************************************************
// MetaModel::Constant
/** Meta model object representing a constant value. */
//************************************************************************************************

class Constant: public Assignment
{
public:
	DECLARE_CLASS (Constant, Assignment)
	static Constant* createFromAttributes (const Attributes& a);
};

//************************************************************************************************
// MetaModel:: Enumeration
/** Meta model object representing an enumeration. Assumes integer
 type. TODO, future improvement: add type attribute. */
//************************************************************************************************

class Enumeration: public Documented
{
public:
	DECLARE_CLASS (Enumeration, Documented)
	static Enumeration* createFromAttributes (const Attributes& a);

	PROPERTY_MUTABLE_CSTRING (name, Name) // Required
	PROPERTY_BOOL (autoValue, AutoValue) // Optional, auto assign enumerator values based on their index

	Enumeration ();
	const ObjectArray& getEnumerators () const;
	void addEnumerator (Enumerator* enumerator);

	// Documented
	bool load (const Attributes& a) override;
	bool save (Attributes& a) const override;

protected:
	ObjectArray enumerators;
};

//************************************************************************************************
// MetaModel::Enumerator
/** Meta model object representing an enumerator inside an enumeration.
 Reminder: this is a typed value to handle expressions. TODO, future: typically
 the enumerator type should be inferred from the parent enum. */
//************************************************************************************************

class Enumerator: public Assignment
{
public:
	DECLARE_CLASS (Enumerator, Assignment)
	static Enumerator* createFromAttributes (const Attributes& a);
};

} // GeneratorModel
} // namespace CCL

#endif // _cclgeneratormodel_h
