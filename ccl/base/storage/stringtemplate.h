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
// Filename    : stringtemplate.h
// Description : String template class
//
//************************************************************************************************

#ifndef _ccl_stringtemplate_h
#define _ccl_stringtemplate_h

#include "ccl/base/objectnode.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

class Attributes;
class StringTemplate;

//************************************************************************************************
// StringTemplateFilter
/** Abstract value modification functon, for use with StringTemplateEnvironment. */
//************************************************************************************************

class StringTemplateFilter: public Object
{
public:
	virtual StringID getID () const = 0;
	virtual void apply (Variant& value, const Attributes* context) = 0;
};

//************************************************************************************************
// StringTemplateEnvironment
/* String template environment, provides additional filters to StringTemplate class. */
//************************************************************************************************

class StringTemplateEnvironment
{
public:
	StringTemplateEnvironment ();

	const ObjectArray& getFilters () const;
	void registerFilter (StringTemplateFilter* filter);
	void setTemplatesFolder (UrlRef path);
	
	/** Create template with this environment installed. */
	StringTemplate* loadTemplate (UrlRef path);

	/** Create template with this environment installed.
	 @param[in] templateName  name (only) of a template expected in templatesFolder */
	StringTemplate* loadTemplate (StringRef templateName);

	/** Set renderer option, applied to all templates created via loadTemplate (). */
	void setOption (StringID optionId, VariantRef value);
	void getOption (Variant& value, StringID optionId) const;

protected:
	
	/** String filter functions available to any template created
	 through this environment via loadTemplate (), extending any
	 StringTemplate built-in filters. */
	ObjectArray filters;

	Url templatesFolder;
	Attributes options; ///< Renderer options.
};

//************************************************************************************************
// StringTemplate
//************************************************************************************************

class StringTemplate: public Unknown
{
public:
	class StringFilter;

	DECLARE_STRINGID_MEMBER (kOptionTrimBlocks)

	StringTemplate ();
	StringTemplate (StringRef source);

	bool loadFromFile (UrlRef path);
	String render (const Attributes& data) const;

protected:
	friend class StringTemplateEnvironment; // Inject environment
	
	// Parsing
	class VisitableNode;
	class RootNode;
	class TextNode;
	class LoopNode;
	class IfNode;
	class ElseNode;
	class EndIfNode;
	class PlaceholderNode;
	class IncludeNode;
	class Parser;

	// Rendering
	class DataBinder;
	class NodeVisitor;
	class Renderer;

	// Built-in filters
	class StringLower;
	class StringUpper;
	class StringCapitalize;
	class StringDecapitalize;
	class StringEscape;

	StringTemplateEnvironment* env; ///< Optional environment.
	String source; ///< Template string to render.
	ObjectArray filters; ///< Built-in string filters.

	void setEnvironment (StringTemplateEnvironment& env);
	const ObjectArray& getFilters () const;
	const StringTemplateEnvironment* getEnvironment () const;
};

//************************************************************************************************
// StringTemplate::Node
/** Abstract: (AST) visitable node for use with visitor pattern. */
//************************************************************************************************

class StringTemplate::VisitableNode: public ObjectNode
{
public:
	DECLARE_CLASS (VisitableNode, ObjectNode)
	virtual void accept (NodeVisitor& visitor) const;
};

//************************************************************************************************
// StringTemplate::RootNode
//************************************************************************************************

class StringTemplate::RootNode: public VisitableNode
{
public:
	DECLARE_CLASS (RootNode, ObjectNode)

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;
};

//************************************************************************************************
// StringTemplate::TextNode
//************************************************************************************************

class StringTemplate::TextNode: public VisitableNode
{
public:
	DECLARE_CLASS (TextNode, VisitableNode)

	TextNode (StringRef text);
	TextNode ();

	StringRef getText () const;

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;

protected:
	String text;
};

//************************************************************************************************
// StringTemplate::PlaceholderNode
//************************************************************************************************

class StringTemplate::PlaceholderNode: public VisitableNode
{
public:
	DECLARE_CLASS (PlaceholderNode, VisitableNode)

	static PlaceholderNode* createFromString (StringRef nodeText);

	PlaceholderNode (StringRef variableName);
	PlaceholderNode ();

	StringRef getVariableName () const;
	Vector<MutableCString> getFilterIds () const;

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;

protected:
	DECLARE_STRINGID_MEMBER (kPipeSeparator)
	
	String variableName;
	Vector<MutableCString> filterIds;
};

//************************************************************************************************
// StringTemplate::IncludeNode
//************************************************************************************************

class StringTemplate::IncludeNode: public VisitableNode
{
public:
	DECLARE_CLASS (IncludeNode, VisitableNode)

	static IncludeNode* createFromString (StringRef nodeText);

	StringRef getTemplateName () const;

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;

protected:
	String templateName; ///< Name of template file to include without path, e.g. "common.in"
};

//************************************************************************************************
// StringTemplate::LoopNode
//************************************************************************************************

class StringTemplate::LoopNode: public VisitableNode
{
public:
	DECLARE_CLASS (LoopNode, VisitableNode)

	static LoopNode* createFromString (StringRef nodeText);

	StringRef getVariable () const;
	StringRef getListName () const;

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;

protected:
	String variable;
	String listName;
};

