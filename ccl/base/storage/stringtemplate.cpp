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
// Filename    : stringtemplate.cpp
// Description : String template class
//
//************************************************************************************************

#include "stringtemplate.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/textfile.h"

using namespace CCL;

//************************************************************************************************
// StringTemplate::StringLower
/** Transform string to lower chars string, has no effect on non-string values .*/
//************************************************************************************************

class StringTemplate::StringLower: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override { return CSTR ("lower"); }
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::StringLower::apply (Variant& value, const Attributes* context)
{
	if(value.isString ())
		value.fromString (value.asString ().toLowercase ());
}

//************************************************************************************************
// StringTemplate::StringUpper
/** Transform string to upper chars string, has no effect on non-string values .*/
//************************************************************************************************

class StringTemplate::StringUpper: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override { return CSTR ("upper"); }
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::StringUpper::apply (Variant& value, const Attributes* context)
{
	if(value.isString ())
		value.fromString (value.asString ().toUppercase ());
}

//************************************************************************************************
// StringTemplate::StringCapitalize
/** Transform first string character to uppercase, has no effect on non-string values .*/
//************************************************************************************************

class StringTemplate::StringCapitalize: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override { return CSTR ("capitalize"); };
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::StringCapitalize::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
		return;

	String valueString = value.asString ();
	if(valueString.isEmpty ())
		return;

	String modified;
	modified << valueString.subString (0, 1).toUppercase ();
	if(valueString.length () > 1)
		modified << valueString.subString (1);

	value.fromString (modified);
}

//************************************************************************************************
// StringTemplate::StringDecapitalize
/** Transform first string character to lowercase, has no effect on non-string values .*/
//************************************************************************************************

class StringTemplate::StringDecapitalize: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override { return CSTR ("decapitalize"); };
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::StringDecapitalize::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
		return;

	String valueString = value.asString ();
	if(valueString.isEmpty ())
		return;

	String modified;
	modified << valueString.subString (0, 1).toLowercase ();
	if(valueString.length () > 1)
		modified << valueString.subString (1);

	value.fromString (modified);
}

//************************************************************************************************
// StringTemplate::StringEscape
/** Escape string values in quotes, has no effect on non-string values .*/
//************************************************************************************************

class StringTemplate::StringEscape: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override { return CSTR ("escapestring"); };
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::StringEscape::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
		return;

	String valueString = value.asString ();
	if(valueString.isEmpty ())
		return;

	String quotedString = String () << "\"" << valueString << "\"";
	value.fromString (quotedString);
}

//************************************************************************************************
// StringTemplateEnvironment
//************************************************************************************************

