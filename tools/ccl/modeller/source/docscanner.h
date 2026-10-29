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
// Filename    : docscanner.h
// Description : Documentation Scanner
//
//************************************************************************************************

#ifndef _docscanner_h
#define _docscanner_h

#include "macrocallscanner.h"
#include "sourcefiletraverser.h"
#include "sourcelexer.h"

#include "ccl/base/storage/url.h"
#include "ccl/extras/modeling/classmodel.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

interface IProgressNotify;

namespace Model {
class ClassRepository; }

//************************************************************************************************
// DocumentationScanner
//************************************************************************************************

class DocumentationScanner: public Unknown,
							public SourceFileVisitor
{
public:
	typedef DocumentationScanner* (*CreateScannerFunc) ();

	class SourceFileParser;
	class DoxyCommentParser;

	struct DocuSnippet;
	struct Define;
	struct EnumValueConstant;
	struct EnumInfo;
	struct ClassMethodList;
	struct ClassPropertyList;

	static void registerScannerType (StringRef modelTitlePart, CreateScannerFunc createFunc);
	static void registerDefaultScannerType (CreateScannerFunc createFunc);
	static DocumentationScanner* createScannerForModel (Model::ClassRepository& repository);
	static int scopedCompare (CStringRef scopedName, CStringRef partScope);

	DocumentationScanner ();

	virtual void applyToModel (Model::ClassRepository& repository);
	virtual bool isMatchingFolder (UrlRef folder) override;

	bool scanCode (const Vector<Url>& paths, IProgressNotify* progress = nullptr);

	// SourceFileVisitor
	void visitFile (UrlRef path) override;

protected:
	struct ScannerType
	{
		String modelTitlePart;
		CreateScannerFunc createFunc;
	};

	static LinkedList<ScannerType> scannerTypes;
	static CreateScannerFunc defaultCreateFunc;

	DECLARE_STRINGID_MEMBER (kUndefinedElementType)
	DECLARE_STRINGID_MEMBER (kNamespace)
	DECLARE_STRINGID_MEMBER (kClass)
	DECLARE_STRINGID_MEMBER (kEnum)
	DECLARE_STRINGID_MEMBER (kEnumValue)
	DECLARE_STRINGID_MEMBER (kScope)
	DECLARE_STRINGID_MEMBER (kOpaqueScope)
	DECLARE_STRINGID_MEMBER (kTemplate)
	DECLARE_STRINGID_MEMBER (kTemplateArg)
	DECLARE_STRINGID_MEMBER (kEnumInfo)
	DECLARE_STRINGID_MEMBER (kEnumInfoValue)
	DECLARE_STRINGID_MEMBER (kClassMethodList)
	DECLARE_STRINGID_MEMBER (kClassMethod)
	DECLARE_STRINGID_MEMBER (kClassPropertyList)
	DECLARE_STRINGID_MEMBER (kClassProperty)
	DECLARE_STRINGID_MEMBER (kConstant)

	LinkedList<Define> defines;
	LinkedList<EnumValueConstant> enumConstants;
	LinkedList<AutoPtr<EnumInfo>> enumsInfos;
	LinkedList<AutoPtr<ClassMethodList>> classMethodLists;
	LinkedList<AutoPtr<ClassPropertyList>> classPropertyLists;
	LinkedList<AutoPtr<DocuSnippet>> docuSnippets;
	IProgressNotify* currentScanProgress;

	virtual bool beforeScan (UrlRef folder, IProgressNotify* progress);
	virtual void postScan ();
	virtual bool handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token);
	virtual bool isDocumentableElementType (CStringRef type) const;
	virtual bool isScopingElementType (CStringRef type) const;

	bool applyDocu (Model::Element& modelElement, DocuSnippet* docu) const;
	bool applyMethods (Model::Class& modelClass) const;
	bool applyProperties (Model::Class& modelClass) const;

	DocuSnippet* findDocuByTargetName (CStringRef elementType, CStringRef targetName) const;
	DocuSnippet* findScopedDocu (CStringRef elementType, CStringRef scopedName) const;
};

//************************************************************************************************
// DocumentationScanner::DocuSnippet
//************************************************************************************************

