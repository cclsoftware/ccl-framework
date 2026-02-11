//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : xstringparser.h
// Description : Parser
//
//************************************************************************************************

#ifndef _xstringparser_h
#define _xstringparser_h

#include "xstringmodel.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/base/enumdef.h"

namespace CCL {
class StringList;
interface IUrlFilter; }

namespace XString {

//************************************************************************************************
// Parser
//************************************************************************************************

class Parser: public CCL::Object
{
public:
	enum class Result
	{
		kFileOk, ///< File could be processed (not malformed, is supported format)
		kFileInvalid, ///< File has technical issue, may be malformed
		kFileUnsupported, ///< File is ok but not supported
		kFileNotParsed, ///< File has not been parsed yet
		kFileInvalidRoot ///< File root criteria not met
	};

	Parser (Bundle& bundle, UrlRef path);

	static void setBaseFolder (UrlRef path);	///< set base folder for relative references

	PROPERTY_STRING (fileName, FileName)
	PROPERTY_VARIABLE (int, lineNumber, LineNumber)
	
	virtual Result parse () = 0;

protected:
	Bundle& bundle;
	UrlRef path;

	void addKey (StringID scopeName, StringID key);
	void addUnicodeKey (StringID scopeName, StringRef unicodeKey);
};

//************************************************************************************************
// SourceParser
//************************************************************************************************

class SourceParser: public Parser
{
public:
	SourceParser (Bundle& bundle, UrlRef path);

	static CCL::IUrlFilter& getFilter ();

	static CCL::String escape (StringRef string);
	static CCL::String unescape (StringRef string);
	static CCL::MutableCString getLiteral (StringRef line);
	static CCL::MutableCString getLiteralWithKeyword (StringRef line, StringRef keyword);
	static int getMultipleLiterals (CCL::StringList& literals, StringRef line);

	// Parser
	Result parse () override;
};

//************************************************************************************************
// XmlParser
//************************************************************************************************

class XmlModel;
class ScopeProvider;

class XmlParser: public Parser
{
public:
	static CCL::IUrlFilter& getFilter ();

	XmlParser (Bundle& bundle, UrlRef path);

	// Parser
	Result parse () override;

protected:
	XmlModel* model;

	void parseWithChilds (CCL::XmlNode& node);

	virtual void parseNode (CCL::XmlNode& node);
};

//************************************************************************************************
// RootElement
//************************************************************************************************

class RootElement: public CCL::Object
{
public:
	DECLARE_CLASS (RootElement, CCL::Object)

	static RootElement* create (const CCL::Attributes& a);

	PROPERTY_STRING (name, Name)

	bool load (const CCL::Attributes& a);
	bool matches (const CCL::XmlNode& node) const;

protected:
	CCL::ObjectArray conditions;

	RootElement ();
};

//************************************************************************************************
// XmlModel
//************************************************************************************************

class XmlModel: public CCL::Object
{
public:
	DECLARE_CLASS (XmlModel, CCL::Object)

	static XmlModel* create (const CCL::Attributes& a);

	// Common attributes.
	DECLARE_STRINGID_MEMBER (kAttrKind)
	DECLARE_STRINGID_MEMBER (kAttrName)
	DECLARE_STRINGID_MEMBER (kAttrValue)

	// Common objects.
	DECLARE_STRINGID_MEMBER (kScope)

	~XmlModel ();

	PROPERTY_STRING (inherit, Inherit)  ///< Root name of model to inherit.
	PROPERTY_BY_REFERENCE (CCL::StringList, extensions, Extensions) ///< File extensions.

	bool load (const CCL::Attributes& a);
	void merge (const XmlModel& other);
	bool getScope (CCL::String& value, CCL::XmlNode& node) const;
	bool isValidRoot (const CCL::XmlNode& node) const;
	CCL::String getRootName () const;
	CCL::Iterator* newIterator () const;

protected:
	RootElement* root;
	CCL::ObjectArray matchers;
	ScopeProvider* scopeProvider;
	CCL::StringList extensions;

	XmlModel ();
};

//************************************************************************************************
// ModelUtil
//************************************************************************************************

class ModelUtil
{
public:
	/** Load attribute as list or single value. */
	static void loadStrings (CCL::StringList& list, CCL::StringID attrId, const CCL::Attributes& a);

	/** Set value from a list of candidate attributes, priority sensitive. */
	static void setFromPriorityList (CCL::String& value, const CCL::StringList& candidates, const CCL::XmlNode& node);
};

//************************************************************************************************
// ScopeHandler
//************************************************************************************************

class ScopeProvider: public CCL::Object
{
public:
	DECLARE_CLASS (ScopeProvider, CCL::Object)

	static ScopeProvider* create (const CCL::Attributes& a);

	virtual bool get (CCL::String& value, CCL::XmlNode* node) const;
	virtual void load (const CCL::Attributes& a);
};

//************************************************************************************************
// StaticScope
/** Provide static scope value, i.e. not depending on xml node. */
//************************************************************************************************

class StaticScope: public ScopeProvider
{
public:
	DECLARE_CLASS (StaticScope, ScopeProvider)
	DECLARE_STRINGID_MEMBER (kObjectID)

