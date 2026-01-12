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
// Filename    : ccl/extras/modeling/docscanner.h
// Description : Documentation Scanner
//
//************************************************************************************************

#ifndef _docscanner_h
#define _docscanner_h

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
// SourceCodeElemet
//************************************************************************************************

namespace SourceCodeElement
{
	extern StringID kUndefined;
	extern StringID kNamespace;
	extern StringID kClass;
	extern StringID kEnum;
	extern StringID kEnumValue;
	extern StringID kScope;
	extern StringID kTemplate;
	extern StringID kTemplateArg;
	extern StringID kEnumInfo;
	extern StringID kEnumInfoValue;
	extern StringID kClassMethodList;
	extern StringID kClassMethod;
	extern StringID kClassPropertyList;
	extern StringID kClassProperty;
	extern StringID kConstant; 

	typedef MutableCString Type;
	typedef CStringRef TypeRef;

	struct Define; 
	struct EnumValueConstant;
	struct EnumInfo;
	struct DocuSnippet;
	struct ClassMethodList;
	struct ClassPropertyList;
}

//************************************************************************************************
// SourceCodeElement::Define
//************************************************************************************************

struct SourceCodeElement::Define: public Unknown
{
	MutableCString name;
	MutableCString value;

	bool resolve (MutableCString& str);
	bool resolveParts (MutableCString& str);
};

//************************************************************************************************
// SourceCodeElement::EnumValueConstant
// enum Example { kEnumItem = kConstant };
//************************************************************************************************

struct SourceCodeElement::EnumValueConstant: public Unknown
{
	MutableCString enumItemName;
	MutableCString scopedName; // item name with scope
	MutableCString constantName;		
	SharedPtr<DocuSnippet> docu;
};

//************************************************************************************************
// SourceCodeElement::EnumInfo
//************************************************************************************************

struct SourceCodeElement::EnumInfo: public Unknown
{
	struct Item: public Unknown
	{
		MutableCString name;
		SharedPtr<DocuSnippet> docu;
	};
	MutableCString name;
	LinkedList <AutoPtr <Item> > items;			
	SharedPtr<DocuSnippet> docu;
};

//************************************************************************************************
// SourceCodeElement::ClassMethodList - list of scriptable methods of a class (BEGIN_METHOD_NAMES)
//************************************************************************************************

struct SourceCodeElement::ClassMethodList: public Unknown
{
	struct Method: public Unknown
	{
		MutableCString name;			
		MutableCString args;			
		MutableCString returnValue;			
		SharedPtr<DocuSnippet> docu;
	};

	MutableCString className; // name of class 
	LinkedList <AutoPtr <Method> > methods;
};
	
//************************************************************************************************
// SourceCodeElement::ClassPropertyList - list of scriptable properties of a class (BEGIN_PROPERTY_NAMES)
//************************************************************************************************

struct SourceCodeElement::ClassPropertyList: public Unknown
{
	struct Property: public Unknown
	{
		MutableCString name;			
		SharedPtr<DocuSnippet> docu;
	};

	MutableCString className; // name of class 
	LinkedList <AutoPtr <Property> > properties;
};
	
//************************************************************************************************
// SourceCodeElement::DocuSnippet
//************************************************************************************************
	
struct SourceCodeElement::DocuSnippet: public Unknown
{
	MutableCString targetName; // class or enum item identifier
	MutableCString scopedName; // name with scope
	String brief;
	String details;
	String code; // code example, parsed from comments section \code to \endcode
	String codeLang; // code language, parsed from \code command (example: "\code{.cpp}"}
	Model::Documentation::LinkList links;
	SourceCodeElement::Type elementType;

	DocuSnippet (SourceCodeElement::TypeRef _elementType): elementType (_elementType) {}
	int scopedCompare (CStringRef partScope) const;
	int scopeCount () const;
};

//************************************************************************************************
// DocumentationScanner - 
//************************************************************************************************