struct DocumentationScanner::DocuSnippet: public Unknown
{
	MutableCString targetName;		//< Class or enum item identifier.
	MutableCString scopedName;		//< Name with scope.
	String brief;					//< Parsed brief description.
	String details;					//< Parsed detailed description.
	String code;					//< Code example, parsed from comments section \code to \endcode.
	String codeLang;				//< Code language, parsed from \code command (example: "\code{.cpp}").
	Model::Documentation::LinkList links;	//< \see cross-reference links.
	MutableCString elementType;		//< One of the kXxx element-type constants this snippet was captured for.
	bool linksOnly;					//< True for a snippet synthesized purely to carry a cross-reference link.

	DocuSnippet (CStringRef _elementType)
	: elementType (_elementType),
	  linksOnly (false)
	{}

	int scopedCompare (CStringRef partScope) const;
	int scopeCount () const;
};

//************************************************************************************************
// DocumentationScanner::Define
//************************************************************************************************

struct DocumentationScanner::Define
{
	MutableCString name;
	MutableCString value;

	bool resolve (MutableCString& str) const;
	bool resolveParts (MutableCString& str) const;
};

//************************************************************************************************
// DocumentationScanner::EnumValueConstant
// enum Example { kEnumItem = kConstant };
//************************************************************************************************

struct DocumentationScanner::EnumValueConstant
{
	MutableCString enumItemName;
	MutableCString scopedName;
	MutableCString constantName;
	SharedPtr<DocuSnippet> docu;
};

//************************************************************************************************
// DocumentationScanner::EnumInfo
//************************************************************************************************

struct DocumentationScanner::EnumInfo: public Unknown
{
	struct ScannedEnumerator
	{
		MutableCString name;
		SharedPtr<DocuSnippet> docu;
	};

	MutableCString name;
	LinkedList<ScannedEnumerator> items;
	SharedPtr<DocuSnippet> docu;
};

//************************************************************************************************
// DocumentationScanner::ClassMethodList - list of scriptable methods of a class (BEGIN_METHOD_NAMES)
//************************************************************************************************

struct DocumentationScanner::ClassMethodList: public Unknown
{
	// Kept ref-counted (unlike ScannedMethod below): this struct owns a LinkedList, and Core::LinkedList
	// has no deep-copy support - only a shallow (pointer) copy - so a struct holding one must live
	// at one stable heap address (via AutoPtr) rather than ever be copied by value into a list.

	struct ScannedMethod
	{
		MutableCString name;
		MutableCString args;
		MutableCString returnValue;
		SharedPtr<DocuSnippet> docu;
	};

	MutableCString className;
	LinkedList<ScannedMethod> methods;
};

//************************************************************************************************
// DocumentationScanner::ClassPropertyList - list of scriptable properties of a class (BEGIN_PROPERTY_NAMES)
//************************************************************************************************

struct DocumentationScanner::ClassPropertyList: public Unknown
{
	struct ScannedProperty
	{
		MutableCString name;
		SharedPtr<DocuSnippet> docu;
	};

	MutableCString className;
	LinkedList<ScannedProperty> properties;
};

//************************************************************************************************
// DocumentationScanner::DoxyCommentParser
//************************************************************************************************

class DocumentationScanner::DoxyCommentParser
{
public:
	/** Parse brief, details, links and code example from a doxygen comment.
		@param[in] comment  doxygen comment to parse
		@param[out] target  target object to set brief, details, links and code example for */
	static bool parse (DocuSnippet& target, StringRef comment);

protected:
	/** Get the number of arguments a doxygen command takes, for commands parseDoxyCommand needs
		to distinguish (0, 1 or 2) - most take exactly one, so that's the fallback.
		@param[in] command  doxygen command name, without the leading '\' or '@' */
	static int getCommandArgCount (StringRef command);

	/** Parse doxygen commands \see or \code{.lang}, skip any other.
		@param[out] i  parsing index in comment, incremented
		@param[in] comment  doxygen comment to parse command from
		@param[out] target  target object to set links or code example for */
	static void parseCommand (int& i, DocuSnippet& target, StringRef comment);

	/** Extract doxygen command arguments, starting at 'i'.
		@param[out] arguments  vector the parsed arguments are pushed to, is not cleared initially
		@param[out] i  parsing index in comment, incremented
		@param[in] numArgs  expected number of arguments
		@param[in] comment  doxygen comment to read command arguments from */
	static void collectArguments (Core::Vector<String>& arguments, int& i, const int numArgs, StringRef comment);