	bool get (CCL::String& value, CCL::XmlNode* node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::String value;
};

//************************************************************************************************
// ParentScope
/** Retrieve scope from a parent xml element. */
//************************************************************************************************

class ParentScope: public ScopeProvider
{
public:
	DECLARE_CLASS (ParentScope, ScopeProvider)
	DECLARE_STRINGID_MEMBER (kObjectID)

	bool get (CCL::String& value, CCL::XmlNode* node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::StringList element; // Parent element name, priority list.
	CCL::StringList attribute; // Name of parent attribute, priority list.
	CCL::String fallback; // Optional: fallback value if parent lookup fails.
};

//************************************************************************************************
// Condition
//************************************************************************************************

class Condition: public CCL::Object
{
public:
	enum Operators
	{
		kEqual = 1 << 0,
		kNotEqual = 1 << 1,

		kDefaultOperator = kEqual
	};

	DECLARE_CLASS (Condition, CCL::Object)
	DECLARE_ENUMINFO (kOperators)

	static Condition* create (const CCL::Attributes& a);

	virtual bool matches (const CCL::XmlNode& node) const;
	virtual void load (const CCL::Attributes& a);

protected:
	int operation;
};

//************************************************************************************************
// ElementNameCondition
/** Check for attribute containing element name. */
//************************************************************************************************

class ElementNameCondition: public Condition
{
public:
	DECLARE_CLASS (ElementNameCondition, Condition)
	DECLARE_STRINGID_MEMBER (kObjectID)

	// Condition
	bool matches (const CCL::XmlNode& node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::String name; ///< Element name to match.
};

//************************************************************************************************
// AttributeValueCondition
/** Check for sibling attribute with a specific value. */
//************************************************************************************************

class AttributeValueCondition: public Condition
{
public:
	DECLARE_CLASS (AttributeValueCondition, Condition)
	DECLARE_STRINGID_MEMBER (kObjectID)

	// Condition
	bool matches (const CCL::XmlNode& node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::String name;  ///< Attribute name to match.
	CCL::String value;  ///< Attribute value to match.
};

//************************************************************************************************
// Matcher
//************************************************************************************************

class Matcher: public CCL::Object
{
public:
	DECLARE_CLASS (Matcher, CCL::Object)

	// Options
	enum Flags
	{
		kSplitValue = 1 << 0 ///< Split value into separate strings

		// ...
	};

	static Matcher* create (const CCL::Attributes& a);

	DECLARE_STRINGID_MEMBER (kOptionSplit)

	~Matcher ();

	PROPERTY_FLAG (flags, kSplitValue, isSplitValue)

	virtual bool getValue (CCL::String& value, const CCL::XmlNode& node) const;
	virtual void load (const CCL::Attributes& a);

	bool matches (const CCL::XmlNode& node) const;
	bool getScope (CCL::String& scope, CCL::XmlNode& node) const;

protected:
	int flags;
	ScopeProvider* scopeProvider;
	CCL::ObjectArray conditions;

	Matcher ();

	virtual bool matchProperties (const CCL::XmlNode& node) const;
};

//************************************************************************************************
// AttributeMatcher
/** Match a node by attribute name, read string from attribute 'name'. */
//************************************************************************************************

class AttributeMatcher: public Matcher
{
public:
	DECLARE_CLASS (AttributeMatcher, Matcher)
	DECLARE_STRINGID_MEMBER (kObjectID)

	// Matcher
	bool getValue (CCL::String& value, const CCL::XmlNode& node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::String name; ///< Name of the attribute providing the string.

	bool matchProperties (const CCL::XmlNode& node) const override;
};

//************************************************************************************************
// ElementMatcher
/** Match a node by element name, read string from 'attribute'. */
//************************************************************************************************

class ElementMatcher: public Matcher
{
public:
	DECLARE_CLASS (ElementMatcher, Matcher)
	DECLARE_STRINGID_MEMBER (kObjectID)

	// Matcher
	bool getValue (CCL::String& value, const CCL::XmlNode& node) const override;
	void load (const CCL::Attributes& a) override;

protected:
	CCL::String name; ///< Name of the element.
	CCL::StringList attribute; ///< Attribute to read string from as priority list.
	bool readText; ///< Try element text before attribute (default: off).

	bool matchProperties (const CCL::XmlNode& node) const override;
};

//************************************************************************************************
// XmlModelRegistry
/** Load and organize models. Maintains a single model per format (root element) name. */
//************************************************************************************************

class XmlModelRegistry: public CCL::StaticSingleton<XmlModelRegistry>
{
public:
	XmlModelRegistry ();

	PROPERTY_STRING (rootFilter, RootFilter) ///< May import model for this root only.

	XmlModel* find (CCL::StringRef rootName) const;
	void load (CCL::UrlRef folder);
	void loadBuiltIns ();
	int countModels () const;
	CCL::Iterator* newIterator () const;

protected:
	static CCL::IUrlFilter& getFilter ();

	CCL::ObjectArray models;

	void add (XmlModel* model);
	void addFromFile (CCL::UrlRef path);
	void loadStream (CCL::IStream& stream, CCL::StringRef fileInfo);
};

} // namespace XString

#endif // _xstringparser_h