class DocumentationScanner: public Unknown
{
public:
	typedef DocumentationScanner* (*CreateScannerFunc) ();
	static bool registerScannerType (StringRef modelTitlePart, CreateScannerFunc createFunc);
	static DocumentationScanner* createScannerForModel (Model::ClassRepository& repository);
	static int scopedCompare (CStringRef scopedName, CStringRef partScope);

	class SourceFileParser;
	struct Token; // used with SourceFileParser

	DocumentationScanner ();

	bool scanCode (UrlRef folder, IProgressNotify* progress = nullptr);

	virtual bool applyToModel (Model::ClassRepository& repository);

	virtual bool isMatchingFolder (UrlRef folder);
	virtual void postScan ();	
	virtual bool handleMacros (SourceFileParser& parser, Token& token);
	virtual bool isDocumentableElementType (SourceCodeElement::TypeRef type) const;
	virtual bool isScopingElementType (SourceCodeElement::TypeRef type) const; // the element type is a parent of a documentable element and should be used as scope 

protected:
	class LineReader;
	struct ScannerType
	{
		String modelTitlePart;
		CreateScannerFunc createFunc;
	};
	static LinkedList <ScannerType> scannerTypes;

	using DocuSnippet = SourceCodeElement::DocuSnippet;

	LinkedList <AutoPtr <SourceCodeElement::Define> >  defines;
	LinkedList <AutoPtr <SourceCodeElement::EnumValueConstant> > enumConstants;
	LinkedList <AutoPtr <SourceCodeElement::EnumInfo> > enumsInfos;
	LinkedList <AutoPtr <SourceCodeElement::ClassMethodList> > classMethodLists;
	LinkedList <AutoPtr <SourceCodeElement::ClassPropertyList> > classPropertyLists;

	LinkedList <AutoPtr <DocuSnippet> > docuSnippets;

	bool scanFolder (UrlRef folder, IProgressNotify* progress, bool inMatchingFolder);
	bool applyDocu (Model::Element& modelElement, DocuSnippet* docu) const;
	bool applyMethods (Model::Class& modelClass);
	bool applyProperties (Model::Class& modelClass);
};

//************************************************************************************************
// DocumentationScanner::Token
//************************************************************************************************

struct DocumentationScanner::Token
{
	enum Type 
	{
		kUndefined,
		kIdentifier, 
		kNumber,
		kString,
		kChar,
		kOperator
	};
	Type type;
	MutableCString text;

	Token (Type type = kUndefined): type (type) {}
	Token (Type type, CStringRef text): type (type), text (text) {}
};

//************************************************************************************************
// DocumentationScanner::SourceFileParser - scan sounce code files
//************************************************************************************************

class DocumentationScanner::SourceFileParser: public Unknown
{
public:
	SourceFileParser (DocumentationScanner& scanner);

	bool parseFile (UrlRef file, IProgressNotify* progress);
	
	static IUrlFilter& getFilter ();
	
	struct Element
	{		
		SourceCodeElement::Type type;
		MutableCString data; // data is used to link the element to its documentation
		int scanningPart;    // scanning part is an implementation helper

		Element (SourceCodeElement::TypeRef type = SourceCodeElement::kUndefined): type (type), scanningPart (0) {}
		Element (SourceCodeElement::TypeRef type, CStringRef data): type (type), data (data), scanningPart (0) {}
	};		

	Element* getLastNonScopeElement () const;
	bool pushElement (const Element& element, bool handleRecentDoxyComment = true);
	void popElement ();
	void flushRecentDoxyComment ();

protected:
	DocumentationScanner& scanner;	

	Vector<Element> elementStack;
	Element lastPoppedElement;
	String recentLeadingDoxyComment;
	String file; // for debugging

	void parse (LineReader& reader, Token& token);
	bool nextToken (LineReader& reader, Token& token);

	bool handlePreprocessor (LineReader& reader);
	bool handleComment (LineReader& reader);
	bool handleElement (LineReader& reader, Token& token);