StringTemplateEnvironment::StringTemplateEnvironment ()
{
	filters.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& StringTemplateEnvironment::getFilters () const
{
	return filters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplateEnvironment::registerFilter (StringTemplateFilter* filter)
{
	// Register a new string filter, do not register if
	// filter with ID about to add already exists.

	StringID incomingId = filter->getID ();
	ForEach (filters, StringTemplateFilter, existingFilter)
		if(existingFilter && existingFilter->getID ().compare (incomingId) == 0)
		{
			ASSERT (false)
			return;
		}
	EndFor

	filters.add (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplateEnvironment::setTemplatesFolder (UrlRef path)
{
	templatesFolder = path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate* StringTemplateEnvironment::loadTemplate (UrlRef path)
{
	// Load template and inject this environment.

	StringTemplate* stringTemplate = NEW StringTemplate;
	if(stringTemplate->loadFromFile (path) == false)
		return nullptr;

	stringTemplate->setEnvironment (*this);
	return stringTemplate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate* StringTemplateEnvironment::loadTemplate (StringRef templateName)
{
	// This method requires a templates "working" folder.

	SOFT_ASSERT (!templatesFolder.isEmpty (), "string template: use of {% include %} requires environment templates folder")
	ASSERT (!templatesFolder.isEmpty ())
	if(templatesFolder.isEmpty ())
		return nullptr;

	Url templatePath = templatesFolder;
	templatePath.descend (templateName);

	return loadTemplate (templatePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplateEnvironment::setOption (StringID optionId, VariantRef value)
{
	options.setAttribute (optionId, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplateEnvironment::getOption (Variant& value, StringID optionId) const
{
	options.getAttribute (value, optionId);
}

//************************************************************************************************
// StringTemplate
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (StringTemplate, kOptionTrimBlocks, "trimBlocks");

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::StringTemplate (StringRef _source)
: source (_source),
  env (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::StringTemplate ()
: StringTemplate ("")
{
	filters.objectCleanup (true);

	// Install default filters.
	filters.add (NEW StringLower);
	filters.add (NEW StringUpper);
	filters.add (NEW StringCapitalize);
	filters.add (NEW StringDecapitalize);
	filters.add (NEW StringEscape);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringTemplate::render (const Attributes& data) const
{
	bool trimBlocks = false;
	if(env != nullptr)
	{
		Variant attrValue;
		env->getOption (attrValue, kOptionTrimBlocks);
		trimBlocks = attrValue;
	}

	RootNode node;
	Parser::load (node, source, trimBlocks);

	Renderer renderer (*this, data);
	renderer.visit (node);

	return renderer.getOuputString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTemplate::loadFromFile (UrlRef path)
{
	// Load as raw string to preserve line endings.
	source = TextUtils::loadRawString (path);
	return source.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::setEnvironment (StringTemplateEnvironment& _env)
{
	// Should be set only once when the template
	// is created by the environment.

	ASSERT (env == nullptr)
	env = &_env;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& StringTemplate::getFilters () const
{
	return filters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StringTemplateEnvironment* StringTemplate::getEnvironment () const
{
	return env;
}

//************************************************************************************************
// StringTemplate::VisitableNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::VisitableNode, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::VisitableNode::accept (NodeVisitor& visitor) const
{
	SOFT_ASSERT (false, "string template: unsupported node")
}

//************************************************************************************************
// StringTemplate::RootNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::RootNode, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::RootNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::TextNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::TextNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::TextNode::TextNode (StringRef _text)
: text (_text)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::TextNode::TextNode ()
: TextNode ("")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::TextNode::getText () const
{
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::TextNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::PlaceholderNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::PlaceholderNode, VisitableNode)
DEFINE_STRINGID_MEMBER_ (StringTemplate::PlaceholderNode, kPipeSeparator, "|")

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::PlaceholderNode* StringTemplate::PlaceholderNode::createFromString (StringRef nodeText)
{
	PlaceholderNode* node = NEW PlaceholderNode;

	// No filters set.
	if(nodeText.contains (String (kPipeSeparator)) == false)
	{
		node->variableName = nodeText;
		return node;
	}

	// First token is variable name, rest is one or multiple filters.
	// TODO, future: assumes that filters have no arguments

	String cmd = nodeText;
	AutoPtr<IStringTokenizer> tokenizer (cmd.tokenize (String (kPipeSeparator)));
	if(!tokenizer)
		return nullptr;

	// Support unlimited number of filters.

	bool firstToken = true;
	uchar delimiter = 0;
	while(!tokenizer->done ())
	{
		String token = tokenizer->nextToken (delimiter);
		token = token.trimWhitespace ();

		if(firstToken)
		{
			ASSERT (token.isEmpty () == false)
			node->variableName = token;
			firstToken = false;
			continue;
		}

		MutableCString filterId = token;
		if(!filterId.isEmpty ())
			node->filterIds.add (filterId);
	}

	return node;
}

//************************************************************************************************
// StringTemplate::IncludeNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::IncludeNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::IncludeNode* StringTemplate::IncludeNode::createFromString (StringRef nodeText)
{
	IncludeNode* node = NEW IncludeNode;

	// Example nodeText: "include sometemplate.in"

	String cmd = nodeText;
	cmd.replace ("include", "");
	node->templateName = cmd.trimWhitespace ();

	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::IncludeNode::getTemplateName () const
{
	return templateName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::IncludeNode::accept (NodeVisitor & visitor) const
{
	visitor.visit (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::PlaceholderNode::PlaceholderNode (StringRef variableName)
: variableName (variableName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::PlaceholderNode::PlaceholderNode ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::PlaceholderNode::getVariableName () const
{
	return variableName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vector<MutableCString> StringTemplate::PlaceholderNode::getFilterIds () const
{
	return filterIds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::PlaceholderNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::LoopNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::LoopNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::LoopNode* StringTemplate::LoopNode::createFromString (StringRef nodeText)
{
	// Disassemble command 'for var in variables' into
	// individual attributes, 'var' and 'variables'.

	LoopNode* node = NEW LoopNode;

	String cmd = nodeText;
	AutoPtr<IStringTokenizer> tokenizer (cmd.tokenize (CCLSTR (" ")));
	if(!tokenizer)
		return nullptr;

	uchar delimiter = 0;
	for(int tokenIndex = 0; tokenIndex < 4; tokenIndex++)
	{
		StringRef token (tokenizer->nextToken (delimiter));
		if(tokenIndex == 1)
			node->variable = token;
		else if(tokenIndex == 3)
			node->listName = token;
	}

	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::LoopNode::getVariable () const
{
	return variable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::LoopNode::getListName () const
{
	return listName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::LoopNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::IfNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::IfNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::IfNode* StringTemplate::IfNode::createFromString (StringRef nodeText)
{
	IfNode* node = NEW IfNode;

	// Get condition statement without trailing "if ".
	String conditionString = nodeText.subString (nodeText.index (" ") + 1);
	conditionString.trimWhitespace ();
	node->statement = conditionString;
	ASSERT (node->statement.isEmpty () == false)

	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::IfNode::getStatement () const
{
	return statement;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::IfNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::ElseNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::ElseNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::ElseNode* StringTemplate::ElseNode::createFromString (StringRef nodeText)
{
	// This node has no extra statements.
	return NEW ElseNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::ElseNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::EndIfNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StringTemplate::EndIfNode, VisitableNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::EndIfNode* StringTemplate::EndIfNode::createFromString (StringRef nodeText)
{
	// This node has no extra statements.
	return NEW EndIfNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::EndIfNode::accept (NodeVisitor& visitor) const
{
	visitor.visit (*this);
}

//************************************************************************************************
// StringTemplate::Parser
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementForEach, "for")
DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementEndFor, "endfor")
DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementIf, "if")
DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementElse, "else")
DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementEndIf, "endif")
DEFINE_STRINGID_MEMBER_ (StringTemplate::Parser, kStatementInclude, "include")

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Parser::load (RootNode& rootNode, StringRef templateString, bool trimBlocks)
{
	String buffer;
	StringWriter<256> writer (buffer);

	auto readBuffer = [&writer, &buffer] () -> String
	{
		writer.flush ();
		String result = buffer;
		buffer.empty ();
		
		return result;
	};

	Stack<VisitableNode*> nodeStack;
	nodeStack.push (&rootNode);
	bool skipNewlinePending = false;

	ParserState state = ParserState::kText;
	const int templateLength = templateString.length ();
	for (int i = 0; i < templateLength; i++)
	{
		// Reminder: has no effect if there is no immediate
		// newline after the end of a control statement.
		if(skipNewlinePending)
		{
			skipNewline (i, templateString);
			skipNewlinePending = false;
		}

		char ch = templateString[i];

		// Detect special statements.
		bool placeholderStart = ch == '{' && i + 1 < templateLength && templateString[i + 1] == '{';
		bool placeholderEnd = ch == '}' && i + 1 <  templateLength && templateString[i + 1] == '}';
		bool controlStructureStart = ch == '{' && i + 1 < templateLength && templateString[i + 1] == '%';
		bool controlStructureEnd = ch == '%' && i + 1 <  templateLength && templateString[i + 1] == '}';

		VisitableNode* activeNode = nodeStack.peek ();
		switch(state)
		{
		case ParserState::kText :
		{
			// State machine parses text, i.e. not a special statement.
			// Check if a special statement is about to start, otherwise
			// continue to contribute to text token.

			if(placeholderStart)
			{
				String text = readBuffer ();
				activeNode->addChild (NEW TextNode (text));

				// Update state, skip next '{'.
				state = ParserState::kPlaceholder;
				advanceCursor (i);
			}
			else if(controlStructureStart)
			{
				String text = readBuffer ();
				activeNode->addChild (NEW TextNode (text));

				// Update state, skip '%'.
				state = ParserState::kControlStructure;
				advanceCursor (i);
			}
			else if(placeholderEnd || controlStructureEnd)
			{
				// Sanity check invalid template syntax.
				ASSERT (false)
			}
			else
			{
				writer.append (ch);
			}
			break;
		}

		case ParserState::kPlaceholder :
		{
			// State machine parses a placeholder statement '{{ value }}'.
			// Check for statement end, otherwise contribute to 'value'.

			if(placeholderEnd)
			{
				String text = readBuffer ().trimWhitespace ();
				activeNode->addChild (PlaceholderNode::createFromString (text));

				// Update state, skip next '}'.
				state = ParserState::kText;
				advanceCursor (i);
			}
			else if(placeholderStart || controlStructureStart || controlStructureEnd)
			{
				// Sanity check invalid template syntax.
				ASSERT (false)
			}
			else
			{
				// Assemble string contained in "{{ ... }}"
				writer.append (ch);
			}
			break;
		}

		case ParserState::kControlStructure :
		{
			/* State machine parses a control structure '{% statement %}'.
			 Note that controlStructureEnd refers to the end of the single
			 statement, not necessarily the end of the scope spanned by
			 the statement. */
			
			if(controlStructureEnd)
			{
				MutableCString statement = readBuffer ().trimWhitespace ();

				// Block: {% for ... %} ... {% endfor %}
				if(statement.startsWith (kStatementForEach))
				{
					LoopNode* loopNode = LoopNode::createFromString (String (statement));
					activeNode->addChild (loopNode);

					// This is a node with begin/end semantics.
					// Contribute all future nodes to this one until its scope ends.
					nodeStack.push (loopNode);
				}
				else if(statement.startsWith (kStatementEndFor))
				{
					// Don't delete the LoopNode from ObjectNode tree,
					// just delete the entry from the stack.
					nodeStack.pop ();
				}
				else if(statement.startsWith (kStatementInclude))
				{
					IncludeNode* includeNode = IncludeNode::createFromString (String (statement));
					activeNode->addChild (includeNode);
				}

				// Block: {% if ... %} ... {% endif %}
				if(statement.startsWith (kStatementIf))
				{
					IfNode* ifNode = IfNode::createFromString (String (statement));
					activeNode->addChild (ifNode);

					// This is a node with begin/end semantics.
					// Contribute all future nodes to this one until its scope ends.
					nodeStack.push (ifNode);
				}
				else if(statement.startsWith (kStatementElse))
				{
					// Cancel active IfNode.
					nodeStack.pop ();
					activeNode = nodeStack.peek ();

					ElseNode* elseNode = ElseNode::createFromString (String (statement));
					activeNode->addChild (elseNode);

					// This is a node with begin/end semantics.
					// Contribute all future nodes to this one until its scope ends.
					nodeStack.push (elseNode);
				}
				else if(statement.startsWith (kStatementEndIf))
				{
					// Don't delete the IfNode or ElseNode from ObjectNode tree,
					// just delete the entry from the stack.
					nodeStack.pop ();
					activeNode = nodeStack.peek ();

					EndIfNode* endIfNode = EndIfNode::createFromString (String (statement));
					activeNode->addChild (endIfNode);

					// Don't push to stack, this node has no elements.
				}

				// Update state, skip next '}'.
				state = ParserState::kText;
				advanceCursor (i);

				// Schedule newline skip starting at next
				// character in outer loop.
				if(trimBlocks)
					skipNewlinePending = true;
			}
			else if(controlStructureStart || placeholderStart || placeholderEnd)
			{
				// Sanity check invalid template syntax.
				ASSERT (false)
			}
			else
			{
				// Assemble string contained in "{% ... %}"
				writer.append (ch);
			}
			break;
		}
		}
	}

	// Append any text remaining after the last command statement.
	String text = readBuffer ();
	if(!text.isEmpty ())
		nodeStack.peek ()->addChild (NEW TextNode (text));

	nodeStack.pop ();
	ASSERT (nodeStack.isEmpty ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Parser::skipNewline (int& i, StringRef templateString)
{
	// Make cursor 'i' skip newline immediately followed after position 'i'.
	// Cover CR, LF and CRLF. The line ending format is typically not known
	// any may also not be consistent.

	int lastIndex = templateString.length () - 1;
	if(i > lastIndex)
		return;

	char c = templateString[i];

	// LF
	if(c == '\n')
	{	
		advanceCursor (i);
		return;
	}

	// CR or CRLF.
	if(c == '\r')
	{
		advanceCursor (i);

		// Peek next
		if(i <= lastIndex && templateString[i] == '\n')
			advanceCursor (i);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Parser::advanceCursor (int& position)
{
	position++;
}

//************************************************************************************************
// StringTemplate::Renderer
//************************************************************************************************

StringTemplate::Renderer::Renderer (const StringTemplate& _template, const Attributes& _data)
: stringTemplate (_template),
  data (_data)
{
	binder.pushBinding (DataBinder::kGlobalScope, _data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringTemplate::Renderer::~Renderer ()
{
	binder.popBinding (DataBinder::kGlobalScope);
	ASSERT (binder.hasBindings () == false)
	ASSERT (activeConditionals.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTemplate::Renderer::getOuputString () const
{
	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const RootNode& node)
{
	traverse (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const TextNode& node)
{
	output << node.getText ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const PlaceholderNode& node)
{
	MutableCString variableName = node.getVariableName ();

	Variant value;
	binder.getAttributeValue (value, variableName);
	const Attributes* context = binder.getAttributes (variableName);
	applyFilters (value, node.getFilterIds (), context);

	String valueString = value.toString ();
	output << valueString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const LoopNode& node)
{
	MutableCString listObjectId = node.getListName ();
	MutableCString iteratedVariable = node.getVariable ();

	// Specified list may very well not exist. User may have a typo
	// in the template or the template data does not contain it.

	auto it = binder.getAttributesIterator (listObjectId);
	SOFT_ASSERT (it != nullptr, "string template: list object not found")
	if(it)
	{
		Attributes loopAttributes;
		binder.pushBinding (DataBinder::kLoopVariable, loopAttributes);

		int loopIndex = 0;
		IterForEach (it, Attributes, attrs)

			// Maintain "loop" helper variable.
			bool lastIteration = it->done ();
			binder.setAttributeValue (DataBinder::kLoopVariable, DataBinder::kLoopAttributeLast, lastIteration);
			binder.setAttributeValue (DataBinder::kLoopVariable, DataBinder::kLoopAttributeIndex, loopIndex);

			// Register a binding variable for this loop variable. When processing
			// inner nodes, the attributes associated with this variable can be
			// recalled via the binding.

			binder.pushBinding (iteratedVariable, *attrs);

			traverse (node);

			// Remove, next iteration registers new variable.
			binder.popBinding (iteratedVariable);
			loopIndex++;
		EndFor

		binder.popBinding (DataBinder::kLoopVariable);
	}

	// Reminder: unlike If ... Endif, there is no terminating node for "endfor"
	// since it is typically not needed and would also not contain and children.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const IfNode& node)
{
	ConditionalStatement statement;
	activeConditionals.add (statement);

	if(resolveCondition (node.getStatement ()))
		traverse (node);
	else
		activeConditionals.last ().pendingElse = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const ElseNode& node)
{
	// Check if there were any unsatisfied if-statements.
	ASSERT (activeConditionals.isEmpty () == false)
	if(activeConditionals.isEmpty ())
		return;

	ConditionalStatement& statement = activeConditionals.last ();
	if(statement.pendingElse == false)
		return;
	
	traverse (node);
	statement.pendingElse = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const EndIfNode& node)
{
	// Check if there were any unresolved if-else-statements.
	ASSERT (activeConditionals.isEmpty () == false)
	if(activeConditionals.isEmpty ())
		return;

	activeConditionals.removeLast ();

	// This node is only used for terminating an if-else statement. Expect
	// it to not have any children nodes, thus no further traversal needed.

	ASSERT (node.countChildren () == 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::visit (const IncludeNode& node)
{
	StringTemplateEnvironment* environment = stringTemplate.env;
	if(environment == nullptr)
		return;

	AutoPtr<StringTemplate> subTemplate = environment->loadTemplate (node.getTemplateName ());
	if(subTemplate == nullptr)
		return;

	String subTemplateString = subTemplate->render (data);
	output << subTemplateString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::applyFilters (Variant& value, const Vector<MutableCString>& filterIds, const Attributes* context) const
{
	auto apply = [&] (StringID filterId, const ObjectArray& filters) -> bool
	{
		ForEach (filters, StringTemplateFilter, filter)
			if(filter != nullptr && (filter->getID () == filterId))
			{
				filter->apply (value, context);
				return true;
			}
		EndFor

		return false;
	};

	// Rule: a filter for given id is processed only once. Filters
	// provided by the environment have higher priority (so they can
	// override built-in filters).

	for(StringID id : filterIds)
	{
		// Environment is optional.
		const StringTemplateEnvironment* environment = stringTemplate.getEnvironment ();
		if(environment != nullptr)
			apply (id, environment->getFilters ());

		// Built-in filters.
		if(apply (id, stringTemplate.getFilters ()))
			continue;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::Renderer::traverse (const VisitableNode& node)
{
	int childCount = node.countChildren ();
	for(int childIndex = 0; childIndex < childCount; childIndex++)
	{
		VisitableNode* childNode = ccl_cast<VisitableNode> (node.getChildNode (childIndex));
		ASSERT (childNode != nullptr)
		if(childNode == nullptr)
			continue;

		childNode->accept (*this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTemplate::Renderer::resolveCondition (StringRef statement)
{
	// Reminder: starting point only, incomplete. TODO, future: resolve
	// complex statements.

	Vector<String> tokens;
	AutoPtr<IStringTokenizer> tokenizer (statement.tokenize (String (" ")));
	if(!tokenizer)
		return false;

	uchar delimiter = 0;
	while(!tokenizer->done ())
		tokens.add (tokenizer->nextToken (delimiter));

	// Assume that if only one token is available that it refers
	// to a variable which should be checked for truthness.

	if(tokens.count () == 1)
	{
		MutableCString statement = tokens[0];

		if(statement.startsWith (DataBinder::kLoopVariable))
		{
			// "Is last loop element".
			Variant value;
			binder.getAttributeValue (value, statement);
			return value.asBool ();
		}
		else
		{
			// "String is empty".
			Variant value;
			binder.getAttributeValue (value, statement);
			if(value.isString ())
				return value.toString ().isEmpty () == false;
		}

		// ...
	}

	// ...

	return false;
}

//************************************************************************************************
// StringTemplate::DataBinder
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (StringTemplate::DataBinder, kGlobalScope, "global") // global binding scope
DEFINE_STRINGID_MEMBER_ (StringTemplate::DataBinder, kScopeSeparator, ".")
DEFINE_STRINGID_MEMBER_ (StringTemplate::DataBinder, kLoopVariable, "loop")
DEFINE_STRINGID_MEMBER_ (StringTemplate::DataBinder, kLoopAttributeLast, "last") // loop.last
DEFINE_STRINGID_MEMBER_ (StringTemplate::DataBinder, kLoopAttributeIndex, "index") // loop.index

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringTemplate::DataBinder::hasBindings () const
{
	return bindings.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::DataBinder::pushBinding (StringID variable, const Attributes& data)
{
	AutoPtr<Attributes> attrs = NEW Attributes;
	attrs->copyFrom (data);

	bindings.add ({variable, attrs});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::DataBinder::popBinding (StringID variable)
{
	// Bindings works as a stack, remove from the end.
	// Example: there may be multiple active "loop".

	for(int index = bindings.count () - 1; index >= 0; index--)
	{
		const VariableBinding& binding = bindings.at (index);
		if(binding.name == variable)
		{
			bindings.removeAt (index);
			return;
		}
	}

	ASSERT (false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* StringTemplate::DataBinder::lookupBinding (StringID variable) const
{
	// Can be "global" or a variable name but never a composed path like "parent.somevalue".
	// Bindings works as a stack, check most recent added items first.
	// Example: there may be multiple active "loop".

	ASSERT (variable.contains (kScopeSeparator) == false)

	for(int index = bindings.count () - 1; index >= 0; index--)
	{
		const VariableBinding& binding = bindings.at (index);
		if(binding.name == variable)
			return binding.data;
	}

	// Variable does not exist.
	ASSERT (false)
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::DataBinder::getAttributeValue (Variant& value, StringID variable) const
{
	// Support both global scope as well as variable scope.

	Attributes* data = getAttributes (variable);
	ASSERT (data != nullptr)
	if(data == nullptr)
		return;

	MutableCString attributeId = getID (variable);

	MutableCString assertMessage;
	assertMessage.appendFormat ("string template: variable {{ %s }} not found", variable.str ());
	SOFT_ASSERT (data->contains (attributeId), assertMessage.str ())
	if(!data->contains (attributeId))
		return;

	Variant attributeValue;
	data->getAttribute (attributeValue, attributeId);
	value = attributeValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTemplate::DataBinder::setAttributeValue (StringID variable, StringID attributeId, const Variant& value) const
{
	MutableCString variablePath = variable;
	variablePath.append (".");
	variablePath.append (attributeId);

	Attributes* data = getAttributes (variablePath);
	ASSERT (data != nullptr)
	if(data == nullptr)
		return;

	data->setAttribute (attributeId, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* StringTemplate::DataBinder::getAttributes (StringID variable) const
{
	MutableCString scope = getScope (variable);
	return lookupBinding (scope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* StringTemplate::DataBinder::getAttributesIterator (StringID listObjectId) const
{
	// Support both global scope as well as variable scope.

	const Attributes* data = getAttributes (listObjectId);
	if(data == nullptr)
		return nullptr;

	MutableCString objectId = getID (listObjectId);
	return data->newQueueIterator (objectId, ccl_typeid<Attributes> ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringTemplate::DataBinder::getScope (StringID variable)
{
	// Request for "somevalue" -> scope is global (no parent)
	if(variable.contains (kScopeSeparator) == false)
		return String (kGlobalScope);

	// Request for "parent.somevalue" -> scope is "parent".
	return String (variable.subString (0, variable.index (kScopeSeparator)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StringTemplate::DataBinder::getID (StringID variable)
{
	// Request for "somevalue" -> id is "somevalue"
	if(variable.contains (kScopeSeparator) == false)
		return String (variable);

	// Request for "parent.somevalue" -> scope is "parent", id is "somevalue".
	return String (variable.subString (variable.index (kScopeSeparator) + 1, -1));
}