//************************************************************************************************
// StringTemplate::IfNode
//************************************************************************************************

class StringTemplate::IfNode: public VisitableNode
{
public:
	DECLARE_CLASS (IfNode, VisitableNode)

	static IfNode* createFromString (StringRef nodeText);

	StringRef getStatement () const;

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;

protected:
	String statement;
};

//************************************************************************************************
// StringTemplate::ElseNode
//************************************************************************************************

class StringTemplate::ElseNode: public VisitableNode
{
public:
	DECLARE_CLASS (ElseNode, VisitableNode)

	static ElseNode* createFromString (StringRef nodeText);

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;
};

//************************************************************************************************
// StringTemplate::EndIfNode
//************************************************************************************************

class StringTemplate::EndIfNode: public VisitableNode
{
public:
	DECLARE_CLASS (EndIfNode, VisitableNode)

	static EndIfNode* createFromString (StringRef str);

	// VisitableNode
	void accept (NodeVisitor& visitor) const override;
};

//************************************************************************************************
// StringTemplate::Parser
/* Convert template file to node tree. */
//************************************************************************************************

class StringTemplate::Parser
{
public:
	/** Convert source string to node model. */
	static void load (RootNode& node, StringRef templateString, bool trimBlocks);

protected:
	// Parser state machine states.
	enum class ParserState
	{
		kText, // Processing plain text
		kPlaceholder, // Processing placeholder statement
		kControlStructure // Processing control structure statement (for loop, ...)
	};

	DECLARE_STRINGID_MEMBER (kStatementForEach)
	DECLARE_STRINGID_MEMBER (kStatementEndFor)
	DECLARE_STRINGID_MEMBER (kStatementIf)
	DECLARE_STRINGID_MEMBER (kStatementElse)
	DECLARE_STRINGID_MEMBER (kStatementEndIf)
	DECLARE_STRINGID_MEMBER (kStatementInclude)

	static void skipNewline (int& i, StringRef templateString);
	static void advanceCursor (int& position);
};

//************************************************************************************************
// StringTemplate::DataBinder
//************************************************************************************************

class StringTemplate::DataBinder
{
public:
	DECLARE_STRINGID_MEMBER (kGlobalScope)
	DECLARE_STRINGID_MEMBER (kLoopVariable)
	DECLARE_STRINGID_MEMBER (kLoopAttributeLast)
	DECLARE_STRINGID_MEMBER (kLoopAttributeIndex)

	bool hasBindings () const;
	void pushBinding (StringID variable, const Attributes& data);
	void popBinding (StringID variable);
	
	void getAttributeValue (Variant& value, StringID scopedVariable) const;
	void setAttributeValue (StringID scopedVariable, StringID attributeId, const Variant& value) const;
	Attributes* getAttributes (StringID scopedVariable) const;
	Iterator* getAttributesIterator (StringID listObjectId) const;

protected:
	struct VariableBinding
	{
		MutableCString name = nullptr; ///< Name used by template
		AutoPtr<Attributes> data; ///< Associated container

		bool operator == (const VariableBinding& other) const { return other.name == name; }
	};

	static String getScope (StringID variable);
	static String getID (StringID variable);
	Attributes* lookupBinding (StringID variable) const;

	DECLARE_STRINGID_MEMBER (kScopeSeparator)

	Vector<VariableBinding> bindings;
};

//************************************************************************************************
// StringTemplate::NodeVisitor
/** Abstract: (AST) node visitor class. */
//************************************************************************************************

class StringTemplate::NodeVisitor
{
public:
	virtual ~NodeVisitor() {}

	virtual void visit (const RootNode& node) = 0;
	virtual void visit (const TextNode& node) = 0;
	virtual void visit (const PlaceholderNode& node) = 0;
	virtual void visit (const LoopNode& node) = 0;
	virtual void visit (const IfNode& node) = 0;
	virtual void visit (const ElseNode& node) = 0;
	virtual void visit (const EndIfNode& node) = 0;
	virtual void visit (const IncludeNode& node) = 0;

	// ...
};

//************************************************************************************************
// StringTemplate::Renderer
//************************************************************************************************

class StringTemplate::Renderer: public NodeVisitor
{
public:
	Renderer (const StringTemplate& stringTemplate, const Attributes& data);
	~Renderer ();

	StringRef getOuputString () const;

	// NodeVisitor
	void visit (const RootNode& node) override;
	void visit (const TextNode& node) override;
	void visit (const PlaceholderNode& node) override;
	void visit (const LoopNode& node) override;
	void visit (const IfNode& node) override;
	void visit (const ElseNode& node) override;
	void visit (const EndIfNode& node) override;
	void visit (const IncludeNode& node) override;

protected:
	// Helper struct to manage if-else branches.
	struct ConditionalStatement
	{
		bool pendingElse = false; ///< Possibly upcoming else branch needs to be evaluated
	};

	const StringTemplate& stringTemplate;
	String output;
	DataBinder binder;
	const Attributes& data;
	Vector<ConditionalStatement> activeConditionals;

	void applyFilters (Variant& value, const Vector<MutableCString>& filterIds, const Attributes* context) const;
	void traverse (const VisitableNode& node);
	bool resolveCondition (StringRef statement);
};

} // namespace CCL

#endif // _ccl_stringtemplate_h