	bool handleEnum (LineReader& reader, Token& token);
	bool handleEnumValueConstant (Element& element, LineReader& reader, Token& token);
	bool handleEnumInfo (Element& element, LineReader& reader, Token& token);
	bool handleClassMethods (Element& element, LineReader& reader, Token& token);
	bool handleClassProperties (Element& element, LineReader& reader, Token& token);
	bool onElementType (LineReader& reader, Token& token, SourceCodeElement::TypeRef type);
    bool getScopedName (MutableCString& scopedName, const Element& element) const;

	enum DoxyCommentType {kLeading, kTrailing};
	void handleDoxyComment (String& comment, DoxyCommentType type);
	int getDoxyCommandArgCount (StringRef command) const;
	bool parseDoxyComment (DocuSnippet& target, StringRef comment) const;
	void parseDoxyCommand (int& i, DocuSnippet& target, StringRef comment) const;
	void collectArguments (Core::Vector<String>& arguments, int& i, const int numArgs, StringRef comment) const;

	bool isDocumentableElement (const Element& e);
	String& cleanupParsedString (String& str) const;
};

//************************************************************************************************
// GuiDocuScanner
//************************************************************************************************

class GuiDocuScanner: public DocumentationScanner
{
public:
	static bool isSkinClassModel (Model::ClassRepository& repository);
	static bool isVisualStyleClassModel (Model::ClassRepository& repository);

	using SuperClass = DocumentationScanner;
	GuiDocuScanner ();

	// DocumentationModelHandler
	void postScan () override;
	bool applyToModel (Model::ClassRepository& repository) override;
	bool isMatchingFolder (UrlRef folder) override;	
	bool handleMacros (SourceFileParser& parser, Token& token) override;
	bool isDocumentableElementType (SourceCodeElement::TypeRef type) const override;
	bool isScopingElementType (SourceCodeElement::TypeRef type) const override;

protected:	
	struct MacroHandler;

	static StringID	kSkinElement;
	static StringID	kSkinElementMemberList;
	static StringID	kSkinElementMember;
	static StringID	kSkinEnum;
	static StringID	kSkinClassDeclaration;
	static StringID	kStyleDef;
	static StringID	kVisualStyleClass;
	static StringID	kVisualStyleProperty;

	// gui option definitions
	struct OptionList: public Unknown
	{
		struct Item: public Unknown
		{
			MutableCString skinName;
			MutableCString cppName;
			SharedPtr<DocuSnippet> docu;
		};
		MutableCString name;
		LinkedList <AutoPtr <Item> > items;			
	};

	// visual styles
	struct VisualStyle: public Unknown
	{
		struct Property: public Unknown
		{
			Property (CStringPtr _type = nullptr): type (_type) {}
			MutableCString type;
			MutableCString name;
			SharedPtr<DocuSnippet> docu;
		};
		MutableCString name;
		MutableCString skinTag;

		LinkedList <AutoPtr <Property> > properties;
		SharedPtr<DocuSnippet> docu;
	};

	// skin element 
	struct SkinElement: public Unknown
	{
		MutableCString elementClass;
		MutableCString tagName;
		MutableCString groupName;
		MutableCString relatedClass;
		SharedPtr<DocuSnippet> docu;

		struct Member: public Unknown
		{
			MutableCString tagName;
			MutableCString typeName;
			SharedPtr<DocuSnippet> docu;
		};
		LinkedList <AutoPtr <Member> > members;
	};
	// skin enum
	struct SkinEnum: public Unknown
	{
		MutableCString skinClassName;
		MutableCString enumName;
		MutableCString cppStyleDefName;
		MutableCString parentSkinClassName;
		MutableCString parentEnumName;

		SharedPtr<OptionList> optionList; // resolved
	};

	LinkedList<AutoPtr <OptionList> > optionLists;
	LinkedList<AutoPtr <VisualStyle> > visualStyles;
	LinkedList<AutoPtr <SkinElement> > skinElements;
	LinkedList<AutoPtr <SkinEnum> > skinEnums;

	bool isSkinSnippet (DocuSnippet* snippet) const;
	
	bool applyToSkinModel (Model::ClassRepository& repository);
	bool applyToVisualStyleModel (Model::ClassRepository& repository);
};

} // namespace CCL

#endif // _docscanner_h