	static String& cleanupParsedString (String& str);
};

//************************************************************************************************
// DocumentationScanner::SourceFileParser
//************************************************************************************************

class DocumentationScanner::SourceFileParser
{
public:
	static IUrlFilter& getFilter ();

	struct SourceElement
	{
		MutableCString type;	//< One of the kXxx element-type constants (kNamespace, kClass, kScope, ...).
		MutableCString data;	//< The element's own name, if any - links it to its documentation.
		int scanningPart;		//< Implementation helper - current argument/part index while scanning.

		SourceElement (CStringRef _type = kUndefinedElementType)
		: type (_type),
		  scanningPart (0)
		{}

		SourceElement (CStringRef _type, CStringRef _data)
		: type (_type),
		  data (_data),
		  scanningPart (0)
		{}
	};

	SourceFileParser (DocumentationScanner& scanner);

	bool parseFile (UrlRef file, IProgressNotify* progress);
	void getLookupScopes (Vector<MutableCString>& scopes) const;
	SourceElement* getLastNonScopeElement () const;
	bool pushElement (const SourceElement& element, bool handleRecentDoxyComment = true);
	void popElement ();
	void flushRecentDoxyComment ();
	bool nextToken (SourceLexer& reader, SourceToken& token);

protected:
	enum DoxyCommentType
	{
		kLeading,
		kTrailing
	};

	struct UsingDirective
	{
		MutableCString scope;	//< Declaring scope.
		MutableCString target;	//< Namespace as written.
	};

	DocumentationScanner& scanner;
	Vector<SourceElement> elementStack;
	Vector<UsingDirective> usingDirectives;
	SourceElement lastPoppedElement;
	String recentLeadingDoxyComment;
	String file; // for debugging

	static bool isFunctionBodyMacro (const SourceToken& token);

	void parse (SourceLexer& reader, SourceToken& token);
	void handleCommentToken (const SourceToken& token);
	void handlePreprocessorToken (const SourceToken& token) const;
	bool handleElement (SourceLexer& reader, SourceToken& token);
	bool handleEnum (SourceLexer& reader, SourceToken& token);
	bool handleEnumValueConstant (SourceElement& element, SourceLexer& reader, SourceToken& token);
	bool parseEnumInfo (SourceLexer& reader, SourceToken& token);
	bool parseClassMethods (SourceLexer& reader, SourceToken& token);
	bool parseClassProperties (SourceLexer& reader, SourceToken& token);
	bool parseNamespaceConstant (SourceLexer& reader, SourceToken& token);
	bool handleElementType (SourceLexer& reader, SourceToken& token, CStringRef type);
	bool parseUsingDirective (SourceLexer& reader, SourceToken& token);
	bool parseFunctionBodyMacro (SourceLexer& reader, SourceToken& token);
	void getCurrentScope (MutableCString& scope) const;
	bool getScopedName (MutableCString& scopedName, const SourceElement& element) const;
	void handleDoxyComment (String& comment, DoxyCommentType type) const;
	bool isDocumentableElement (const SourceElement& e) const;
};

//************************************************************************************************
// TypeLibDocuScanner
//************************************************************************************************

class TypeLibDocuScanner: public DocumentationScanner
{
public:
	using SuperClass = DocumentationScanner;

	TypeLibDocuScanner ();

	// DocumentationScanner
	void applyToModel (Model::ClassRepository& repository) override;

protected:
	struct AliasCollector;
	struct MacroHandler;
	struct MemberMacroAlias;
	struct ScannedMember;
	struct ScannedEntry;
	struct ScannedEnum;

	enum class EntryKind
	{
		kAny,
		kClassBackedOnly,
		kPlainOnly
	};

	DECLARE_STRINGID_MEMBER (kTypeLibClass)
	DECLARE_STRINGID_MEMBER (kTypeLibEntry)
	DECLARE_STRINGID_MEMBER (kTypeLibMemberList)
	DECLARE_STRINGID_MEMBER (kTypeLibMember)

	static bool isQualifiedIdentifier (CStringRef text);

	LinkedList<AutoPtr<ScannedEntry>> entries;
	LinkedList<ScannedEnum> enums;
	LinkedList<MemberMacroAlias> memberMacroAliases;
	ScannedEntry* lastDefinedEntry;

