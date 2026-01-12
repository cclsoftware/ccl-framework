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
// Filename    : ccl/extras/modeling/docscanner.cpp
// Description : Documentation Scanner
//
//************************************************************************************************

#include "ccl/extras/modeling/docscanner.h"
#include "ccl/extras/modeling/classrepository.h"

#include "ccl/base/storage/textfile.h"
#include "ccl/base/message.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/base/iprogress.h"

using namespace CCL;
using namespace Model;

//************************************************************************************************
// CCL_KERNEL_INIT
//************************************************************************************************

CCL_KERNEL_INIT (DocumentationScanner)
{
	struct Helper {	static DocumentationScanner* createGuiScanner () {return NEW GuiDocuScanner; } };
	DocumentationScanner::registerScannerType ("skin", Helper::createGuiScanner);
	DocumentationScanner::registerScannerType ("visual", Helper::createGuiScanner);
	return true;
}

StringID SourceCodeElement::kUndefined = CString::kEmpty;
StringID SourceCodeElement::kNamespace = CSTR ("namespace");
StringID SourceCodeElement::kClass = CSTR ("class");
StringID SourceCodeElement::kEnum = CSTR ("enum");
StringID SourceCodeElement::kEnumValue = CSTR ("enumValue");
StringID SourceCodeElement::kScope = CSTR ("scope");
StringID SourceCodeElement::kTemplate = CSTR ("template");
StringID SourceCodeElement::kTemplateArg= CSTR ("templateArg");
StringID SourceCodeElement::kEnumInfo = CSTR ("enumInfo");
StringID SourceCodeElement::kEnumInfoValue = CSTR ("enumInfoValue");
StringID SourceCodeElement::kClassMethodList = CSTR ("methodList");
StringID SourceCodeElement::kClassMethod = CSTR ("classMethod");
StringID SourceCodeElement::kClassPropertyList = CSTR ("propertyList");
StringID SourceCodeElement::kClassProperty = CSTR ("classProperty");
StringID SourceCodeElement::kConstant = CSTR ("const");

//************************************************************************************************
// SourceCodeElement::Define
//************************************************************************************************

