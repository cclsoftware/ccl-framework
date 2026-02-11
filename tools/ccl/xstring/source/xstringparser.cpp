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
// Filename    : xstringparser.cpp
// Description : Parser
//
//************************************************************************************************

#include "xstringparser.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPORT_BINARY_RESOURCE (BuiltInModels) // builtinmodels.cpp

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace XString {

//************************************************************************************************
// TranslationXmlTreeParser
//************************************************************************************************

class TranslationXmlTreeParser: public CCL::XmlTreeParser
{
public:
	TranslationXmlTreeParser ();

	// XmlTreeParser
	CCL::tresult CCL_API startElement (CCL::StringRef name, const CCL::IStringDictionary& attributes) override;
	CCL::tresult CCL_API endElement (CCL::StringRef name) override;
	CCL::tresult CCL_API characterData (const CCL::uchar* data, int length, CCL::tbool isCDATA) override;
	CCL::tresult CCL_API processingInstruction (CCL::StringRef target, CCL::StringRef data) override;

protected:
	bool skipping;
};

} // namespace XString

using namespace CCL;
using namespace XString;

static Url theBaseFolder;

//************************************************************************************************
// TranslationXmlTreeParser
//************************************************************************************************

TranslationXmlTreeParser::TranslationXmlTreeParser ()
: skipping (false)
{
	setTextEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationXmlTreeParser::startElement (StringRef name, const CCL::IStringDictionary& attributes)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::startElement (name, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationXmlTreeParser::endElement (StringRef name)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::endElement (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationXmlTreeParser::characterData (const uchar* data, int length, tbool isCDATA)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::characterData (data, length, isCDATA);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationXmlTreeParser::processingInstruction (StringRef _target, StringRef _data)
{
	MutableCString target (_target);
	MutableCString data (_data);

	if(target == "xstring")
	{
		skipping = !data.isEmpty ();
		return kResultOk;
	}

	return XmlTreeParser::processingInstruction (_target, _data);
}

//************************************************************************************************
// Parser
//************************************************************************************************

void Parser::setBaseFolder (UrlRef path)
{
	theBaseFolder.assign (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parser::Parser (Bundle& bundle, UrlRef path)
: bundle (bundle),
  path (path),
  lineNumber (0)
{
	ASSERT (!theBaseFolder.isEmpty ())
	if(!theBaseFolder.isEmpty ())
	{
		Url relPath (path);
		relPath.makeRelative (theBaseFolder);
		fileName = relPath.getPath ();
	}
	else
		fileName = path.getPath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::addKey (StringID scopeName, StringID key)
{
	Reference r (scopeName, fileName, lineNumber);

	int index = bundle.countEntries () + 1;

	String escapedKey (SourceParser::escape (String (key)));
	String escapedScope (SourceParser::escape (String (scopeName)));

	String log;
	log << "(" << index << ") " << fileName << ":" << lineNumber;
	log << " >> [" << escapedScope << "] \"" << escapedKey << "\"";
	Logging::debug (log);
	
	bundle.addOccurance (key, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parser::addUnicodeKey (StringID scopeName, StringRef unicodeKey)
{
	AutoPtr<IXmlWriter> writer (System::CreateXmlWriter ());
	ASSERT (writer != nullptr)
	MutableCString asciiKey;
	writer->encode (asciiKey, unicodeKey);
	addKey (scopeName, asciiKey);
}

//************************************************************************************************
// SourceParser
//************************************************************************************************

IUrlFilter& SourceParser::getFilter ()
{
	static AutoPtr<FileTypeFilter> theFilter;
	if(theFilter == nullptr)
	{
		theFilter = NEW FileTypeFilter;
		theFilter->addFileType (FileType (nullptr, "cpp"));
		theFilter->addFileType (FileType (nullptr, "h"));
		theFilter->addFileType (FileType (nullptr, "js"));
		theFilter->addFileType (FileType (nullptr, "mm"));
	}
	return *theFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static const struct { String escaped; String unescaped; String temp; } escapedChars[] =
{
	{CCLSTR ("\\n"), CCLSTR ("\n"), CCLSTR ("")},
	{CCLSTR ("\\r"), CCLSTR ("\r"), CCLSTR ("")},
	{CCLSTR ("\\t"), CCLSTR ("\t"), CCLSTR ("")},
	{CCLSTR ("\\\""), CCLSTR ("\""), CCLSTR ("\n")}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

String SourceParser::escape (StringRef string)
{
	String result (string);
	for(int i = 0; i < ARRAY_COUNT (escapedChars); i++)
	{
		// workaround for endless loop with " -> \"
		if(escapedChars[i].temp.isEmpty ())
			result.replace (escapedChars[i].unescaped, escapedChars[i].escaped);
		else
		{
			result.replace (escapedChars[i].unescaped, escapedChars[i].temp);
			result.replace (escapedChars[i].temp, escapedChars[i].escaped);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SourceParser::unescape (StringRef string)
{
	String result (string);
	for(int i = 0; i < ARRAY_COUNT (escapedChars); i++)
		result.replace (escapedChars[i].escaped, escapedChars[i].unescaped);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SourceParser::getLiteral (StringRef line)
{
	int start = line.index ("\"");
	int end = line.lastIndex ("\"");
	String string = unescape (line.subString (start + 1, end - start - 1));
	return MutableCString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SourceParser::getLiteralWithKeyword (StringRef line, StringRef keyword)
{
	int keyStart = line.index (keyword);
	if(keyStart == -1)
		return MutableCString ();

	// start after keyword
	String s = line.subString (keyStart + keyword.length ());
	int maxChars = s.length ();
	StringChars chars (s);

	String literalString;
	StringWriter<100> literalWriter (literalString);

	bool inside = false;
	uchar prevChar = 0;
	for(int i = 0; i < maxChars; i++)
	{
		uchar c = chars[i];
		if(inside == false)
		{
			if(c == '\"')
				inside = true;
		}
		else
		{
			if(c == '\"')
			{
				if(prevChar != '\\') // not an "\""
					break;
			}

			literalWriter.append (c);
		}
		prevChar = c;
	}

	literalWriter.flush ();
	return MutableCString (unescape (literalString));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SourceParser::getMultipleLiterals (StringList& literals, StringRef _line)
{
	literals.removeAll ();
	String line (_line);
	while(1)
	{
		int index = line.index ("\"");
		if(index < 0)
			break;

		line = line.subString (index + 1);
		index = line.index ("\"");
		if(index < 0)
			break;

		String lit = unescape (line.subString (0, index));
		literals.add (lit);
		line = line.subString (index + 1);
	}
	return literals.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SourceParser::SourceParser (Bundle& bundle, UrlRef path)
: Parser (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parser::Result SourceParser::parse ()
{
	TextFile sourceFile (path, TextFile::kOpen);
	if(!sourceFile.isValid ())
		return Parser::Result::kFileInvalid;

	MutableCString scopeName;
	bool skipping = false;

	String line;
	for(this->lineNumber = 1; sourceFile->readLine (line); this->lineNumber++)
	{
		static const String kBegin (CCLSTR ("BEGIN_XSTRINGS"));
		static const String kEnd (CCLSTR ("END_XSTRINGS"));
		static const String kXString (CCLSTR ("XSTRING"));
		static const String kJSTranslate (CCLSTR ("JSTRANSLATE"));
		static const String kDefineCommand (CCLSTR ("DEFINE_COMMAND"));
		static const String kHiddenCommand (CCLSTR ("CommandFlags::kHidden"));
		static const String kRegisterCommand (CCLSTR ("REGISTER_COMMAND"));
		static const String kSkipOn (CCLSTR ("XSTRINGS_OFF"));
		static const String kSkipOff (CCLSTR ("XSTRINGS_ON"));

		if(line.contains (kSkipOn))
		{
			skipping = true;
			continue;
		}
		else if(line.contains (kSkipOff))
		{
			skipping = false;
			continue;
		}

		if(skipping)
			continue;

		if(line.contains (kBegin))
		{
			scopeName = getLiteral (line);
		}
		else if(line.contains (kEnd))
		{
			scopeName.empty ();
		}
		else if(line.contains (kXString))
		{
			MutableCString key = getLiteral (line);

			addKey (scopeName, key);
		}
		else if(line.contains (kJSTranslate))
		{
			MutableCString key = getLiteralWithKeyword (line, kJSTranslate);
			if(!key.isEmpty ())
				addKey (scopeName, key);
		}
		else if(line.contains (kDefineCommand) || line.contains (kRegisterCommand))
		{
			// Examples:
			// DEFINE_COMMAND ("Category", "Name", ...)
			// REGISTER_COMMAND ("Category", "Name")

			if(line.contains (kHiddenCommand)) // skip hidden commands
				continue;

			StringList literals;
			if(getMultipleLiterals (literals, line) > 0)
			{
				MutableCString category (literals[0]);
				if(!category.isEmpty ())
					addKey ("Command", category);

				if(literals.count () > 1)
				{
					MutableCString name (literals[1]);
					if(!name.isEmpty ())
						addKey ("Command", name);
				}
			}
		}
	}
	return Parser::Result::kFileOk;
}

//************************************************************************************************
// XmlParser
//************************************************************************************************

IUrlFilter& XmlParser::getFilter ()
{
	static AutoPtr<FileTypeFilter> theFilter;
	if(theFilter == nullptr)
	{
		theFilter = NEW FileTypeFilter;

		// Xml file extensions may vary with model. Registry must be
		// initialized at this point and must not change. TODO: improve.
		IterForEach (XmlModelRegistry::instance ().newIterator (), XmlModel, m)
			m->getExtensions ().forEach ([&](StringRef text)
				{
					FileType ft;
					ft.setExtension (text);
					if(!theFilter->getContent ().contains (ft))
						theFilter->addFileType (ft);
				}
			);
		EndFor
	}
	return *theFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlParser::XmlParser (Bundle& bundle, UrlRef path)
: Parser (bundle, path),
  model (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parser::Result XmlParser::parse ()
{
	TranslationXmlTreeParser parser;
	if(!parser.parse (path))
		return Parser::Result::kFileInvalid;

	if(auto* root = parser.getRoot ())
	{
		// Models are associated by root element name, replacing the need for a doctype.
		model = XmlModelRegistry::instance ().find (root->getName ());
		if(model == nullptr)
			return Parser::Result::kFileUnsupported;

		if(model->isValidRoot (*parser.getRoot ()))
			parseWithChilds (*parser.getRoot ());
		else
			return Parser::Result::kFileInvalidRoot;

		model = nullptr;
	}

	return Parser::Result::kFileOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlParser::parseWithChilds (XmlNode& node)
{
	setLineNumber (node.getLineNumber ()); // update line

	parseNode (node);

	ForEach (node, XmlNode, child)
		parseWithChilds (*child);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlParser::parseNode (XmlNode& node)
{
	if(model == nullptr)
		return;

	// A single node may provide attributes for multiple
	// matchers so always check all of them.
	IterForEach (model->newIterator (), Matcher, matcher)
		if(!matcher->matches (node))
			continue;

		// Fallback to model level scope rule if matcher
		// level scope does not provide a value.
		String scope;
		if(!matcher->getScope (scope, node))
			model->getScope (scope, node);
		ASSERT (!scope.isEmpty ())

		String value;
		if(!matcher->getValue (value, node))
			continue;

		ASSERT (!value.isEmpty ())
		if(matcher->isSplitValue ())
		{
			ForEachStringToken (value, String (","), token)
				token.trimWhitespace ();
				if(!token.isEmpty ())
					addUnicodeKey (MutableCString (scope), token);
			EndFor
		}
		else
			addUnicodeKey (MutableCString (scope), value);

	EndFor
}

//************************************************************************************************
// RootElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RootElement, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

RootElement::RootElement ()
{
	conditions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootElement* RootElement::create (const Attributes& a)
{
	RootElement* root = NEW RootElement;
	if(root->load (a))
		return root;

	safe_release (root);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootElement::load (const Attributes& a)
{
	name = a.getString ("name");
	if(name.isEmpty ())
		return false;

	// Conditions, optional.
	auto* it = a.newQueueIterator ("conditions", ccl_typeid<Attributes> ());
	IterForEach (it, Attributes, attr)
		if(auto* cond = Condition::create (*attr))
			conditions.add (cond);
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootElement::matches (const CCL::XmlNode& node) const
{
	if(name != node.getName ())
		return false;

	ArrayForEach (conditions, Condition, c)
		if(!c->matches (node))
			return false;
	EndFor
	return true;
}

//************************************************************************************************
// XmlModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (XmlModel, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (XmlModel, kAttrKind, "kind")
DEFINE_STRINGID_MEMBER_ (XmlModel, kAttrName, "name")
DEFINE_STRINGID_MEMBER_ (XmlModel, kAttrValue, "value")
DEFINE_STRINGID_MEMBER_ (XmlModel, kScope, "scope")

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlModel* XmlModel::create (const Attributes& a)
{
	XmlModel* model = NEW XmlModel;
	if(model->load (a))
		return model;

	safe_release (model);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlModel::XmlModel ()
: root (nullptr),
  scopeProvider (nullptr)
{
	matchers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlModel::~XmlModel ()
{
	safe_release (scopeProvider);
	safe_release (root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlModel::load (const Attributes& a)
{
	if(Attributes* rootObject = a.getAttributes ("root"))
		root = RootElement::create (*rootObject);

	inherit = a.getString ("inherit");

	// Model may introduce a new format or inherit an existing one.
	ASSERT (root != nullptr || !inherit.isEmpty ())
	if(root == nullptr && inherit.isEmpty ())
		return false;

	ModelUtil::loadStrings (extensions, "extensions", a);
	ASSERT (!extensions.isEmpty ())

	// Model level scope handler, optional.
	if(Attributes* scopeObject = a.getAttributes (XmlModel::kScope))
		scopeProvider = ScopeProvider::create (*scopeObject);

	// List of matchers.
	if(auto* it = a.newQueueIterator ("matchers", ccl_typeid<Attributes> ()))
	{
		IterForEach (it, Attributes, attr)
			if(auto* matcher = Matcher::create (*attr))
				matchers.add (matcher);
		EndFor
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModel::merge (const XmlModel& other)
{
	// Current only use case is to overwrite the file
	// extension. Expand this concept if needed.
	extensions.removeAll ();
	extensions.addAllFrom (other.getExtensions ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlModel::getScope (String& value, XmlNode& node) const
{
	if(scopeProvider == nullptr)
		return false;

	return scopeProvider->get (value, &node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlModel::isValidRoot (const XmlNode& node) const
{
	if(root == nullptr)
		return false;

	return root->matches (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String XmlModel::getRootName () const
{
	if(root == nullptr)
		return String ();

	return root->getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* XmlModel::newIterator () const
{
	return matchers.newIterator ();
}

//************************************************************************************************
// ModelUtil
//************************************************************************************************

void ModelUtil::loadStrings (StringList& list, StringID attrId, const Attributes& a)
{
	if(auto* it = a.newQueueIterator (attrId, ccl_typeid<Attribute> ()))
	{
		IterForEach (it, Attribute, attr)
			String value = attr->getValue ();
			list.addOnce (value);
		EndFor
	}
	else
	{
		String value = a.getString (attrId);
		list.addOnce (value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ModelUtil::setFromPriorityList (String& value, const StringList& candidates, const XmlNode& node)
{
	ForEach (candidates, Boxed::String, a)
		value = node.getAttribute (*a);
		if(!value.isEmpty ())
			break;
	EndFor
}

//************************************************************************************************
// ScopeProvider
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScopeProvider, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScopeProvider* ScopeProvider::create (const Attributes& a)
{
	MutableCString kind = a.getCString (XmlModel::kAttrKind);
	if(kind.isEmpty ())
		return nullptr;

	if(kind == StaticScope::kObjectID)
	{
		ScopeProvider* s = NEW StaticScope;
		s->load (a);
		return s;
	}
	else if(kind == ParentScope::kObjectID)
	{
		ScopeProvider* s = NEW ParentScope;
		s->load (a);
		return s;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScopeProvider::get (String& value, XmlNode* node) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScopeProvider::load (const Attributes& a)
{}

//************************************************************************************************
// StaticScope
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StaticScope, ScopeProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (StaticScope, kObjectID, "static")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StaticScope::get (String& value, XmlNode* node) const
{
	value = this->value;
	return !value.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticScope::load (const Attributes& a)
{
	SuperClass::load (a);
	value = a.getString (XmlModel::kAttrValue);
}

//************************************************************************************************
// ParentScope
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ParentScope, ScopeProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (ParentScope, kObjectID, "parent")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParentScope::get (String& value, XmlNode* node) const
{
	if(node == nullptr)
		return false;

	// Lookup parent candidate with respect to element name priority.
	auto lookupParent = [this] (XmlNode* node) -> XmlNode*
	{
		for(XmlNode* parent = node; parent != nullptr; parent = parent->getParentNode ())
		{
			ForEach (element, Boxed::String, e)
				if(parent->getName () == *e)
					return parent;
			EndFor
		}
		return nullptr;
	};

	if(auto* p = lookupParent (node))
		ModelUtil::setFromPriorityList (value, attribute, *p);

	if(value.isEmpty ())
		value = fallback;

	return !value.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParentScope::load (const Attributes& a)
{
	SuperClass::load (a);

	ModelUtil::loadStrings (element, "element", a);
	ModelUtil::loadStrings (attribute, "attribute", a);
	fallback = a.getString ("fallback");

	ASSERT (!element.isEmpty ())
}

//************************************************************************************************
// Condition
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Condition, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_ENUMINFO (Condition::kOperators)
	{"equal", Condition::kEqual},
	{"notequal", Condition::kNotEqual},
END_ENUMINFO

//////////////////////////////////////////////////////////////////////////////////////////////////

Condition* Condition::create (const Attributes& a)
{
	MutableCString kind = a.getCString (XmlModel::kAttrKind);
	if(kind.isEmpty ())
		return nullptr;

	if(kind == ElementNameCondition::kObjectID)
	{
		auto* c = NEW ElementNameCondition;
		c->load (a);
		return c;
	}
	else if(kind == AttributeValueCondition::kObjectID)
	{
		auto* c = NEW AttributeValueCondition;
		c->load (a);
		return c;
	}
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Condition::matches (const XmlNode& node) const
{
	return true;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void Condition::load (const Attributes& a)
{
	String attr = a.getString ("operator");
	if(attr.isEmpty ())
		operation = Condition::kEqual;
	else
		operation = Core::EnumInfo::parseOne (attr, Condition::kOperators, Condition::kDefaultOperator);
}

//************************************************************************************************
// ElementNameCondition
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ElementNameCondition, Condition)
DEFINE_STRINGID_MEMBER_ (ElementNameCondition, kObjectID, "element")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementNameCondition::matches (const XmlNode& node) const
{
	switch(operation)
	{
	case Condition::kEqual:
		return node.getName () == name;
	case Condition::kNotEqual:
		return node.getName () != name;
	default:
		break;
	}

	return false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementNameCondition::load (const Attributes& a)
{
	SuperClass::load (a);
	name = a.getString (XmlModel::kAttrName);
}

//************************************************************************************************
// AttributeValueCondition
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AttributeValueCondition, Condition)
DEFINE_STRINGID_MEMBER_ (AttributeValueCondition, kObjectID, "attribute")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AttributeValueCondition::matches (const XmlNode& node) const
{
	switch(operation)
	{
	case Condition::kEqual:
		return node.getAttribute (name) == value;
	case Condition::kNotEqual:
		return node.getAttribute (name) != value;
	default:
		break;
	}

	return false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValueCondition::load (const Attributes& a)
{
	SuperClass::load (a);
	name = a.getString (XmlModel::kAttrName);
	value = a.getString (XmlModel::kAttrValue);
}

//************************************************************************************************
// Matcher
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Matcher, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (Matcher, kOptionSplit, "split")

//////////////////////////////////////////////////////////////////////////////////////////////////

Matcher::Matcher ()
: flags (0),
  scopeProvider (nullptr)
{
	conditions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Matcher* Matcher::create (const Attributes& a)
{
	MutableCString kind = a.getCString (XmlModel::kAttrKind);
	if(kind.isEmpty ())
		return nullptr;

	if(kind == AttributeMatcher::kObjectID)
	{
		auto* matcher = NEW AttributeMatcher;
		matcher->load (a);
		return matcher;

	}
	else if(kind == ElementMatcher::kObjectID)
	{
		auto* matcher = NEW ElementMatcher;
		matcher->load (a);
		return matcher;
	}
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Matcher::~Matcher ()
{
	safe_release (scopeProvider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Matcher::load (const Attributes& a)
{
	// Options list.
	StringList options;
	ModelUtil::loadStrings (options, "options", a);
	ForEach (options, Boxed::String, opt)
		if(*opt == String (Matcher::kOptionSplit))
			isSplitValue (true);
	EndFor

	// Parse optional scope handler.
	if(Attributes* scopeObject = a.getAttributes (XmlModel::kScope))
		scopeProvider = ScopeProvider::create (*scopeObject);

	// Load conditions.
	auto* it = a.newQueueIterator ("conditions", ccl_typeid<Attributes> ());
	IterForEach (it, Attributes, attr)
		if(auto* filter = Condition::create (*attr))
			conditions.add (filter);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Matcher::getValue (String& value, const XmlNode& node) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Matcher::matches (const XmlNode& node) const
{
	if(!matchProperties (node))
		return false;

	ArrayForEach (conditions, Condition, c)
		if(!c->matches (node))
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Matcher::getScope (String& scope, XmlNode& node) const
{
	if(scopeProvider == nullptr)
		return false;

	return scopeProvider->get (scope, &node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Matcher::matchProperties (const XmlNode& node) const
{
	return false;
}

//************************************************************************************************
// AttributeMatcher
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AttributeMatcher, Matcher)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (AttributeMatcher, kObjectID, "attribute")

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeMatcher::load (const Attributes& a)
{
	SuperClass::load (a);
	name = a.getString (XmlModel::kAttrName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AttributeMatcher::matchProperties (const XmlNode& node) const
{
	// Node must have matching attribute.
	return !node.getAttribute (name).isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

 bool AttributeMatcher::getValue (String& value, const XmlNode& node) const
{
	value = node.getAttribute (name);
	return !value.isEmpty ();
}

//************************************************************************************************
// ElementMatcher
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ElementMatcher, Matcher)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (ElementMatcher, kObjectID, "element");

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementMatcher::getValue (String& value, const XmlNode& node) const
{
	if(readText)
	{
		value = node.getText ();
		if(!value.isEmpty ())
			return true;
	}

	ModelUtil::setFromPriorityList (value, attribute, node);
	return !value.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementMatcher::load (const Attributes& a)
{
	SuperClass::load (a);

	name = a.getString (XmlModel::kAttrName);

	readText = false;
	if(a.contains ("text"))
		readText = a.getBool ("text");

	ModelUtil::loadStrings (attribute, "attribute", a); // Singular element name, "one of".
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementMatcher::matchProperties (const XmlNode& node) const
{
	return node.getName () == name;
}

//************************************************************************************************
// XmlModelRegistry
//************************************************************************************************

XmlModelRegistry::XmlModelRegistry ()
{
	models.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter& XmlModelRegistry::getFilter ()
{
	static AutoPtr<FileTypeFilter> theFilter;
	if(theFilter == nullptr)
	{
		theFilter = NEW FileTypeFilter;
		theFilter->addFileType (FileTypes::Json ());
	}
	return *theFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlModel* XmlModelRegistry::find (StringRef rootName) const
{
	auto matchesRoot = [&] (const XmlModel& m) { return m.getRootName () == rootName; };
	return models.findIf<XmlModel> (matchesRoot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModelRegistry::load (UrlRef path)
{
	IUrlFilter& filter = XmlModelRegistry::getFilter ();
	if(path.isFolder ())
	{
		ForEachFile (System::GetFileSystem ().newIterator (path, IFileIterator::kFiles), filePath)
			if(filter.matches (*filePath))
				addFromFile (*filePath);
		EndFor
	}
	else if(path.isFile ())
	{
		if(filter.matches (path))
			addFromFile (path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModelRegistry::loadBuiltIns ()
{
	MemoryStream stream (BuiltInModels_Ptr, BuiltInModels_Size);
	loadStream (stream, "models.cpp");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int XmlModelRegistry::countModels () const
{
	return models.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* XmlModelRegistry::newIterator () const
{
	return models.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModelRegistry::add (XmlModel* model)
{
	ASSERT (model != nullptr)
	if(model == nullptr)
		return;

	 // Any model about to be added must respect the current filter
	 // so unwanted support for xml formats other than rootFilter
	 // is not inadvertently added.
	auto matchesFilter = [this] (const XmlModel& model) -> bool
	{
		if(rootFilter.isEmpty ())
			return true;

		if(!model.getRootName ().isEmpty ())
			return model.getRootName () == rootFilter;

		if(!model.getInherit ().isEmpty ())
			return model.getInherit () == rootFilter;

		return false;
	};

	if(!matchesFilter (*model))
	{
		safe_release (model);
		return;
	}

	// Inheritance case: model reuses a built-in model but
	// updates certain properties of it. The model to inherit
	// from may not be loaded yet.
	String root = model->getInherit ();
	if(!root.isEmpty ())
	{
		ASSERT (model->getRootName ().isEmpty ())
		XmlModel* existing = find (root);
		if(existing == nullptr)
		{
			String tempFilter = rootFilter;
			setRootFilter (root);
			loadBuiltIns ();
			setRootFilter (tempFilter);
			existing = find (root);
		}
		if(existing)
		{
			Logging::debug ("Updating model '%(1)'", model->getInherit ());
			existing->merge (*model);
		}

		safe_release (model);
		return;
	}

	// Overwrite case: model replaces entire existing model.
	XmlModel* existing = find (model->getRootName ());
	if(existing != nullptr)
	{
		Logging::debug ("Replacing model '%(1)'", model->getRootName ());
		models.remove (existing);
		safe_release (existing);
	}
	else
		Logging::debug ("Adding model '%(1)'", model->getRootName ());

	models.add (model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModelRegistry::addFromFile (UrlRef path)
{
	String displayPath = UrlDisplayString (path);
	Logging::debug ("Loading user model file '%(1)'", displayPath);

	AutoPtr<IStream> stream = File (path).open (IStream::kOpenMode);
	if(stream != nullptr)
		loadStream (*stream, displayPath);
	else
		Logging::error ("Failed to open model file '%(1)", displayPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlModelRegistry::loadStream (IStream& stream, StringRef fileInfo)
{
	Attributes attributes;
	stream.rewind ();
	if(JsonArchive (stream).loadAttributes (nullptr, attributes))
	{
		// File may contain multiple models or single one.
		if(attributes.contains ("models"))
		{
			auto* it = attributes.newQueueIterator ("models", ccl_typeid<Attributes> ());
			IterForEach (it, Attributes, a)
				if(auto* model = XmlModel::create (*a))
					add (model);
			EndFor
		}
		else
		{
			if(auto* model = XmlModel::create (attributes))
				add (model);
		}
	}
	else
		Logging::error ("Failed to parse JSON model file '%(1)'", fileInfo);
}