	void registerMemberMacroAlias (const ScannedFunctionDefine& candidate, UrlRef path);
	const MemberMacroAlias* findMemberMacroAlias (CStringRef macroName) const;
	ScannedEntry* findTypeLibEntry (CStringRef uniqueName) const;
	bool isElementSnippet (DocuSnippet* snippet) const;
	DocuSnippet* findClassDocu (CStringRef className, const Vector<MutableCString>& lookupScopes) const;
	bool applyEntryDocs (Model::ClassRepository& repository, EntryKind kind = EntryKind::kAny) const;
	bool applyEnumDocs (Model::ClassRepository& repository) const;

	// DocumentationScanner
	void postScan () override;
	bool beforeScan (UrlRef folder, IProgressNotify* progress) override;
	bool handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token) override;
	bool isDocumentableElementType (CStringRef type) const override;
	bool isScopingElementType (CStringRef type) const override;
};

//************************************************************************************************
// TypeLibDocuScanner::MemberMacroAlias - manage custom ADD_TYPELIB_MEMBER_WITH_TYPE alias macros
//************************************************************************************************

struct TypeLibDocuScanner::MemberMacroAlias
{
	MutableCString name;
	MutableCString typeName;
};

//************************************************************************************************
// TypeLibDocuScanner::ScannedMember
//************************************************************************************************

struct TypeLibDocuScanner::ScannedMember
{
	MutableCString tagName;
	MutableCString typeName;
	SharedPtr<DocuSnippet> docu;

	ScannedMember (CStringPtr _typeName = nullptr)
	: typeName (_typeName)
	{}
};

//************************************************************************************************
// TypeLibDocuScanner::ScannedEntry - simplification: represents both a type library entry or class
//************************************************************************************************

struct TypeLibDocuScanner::ScannedEntry: public Unknown
{
	// Kept ref-counted: owns a LinkedList (see ClassMethodList's comment), and is persistently
	// aliased by raw pointer (lastDefinedEntry) across separate top-level macro invocations.

	MutableCString uniqueName;
	MutableCString tagName;
	MutableCString elementClass; // TypeLibClass only
	MutableCString relatedClass;
	Vector<MutableCString> elementClassScopes; //< Lookup scopes for elementClass.
	Vector<MutableCString> relatedClassScopes; //< Lookup scopes for relatedClass.
	SharedPtr<DocuSnippet> docu;
	LinkedList<ScannedMember> members;

	/** True if 'name' is this entry's own class or its related class - the two names a \see link
		or a doc lookup may legitimately refer to it by. */
	bool matchesClassName (StringRef name) const
	{
		return (elementClass.isEmpty () == false && name == String (elementClass))
			|| (relatedClass.isEmpty () == false && name == String (relatedClass));
	}
};

//************************************************************************************************
// TypeLibDocuScanner::ScannedEnum
//************************************************************************************************

struct TypeLibDocuScanner::ScannedEnum
{
	MutableCString ownerTagName;
	MutableCString memberName;
	MutableCString enumDefName;
	MutableCString parentOwnerTagName;
	MutableCString parentMemberName;
	SharedPtr<EnumInfo> info; // resolved
};

//************************************************************************************************
// GuiDocuScanner
//************************************************************************************************

class GuiDocuScanner: public TypeLibDocuScanner
{
public:
	using SuperClass = TypeLibDocuScanner;

	// TypeLibDocuScanner
	void applyToModel (Model::ClassRepository& repository) override;
	bool isMatchingFolder (UrlRef folder) override;

protected:
	// Not moved out like the other nested types above: OptionList::ScannedOption is used right below,
	// within this same class body, so OptionList needs to still be complete at this point.
	struct OptionList: public Unknown
	{
		struct ScannedOption
		{
			MutableCString skinName;
			MutableCString cppName;
			SharedPtr<DocuSnippet> docu;
		};
		MutableCString name;
		LinkedList<ScannedOption> items;
	};

	LinkedList<AutoPtr<OptionList>> optionLists;

	bool parseStyleDef (SourceLexer& reader, SourceFileParser& parser, SourceToken& token);
	void applyToSkinModel (Model::ClassRepository& repository) const;
	void applyToVisualStyleModel (Model::ClassRepository& repository) const;

	// TypeLibDocuScanner
	void postScan () override;
	bool handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token) override;
};

} // namespace CCL

#endif // _docscanner_h