bool SourceCodeElement::Define::resolve (MutableCString& str)
{
	if(name == str)
	{
		str = value;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceCodeElement::Define::resolveParts (MutableCString& str)
{
	if(str.contains (name))
	{
		String target (str);
		String defineName (name);
		String defineValue (value);
		target.replace (defineName, defineValue);
		str = target;
		return true;
	}
	return false;
}

//************************************************************************************************
// SourceCodeElement::DocuSnippet
//************************************************************************************************

int SourceCodeElement::DocuSnippet::scopedCompare (CStringRef partScope) const
{
	return DocumentationScanner::scopedCompare (scopedName, partScope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SourceCodeElement::DocuSnippet::scopeCount () const
{
	CStringPtr s1 = scopedName;
	int colonCount = 0;

	while(*s1)
	{
		if(*s1 == ':')
			colonCount++;
		s1++;
	}
	return colonCount / 2;
}

//************************************************************************************************
// DocumentationScanner::LineReader
//************************************************************************************************

class DocumentationScanner::LineReader: public TextFile
{
public:
	LineReader (UrlRef path)
	: TextFile (path, TextFile::kOpen),
	  lineNumber (0),
	  lineChars (nullptr),
	  lineLength (0),
	  lineReadIdx (0),
	  _lineChars (nullptr)
	{}
	~LineReader ()
	{
		releaseLineChars ();
	}

	bool lineDone () const {return lineReadIdx >= lineLength; }
	bool skipLine () { lineReadIdx = lineLength; return true; }

	bool nextLine (bool trim = true)
	{
		releaseLineChars ();
		if(streamer->readLine (line))
		{
			lineNumber++;

			if(trim)
				line.trimWhitespace ();
			lineLength = line.length ();

			_lineChars = NEW StringChars (line);
			lineChars = *_lineChars;
			return true;
		}
		return false;
	}

	bool nextChar ()
	{
		lineReadIdx++;
		while(lineDone ())
			if(nextLine () == false)
				return false;
		return true;
	}

	bool skipWhitespace ()
	{
		while((lineDone () == false && Unicode::isWhitespace (lineChars[lineReadIdx])) || lineDone ())
			if(nextChar () == false)
				return false;
		return true;
	}

	int lineNumber;
	String line;
	const uchar* lineChars;
	int lineLength;
	int lineReadIdx;

private:
	StringChars* _lineChars;
	void releaseLineChars ()
	{
		if(_lineChars)
		{
			delete _lineChars;
			_lineChars = nullptr;
		}
		lineReadIdx = 0;
		lineChars = nullptr;
		lineLength = 0;
	}
};

//************************************************************************************************
// DocumentationScanner
//************************************************************************************************

LinkedList<DocumentationScanner::ScannerType> DocumentationScanner::scannerTypes;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::registerScannerType (StringRef modelTitlePart, CreateScannerFunc createFunc)
{
	for(auto& scannerType : scannerTypes)
		if(scannerType.modelTitlePart == modelTitlePart)
			return false;

	scannerTypes.append ({modelTitlePart, createFunc});
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner* DocumentationScanner::createScannerForModel (Model::ClassRepository& repository)
{
	for(auto& scannerType : scannerTypes)
		if(repository.getTitle ().contains (scannerType.modelTitlePart, false))
			return scannerType.createFunc ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentationScanner::scopedCompare (CStringRef scopedName, CStringRef partScope)
{
	int fullLength = scopedName.length ();
	int partScopeLength = partScope.length ();
	if(fullLength == 0 || partScopeLength == 0)
		return 0;

	if(scopedName == partScope)
		return 3;

	CStringPtr s1Start = scopedName;
	CStringPtr s1 = scopedName;
	s1 += fullLength - 1;

	CStringPtr s2Start = partScope;
	CStringPtr s2End = partScope;
	s2End += partScopeLength - 1;
	CStringPtr s2 = s2End;

	int colonCount = 0;

	while(s1 > s1Start && s2 > s2Start && *s1 == *s2)
	{
		if(*s1 == ':')
			colonCount++;

		s1--;
		s2--;
	}

	if(s2 == s2Start && s1[-1] == ':')
		return colonCount > 0 ? 2 : 1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::DocumentationScanner ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::postScan ()
{
	// resolve defines
	for(auto& define : defines)
	{
		for(auto& docuSnippet : docuSnippets)
		{
			if(define->resolve (docuSnippet->targetName))
				define->resolveParts (docuSnippet->scopedName);
		}

		for(auto& enumInfo : enumsInfos)
		{
			define->resolve (enumInfo->name);
			for(auto& enumInfoItem : enumInfo->items)
				define->resolve (enumInfoItem->name);
		}

		// member lists are not expected to use defines...
	}

	// link docu for enum using constants
	for(auto& constant : enumConstants)
	{	
		for(auto& docuSnippet : docuSnippets)
		{
			if(docuSnippet->elementType == SourceCodeElement::kConstant)
			{
				if(docuSnippet->targetName == constant->constantName)
				{
					constant->docu = docuSnippet;
					break;
				}
			}
		}
	}

	// connect docu for enum infos
	for(auto& enumInfo : enumsInfos)
	{
		for(auto& docuSnippet : docuSnippets)
		{
			if(docuSnippet->elementType == SourceCodeElement::kEnumInfo)
			{
				if(docuSnippet->targetName == enumInfo->name)
				{
					enumInfo->docu = docuSnippet;
					break;
				}
			}
		}

		for(auto& enumInfoItem : enumInfo->items)
		{
			MutableCString scopedMember (enumInfo->name);
			scopedMember.append ("::");
			scopedMember.append (enumInfoItem->name);

			for(auto& docuSnippet : docuSnippets)
			{
				if(docuSnippet->elementType == SourceCodeElement::kEnumInfoValue)
				{
					if(docuSnippet->scopedCompare (scopedMember) > 1)
					{
						enumInfoItem->docu = docuSnippet;
						break;
					}
				}
			}
		}
	}

	// connect docu for class methods
	for(auto& classMethods : classMethodLists)
	{
		for(auto& method : classMethods->methods)
		{
			MutableCString scopedMember (classMethods->className);
			scopedMember.append ("::");
			scopedMember.append (method->name);

			for(auto& docuSnippet : docuSnippets)
			{
				if(docuSnippet->elementType == SourceCodeElement::kClassMethod)
				{
					if(docuSnippet->scopedCompare (scopedMember) > 1)
					{
						method->docu = docuSnippet;
						break;
					}
				}
			}
		}
	}

	// connect docu for class properties
	for(auto& classProperties : classPropertyLists)
	{
		for(auto& prop : classProperties->properties)
		{
			MutableCString scopedMember (classProperties->className);
			scopedMember.append ("::");
			scopedMember.append (prop->name);

			for(auto& docuSnippet : docuSnippets)
			{
				if(docuSnippet->elementType == SourceCodeElement::kClassMethod)
				{
					if(docuSnippet->scopedCompare (scopedMember) > 1)
					{
						prop->docu = docuSnippet;
						break;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyMethods (Model::Class& modelClass)
{
	bool anyChanges = false;

	// methods
	SourceCodeElement::ClassMethodList* methodList = nullptr;
	for(auto& classMethods : classMethodLists)
	{
		if(classMethods->className == modelClass.getName ())
		{
			methodList = classMethods;
			break;
		}
	}

	if(methodList)
	{
		for(auto& scannedMethod : methodList->methods)
		{
			Model::Method* modelMethod = modelClass.findMethod (scannedMethod->name);

			if(modelMethod == nullptr)
			{
				modelMethod = NEW Model::Method (scannedMethod->name);
				modelClass.addMethod (modelMethod);
				anyChanges = true;
			}

			if(scannedMethod->args.isEmpty () == false)
				if(modelMethod->getArguments ().isEmpty ())
				{
					modelMethod->addArgument (NEW Model::MethodArgument (scannedMethod->args)); // all in one...
					anyChanges = true;
				}

			if(scannedMethod->returnValue.isEmpty () == false)
			{
				if(modelMethod->getReturnValue ().getName () != scannedMethod->returnValue)
				{
					modelMethod->getReturnValue ().setName (scannedMethod->returnValue);
					anyChanges = true;
				}
			}

			if(scannedMethod->docu.isValid ())
			{
				if(applyDocu (*modelMethod, scannedMethod->docu))
					anyChanges = true;
			}
		}
	}

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyProperties (Model::Class& modelClass)
{
	bool anyChanges = false;
	// properties
	SourceCodeElement::ClassPropertyList* propertyList = nullptr;
	for(auto& classProps : classPropertyLists)
	{
		if(classProps->className == modelClass.getName ())
		{
			propertyList = classProps;
			break;
		}
	}
	if(propertyList)
	{
		for(auto& scannedProp : propertyList->properties)
		{
			if(scannedProp->docu.isValid ())
			{
				// ??? member ???
				auto modelMember = modelClass.findMember (scannedProp->name);
				if(modelMember)
				{
					if(applyDocu (*modelMember, scannedProp->docu))
						anyChanges = true;
				}
			}
		}
	}

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyToModel (Model::ClassRepository& repository)
{
	bool anyChangesAtAll = false;

	// classes
	ForEach (repository.getClasses (), Model::Class, modelClass)

		if(applyMethods (*modelClass))
			anyChangesAtAll = true;

		if(applyProperties (*modelClass))
			anyChangesAtAll = true;

		DocuSnippet* matchingDocu = nullptr;
		int matchLevel = 0;

		// find matching docu for a class
		for(auto& docuSnippet : docuSnippets)
		{
			if(docuSnippet->elementType == SourceCodeElement::kClass)
			{
				int level = docuSnippet->scopedCompare (modelClass->getName ());
				if(level > matchLevel)
				{
					matchingDocu = docuSnippet;
					matchLevel = level;
					if(matchLevel == 3)
						break;
				}
			}
		}

		if(applyDocu (*modelClass, matchingDocu))
			anyChangesAtAll = true;
	EndFor

	if(anyChangesAtAll)
		repository.signal (Message (kPropertyChanged));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::handleMacros (SourceFileParser& parser, Token& token)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isDocumentableElementType (SourceCodeElement::TypeRef type) const
{
	return type == SourceCodeElement::kClass
		|| type == SourceCodeElement::kEnumValue
		|| type == SourceCodeElement::kEnumInfo
		|| type == SourceCodeElement::kEnumInfoValue
		|| type == SourceCodeElement::kClassMethod
		|| type == SourceCodeElement::kConstant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isScopingElementType (SourceCodeElement::TypeRef type) const
{
	return type == SourceCodeElement::kClass
		|| type == SourceCodeElement::kNamespace
		|| type == SourceCodeElement::kEnumInfo
		|| type == SourceCodeElement::kClassMethodList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::scanFolder (UrlRef folder, IProgressNotify* progress, bool inMatchingFolder)
{
	ForEachFile (System::GetFileSystem ().newIterator (folder), _path)
		UrlRef path = *_path;
		if(path.isFolder ())
		{
			bool folderMatches = inMatchingFolder ? true : isMatchingFolder (path);

			if(progress && folderMatches)
				progress->setProgressText (path.getPath ());

			if(scanFolder (path, progress, folderMatches) == false)
				return false;
		}
		else if(inMatchingFolder && SourceFileParser::getFilter ().matches (path))
		{
			SourceFileParser parser (*this);
			parser.parseFile (path, progress);
		}

		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::scanCode (UrlRef folder, IProgressNotify* progress)
{
	ProgressNotifyScope progressScope (progress);

	if(scanFolder (folder, progress, isMatchingFolder (folder)))
	{
		postScan ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isMatchingFolder (UrlRef folder)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyDocu (Model::Element& target, DocuSnippet* docu) const
{
	if(docu)
	{
		bool anyChanges = false;
		if(target.getDocumentation ().getBriefDescription () != docu->brief)
		{
			target.getDocumentation ().setBriefDescription (docu->brief);
			anyChanges = true;
		}

		if(target.getDocumentation ().getDetailedDescription () != docu->details)
		{
			target.getDocumentation ().setDetailedDescription (docu->details);
			anyChanges = true;
		}

		if(target.getDocumentation ().getCodeExample () != docu->code)
		{
			target.getDocumentation ().setCodeExample (docu->code);
			anyChanges = true;
		}

		if(target.getDocumentation ().getCodeLanguage () != docu->codeLang)
		{
			target.getDocumentation ().setCodeLanguage (docu->codeLang);
			anyChanges = true;
		}

		if(target.getDocumentation ().getLinks ().isEqual (docu->links) == false)
		{
			anyChanges = true;
			target.getDocumentation ().setLinks (docu->links);
		}

		if(anyChanges)
		{
			if(ccl_cast<Model::MainElement> (&target) != nullptr)
				target.deferChanged ();
			return true;
		}
	}
	return false;
}


//************************************************************************************************
// DocumentationScanner::Parser
//************************************************************************************************

static inline bool uStrStartsWith (const uchar* s1, const char* s2)
{
	while(*s1 == *s2 && *s2)
	{
		s1++;
		s2++;
	}

	return (*s2 == 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static inline const uchar* uStrFindStr (const uchar* s1, const char* s2)
{
	while(true)
	{
		if(uStrStartsWith (s1, s2))
			return s1;
		if(*s1)
			s1++;
		else
			return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::SourceFileParser::SourceFileParser (DocumentationScanner& _scanner)
: scanner (_scanner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseFile (UrlRef file, IProgressNotify* progress)
{
	LineReader sourceFile (file);
	if(!sourceFile.isValid ())
		return false;

	this->file = file.getPath ();

	Token token;
	parse (sourceFile, token);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::parse (LineReader& reader, Token& token)
{
	while(true)
	{
		while(reader.lineDone ())
			if(reader.nextLine () == false)
				return;

		if(handleComment (reader))
			continue;

		if(handlePreprocessor (reader))
			continue;

		if(nextToken (reader, token) == false)
			return;

		if(handleElement (reader, token) == false)
			return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handlePreprocessor (LineReader& reader)
{
	if(reader.lineChars == nullptr)
		return false;

	bool isPreprocessor = (reader.lineReadIdx == 0 && reader.lineChars[reader.lineReadIdx] == '#'); // preprocessor
	if(isPreprocessor == false)
		return false;

	String preprocessorData;

	while(true)
	{
		bool directiveContinues = reader.lineChars[reader.lineLength - 1] == '\\';
		if(directiveContinues)
		{
			preprocessorData.append (reader.lineChars, reader.lineLength - 1);
			if(reader.nextLine () == false)
				return false;
			if(reader.lineLength < 1) // ???
				break;
		}
		else
		{
			preprocessorData.append (reader.line);
			reader.nextLine ();
			break;
		}
	}

	MutableCString data (preprocessorData);

	// #define TAG_LAYOUT			_A ("Layout")
	if(data.startsWith ("#define"))
	{
		AutoPtr<SourceCodeElement::Define> define = NEW SourceCodeElement::Define;
		Core::CStringTokenizer tokenizer (data, " \t");

		for(int idx = 0; idx < 5 ;idx++)
		{
			CString token (tokenizer.next ());
			if(token.isEmpty ())
				break;

			if(idx == 1)
			{
				define->name = token;
			}
			else if(idx > 1)
			{
				int quoteIndex = token.index ('"');
				if(quoteIndex >= 0)
				{
					int lastQuoteIndex = token.lastIndex ('"');
					if(lastQuoteIndex > quoteIndex)
					{
						define->value = token.subString (quoteIndex + 1, lastQuoteIndex - quoteIndex - 1);
					}
				}
			}
			if(define->name.isEmpty () == false && define->value.isEmpty () == false)
			{
				scanner.defines.append (define);
				break;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleComment (LineReader& reader)
{
	if(reader.skipWhitespace () == false)
		return false;

	auto& lineChars = reader.lineChars;
	auto& lineIdx = reader.lineReadIdx;
	if(lineChars == nullptr)
		return false;

	if(lineChars[lineIdx] == '/')
	{
		if(lineChars[lineIdx + 1] == '/') // line comment
		{
			if(lineChars[lineIdx + 2] == '/' && lineChars[lineIdx + 3] == '<') // doxy line comment
			{
				String doxyComment (lineChars + lineIdx + 4);
				handleDoxyComment (doxyComment, kTrailing);
			}
			return reader.skipLine ();
		}
		else if(lineChars[lineIdx + 1] == '*') // block comment
		{
			lineIdx += 2;
			bool isDoxyComment = lineChars[lineIdx] == '*';
			if(isDoxyComment)
				lineIdx++;

			String doxyComment;
			auto addToDoxyComment = [&] (const uchar* start, int len)
			{
				if(isDoxyComment)
				{
					if(doxyComment.isEmpty () == false)
					{
						static StringRef kEndline (CCLSTR (ENDLINE));
						doxyComment.append (kEndline);
					}
					doxyComment.append (start, len);
				}
			};

			while(true)
			{
				const uchar* start = lineChars + lineIdx;
				if(start == nullptr)
					return true;

				const uchar* end = uStrFindStr (start, "*/");
				if(end)
				{
					int len = int (end - start);
					addToDoxyComment (start, len);
					doxyComment.trimWhitespace ();
					lineIdx += len + 2;	 // + 2 "*/"
					break;
				}
				else
				{
					addToDoxyComment (start, -1);
					reader.nextLine (false);
				}
			}

			if(doxyComment.isEmpty () == false)
				recentLeadingDoxyComment = doxyComment;

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::nextToken (LineReader& reader, Token& token)
{
	token.text.empty ();
	token.type = Token::kUndefined;

	if(reader.skipWhitespace () == false)
		return false;

	auto& lineChars = reader.lineChars;
	auto& lineIdx = reader.lineReadIdx;

	uchar c = lineChars[lineIdx];
	if(Unicode::isDigit(c))
	{
		token.type = Token::kNumber;
		CStringWriter<128> writer (token.text);
		while(Unicode::isDigit (c))
		{
			writer.append (c);
			c = lineChars[++lineIdx];
		}
		writer.flush ();
	}
	else if(Unicode::isAlphaNumeric (c) || c == '_')
	{
		token.type = Token::kIdentifier;
		CStringWriter<128> writer (token.text);
		while(Unicode::isAlphaNumeric (c) || c == '_')
		{
			writer.append (c);
			c = lineChars[++lineIdx];
		}
		writer.flush ();
	}
	else if(c == '"' || c == '\'') // string / char
	{
		token.type = c == '"' ? Token::kString : Token::kChar;
		CStringWriter<128> writer (token.text);
		while(true)
		{
			uchar c2 = lineChars[++lineIdx];
			if((c2 == c && lineChars[lineIdx-1] != '\\') || reader.lineDone ())
			{
				lineIdx++;
				break;
			}
			writer.append (c2);
		}
		writer.flush ();
	}
	else if(c != 0)
	{
		token.type = Token::kOperator;
		token.text.append (c);
		lineIdx++;

		if(c==':' && lineChars[lineIdx] == ':')
		{
			token.text.append (c);
			lineIdx++;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::pushElement (const Element& e, bool handleRecentDoxyComment)
{
	elementStack.add (e);

	lastPoppedElement = Element ();
	if(handleRecentDoxyComment)
		flushRecentDoxyComment ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::popElement ()
{
	if(elementStack.count () > 0)
	{
		flushRecentDoxyComment ();

		lastPoppedElement = elementStack.last ();
		elementStack.removeLast ();
	}

	recentLeadingDoxyComment.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::SourceFileParser::Element* DocumentationScanner::SourceFileParser::getLastNonScopeElement () const
{
	for(int i = elementStack.count () - 1; i >= 0; i--)
	{
		if(elementStack [i].type != SourceCodeElement::kScope)
			return &elementStack [i];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::onElementType (LineReader& reader, Token& token, SourceCodeElement::TypeRef type)
{
	if(nextToken (reader, token) == false)
		return false;

	if(token.type != Token::kIdentifier)
		return handleElement (reader, token);

	if(token.text.isEmpty ())
	{
		ASSERT (0)
		return false;
	}

	Element e (type, token.text);
	return pushElement (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleEnum (LineReader& reader, Token& token)
{
	if(elementStack.count () >= 2)
	{
		if(elementStack.last ().type == SourceCodeElement::kScope)
		{
			const Element& e2 = elementStack.at (elementStack.count () - 2);
			if(e2.type == SourceCodeElement::kEnum)
			{
				Element e (SourceCodeElement::kEnumValue, token.text);
				return pushElement (e);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleEnumValueConstant (Element& element, LineReader& reader, Token& token)
{
	if(element.type == SourceCodeElement::kEnumValue)
	{
		bool nextTokenHandled = false;
		if(nextToken (reader, token))
		{
			if(token.type == Token::kIdentifier)
			{
				nextTokenHandled = true;
				if(CString::isDigit (token.text.firstChar ()) == false)
				{
					MutableCString constantName = token.text;

					nextTokenHandled = false;
					if(nextToken (reader, token))
					{
						if(token.type == Token::kOperator && token.text == "::") // scoped ? (one level)
						{
							nextTokenHandled = true;
							nextToken (reader, token);
							if(token.type == Token::kIdentifier)
							{
								constantName.append ("::");
								constantName.append (token.text);
							}
							else
							{
								nextTokenHandled = false;
								constantName.empty ();
							}
						}								
					}

					if(constantName.isEmpty () == false)
					{
						AutoPtr<SourceCodeElement::EnumValueConstant> constant = NEW SourceCodeElement::EnumValueConstant;
						constant->enumItemName = elementStack.last ().data;
						getScopedName (constant->scopedName, elementStack.last ());
						constant->constantName = constantName;
						scanner.enumConstants.append (constant);
					}	
				}
			}
		}
		if(nextTokenHandled == false)
			return handleElement (reader, token);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleEnumInfo (SourceFileParser::Element& element, LineReader& reader, Token& token)
{
	static AutoPtr<SourceCodeElement::EnumInfo> currentEnumInfo;
	static AutoPtr<SourceCodeElement::EnumInfo::Item> currentEnumInfoItem;

	auto getPartString = [&] () -> MutableCString*
	{
		if(element.type == SourceCodeElement::kEnumInfo)
		{
			if(currentEnumInfo != nullptr && element.scanningPart == 0)
				return &currentEnumInfo->name;
		}
		else if(element.type == SourceCodeElement::kEnumInfoValue)
		{
			if(currentEnumInfoItem != nullptr && element.scanningPart == 0)
				return &currentEnumInfoItem->name;
		}
		return nullptr;
	};

	if(element.type == SourceCodeElement::kEnumInfo)
	{
		if(token.type == Token::kIdentifier || token.type == Token::kString)
		{
			if(token.type == Token::kIdentifier && token.text == "END_ENUMINFO")
			{
				if(currentEnumInfo && currentEnumInfo->items.isEmpty () == false)
					scanner.enumsInfos.append (currentEnumInfo);
				currentEnumInfo = nullptr;
				currentEnumInfoItem = nullptr;

				popElement ();
			}
			else if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == "(")
			{
				if(currentEnumInfo == nullptr && element.scanningPart == 0)
					currentEnumInfo = NEW SourceCodeElement::EnumInfo;
			}
			else if(token.text == ")")
			{
				if(currentEnumInfo)
				{
					if(element.data.isEmpty ())
						element.data = currentEnumInfo->name;
					flushRecentDoxyComment ();
				}
			}
			else if(token.text == "{")
			{
				ASSERT (currentEnumInfoItem == nullptr)
				currentEnumInfoItem = NEW SourceCodeElement::EnumInfo::Item;
				pushElement (SourceCodeElement::kEnumInfoValue, false);
			}
		}
	}
	else if(element.type == SourceCodeElement::kEnumInfoValue)
	{
		if(token.type == Token::kIdentifier || token.type == Token::kString)
		{
			if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == "}")
			{
				if(currentEnumInfoItem && currentEnumInfo && element.type == SourceCodeElement::kEnumInfoValue)
				{
					element.data = currentEnumInfoItem->name;
					popElement ();
					currentEnumInfo->items.append (currentEnumInfoItem);
				}
				currentEnumInfoItem = nullptr;
			}
			else if(token.text == ",")
			{
				if(currentEnumInfoItem)
					element.scanningPart++;
			}
		}
	}

	if(token.type == Token::kOperator && token.text == "::")
	{
		if(auto string = getPartString ())
			string->append (token.text);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleClassMethods (SourceFileParser::Element& element, LineReader& reader, Token& token)
{
	static AutoPtr<SourceCodeElement::ClassMethodList> currentMethodList;
	static AutoPtr<SourceCodeElement::ClassMethodList::Method> currentMethod;

	if(element.type == SourceCodeElement::kClassMethodList)
	{
		auto getPartString = [&] () -> MutableCString*
		{
			if(currentMethodList != nullptr && element.scanningPart == 0)
				return &currentMethodList->className;
			return nullptr;
		};

		if(token.type == Token::kIdentifier)
		{
			if(token.text == "END_METHOD_NAMES")
			{
				if(currentMethodList && currentMethodList->methods.isEmpty () == false)
					scanner.classMethodLists.append (currentMethodList);
				currentMethodList = nullptr;
				currentMethod = nullptr;

				popElement ();
			}
			else if(token.text == "DEFINE_METHOD_NAME" || token.text == "DEFINE_METHOD_ARGS" || token.text == "DEFINE_METHOD_ARGR")
			{
				ASSERT (currentMethod == nullptr)
				currentMethod = NEW SourceCodeElement::ClassMethodList::Method;
				pushElement (SourceCodeElement::kClassMethod, false);
			}
			else if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == "(")
			{
				if(currentMethodList == nullptr && element.scanningPart == 0)
					currentMethodList = NEW SourceCodeElement::ClassMethodList;
			}
			else if(token.text == ")")
			{
				if(currentMethodList)
				{
					if(element.data.isEmpty ())
						element.data = currentMethodList->className;
				}
			}
			else if(token.text == "::" )
			{
				if(auto string = getPartString ())
					string->append (token.text);
			}
		}
	}
	else if(element.type == SourceCodeElement::kClassMethod)
	{
		auto getPartString = [&] () -> MutableCString*
		{
			if(currentMethod != nullptr)
			{
				switch(element.scanningPart)
				{
				case 0: return &currentMethod->name;
				case 1: return &currentMethod->args;
				case 2: return &currentMethod->returnValue;
				}
			}
			return nullptr;
		};

		if(token.type == Token::kIdentifier || token.type == Token::kString)
		{
			if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == ")")
			{
				if(currentMethodList && currentMethod && element.type == SourceCodeElement::kClassMethod)
				{
					element.data = currentMethod->name;
					popElement ();
					currentMethodList->methods.append (currentMethod);
				}
				currentMethod = nullptr;
			}
			else if(token.text == ",")
			{
				element.scanningPart++;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleClassProperties (Element& element, LineReader& reader, Token& token)
{
	static AutoPtr<SourceCodeElement::ClassPropertyList> currentPropertyList;
	static AutoPtr<SourceCodeElement::ClassPropertyList::Property> currentProperty;

	if(element.type == SourceCodeElement::kClassPropertyList)
	{
		auto getPartString = [&] () -> MutableCString*
		{
			if(currentPropertyList != nullptr && element.scanningPart == 0)
				return &currentPropertyList->className;
			return nullptr;
		};

		if(token.type == Token::kIdentifier)
		{
			if(token.text == "END_PROPERTY_NAMES")
			{
				if(currentPropertyList && currentPropertyList->properties.isEmpty () == false)
					scanner.classPropertyLists.append (currentPropertyList);
				currentPropertyList = nullptr;
				currentProperty = nullptr;

				popElement ();
			}
			else if(token.text == "DEFINE_PROPERTY_NAME" || token.text == "DEFINE_PROPERTY_TYPE" || token.text == "DEFINE_PROPERTY_CLASS" || token.text == "DEFINE_PROPERTY_METACLASS" || token.text == "DEFINE_PROPERTY_CONTAINER")
			{
				ASSERT (currentProperty == nullptr)
				currentProperty = NEW SourceCodeElement::ClassPropertyList::Property;
				pushElement (SourceCodeElement::kClassProperty, false);
			}
			else if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == "(")
			{
				if(currentPropertyList == nullptr && element.scanningPart == 0)
					currentPropertyList = NEW SourceCodeElement::ClassPropertyList;
			}
			else if(token.text == ")")
			{
				if(currentPropertyList)
				{
					if(element.data.isEmpty ())
						element.data = currentPropertyList->className;
				}
			}
			else if(token.text == "::" )
			{
				if(auto string = getPartString ())
					string->append (token.text);
			}
		}
	}
	else if(element.type == SourceCodeElement::kClassProperty)
	{
		auto getPartString = [&] () -> MutableCString*
		{
			if(currentProperty != nullptr && element.scanningPart == 0)
				return &currentProperty->name;
			return nullptr;
		};

		if(token.type == Token::kIdentifier || token.type == Token::kString)
		{
			if(auto string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == Token::kOperator)
		{
			if(token.text == ")")
			{
				if(currentPropertyList && currentProperty)
				{
					element.data = currentProperty->name;
					popElement ();
					currentPropertyList->properties.append (currentProperty);
				}
				currentProperty = nullptr;
			}
			else if(token.text == ",")
			{
				element.scanningPart++;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleElement (LineReader& reader, Token& token)
{
	if(scanner.handleMacros (*this, token))
		return true;

	auto lastElement = getLastNonScopeElement ();
	if(lastElement)
	{
		if(lastElement->type == SourceCodeElement::kEnumInfo || lastElement->type == SourceCodeElement::kEnumInfoValue)
		{
			return handleEnumInfo (*lastElement, reader, token);
		}
		else if(lastElement->type == SourceCodeElement::kClassMethodList || lastElement->type == SourceCodeElement::kClassMethod)
		{
			return handleClassMethods (*lastElement, reader, token);
		}
		else if(lastElement->type == SourceCodeElement::kClassPropertyList || lastElement->type == SourceCodeElement::kClassProperty)
		{
			return handleClassProperties (*lastElement, reader, token);
		}
	}

	if(token.type == Token::kIdentifier)
	{
		if(token.text == "namespace")
			return onElementType (reader, token, SourceCodeElement::kNamespace);

		else if(token.text == "class")
		{
			if(elementStack.last ().type != SourceCodeElement::kTemplate && elementStack.last ().type != SourceCodeElement::kTemplateArg)
				return onElementType (reader, token, SourceCodeElement::kClass);
			return true;
		}

		else if(token.text == "struct" || token.text == "union" || token.text == "interface")
			return onElementType (reader, token, SourceCodeElement::kClass);

		else if(token.text == "enum")
			return onElementType (reader, token, SourceCodeElement::kEnum);

		else if(token.text == "DEFINE_ENUM")
		{
			if(nextToken (reader, token) == false) // expect a '('
				return false;
			return onElementType (reader, token, SourceCodeElement::kEnum);
		}

		else if(token.text == "BEGIN_ENUMINFO")
			return pushElement (SourceCodeElement::kEnumInfo, false);

		else if(token.text == "BEGIN_METHOD_NAMES")
			return pushElement (SourceCodeElement::kClassMethodList, false);

		else if(token.text == "BEGIN_PROPERTY_NAMES")
			return pushElement (SourceCodeElement::kClassPropertyList, false);

		else if(token.text == "using")
			return reader.skipLine ();

		else if(token.text == "template")
			return pushElement (SourceCodeElement::kTemplate);

		else if(token.text == "static")
		{
			if(lastElement == nullptr || (lastElement && lastElement->type == SourceCodeElement::kNamespace))
			{	
				if(nextToken (reader, token))
				{
					if(token.type == Token::kIdentifier)
					{
						if(token.text =="const" || token.text == "constexpr")
							if(nextToken (reader, token))
								if(token.type == Token::kIdentifier) // type
									return onElementType (reader, token, SourceCodeElement::kConstant);					
					}
				}
			}
		}
		else if(handleEnum (reader, token))
			return true;
	}
	else if(token.type == Token::kOperator)
	{
		if(recentLeadingDoxyComment.isEmpty () == false)
			recentLeadingDoxyComment.empty ();

		if(token.text == ";")
		{
			if(elementStack.last ().type == SourceCodeElement::kEnumValue || elementStack.last ().type == SourceCodeElement::kConstant)
				popElement ();

			if(elementStack.last ().type == SourceCodeElement::kClass || elementStack.last ().type == SourceCodeElement::kEnum || elementStack.last ().type == SourceCodeElement::kEnumInfo)
				popElement ();
		}
		else if(token.text == "{")
		{
			pushElement (SourceCodeElement::kScope);
		}
		else if(token.text == "}")
		{
			if(elementStack.last ().type == SourceCodeElement::kEnumValue || elementStack.last ().type == SourceCodeElement::kEnumInfoValue)
				popElement ();

			SOFT_ASSERT (elementStack.last ().type == SourceCodeElement::kScope, "expected closing scope element")
			popElement ();

			if(elementStack.last ().type == SourceCodeElement::kNamespace)
				popElement ();
		}
		else if(token.text == ",")
		{
			if(elementStack.last ().type == SourceCodeElement::kEnumValue)
				popElement ();
		}
		else if(token.text == "<")
		{
			if(elementStack.last ().type == SourceCodeElement::kTemplate || elementStack.last ().type == SourceCodeElement::kTemplateArg)
				pushElement (SourceCodeElement::kTemplateArg);
		}
		else if(token.text == ">")
		{
			if(elementStack.last ().type == SourceCodeElement::kTemplateArg)
				popElement ();
			if(elementStack.last ().type == SourceCodeElement::kTemplate)
				popElement ();
		}
		else if(token.text == "=")
		{			
			if(elementStack.last ().type == SourceCodeElement::kEnumValue)
			{
				return handleEnumValueConstant (elementStack.last (), reader, token);				
			}			
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::flushRecentDoxyComment ()
{
	if(recentLeadingDoxyComment.isEmpty () == false)
	{
		handleDoxyComment (recentLeadingDoxyComment, kLeading);
		recentLeadingDoxyComment.empty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentationScanner::SourceFileParser::getDoxyCommandArgCount (StringRef command) const
{
	static StringRef kEnd = CCLSTR ("end");
	if(command.startsWith (kEnd, false))
		return 0;

	static const String kCommandsWithNoArg [] =
	{
		CCLSTR ("a"),
		CCLSTR ("b"),
		CCLSTR ("c"),
		CCLSTR ("e"),
		CCLSTR ("n"),
		CCLSTR ("p"),
		CCLSTR ("em"),
		CCLSTR ("code"),
		CCLSTR ("verbatim"),
		CCLSTR ("brief")
	};

	int numCommandWithNoArgs = ARRAY_COUNT (kCommandsWithNoArg);
	for(int i = 0; i < numCommandWithNoArgs; i++)
		if(kCommandsWithNoArg[i].compare (command, false) == 0)
			return 0;

	static const String kCommandsWith2Args [] =
	{
		CCLSTR ("section"),
		CCLSTR ("subsection"),
		CCLSTR ("subsubsection"),
		CCLSTR ("page"),
		CCLSTR ("subpage"),
		CCLSTR ("weakgroup")
	};

	int numCommandWith2Args = ARRAY_COUNT (kCommandsWith2Args);
	for(int i = 0; i < numCommandWith2Args; i++)
		if(kCommandsWith2Args[i].compare (command, false) == 0)
			return 2;

	// all others have 1 (or at least we dont care...)
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Parse brief, details, links and code example from a doxygen comment.
	@param[in] comment  doxygen comment to parse
	@param[out] target  target object to set brief, details, links and code example for */

bool DocumentationScanner::SourceFileParser::parseDoxyComment (DocuSnippet& target, StringRef comment) const
{
	if(comment.isEmpty ())
		return false;

	// workaround lambda for String::append(uchar)
	auto appendChar = [](String &out, uchar c) -> void
	{
		String s;
		StringWriter<1> writer (s, true);
		writer.append (c);
		out << s;
	};

	int len = comment.length ();
	String result;
	for(int i = 0; i < len; ++i)
	{
		uchar c = comment.at (i);
		static const Vector<uchar> controlChars = { '\\', '@' };
		if(controlChars.contains (c))
		{
			uchar next = comment.at (++i);
			// skip redundancies like '@@'
			if(controlChars.contains (next))
				++i;
			else if(next == '.')
				// escaped '.' not to be used as end of brief
				appendChar (result, next);
			else if(Unicode::isAlpha (next))
			{
				parseDoxyCommand (i, target, comment);
				// reverse iteration by one to first character after last command (which can be a '\n'),
				// this character would otherwise be skipped by the for loop increment of i
				i--;
			}
		}
		// interpret quotations as text, do not scan for commands
		else if(c == '"')
		{
			int rangeStart = i;
			uchar next = comment.at (++i);
			while(next != '"' && i < len)
				next = comment.at (++i);
			// extend range by one to include closing "
			result.append (String () << comment.subString (rangeStart, i - rangeStart + 1));
		}
		else if (c == '.')
		{
			// assume end of brief on occurence of non-escaped '.', set brief only once
			if(target.brief.isEmpty ())
			{
				target.brief = cleanupParsedString (result);
				// with brief being set, any further parsing contributes to 'details'
				result = "";
			}
			else
				appendChar (result, c);
		}
		else
			appendChar (result, c);
	}

	result = cleanupParsedString (result);

	// set result as details only if there is a brief already
	if(!result.isEmpty ())
		if(target.brief.isEmpty ())
			target.brief = result;
		else
			target.details = result;

	return target.brief.isEmpty () == false || target.links.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/**
	Parse doxygen commands \see or \code{.lang}, skip any other.
	@param[out] i  parsing index in comment, incremented
	@param[in] comment  doxygen comment to parse command from
	@param[out] target  target object to set links or code example for
*/

void DocumentationScanner::SourceFileParser::parseDoxyCommand (int& i, DocuSnippet& target, StringRef comment) const
{
	int commandBegin = i;
	int len = comment.length ();

	// assume token ends on next whitespace
	while(i < len)
	{
		if(Unicode::isWhitespace (comment.at (i)))
			break;
		i++;
	}

	String command = comment.subString (commandBegin, i - commandBegin);

	// special case: process doxygen command '\code{.lang}'
	if(command.startsWith (CCLSTR ("code"), false))
	{
		// extract code language: '\code{.xml}'  -> 'xml'
		int languageStart = command.index (".");
		int languageEnd = command.index ("}");
		if(languageStart >= 0 && languageEnd > languageStart)
		{
			int languageLen = languageEnd - languageStart - 1;
			target.codeLang = command.subString (languageStart + 1, languageLen);
		}

		// determine start and end of code segment (\code until occurence of \endcode)
		int codeStart = i;
		int codeEnd = codeStart;

		// on missing \endcode, set iterator to end of comment to not interpret remaining details text as code
		static const String kDoxygenEndCode = CCLSTR("\\endcode");
		codeEnd = comment.index (kDoxygenEndCode);
		if(codeEnd == -1)
		{
			i = len;
			return;
		}

		String codeExample = comment.subString (codeStart, codeEnd - codeStart);
		target.code = cleanupParsedString (codeExample);

		// continue iteration after '\endcode'
		i = ccl_max (i, codeEnd + kDoxygenEndCode.length ());
		return;
	}

	// process any other command that is of format '\command arg1, arg2, ...'
	int numArgs = getDoxyCommandArgCount (command);

	// case: no arguments to process, just advance to next non-whitespace character and return
	if(numArgs == 0)
	{
		while(Unicode::isWhitespace (comment.at (i)))
			i++;

		return;
	}

	// case: at least one argument, extract all arguments for the given command
	// for \see command, set target links; note that the argument collection is always
	// required as it advances the parser index
	Core::Vector<String> args;
	collectArguments (args, i, numArgs, comment);
	if(command.compare (CCLSTR ("see"), false) == 0)
	{
		VectorForEach (args, String, a)
			target.links.add (a);
		EndFor
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/**
	 Extract doxygen command arguments, starting at 'i'.
	 @param[out] arguments  vector the parsed arguments are pushed to, is not cleared initially
	 @param[out] i  parsing index in comment, incremented
	 @param[in] numArgs  expected number of arguments
	 @param[in] comment  doxygen comment to read command arguments from 	*/

void DocumentationScanner::SourceFileParser::collectArguments (Core::Vector<String>& arguments, int& i, const int numArgs, StringRef comment) const
{
	int argIndex = 0;
	while(argIndex < numArgs && i < comment.length ())
	{
		while(Unicode::isWhitespace (comment.at (i)))
			i++;

		// parse argument
		int argStart = i;
		auto isArgChar = [](uchar c) -> bool
		{
			return Unicode::isAlphaNumeric (c) || c == '_' || c == ':';
		};
		while(isArgChar (comment.at (i)))
			i++;

		arguments.add (comment.subString (argStart, i - argStart));
		argIndex++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::isDocumentableElement (const Element& e)
{
	if(e.data.isEmpty ())
		return false;

	return scanner.isDocumentableElementType (e.type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& DocumentationScanner::SourceFileParser::cleanupParsedString (String& str) const
{
	if(str.isEmpty ())
		return str;

	// export with LF over CRLF
	str.replace (String::getLineEnd (Text::kCRLFLineFormat), String::getLineEnd (Text::kLFLineFormat), false);
	return str.trimWhitespace ();
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::getScopedName (MutableCString& scopedName, const Element& element) const
{
	int stackDepth = elementStack.count ();
	if(stackDepth > 0)
	{
		static CStringRef kScope (CSTR ("::"));
		int scopeElements = stackDepth;
		if(&element == &elementStack.last ())
			scopeElements--;
		for(int j = 0; j < scopeElements; j++)
		{
			const Element& scopeElement = elementStack.at (j);
			if(scopeElement.data.isEmpty () == false)
			{
				if(scanner.isScopingElementType (scopeElement.type))
				{
					if(scopedName.isEmpty () == false)
						scopedName.append (kScope);
					scopedName.append (scopeElement.data);
				}
			}
		}
		if(scopedName.isEmpty () == false)
			scopedName.append (kScope);
		scopedName.append (element.data);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::handleDoxyComment (String& comment, DoxyCommentType type)
{
	int stackDepth = elementStack.count ();
	if(stackDepth > 0)
	{
		const Element& related = (type == kLeading || lastPoppedElement.type == SourceCodeElement::kUndefined) ? elementStack.last () : lastPoppedElement;
		if(isDocumentableElement (related))
		{
			AutoPtr<DocuSnippet> snippet = NEW DocuSnippet (related.type);
			if(parseDoxyComment (*snippet, comment))
			{
				snippet->targetName = related.data;
				getScopedName (snippet->scopedName, related);

				scanner.docuSnippets.append (snippet);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter& DocumentationScanner::SourceFileParser::getFilter ()
{
	static AutoPtr<FileTypeFilter> theFilter;
	if(theFilter == nullptr)
	{
		theFilter = NEW FileTypeFilter;
		theFilter->addFileType (FileType (nullptr, "cpp"));
		theFilter->addFileType (FileType (nullptr, "h"));
	}
	return *theFilter;
}



//************************************************************************************************
// GuiDocuScanner::MacroHandler
//************************************************************************************************

struct GuiDocuScanner::MacroHandler
{
	MacroHandler (GuiDocuScanner& scanner, SourceFileParser& parser): scanner (scanner), parser (parser) {}
	bool handleMacros (Token& token);

private:
	GuiDocuScanner& scanner;
	SourceFileParser& parser;

	bool handleSkinElement (SourceFileParser::Element& e, Token& token);
	bool handleSkinElementMembers (SourceFileParser::Element& e, Token& token);
	bool handleSkinEnum (SourceFileParser::Element& e, Token& token);
	bool handleStyleDef (SourceFileParser::Element& e, Token& token);
	bool handleVisualStyle (SourceFileParser::Element& e, Token& token);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleMacros (Token& token)
{
	auto lastElementPtr = parser.getLastNonScopeElement ();
	if(lastElementPtr)
	{
		auto& lastElement = *lastElementPtr;
		if(lastElement.type == kSkinElement) // DEFINE_SKIN_ELEMENT
		{
			return handleSkinElement (lastElement, token);
		}
		else if(lastElement.type == kSkinElementMemberList || lastElement.type == kSkinElementMember) // BEGIN_SKIN_ELEMENT_MEMBERS
		{
			return handleSkinElementMembers (lastElement, token);
		}
		else if(lastElement.type == kSkinEnum) // DEFINE_SKIN_ENUMERATION
		{
			return handleSkinEnum (lastElement, token);
		}
		else if(lastElement.type == kSkinClassDeclaration) // DECLARE_SKIN_ELEMENT_CLASS
		{
			if(token.type == Token::kIdentifier)
			{
				if(lastElement.data.isEmpty ())
					lastElement.data = token.text; // class Name
			}
			else if(token.type == Token::kOperator)
			{
				if(token.text == ")")
					parser.popElement ();
			}

			return true;
		}
		else if(lastElement.type == kStyleDef) // BEGIN_STYLEDEF
		{
			return handleStyleDef (lastElement, token);
		}
		else if(lastElement.type == kVisualStyleClass || lastElement.type == kVisualStyleProperty) // BEGIN_VISUALSTYLE_CLASS
		{
			return handleVisualStyle (lastElement, token);
		}
	}

	if(token.type == Token::kIdentifier)
	{
		if(token.text == "DEFINE_SKIN_ELEMENT" || token.text == "DEFINE_SKIN_ELEMENT_ABSTRACT")
			return parser.pushElement (kSkinElement);
		else if(token.text.startsWith ("BEGIN_SKIN_ELEMENT") && token.text.endsWith ("WITH_MEMBERS")) // include BASE and ABSTRACT
		{
			parser.pushElement (kSkinElementMemberList);
			return parser.pushElement (kSkinElement);
		}
		else if(token.text == "DEFINE_SKIN_ENUMERATION" || token.text == "DEFINE_SKIN_ENUMERATION_PARENT")
			return parser.pushElement (kSkinEnum);
		else if(token.text == "DECLARE_SKIN_ELEMENT_CLASS")
			return parser.pushElement (kSkinClassDeclaration, false);
		else if(token.text == "BEGIN_STYLEDEF")
			return parser.pushElement (kStyleDef);
		else if(token.text == "BEGIN_VISUALSTYLE_CLASS" || token.text == "BEGIN_VISUALSTYLE_BASE")
			return parser.pushElement (kVisualStyleClass, false);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleSkinElement (SourceFileParser::Element& element, Token& token)
{
	static AutoPtr<SkinElement> currentSkinElement;

	// DEFINE_SKIN_ELEMENT
	// example: DEFINE_SKIN_ELEMENT (RowElement, VerticalElement, TAG_ROW, DOC_GROUP_LAYOUT, 0)

	auto getPartString = [&] () -> MutableCString*
	{
		if(currentSkinElement != nullptr)
		{
			switch(element.scanningPart)
			{
			case 0 : return &currentSkinElement->elementClass;
			case 2 : return &currentSkinElement->tagName;
			case 3 : return &currentSkinElement->groupName;
			case 4 : return &currentSkinElement->relatedClass;
			}
		}
		return nullptr;
	};

	if(token.type == Token::kOperator)
	{
		if(token.text == "(")
		{
			if(currentSkinElement == nullptr && element.scanningPart == 0)
				currentSkinElement = NEW SkinElement;
		}
		else if(token.text == ",")
		{
			element.scanningPart++;
		}
		else if(token.text == ")")
		{
			if(currentSkinElement && currentSkinElement->tagName.isEmpty () == false)
				scanner.skinElements.append (currentSkinElement);

			currentSkinElement = nullptr;
			parser.popElement ();
		}
		else if(token.text == "::" )
		{
			if(auto string = getPartString ())
				string->append (token.text);
		}
	}
	else if(token.type == Token::kIdentifier || token.type == Token::kString)
	{
		if(auto string = getPartString ())
			string->append (token.text);
	}
	return true;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleSkinElementMembers (SourceFileParser::Element& element, Token& token)
{
	static SkinElement* targetSkinElement = nullptr;
	static AutoPtr<SkinElement::Member> currentMember;

	if(token.type == Token::kOperator && token.text == ",")
	{
		element.scanningPart++;
		return true;
	}

	if(element.type == kSkinElementMemberList)
	{
		if(token.type == Token::kIdentifier)
		{
			if(token.text == "END_SKIN_ELEMENT_WITH_MEMBERS")
			{
				targetSkinElement = nullptr;
				parser.popElement ();
			}
			else if(element.scanningPart == 0 && targetSkinElement == nullptr)
			{
				AutoPtr <SkinElement> scannedElement = scanner.skinElements.getLast ();
				targetSkinElement = scannedElement;
				element.data = scannedElement->elementClass; // needed for scope
			}

			if(targetSkinElement && (token.text == "ADD_SKIN_ELEMENT_MEMBER"))
				parser.pushElement (kSkinElementMember, false);
		}

		return true;
	}
	else if(element.type == kSkinElementMember)
	{
		if(token.type == Token::kOperator)
		{
			if(token.text == "(")
			{
				if(currentMember == nullptr && element.scanningPart == 0)
					currentMember = NEW SkinElement::Member;
			}
			else if(token.text == ")")
			{
				if(currentMember)
				{
					if(targetSkinElement && currentMember->tagName.isEmpty () == false)
						targetSkinElement->members.append (currentMember);

					element.data = currentMember->tagName;
				}
				currentMember = nullptr;
				parser.popElement ();
			}
		}
		else if(token.type == Token::kIdentifier || token.type == Token::kString)
		{
			if(currentMember)
			{
				if(element.scanningPart == 0)
					currentMember->tagName = token.text;
				else if(element.scanningPart == 1)
					currentMember->typeName = token.text;
			}
		}
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleSkinEnum (SourceFileParser::Element& element, Token& token)
{
	static AutoPtr<SkinEnum> currentSkinEnum;

	auto getPartString = [&] () -> MutableCString&
	{
		if(currentSkinEnum != nullptr)
		{
			switch(element.scanningPart)
			{
			case 0 : return currentSkinEnum->skinClassName;
			case 1 : return currentSkinEnum->enumName;
			case 2 : return currentSkinEnum->cppStyleDefName;
			case 3 : return currentSkinEnum->parentSkinClassName; // DEFINE_SKIN_ENUMERATION_PARENT only
			case 4 : return currentSkinEnum->parentEnumName; // DEFINE_SKIN_ENUMERATION_PARENT only
			}
		}
		ASSERT (0)
		static MutableCString error;
		return error.empty ();
	};

	if(token.type == Token::kIdentifier || token.type == Token::kString)
	{
		getPartString ().append (token.text);
	}
	else if(token.type == Token::kOperator)
	{
		if(token.text == "(")
		{
			if(currentSkinEnum == nullptr && element.scanningPart == 0)
				currentSkinEnum = NEW SkinEnum;
		}
		else if(token.text == ",")
		{
			element.scanningPart++;
		}
		else if(token.text == ")")
		{
			if(currentSkinEnum)
				scanner.skinEnums.append (currentSkinEnum);
			currentSkinEnum = nullptr;
			parser.popElement ();
		}
		else if(token.text == "::" )
			getPartString ().append (token.text);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleStyleDef (SourceFileParser::Element& element, Token& token)
{
	static AutoPtr<OptionList> currentStyleDef;
	static AutoPtr<OptionList::Item> currentStyleDefItem;

	auto getPartString = [&] () -> MutableCString*
	{
		if(element.scanningPart == 0)
		{
			if(currentStyleDef != nullptr)
				return &currentStyleDef->name;
		}
		else if(currentStyleDefItem != nullptr)
		{
			switch(element.scanningPart)
			{
			case 1 : return &currentStyleDefItem->skinName;
			case 2 : return &currentStyleDefItem->cppName;
			}
		}
		return nullptr;
	};

	if(token.type == Token::kIdentifier || token.type == Token::kString)
	{
		if(token.type == Token::kIdentifier && token.text == "END_STYLEDEF")
		{
			if(currentStyleDef && currentStyleDef->items.isEmpty () == false)
				scanner.optionLists.append (currentStyleDef);
			currentStyleDef = nullptr;
			currentStyleDefItem = nullptr;

			parser.popElement ();
		}
		else if(auto string = getPartString ())
			string->append (token.text);
	}
	else if(token.type == Token::kOperator)
	{
		if(token.text == "(")
		{
			if(currentStyleDef == nullptr && element.scanningPart == 0)
				currentStyleDef = NEW OptionList;
		}
		else if(token.text == ")")
		{
			element.scanningPart++;
		}
		else if(token.text == "{")
		{
			currentStyleDefItem = NEW OptionList::Item;
			element.scanningPart = 1;
		}
		else if(token.text == ",")
		{
			if(currentStyleDefItem)
				element.scanningPart++;
		}
		else if(token.text == "}")
		{
			if(currentStyleDefItem && currentStyleDef)
				currentStyleDef->items.append (currentStyleDefItem);

			currentStyleDefItem = nullptr;
		}
		else if(token.text == "::" )
		{
			if(auto string = getPartString ())
				string->append (token.text);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::MacroHandler::handleVisualStyle (SourceFileParser::Element& element, Token& token)
{
	static AutoPtr<VisualStyle> currentStyle;
	static AutoPtr<VisualStyle::Property> currentStyleProperty;
/*
	BEGIN_VISUALSTYLE_CLASS (Button, VisualStyle, "Button")
		ADD_VISUALSTYLE_IMAGE  ("left") ///< docu for the beast
		ADD_VISUALSTYLE_IMAGE  ("right")
		ADD_VISUALSTYLE_COLOR  ("textcolor.on")
		ADD_VISUALSTYLE_COLOR  ("backcolor.on")
		ADD_VISUALSTYLE_METRIC ("padding")
		ADD_VISUALSTYLE_METRIC ("colorize.disabled")
	END_VISUALSTYLE_CLASS (Button)
*/
	auto getPartString = [&] (bool isString) -> MutableCString*
	{
		if(element.type == kVisualStyleClass)
		{
			if(currentStyle != nullptr)
			{
				if(isString)
					return &currentStyle->skinTag;
				else if(element.scanningPart == 0)
					return &currentStyle->name;
			}
		}
		else if(element.type == kVisualStyleProperty)
		{
			if(currentStyleProperty != nullptr && element.scanningPart == 0)
				return &currentStyleProperty->name;
		}

		return nullptr;
	};

	auto pushProperty = [&] (CStringPtr type)
	{
		parser.flushRecentDoxyComment ();

		currentStyleProperty = NEW VisualStyle::Property (type);
		parser.pushElement (kVisualStyleProperty);
	};

	if(token.type == Token::kIdentifier)
	{
		if(token.text == "END_VISUALSTYLE_CLASS")
		{
			if(currentStyle && currentStyle->name.isEmpty () == false)
				scanner.visualStyles.append (currentStyle);

			currentStyle = nullptr;
			currentStyleProperty = nullptr;

			parser.popElement ();
		}
		else if(token.text == "ADD_VISUALSTYLE_COLOR")
			pushProperty ("color");
		else if(token.text == "ADD_VISUALSTYLE_METRIC")
			pushProperty ("metric");
		else if(token.text == "ADD_VISUALSTYLE_FONT")
			pushProperty ("font");
		else if(token.text == "ADD_VISUALSTYLE_ALIGN")
			pushProperty ("align");
		else if(token.text == "ADD_VISUALSTYLE_OPTIONS")
			pushProperty ("enum");
		else if(token.text == "ADD_VISUALSTYLE_IMAGE")
			pushProperty ("image");

		else
		{
			if(auto string = getPartString (false))
				string->append (token.text);
		}
	}
	else if(token.type == Token::kString)
	{
		if(auto string = getPartString (true))
			string->append (token.text);
	}
	else if(token.type == Token::kOperator)
	{
		if(token.text == "(")
		{
			if(currentStyle == nullptr && element.scanningPart == 0)
				currentStyle = NEW VisualStyle;
		}
		else if(token.text == ",")
		{
			if(element.scanningPart == 0 && currentStyle)
				element.data = currentStyle->name;

			element.scanningPart++;
		}
		else if(token.text == ")")
		{
			if(currentStyleProperty)
			{
				if(currentStyle && currentStyleProperty->name.isEmpty () == false)
				{
					element.data = currentStyleProperty->name;
					currentStyle->properties.append (currentStyleProperty);
				}
				currentStyleProperty = nullptr;
				parser.popElement ();
			}
		}
	}

	return true;
}

//************************************************************************************************
// GuiDocuScanner
//************************************************************************************************

bool GuiDocuScanner::isSkinClassModel (Model::ClassRepository& repository)
{
	return repository.getTitle ().contains ("skin", false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isVisualStyleClassModel (Model::ClassRepository& repository)
{
	return repository.getTitle ().contains ("visual", false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID GuiDocuScanner::kSkinElement = CSTR ("skinElement");
StringID GuiDocuScanner::kSkinElementMemberList = CSTR ("skinElementMemberList");
StringID GuiDocuScanner::kSkinElementMember = CSTR ("skinElementMember");
StringID GuiDocuScanner::kSkinEnum = CSTR ("skinEnum");
StringID GuiDocuScanner::kSkinClassDeclaration = CSTR ("skinClass");

StringID GuiDocuScanner::kStyleDef = CSTR ("styleDef");
StringID GuiDocuScanner::kVisualStyleClass = CSTR ("visualStyle");
StringID GuiDocuScanner::kVisualStyleProperty = CSTR ("visualStyleProperty");

//////////////////////////////////////////////////////////////////////////////////////////////////

GuiDocuScanner::GuiDocuScanner ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isSkinSnippet (DocuSnippet* snippet) const
{
	if(snippet->elementType == kVisualStyleClass
		|| snippet->elementType == kVisualStyleProperty)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GuiDocuScanner::postScan ()
{
	SuperClass::postScan ();

	for(auto& define : defines)
	{
		ListForEach (skinElements, AutoPtr <SkinElement>, skinElement)
			define->resolve (skinElement->tagName);
			define->resolve (skinElement->groupName);
			ListForEach (skinElement->members, AutoPtr <SkinElement::Member>, member)
				define->resolve (member->tagName);
				define->resolve (member->typeName);
			EndFor
		EndFor
		ListForEach (skinEnums, AutoPtr <SkinEnum>, skinEnum)
			define->resolve (skinEnum->skinClassName);
			define->resolve (skinEnum->enumName);
			define->resolve (skinEnum->cppStyleDefName);
			define->resolve (skinEnum->parentSkinClassName);
			define->resolve (skinEnum->parentEnumName);
		EndFor
	}

	// ---------------------------------------------------------------
	// connect styledefs with docu for corresponding enum values
	// + connect with corresponding skin enum
	// ---------------------------------------------------------------
	ListForEach (optionLists, AutoPtr <OptionList>, optionList)

		ListForEach (optionList->items, AutoPtr<OptionList::Item>, item)
			
			for(auto& constant : enumConstants)
			{	
				if(constant->docu.isValid ())
				{
					if(scopedCompare (constant->scopedName, item->cppName) > 0)
					{				
						item->docu = constant->docu;
						break;
					}
				}
			}
			if(item->docu.isValid () == false)
			{
				ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
					if(docuSnippet->elementType == SourceCodeElement::kEnumValue)
					{
						if(docuSnippet->scopedCompare (item->cppName) > 0)
						{
							item->docu = docuSnippet;
							break;
						}
					}
				EndFor
			}
		EndFor

		// find corresponding skin element
		ListForEach (skinEnums, AutoPtr<SkinEnum>, skinEnum)
			if(skinEnum->cppStyleDefName == optionList->name)
			{
				skinEnum->optionList = optionList;
			}
		EndFor
	EndFor

	// ---------------------------------------------------------------
	// find docu for skin elements. if the element class is documented itself, use it.
	// else if the related ccl class has docu, use it
	// ---------------------------------------------------------------
	LinkedList<DocuSnippet*> skinLinkedSnippetsWithLinks;

	ListForEach (skinElements, AutoPtr<SkinElement>, skinElement)
		DocuSnippet* snippet = nullptr;
		// docu for skin element itself wins
		ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
			if(isSkinSnippet (docuSnippet) && skinElement->elementClass.isEmpty () == false)
			{
				if(docuSnippet->targetName == skinElement->elementClass)
				{
					snippet = docuSnippet;
					break;
				}
			}
		EndFor
		if(snippet == nullptr)
		{
			ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
				if(isSkinSnippet (docuSnippet) && docuSnippet->scopedName.startsWith ("CCL") && docuSnippet->scopeCount () ==  1)
				{
					if(skinElement->relatedClass.isEmpty () == false)
					{
						if(docuSnippet->targetName == skinElement->relatedClass)
						{
							snippet = docuSnippet;
							break;
						}
					}
				}
			EndFor
		}

		if(snippet)
		{
			skinElement->docu = snippet;
			if(snippet->links.isEmpty () == false)
				skinLinkedSnippetsWithLinks.append (snippet);
		}

		// connect docu for memebers
		ListForEach (skinElement->members, AutoPtr<SkinElement::Member>, member)
			MutableCString scopedMember (skinElement->elementClass);
			scopedMember.append ("::");
			scopedMember.append (member->tagName);

			ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
				if(docuSnippet->elementType == kSkinElementMember)
				{
					if(docuSnippet->scopedCompare (scopedMember) > 1)
					{
						member->docu = docuSnippet;
						break;
					}
				}
			EndFor
		EndFor
	EndFor

	// ---------------------------------------------------------------
	// replace links to classes with links to skin tags
	// ---------------------------------------------------------------

	ListForEach (skinLinkedSnippetsWithLinks, DocuSnippet*, docuSnippet)
		for(int i = 0; i < docuSnippet->links.count (); i++)
		{
			String& link = docuSnippet->links.at (i);
			ListForEach (skinElements, AutoPtr<SkinElement>, skinElement)
				bool matches = false;
				if(skinElement->elementClass.isEmpty () == false)
				{
					if(link == String (skinElement->elementClass))
						matches = true;
				}
				if(matches == false)
				{
					if(skinElement->relatedClass.isEmpty () == false)
					{
						if(link == String (skinElement->relatedClass))
							matches = true;
					}
				}
				if(matches)
				{
					link = String (skinElement->tagName);
					break;
				}
			EndFor
		}
	EndFor

	// ---------------------------------------------------------------
	// find docu for visual style elements + create links
	// ---------------------------------------------------------------

	ListForEach (visualStyles, AutoPtr<VisualStyle>, visualStyle)
		ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
			if(docuSnippet->elementType == kVisualStyleClass)
			{
				if(docuSnippet->targetName == visualStyle->name)
				{
					visualStyle->docu = docuSnippet;
					break;
				}
			}
		EndFor

		ListForEach (visualStyle->properties, AutoPtr<VisualStyle::Property>, prop)
			MutableCString scopedMember (visualStyle->name);
			scopedMember.append ("::");
			scopedMember.append (prop->name);

			ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
				if(docuSnippet->elementType == kVisualStyleProperty)
				{
					if(docuSnippet->scopedCompare (scopedMember) > 1)
					{
						prop->docu = docuSnippet;
						break;
					}
				}
			EndFor
		EndFor

		// lookup corresponding skin element and create link and back link
		ListForEach (skinElements, AutoPtr<SkinElement>, skinElement)
			if(skinElement->tagName == visualStyle->name)
			{
				if(skinElement->docu == nullptr)
					skinElement->docu = AutoPtr<DocuSnippet> (NEW DocuSnippet (kSkinElement));
				String styleLink (visualStyle->skinTag);
				if(skinElement->docu->links.contains (styleLink) == false)
					skinElement->docu->links.add (styleLink);

				if(visualStyle->docu == nullptr)
					visualStyle->docu = AutoPtr<DocuSnippet> (NEW DocuSnippet (kVisualStyleClass));
				String skinLink (skinElement->tagName);
				if(visualStyle->docu->links.contains (skinLink) == false)
					visualStyle->docu->links.add (skinLink);

				break;
			}
		EndFor
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::applyToModel (Model::ClassRepository& repository)
{
	if(isSkinClassModel (repository))
		return applyToSkinModel (repository);

	else if(isVisualStyleClassModel (repository))
		return applyToVisualStyleModel (repository);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isMatchingFolder (UrlRef folder)
{
	static StringRef kGui = CCLSTR ("/gui"); // ccl/gui, ccl/public/gui, core/gui, core/public/gui
	static StringRef kMeta = CCLSTR ("/ccl/meta");
	return folder.getPath ().contains (kGui, false) || folder.getPath ().contains (kMeta, false);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isDocumentableElementType (SourceCodeElement::TypeRef type) const
{
	return SuperClass::isDocumentableElementType (type)
		|| type == kSkinClassDeclaration
		|| type == kSkinElementMember
		|| type == kVisualStyleClass
		|| type == kVisualStyleProperty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isScopingElementType (SourceCodeElement::TypeRef type) const
{
	return SuperClass::isScopingElementType (type)
		|| type == kSkinElementMemberList
		|| type == kVisualStyleClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::handleMacros (SourceFileParser& parser, Token& token)
{
	return MacroHandler (*this, parser).handleMacros (token);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::applyToSkinModel (Model::ClassRepository& repository)
{
	bool anyChangesAtAll = false;

	// classes
	ForEach (repository.getClasses (), Model::Class, modelClass)
		ListForEach (skinElements, AutoPtr <SkinElement>, skinElement)
			if(skinElement->tagName == modelClass->getName ())
			{
				if(applyDocu (*modelClass, skinElement->docu))
					anyChangesAtAll = true;

				// members
				ObjectArray members;
				modelClass->getMembers (members, false);
				ForEach (members, Model::Member, modelMember)

					ListForEach (skinElement->members, AutoPtr <SkinElement::Member>, skinMember)
						if(skinMember->tagName == modelMember->getName ())
						{
							if(applyDocu (*modelMember, skinMember->docu))
								anyChangesAtAll = true;

							break;
						}
					EndFor
				EndFor

				break;
			}
		EndFor
	EndFor

	// enumeration values
	ForEach (repository.getEnumerations (), Model::Enumeration, enumeration)
		OptionList* optionList = nullptr;
		ListForEach (skinEnums, AutoPtr <SkinEnum>, skinEnum)
			if(skinEnum->optionList)
			{
				MutableCString enumName = MutableCString (skinEnum->skinClassName).append (".").append (skinEnum->enumName);
				if(enumName == enumeration->getName ())
				{
					optionList = skinEnum->optionList;
					break;
				}
			}
		EndFor

		if(optionList)
		{
			bool anyChanges = false;
			ObjectArray enumerators;
			enumeration->getEnumerators (enumerators, true);
			ForEach (enumerators, Model::Enumerator, enumerator)
				ListForEach (optionList->items, AutoPtr<OptionList::Item>, item)
					if(item->docu && item->skinName == enumerator->getName ())
					{
						if(applyDocu (*enumerator, item->docu))
							anyChanges = true;
					}
				EndFor
			EndFor
			if(anyChanges)
			{
				enumeration->deferChanged ();
				anyChangesAtAll = true;
			}
		}
	EndFor

	if(anyChangesAtAll)
		repository.signal (Message (kPropertyChanged)); // not sure what this is good for...

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::applyToVisualStyleModel (Model::ClassRepository& repository)
{
	bool anyChangesAtAll = false;

	ForEach (repository.getClasses (), Model::Class, modelClass)
		ListForEach (visualStyles, AutoPtr <VisualStyle>, visualStyle)
			if(visualStyle->skinTag == modelClass->getName ())
			{
				if(applyDocu (*modelClass, visualStyle->docu))
					anyChangesAtAll = true;

				// sub items
				bool anyChanges = false;
				ObjectArray members;
				modelClass->getMembers (members, false);
				ForEach (members, Model::Member, member)
					ListForEach (visualStyle->properties, AutoPtr<VisualStyle::Property>, prop)
						if(prop->name == member->getName ())
						{
							if(applyDocu (*member, prop->docu))
								anyChanges = true;
							break;
						}
					EndFor
				EndFor
				if(anyChanges)
				{
					modelClass->deferChanged ();
					anyChangesAtAll = true;
				}
				break;
			}
		EndFor
	EndFor

	if(anyChangesAtAll)
		repository.signal (Message (kPropertyChanged));

	return true;
}
