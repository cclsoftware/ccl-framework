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
// Filename    : docscanner.cpp
// Description : Documentation Scanner
//
//************************************************************************************************

#include "docscanner.h"
#include "ccl/extras/modeling/classrepository.h"

#include "ccl/base/storage/textfile.h"
#include "ccl/base/message.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/base/iprogress.h"

using namespace CCL;
using namespace Model;

//************************************************************************************************
// CCL_KERNEL_INIT
//************************************************************************************************

CCL_KERNEL_INIT (DocumentationScanner)
{
	DocumentationScanner::registerDefaultScannerType (
		[] () -> DocumentationScanner* { return NEW TypeLibDocuScanner; }
	);
	
	DocumentationScanner::registerScannerType ("skin",
		[] () -> DocumentationScanner* { return NEW GuiDocuScanner; }
	);
	
	DocumentationScanner::registerScannerType ("visual",
		[] () -> DocumentationScanner* { return NEW GuiDocuScanner; }
	);

	return true;
}

//************************************************************************************************
// DocumentationScanner
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kUndefinedElementType, "")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kNamespace, "namespace")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kClass, "class")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kEnum, "enum")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kEnumValue, "enumValue")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kScope, "scope")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kOpaqueScope, "opaqueScope")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kTemplate, "template")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kTemplateArg, "templateArg")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kEnumInfo, "enumInfo")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kEnumInfoValue, "enumInfoValue")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kClassMethodList, "methodList")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kClassMethod, "classMethod")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kClassPropertyList, "propertyList")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kClassProperty, "classProperty")
DEFINE_STRINGID_MEMBER_ (DocumentationScanner, kConstant, "const")

//////////////////////////////////////////////////////////////////////////////////////////////////

LinkedList<DocumentationScanner::ScannerType> DocumentationScanner::scannerTypes;
DocumentationScanner::CreateScannerFunc DocumentationScanner::defaultCreateFunc = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::DocumentationScanner ()
: currentScanProgress (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::registerScannerType (StringRef modelTitlePart, CreateScannerFunc createFunc)
{
	for(ScannerType& scannerType : scannerTypes)
		if(scannerType.modelTitlePart == modelTitlePart)
			return;

	scannerTypes.append ({modelTitlePart, createFunc});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::registerDefaultScannerType (CreateScannerFunc createFunc)
{
	ASSERT (defaultCreateFunc == nullptr)
	if(defaultCreateFunc != nullptr)
		return;

	defaultCreateFunc = createFunc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner* DocumentationScanner::createScannerForModel (ClassRepository& repository)
{
	for(ScannerType& scannerType : scannerTypes)
		if(repository.getTitle ().contains (scannerType.modelTitlePart, false))
			return scannerType.createFunc ();

	return defaultCreateFunc ? defaultCreateFunc () : nullptr;
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

void DocumentationScanner::applyToModel (ClassRepository& repository)
{
	bool anyChanges = false;

	ForEach (repository.getClasses (), Class, modelClass)

		if(applyMethods (*modelClass))
			anyChanges = true;

		if(applyProperties (*modelClass))
			anyChanges = true;

		DocuSnippet* matchingDocu = nullptr;
		int matchLevel = 0;

		// find matching docu for a class
		for(AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
		{
			if(docuSnippet->elementType == kClass)
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
			anyChanges = true;
	EndFor

	if(anyChanges)
		repository.signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isMatchingFolder (UrlRef folder)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::scanCode (const Vector<Url>& paths, IProgressNotify* progress)
{
	// All paths (folders or single files) are scanned as one source tree: every path's pre-pass
	// runs before any path gets parsed, and defines are resolved across all of them in postScan().

	ProgressNotifyScope progressScope (progress);

	currentScanProgress = progress;
	bool scanned = true;
	for(UrlRef path : paths)
		if(!(scanned = beforeScan (path, progress)))
			break;

	if(scanned)
		for(UrlRef path : paths)
			if(!(scanned = SourceFileTraverser::traverse (path, SourceFileParser::getFilter (), *this, progress)))
				break;
	currentScanProgress = nullptr;

	if(scanned)
	{
		postScan ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::visitFile (UrlRef path)
{
	SourceFileParser parser (*this);
	parser.parseFile (path, currentScanProgress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::beforeScan (UrlRef folder, IProgressNotify* progress)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::postScan ()
{
	// resolve defines
	for(Define& define : defines)
	{
		for(AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
		{
			if(define.resolve (docuSnippet->targetName))
				define.resolveParts (docuSnippet->scopedName);
		}

		for(AutoPtr<EnumInfo>& enumInfo : enumsInfos)
		{
			define.resolve (enumInfo->name);
			for(EnumInfo::ScannedEnumerator& enumInfoItem : enumInfo->items)
				define.resolve (enumInfoItem.name);
		}

		// member lists are not expected to use defines...
	}

	// link docu for enum using constants
	for(EnumValueConstant& constant : enumConstants)
	{
		constant.docu = findDocuByTargetName (kConstant, constant.constantName);

		// the constant can itself be an enum value documented elsewhere, e.g. IParameter::kInteger = kParamTypeInteger
		if(constant.docu == nullptr)
		{
			for(const AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
				if(docuSnippet->elementType == kEnumValue && docuSnippet->scopedCompare (constant.constantName) > 0)
				{
					constant.docu = docuSnippet;
					break;
				}
		}
	}

	// connect docu for enum infos
	for(AutoPtr<EnumInfo>& enumInfo : enumsInfos)
	{
		enumInfo->docu = findDocuByTargetName (kEnumInfo, enumInfo->name);

		for(EnumInfo::ScannedEnumerator& enumInfoItem : enumInfo->items)
		{
			MutableCString scopedMember (enumInfo->name);
			scopedMember.append ("::");
			scopedMember.append (enumInfoItem.name);

			enumInfoItem.docu = findScopedDocu (kEnumInfoValue, scopedMember);
		}
	}

	// connect docu for class methods
	for(AutoPtr<ClassMethodList>& classMethods : classMethodLists)
	{
		for(ClassMethodList::ScannedMethod& method : classMethods->methods)
		{
			MutableCString scopedMember (classMethods->className);
			scopedMember.append ("::");
			scopedMember.append (method.name);

			method.docu = findScopedDocu (kClassMethod, scopedMember);
		}
	}

	// connect docu for class properties
	for(AutoPtr<ClassPropertyList>& classProperties : classPropertyLists)
	{
		for(ClassPropertyList::ScannedProperty& prop : classProperties->properties)
		{
			MutableCString scopedMember (classProperties->className);
			scopedMember.append ("::");
			scopedMember.append (prop.name);

			prop.docu = findScopedDocu (kClassProperty, scopedMember);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isDocumentableElementType (CStringRef type) const
{
	return type == kClass
		|| type == kEnumValue
		|| type == kEnumInfo
		|| type == kEnumInfoValue
		|| type == kClassMethod
		|| type == kClassProperty
		|| type == kConstant;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::isScopingElementType (CStringRef type) const
{
	// SourceElement type is a parent of a documentable element and should be used as scope.

	return type == kClass
		|| type == kNamespace
		|| type == kEnumInfo
		|| type == kClassMethodList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyDocu (Element& target, DocuSnippet* docu) const
{
	if(docu)
	{
		bool anyChanges = false;

		if(docu->linksOnly)
		{
			// No real comment behind this snippet - only merge its link(s) into whatever links the
			// target already has, rather than treating its (empty) brief/detailed/code as the new
			// truth and clearing content that came from an unrelated source.

			Documentation::LinkList mergedLinks = target.getDocumentation ().getLinks ();
			for(const String& link : docu->links)
				if(mergedLinks.contains (link) == false)
					mergedLinks.add (link);

			if(mergedLinks.isEqual (target.getDocumentation ().getLinks ()) == false)
			{
				target.getDocumentation ().setLinks (mergedLinks);
				anyChanges = true;
			}
		}
		else
		{
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
		}

		if(anyChanges)
		{
			if(ccl_cast<MainElement> (&target) != nullptr)
				target.deferChanged ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyMethods (Class& modelClass) const
{
	bool anyChanges = false;

	ClassMethodList* methodList = nullptr;
	for(AutoPtr<ClassMethodList>& classMethods : classMethodLists)
	{
		if(classMethods->className == modelClass.getName ())
		{
			methodList = classMethods;
			break;
		}
	}

	if(methodList)
	{
		for(ClassMethodList::ScannedMethod& scannedMethod : methodList->methods)
		{
			Method* modelMethod = modelClass.findMethod (scannedMethod.name);

			if(modelMethod == nullptr)
			{
				modelMethod = NEW Method (scannedMethod.name);
				modelClass.addMethod (modelMethod);
				anyChanges = true;
			}

			if(scannedMethod.args.isEmpty () == false)
				if(modelMethod->getArguments ().isEmpty ())
				{
					modelMethod->addArgument (NEW MethodArgument (scannedMethod.args)); // all in one...
					anyChanges = true;
				}

			if(scannedMethod.returnValue.isEmpty () == false)
			{
				if(modelMethod->getReturnValue ().getName () != scannedMethod.returnValue)
				{
					modelMethod->getReturnValue ().setName (scannedMethod.returnValue);
					anyChanges = true;
				}
			}

			if(scannedMethod.docu.isValid ())
			{
				if(applyDocu (*modelMethod, scannedMethod.docu))
					anyChanges = true;
			}
		}
	}

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::applyProperties (Class& modelClass) const
{
	bool anyChanges = false;

	ClassPropertyList* propertyList = nullptr;
	for(AutoPtr<ClassPropertyList>& classProps : classPropertyLists)
	{
		if(classProps->className == modelClass.getName ())
		{
			propertyList = classProps;
			break;
		}
	}
	if(propertyList)
	{
		for(ClassPropertyList::ScannedProperty& scannedProp : propertyList->properties)
		{
			if(scannedProp.docu.isValid ())
			{
				Member* modelMember = modelClass.findMember (scannedProp.name);
				if(modelMember)
				{
					if(applyDocu (*modelMember, scannedProp.docu))
						anyChanges = true;
				}
			}
		}
	}

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::DocuSnippet* DocumentationScanner::findDocuByTargetName (CStringRef elementType, CStringRef targetName) const
{
	// First docuSnippet with the given elementType and an exact targetName match, or nullptr.

	for(const AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
		if(docuSnippet->elementType == elementType && docuSnippet->targetName == targetName)
			return docuSnippet;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::DocuSnippet* DocumentationScanner::findScopedDocu (CStringRef elementType, CStringRef scopedName) const
{
	// First docuSnippet with the given elementType whose scope matches scopedName, or nullptr.

	for(const AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
		if(docuSnippet->elementType == elementType && docuSnippet->scopedCompare (scopedName) > 1)
			return docuSnippet;
	return nullptr;
}

//************************************************************************************************
// DocumentationScanner::DocuSnippet
//************************************************************************************************

int DocumentationScanner::DocuSnippet::scopedCompare (CStringRef partScope) const
{
	return DocumentationScanner::scopedCompare (scopedName, partScope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentationScanner::DocuSnippet::scopeCount () const
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
// DocumentationScanner::Define
//************************************************************************************************

bool DocumentationScanner::Define::resolve (MutableCString& str) const
{
	if(name == str)
	{
		str = value;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::Define::resolveParts (MutableCString& str) const
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
// DocumentationScanner::DoxyCommentParser
//************************************************************************************************

bool DocumentationScanner::DoxyCommentParser::parse (DocuSnippet& target, StringRef comment)
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

	bool insideQuotes = false;

	int len = comment.length ();
	String result;
	for(int i = 0; i < len; ++i)
	{
		uchar c = comment.at (i);
		static const Vector<uchar> controlChars = { '\\', '@' };

		if(controlChars.contains (c) && !insideQuotes)
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
				parseCommand (i, target, comment);
				// reverse iteration by one to first character after last command (which can be a '\n'),
				// this character would otherwise be skipped by the for loop increment of i
				i--;
			}
		}
		else if(c == '.' && !insideQuotes)
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
		else if(c == '"')
		{
			insideQuotes = !insideQuotes;
			appendChar (result, c);
		}
		else
			appendChar (result, c);
	}

	// syntax error in comment: uneven number of quote characters
	ASSERT (insideQuotes == false)

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

int DocumentationScanner::DoxyCommentParser::getCommandArgCount (StringRef command)
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

void DocumentationScanner::DoxyCommentParser::parseCommand (int& i, DocuSnippet& target, StringRef comment)
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
	int numArgs = getCommandArgCount (command);

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

void DocumentationScanner::DoxyCommentParser::collectArguments (Core::Vector<String>& arguments, int& i, const int numArgs, StringRef comment)
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

String& DocumentationScanner::DoxyCommentParser::cleanupParsedString (String& str)
{
	if(str.isEmpty ())
		return str;

	// export with LF over CRLF
	str.replace (String::getLineEnd (Text::kCRLFLineFormat), String::getLineEnd (Text::kLFLineFormat), false);
	return str.trimWhitespace ();
}

// DocumentationScanner::Parser
//************************************************************************************************


DocumentationScanner::SourceFileParser::SourceFileParser (DocumentationScanner& _scanner)
: scanner (_scanner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter& DocumentationScanner::SourceFileParser::getFilter ()
{
	return SourceFileTraverser::cppSourceFilter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseFile (UrlRef file, IProgressNotify* progress)
{
	AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (file);
	String rawUnicode;
	if(fileStream)
	{
		// Copy the whole file into memory at once, for performance reasons: TextStreamer reads one
		// character at a time, and without this, each of those turns into its own ReadFile() call
		// all the way down.
		AutoPtr<IMemoryStream> inMemory = NEW MemoryStream;
		if(System::GetFileUtilities ().copyStream (*inMemory, *fileStream))
		{
			inMemory->seek (0, IStream::kSeekSet);
			rawUnicode = TextUtils::loadRawString (*inMemory);
		}
	}
	MutableCString raw (rawUnicode, Text::kASCII);

	this->file = file.getPath ();

	SourceLexer lexer (raw);
	SourceToken token;
	parse (lexer, token);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::SourceFileParser::SourceElement* DocumentationScanner::SourceFileParser::getLastNonScopeElement () const
{
	for(int i = elementStack.count () - 1; i >= 0; i--)
	{
		if(elementStack [i].type != kScope)
			return &elementStack [i];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::pushElement (const SourceElement& e, bool handleRecentDoxyComment)
{
	elementStack.add (e);

	lastPoppedElement = SourceElement ();
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

void DocumentationScanner::SourceFileParser::flushRecentDoxyComment ()
{
	if(recentLeadingDoxyComment.isEmpty () == false)
	{
		handleDoxyComment (recentLeadingDoxyComment, kLeading);
		recentLeadingDoxyComment.empty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::nextToken (SourceLexer& reader, SourceToken& token)
{
	// Pulls the next meaningful token, transparently routing any comment/preprocessor tokens along
	// the way to handleCommentToken()/handlePreprocessorToken() - so any on-demand caller (e.g.
	// handleElementType()) sees the same comment handling as the main parse() loop.

	while(reader.next (token))
	{
		if(token.type == SourceToken::kComment)
		{
			handleCommentToken (token);
			continue;
		}
		if(token.type == SourceToken::kPreprocessor)
		{
			handlePreprocessorToken (token);
			continue;
		}
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::parse (SourceLexer& reader, SourceToken& token)
{
	while(nextToken (reader, token))
		if(handleElement (reader, token) == false)
			return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::handleCommentToken (const SourceToken& token)
{
	// token.text is the comment's raw text, delimiters included (see SourceToken::kComment) - so
	// classification happens here, not in the lexer. Same rules as before: a line comment is doxy
	// only when it reads '///<' (trailing), a block comment is doxy when it reads '/**' (leading) -
	// a bare '///' or '/*' is plain, not doxy.

	CStringPtr raw = token.text.str ();
	int len = token.text.length ();

	if(len >= 2 && raw[1] == '/') // line comment
	{
		if(len >= 4 && raw[2] == '/' && raw[3] == '<')
		{
			String doxyComment (token.text.subString (4, len - 4));
			handleDoxyComment (doxyComment, kTrailing);
		}
		return;
	}

	// Block comment. Doxy iff the character right after "/*" is another '*' (e.g. "/** ...").
	if(len >= 3 && raw[2] == '*')
	{
		int contentEnd = len;
		if(len >= 4 && raw[len - 2] == '*' && raw[len - 1] == '/')
			contentEnd = len - 2; // well-terminated: strip the trailing "*/"

		String doxyComment (token.text.subString (3, contentEnd - 3));
		doxyComment.trimWhitespace ();
		if(doxyComment.isEmpty () == false)
			recentLeadingDoxyComment = doxyComment;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::handlePreprocessorToken (const SourceToken& token) const
{
	// #define extraction now goes through MacroCallScanner::extractDefine() - the same, more careful
	// extraction ClassScanner uses (function-like-macro exclusion, word boundary), rather than this
	// scanner's previous separate, weaker version (which required a quoted value and used a plain
	// whitespace tokenizer). A genuine upgrade, not just a reimplementation: e.g. a non-quoted
	// object-like #define value, which the old code silently ignored, is now captured correctly.

	ScannedDefine scanned;
	if(!MacroCallScanner::extractDefine (scanned, token))
		return;

	Define define;
	define.name = scanned.name;
	define.value = scanned.value;

	// A tag #define's value isn't always a bare quoted literal (e.g. Skin's TAG_IMAGE "Image") -
	// it may instead wrap the literal in a single-argument string-constructing macro call (e.g.
	// Surface's TAG_BANK CCLSTR ("Bank")). Unwrap that call down to its quoted argument first, so
	// the plain-literal case below still applies uniformly.

	if(define.value.lastChar () == ')')
	{
		int openParen = define.value.index ('(');
		int firstQuote = define.value.index ('"');
		int lastQuote = define.value.lastIndex ('"');
		if(openParen >= 0 && firstQuote > openParen && lastQuote > firstQuote)
			define.value = define.value.subString (firstQuote, lastQuote - firstQuote + 1);
	}

	// extractDefine() deliberately keeps a quoted value's quotes (ClassScanner's own consumer
	// needs to tell a literal from a bare expression) - but Define::resolve()/resolveParts()
	// substitute the value directly in place of a scanned identifier (e.g. TAG_IMAGE -> Image,
	// not "Image"), so strip one matching pair of surrounding quotes here.

	if(define.value.length () >= 2 && define.value.firstChar () == '"' && define.value.lastChar () == '"')
		define.value = define.value.subString (1, define.value.length () - 2);

	scanner.defines.append (define);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleElement (SourceLexer& reader, SourceToken& token)
{
	if(scanner.handleMacros (reader, *this, token))
		return true;

	SourceElement* lastElement = getLastNonScopeElement ();

	if(token.type == SourceToken::kIdentifier)
	{
		if(token.text == "namespace")
			return handleElementType (reader, token, kNamespace);

		else if(token.text == "class")
		{
			if(elementStack.last ().type != kTemplate && elementStack.last ().type != kTemplateArg)
				return handleElementType (reader, token, kClass);
			return true;
		}

		else if(token.text == "struct" || token.text == "union" || token.text == "interface")
			return handleElementType (reader, token, kClass);

		else if(token.text == "enum")
			return handleElementType (reader, token, kEnum);

		else if(token.text == "DEFINE_ENUM")
		{
			if(nextToken (reader, token) == false) // expect a '('
				return false;
			return handleElementType (reader, token, kEnum);
		}

		else if(token.text == "BEGIN_ENUMINFO")
			return parseEnumInfo (reader, token);

		else if(token.text == "BEGIN_METHOD_NAMES")
			return parseClassMethods (reader, token);

		else if(token.text == "BEGIN_PROPERTY_NAMES")
			return parseClassProperties (reader, token);

		else if(token.text == "using")
			return parseUsingDirective (reader, token);

		else if(token.text == "template")
			return pushElement (kTemplate);

		else if(isFunctionBodyMacro (token))
			return parseFunctionBodyMacro (reader, token);

		else if(token.text == "static" || token.text == "inline")
		{
			if(lastElement == nullptr || (lastElement && lastElement->type == kNamespace))
			{
				if(nextToken (reader, token))
				{
					if(token.type == SourceToken::kIdentifier)
					{
						if(isFunctionBodyMacro (token))
							return parseFunctionBodyMacro (reader, token);

						if(token.text == "const" || token.text == "constexpr")
							if(nextToken (reader, token))
								if(token.type == SourceToken::kIdentifier) // type
									return handleElementType (reader, token, kConstant);
					}
				}
			}
		}

		else if(token.text == "const" || token.text == "constexpr")
		{
			if(lastElement == nullptr || lastElement->type == kNamespace)
				return parseNamespaceConstant (reader, token);
		}
		else if(handleEnum (reader, token))
			return true;
	}
	else if(token.type == SourceToken::kOperator)
	{
		if(recentLeadingDoxyComment.isEmpty () == false)
			recentLeadingDoxyComment.empty ();

		if(token.text == ";")
		{
			if(elementStack.last ().type == kEnumValue || elementStack.last ().type == kConstant)
				popElement ();

			if(elementStack.last ().type == kClass || elementStack.last ().type == kEnum)
				popElement ();
		}
		else if(token.text == "{")
		{
			// A '{' right after a namespace/class/enum header is that element's own body (kScope);
			// any other '{' (function/if/for/while/lambda body) is opaque, so a local declaration
			// inside it isn't mistaken for namespace-scope.

			bool opensDeclaredScope = elementStack.last ().type == kNamespace
				|| elementStack.last ().type == kClass
				|| elementStack.last ().type == kEnum;
			pushElement (opensDeclaredScope ? kScope : kOpaqueScope);
		}
		else if(token.text == "}")
		{
			if(elementStack.last ().type == kEnumValue)
				popElement ();

			SOFT_ASSERT (elementStack.last ().type == kScope || elementStack.last ().type == kOpaqueScope, "expected closing scope element")
			popElement ();

			if(elementStack.last ().type == kNamespace)
				popElement ();
		}
		else if(token.text == ",")
		{
			if(elementStack.last ().type == kEnumValue)
				popElement ();
		}
		else if(token.text == "<")
		{
			if(elementStack.last ().type == kTemplate || elementStack.last ().type == kTemplateArg)
				pushElement (kTemplateArg);
		}
		else if(token.text == ">")
		{
			if(elementStack.last ().type == kTemplateArg)
				popElement ();
			if(elementStack.last ().type == kTemplate)
				popElement ();
		}
		else if(token.text == "=")
		{
			if(elementStack.last ().type == kEnumValue)
			{
				return handleEnumValueConstant (elementStack.last (), reader, token);				
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleEnum (SourceLexer& reader, SourceToken& token)
{
	if(elementStack.count () >= 2)
	{
		if(elementStack.last ().type == kScope)
		{
			const SourceElement& e2 = elementStack.at (elementStack.count () - 2);
			if(e2.type == kEnum)
			{
				SourceElement e (kEnumValue, token.text);
				return pushElement (e);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleEnumValueConstant (SourceElement& element, SourceLexer& reader, SourceToken& token)
{
	if(element.type == kEnumValue)
	{
		bool nextTokenHandled = false;
		if(nextToken (reader, token))
		{
			if(token.type == SourceToken::kIdentifier)
			{
				nextTokenHandled = true;
				if(CString::isDigit (token.text.firstChar ()) == false)
				{
					MutableCString constantName = token.text;

					nextTokenHandled = false;
					if(nextToken (reader, token))
					{
						if(token.type == SourceToken::kOperator && token.text == "::") // scoped ? (one level)
						{
							nextTokenHandled = true;
							nextToken (reader, token);
							if(token.type == SourceToken::kIdentifier)
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
						EnumValueConstant constant;
						constant.enumItemName = elementStack.last ().data;
						getScopedName (constant.scopedName, elementStack.last ());
						constant.constantName = constantName;
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

bool DocumentationScanner::SourceFileParser::parseEnumInfo (SourceLexer& reader, SourceToken& token)
{
	// Owns the whole BEGIN_ENUMINFO...END_ENUMINFO token range itself (rather than being
	// re-entered per-token from handleElement), so 'info'/'item' can be plain local variables
	// instead of class-level scratch members. pushElement/popElement still bracket kEnumInfo/
	// kEnumInfoValue at the same points as before - handleDoxyComment()/getScopedName() rely on
	// elementStack to tag and scope doc comments.

	pushElement (kEnumInfo, false);

	AutoPtr<EnumInfo> info;

	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
		{
			if(token.type == SourceToken::kIdentifier && token.text == "END_ENUMINFO")
			{
				if(info && info->items.isEmpty () == false)
					scanner.enumsInfos.append (info);

				popElement ();
				return true;
			}
			else if(info != nullptr)
				info->name.append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(info == nullptr)
					info = NEW EnumInfo;
			}
			else if(token.text == ")")
			{
				if(info)
				{
					if(elementStack.last ().data.isEmpty ())
						elementStack.last ().data = info->name;
					flushRecentDoxyComment ();
				}
			}
			else if(token.text == "::")
			{
				if(info != nullptr)
					info->name.append (token.text);
			}
			else if(token.text == "{")
			{
				pushElement (kEnumInfoValue, false);

				EnumInfo::ScannedEnumerator item;
				int itemScanningPart = 0;

				while(nextToken (reader, token))
				{
					if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
					{
						if(itemScanningPart == 0)
							item.name.append (token.text);
					}
					else if(token.type == SourceToken::kOperator)
					{
						if(token.text == "}")
						{
							elementStack.last ().data = item.name;
							popElement ();
							if(info)
								info->items.append (item);
							break;
						}
						else if(token.text == ",")
							itemScanningPart++;
						else if(token.text == "::" && itemScanningPart == 0)
							item.name.append (token.text);
					}
				}
			}
		}
	}
	return false; // EOF before END_ENUMINFO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseClassMethods (SourceLexer& reader, SourceToken& token)
{
	pushElement (kClassMethodList, false);

	AutoPtr<ClassMethodList> list;

	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier)
		{
			if(token.text == "END_METHOD_NAMES")
			{
				if(list && list->methods.isEmpty () == false)
					scanner.classMethodLists.append (list);

				popElement ();
				return true;
			}
			else if(token.text == "DEFINE_METHOD_NAME" || token.text == "DEFINE_METHOD_ARGS" || token.text == "DEFINE_METHOD_ARGR")
			{
				pushElement (kClassMethod, false);

				ClassMethodList::ScannedMethod method;
				int methodScanningPart = 0;

				while(nextToken (reader, token))
				{
					if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
					{
						MutableCString* string = nullptr;
						switch(methodScanningPart)
						{
						case 0: string = &method.name; break;
						case 1: string = &method.args; break;
						case 2: string = &method.returnValue; break;
						}
						if(string)
							string->append (token.text);
					}
					else if(token.type == SourceToken::kOperator)
					{
						if(token.text == ")")
						{
							elementStack.last ().data = method.name;
							popElement ();
							if(list)
								list->methods.append (method);
							break;
						}
						else if(token.text == ",")
							methodScanningPart++;
					}
				}
			}
			else if(list != nullptr)
				list->className.append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(list == nullptr)
					list = NEW ClassMethodList;
			}
			else if(token.text == ")")
			{
				if(list)
				{
					if(elementStack.last ().data.isEmpty ())
						elementStack.last ().data = list->className;
				}
			}
			else if(token.text == "::")
			{
				if(list != nullptr)
					list->className.append (token.text);
			}
		}
	}
	return false; // EOF before END_METHOD_NAMES
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseClassProperties (SourceLexer& reader, SourceToken& token)
{
	pushElement (kClassPropertyList, false);

	AutoPtr<ClassPropertyList> list;

	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier)
		{
			if(token.text == "END_PROPERTY_NAMES")
			{
				if(list && list->properties.isEmpty () == false)
					scanner.classPropertyLists.append (list);

				popElement ();
				return true;
			}
			else if(token.text == "DEFINE_PROPERTY_NAME" || token.text == "DEFINE_PROPERTY_TYPE" || token.text == "DEFINE_PROPERTY_CLASS" || token.text == "DEFINE_PROPERTY_METACLASS" || token.text == "DEFINE_PROPERTY_CONTAINER")
			{
				pushElement (kClassProperty, false);

				ClassPropertyList::ScannedProperty property;
				int propertyScanningPart = 0;

				while(nextToken (reader, token))
				{
					if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
					{
						if(propertyScanningPart == 0)
							property.name.append (token.text);
					}
					else if(token.type == SourceToken::kOperator)
					{
						if(token.text == ")")
						{
							elementStack.last ().data = property.name;
							popElement ();
							if(list)
								list->properties.append (property);
							break;
						}
						else if(token.text == ",")
							propertyScanningPart++;
					}
				}
			}
			else if(list != nullptr)
				list->className.append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(list == nullptr)
					list = NEW ClassPropertyList;
			}
			else if(token.text == ")")
			{
				if(list && elementStack.last ().data.isEmpty ())
					elementStack.last ().data = list->className;
			}
			else if(token.text == "::")
			{
				if(list != nullptr)
					list->className.append (token.text);
			}
		}
	}
	return false; // EOF before END_PROPERTY_NAMES
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseNamespaceConstant (SourceLexer& reader, SourceToken& token)
{
	// Recognizes a namespace-scope "const <Type> <Name> = <string-literal>;" (plain, or a single-
	// argument wrapper call like "CSTR (\"...\")") and registers its scope-qualified name as a Define,
	// so a qualified-constant argument captured elsewhere resolves the same way a #define does. Any
	// other shape is abandoned, replaying the current token (e.g. '{' after "method () const").

	MutableCString name;
	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier)
			name = token.text; // last identifier before '=' wins
		else if(token.type == SourceToken::kOperator && token.text == "=")
			break;
		else if(token.type == SourceToken::kOperator &&
			(token.text == "::" || token.text == "*" || token.text == "&" || token.text == "<" || token.text == ">" || token.text == ","))
			continue; // type spec
		else
			return handleElement (reader, token); // abandon
	}

	if(name.isEmpty ())
		return true;

	MutableCString value;
	bool sawLiteral = false;
	bool inWrapperCall = false;
	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kOperator && token.text == ";")
			break;
		else if(token.type == SourceToken::kString)
		{
			value.append (token.text); // concatenate adjacent literals
			sawLiteral = true;
		}
		else if(token.type == SourceToken::kIdentifier && sawLiteral == false && value.isEmpty () && inWrapperCall == false)
			inWrapperCall = true; // e.g. CSTR ("...")
		else if(inWrapperCall && token.type == SourceToken::kOperator && (token.text == "(" || token.text == ")"))
			continue;
		else
			return handleElement (reader, token); // unsupported initializer - abandon
	}

	if(sawLiteral == false)
		return true;

	// Register every suffix of the qualified name - a call site often omits outer namespaces via
	// "using namespace". Never drop below one namespace + the name, to avoid cross-namespace collisions.

	Vector<MutableCString> segments;
	for(int j = 0; j < elementStack.count (); j++)
	{
		const SourceElement& scopeElement = elementStack.at (j);
		if(scopeElement.data.isEmpty () == false && scanner.isScopingElementType (scopeElement.type))
			segments.add (scopeElement.data);
	}
	segments.add (name);

	int maxDrop = (segments.count () >= 2) ? segments.count () - 2 : 0;
	for(int drop = 0; drop <= maxDrop; drop++)
	{
		MutableCString qualifiedName;
		for(int k = drop; k < segments.count (); k++)
		{
			if(qualifiedName.isEmpty () == false)
				qualifiedName.append ("::");
			qualifiedName.append (segments.at (k));
		}

		Define define;
		define.name = qualifiedName;
		define.value = value;
		scanner.defines.append (define);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::handleElementType (SourceLexer& reader, SourceToken& token, CStringRef type)
{
	if(nextToken (reader, token) == false)
		return false;

	if(token.type != SourceToken::kIdentifier)
		return handleElement (reader, token);

	if(token.text.isEmpty ())
	{
		ASSERT (0)
		return false;
	}

	MutableCString name (token.text);

	// An out-of-line nested class/struct definition (e.g. "class Outer::Inner: ...") - keep the
	// whole qualified name instead of truncating to just 'Outer', so its doc comment is never
	// mistaken for (or matched against) a same-named top-level 'Outer' class elsewhere.

	bool replayToken = false;
	if(type == kClass)
	{
		while(nextToken (reader, token))
		{
			if(token.type != SourceToken::kOperator || token.text != "::")
			{
				replayToken = true;
				break;
			}

			if(nextToken (reader, token) == false || token.type != SourceToken::kIdentifier)
				return false;

			name.append ("::");
			name.append (token.text);
		}
	}

	SourceElement e (type, name);
	if(pushElement (e) == false)
		return false;

	return replayToken ? handleElement (reader, token) : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseUsingDirective (SourceLexer& reader, SourceToken& token)
{
	// using namespace A::B; - other using-declarations are skipped.

	if(nextToken (reader, token) == false)
		return false;

	if(token.type != SourceToken::kIdentifier || token.text != "namespace")
	{
		reader.skipToEndOfLine ();
		return true;
	}

	UsingDirective directive;
	getCurrentScope (directive.scope);

	while(nextToken (reader, token))
	{
		if(token.type == SourceToken::kOperator && token.text == ";")
			break;

		if(token.type == SourceToken::kIdentifier || (token.type == SourceToken::kOperator && token.text == "::"))
			directive.target.append (token.text);
	}

	if(directive.target.startsWith ("::"))
		directive.target = directive.target.subString (2);

	if(directive.target.isEmpty () == false)
		usingDirectives.add (directive);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::isFunctionBodyMacro (const SourceToken& token)
{
	// These macros expand to a function header including the opening '{' of its body, the caller
	// writes the body and the closing '}' (see corevector.h, objectarray.h).

	return token.type == SourceToken::kIdentifier
		&& (token.text == "DEFINE_VECTOR_COMPARE"
		|| token.text == "DEFINE_VECTOR_COMPARE_OBJECT"
		|| token.text == "DEFINE_ARRAY_COMPARE");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::parseFunctionBodyMacro (SourceLexer& reader, SourceToken& token)
{
	// Skip the macro arguments and open an opaque scope for the '}' that closes the function body.
	// Without it, that '}' would close the enclosing class or namespace scope.

	if(nextToken (reader, token) == false)
		return false;

	if(token.type != SourceToken::kOperator || token.text != "(") // not a macro call
		return handleElement (reader, token);

	int depth = 1;
	while(depth > 0)
	{
		if(nextToken (reader, token) == false)
			return false;

		if(token.type == SourceToken::kOperator && token.text == "(")
			depth++;
		else if(token.type == SourceToken::kOperator && token.text == ")")
			depth--;
	}

	return pushElement (kOpaqueScope);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::getCurrentScope (MutableCString& scope) const
{
	// e.g. "CCL::SkinElements", empty at global scope

	for(const SourceElement& scopeElement : elementStack)
	{
		if(scopeElement.data.isEmpty () == false && scanner.isScopingElementType (scopeElement.type))
		{
			if(scope.isEmpty () == false)
				scope.append ("::");
			scope.append (scopeElement.data);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationScanner::SourceFileParser::getLookupScopes (Vector<MutableCString>& scopes) const
{
	// Scopes to resolve a name at the current position against, in lookup order: enclosing scopes
	// (innermost first), then namespaces from visible using-directives. Simplified C++ lookup.

	MutableCString currentScope;
	getCurrentScope (currentScope);

	MutableCString scope (currentScope);
	while(true)
	{
		scopes.addOnce (scope);
		if(scope.isEmpty ())
			break;

		int separator = scope.lastIndex (':');
		scope = separator > 0 ? scope.subString (0, separator - 1) : MutableCString ();
	}

	for(const UsingDirective& directive : usingDirectives)
	{
		bool visible = directive.scope.isEmpty () || currentScope == directive.scope;
		if(!visible)
		{
			MutableCString prefix (directive.scope);
			prefix.append ("::");
			visible = currentScope.startsWith (prefix);
		}
		if(!visible)
			continue;

		int count = scopes.count ();
		for(int i = 0; i < count; i++)
		{
			MutableCString nominated (scopes.at (i));
			if(nominated.isEmpty () == false)
				nominated.append ("::");
			nominated.append (directive.target);
			scopes.addOnce (nominated);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::getScopedName (MutableCString& scopedName, const SourceElement& element) const
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
			const SourceElement& scopeElement = elementStack.at (j);
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

void DocumentationScanner::SourceFileParser::handleDoxyComment (String& comment, DoxyCommentType type) const
{
	int stackDepth = elementStack.count ();
	if(stackDepth > 0)
	{
		const SourceElement& related = (type == kLeading || lastPoppedElement.type == kUndefinedElementType) ? elementStack.last () : lastPoppedElement;
		if(isDocumentableElement (related))
		{
			AutoPtr<DocuSnippet> snippet = NEW DocuSnippet (related.type);
			if(DoxyCommentParser::parse (*snippet, comment))
			{
				snippet->targetName = related.data;
				getScopedName (snippet->scopedName, related);

				scanner.docuSnippets.append (snippet);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentationScanner::SourceFileParser::isDocumentableElement (const SourceElement& e) const
{
	if(e.data.isEmpty ())
		return false;

	return scanner.isDocumentableElementType (e.type);
}

//************************************************************************************************
// TypeLibDocuScanner::AliasCollector
/** Run by beforeScan(), before the real parse: finds every "#define ADD_TYPELIB_MEMBER_*(...)" in
	the scanned tree and registers it via registerMemberMacroAlias(), so MacroHandler can recognize
	a call site later without hardcoding the shortcut's name. */
//************************************************************************************************

struct TypeLibDocuScanner::AliasCollector: public SourceFileVisitor
{
	AliasCollector (TypeLibDocuScanner& scanner): scanner (scanner) {}
	void visitFile (UrlRef path) override;

	TypeLibDocuScanner& scanner;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void TypeLibDocuScanner::AliasCollector::visitFile (UrlRef path)
{
	// See SourceFileParser::parseFile() - same reason for reading the whole file into memory once.
	AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (path);
	String rawUnicode;
	if(fileStream)
	{
		AutoPtr<IMemoryStream> inMemory = NEW MemoryStream;
		if(System::GetFileUtilities ().copyStream (*inMemory, *fileStream))
		{
			inMemory->seek (0, IStream::kSeekSet);
			rawUnicode = TextUtils::loadRawString (*inMemory);
		}
	}
	MutableCString raw (rawUnicode, Text::kASCII);

	SourceLexer lexer (raw);
	SourceToken token;
	while(lexer.next (token))
	{
		if(token.type != SourceToken::kPreprocessor)
			continue;

		ScannedFunctionDefine candidate;
		if(!MacroCallScanner::extractFunctionDefine (candidate, token))
			continue;

		if(!candidate.name.startsWith ("ADD_TYPELIB_MEMBER_"))
			continue; // cheap name-prefix filter; registerMemberMacroAlias() does the real shape check

		if(candidate.name == "ADD_TYPELIB_MEMBER_OBJECT" || candidate.name == "ADD_TYPELIB_MEMBER_BOOL" ||
			candidate.name == "ADD_TYPELIB_MEMBER_ENUM" || candidate.name == "ADD_TYPELIB_MEMBER_FLOAT" ||
			candidate.name == "ADD_TYPELIB_MEMBER_INT" || candidate.name == "ADD_TYPELIB_MEMBER_STRING")
			continue; // built-in shortcuts, hardcoded in parseTypeLibMembers() - not real aliases

		scanner.registerMemberMacroAlias (candidate, path);
	}
}

//************************************************************************************************
// TypeLibDocuScanner::MacroHandler
//************************************************************************************************

struct TypeLibDocuScanner::MacroHandler
{
	MacroHandler (TypeLibDocuScanner& scanner, SourceFileParser& parser): scanner (scanner), parser (parser) {}
	bool handleMacros (SourceLexer& reader, SourceToken& token) const;

private:
	TypeLibDocuScanner& scanner;
	SourceFileParser& parser;

	bool parseTypeLibElement (SourceLexer& reader, SourceToken& token, bool withParent) const;
	bool parseTypeLibMembers (SourceLexer& reader, SourceToken& token) const;
	bool parseTypeLibEntry (SourceLexer& reader, SourceToken& token, bool withParent) const;
	bool parseTypeLibEnumeration (SourceLexer& reader, SourceToken& token, bool withParent) const;
	bool parseTypeLibRelatedClass (SourceLexer& reader, SourceToken& token) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::handleMacros (SourceLexer& reader, SourceToken& token) const
{
	// Each of these owns its whole macro call (or BEGIN/END block) itself, pulling tokens directly
	// via parser.nextToken() rather than being re-entered per-token via elementStack dispatch - so
	// the scanned object is a local variable, not a scanner-level scratch member (see also
	// SourceFileParser's parseEnumInfo()/parseClassMethods()/parseClassProperties()).

	if(token.type != SourceToken::kIdentifier)
		return false;

	if(token.text == "DEFINE_TYPELIB_CLASS")
		return parseTypeLibElement (reader, token, false);
	else if(token.text == "DEFINE_TYPELIB_CLASS_WITH_PARENT")
		return parseTypeLibElement (reader, token, true);
	else if(token.text == "DEFINE_TYPELIB_ENTRY" || token.text == "DEFINE_TYPELIB_ENTRY_LOCAL")
		return parseTypeLibEntry (reader, token, false);
	else if(token.text == "DEFINE_TYPELIB_ENTRY_WITH_PARENT")
		return parseTypeLibEntry (reader, token, true);
	else if(token.text == "BEGIN_TYPELIB_MEMBERS")
		return parseTypeLibMembers (reader, token);
	else if(token.text == "DEFINE_TYPELIB_ENUMERATION")
		return parseTypeLibEnumeration (reader, token, false);
	else if(token.text == "DEFINE_TYPELIB_ENUMERATION_WITH_PARENT")
		return parseTypeLibEnumeration (reader, token, true);
	else if(token.text == "DEFINE_TYPELIB_RELATED_CLASS")
		return parseTypeLibRelatedClass (reader, token);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::parseTypeLibElement (SourceLexer& reader, SourceToken& token, bool withParent) const
{
	// DEFINE_TYPELIB_CLASS (Class, entryName, library)
	// DEFINE_TYPELIB_CLASS_WITH_PARENT (Class, Parent, entryName, library)
	// example: DEFINE_TYPELIB_CLASS (RowElement, TAG_ROW, SKIN_TYPELIB)
	// The 'entryName' arg sits at a different index for each macro (1 vs 2), told apart via
	// 'withParent' - 'Parent' (WITH_PARENT only) and the trailing 'library' argument are never
	// captured. Never a documentable/scoping element type, so no elementStack push needed.

	const int kArgClass = 0;
	const int kArgTagName = withParent ? 2 : 1;

	AutoPtr<ScannedEntry> element;
	int scanningPart = kArgClass;

	auto getPartString = [&] () -> MutableCString*
	{
		if(element != nullptr)
		{
			if(scanningPart == kArgClass)
				return &element->elementClass;
			else if(scanningPart == kArgTagName)
				return &element->tagName;
		}
		return nullptr;
	};

	while(parser.nextToken (reader, token))
	{
		if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(element == nullptr && scanningPart == kArgClass)
					element = NEW ScannedEntry;
			}
			else if(token.text == ",")
			{
				scanningPart++;
			}
			else if(token.text == ")")
			{
				if(element && element->tagName.isEmpty () == false)
				{
					element->uniqueName = element->elementClass;
					parser.getLookupScopes (element->elementClassScopes);
					scanner.entries.append (element);
					scanner.lastDefinedEntry = element;
				}

				return true;
			}
			else if(token.text == "::" )
			{
				if(MutableCString* string = getPartString ())
					string->append (token.text);
			}
		}
		else if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
		{
			if(MutableCString* string = getPartString ())
				string->append (token.text);
		}
	}
	return false; // EOF before closing ')'
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::parseTypeLibMembers (SourceLexer& reader, SourceToken& token) const
{
	// Handles BEGIN_TYPELIB_MEMBERS for both families (a class-backed element or a plain entry) - the
	// macro vocabulary inside the block (ADD_TYPELIB_MEMBER(_OBJECT)/ADD_TYPELIB_MEMBER_* shortcuts)
	// is identical either way. Attaches to whichever entry was most recently defined.

	if(scanner.lastDefinedEntry == nullptr)
		return false;

	// Raw, non-owning alias (like lastDefinedEntry itself) - 'entries' is the sole owner.
	ScannedEntry* targetEntry = scanner.lastDefinedEntry;
	scanner.lastDefinedEntry = nullptr;

	parser.pushElement (SourceFileParser::SourceElement (kTypeLibMemberList, targetEntry->uniqueName));

	while(parser.nextToken (reader, token))
	{
		if(token.type != SourceToken::kIdentifier)
			continue;

		if(token.text == "END_TYPELIB_MEMBERS")
		{
			parser.popElement ();
			return true;
		}

		CStringPtr fixedTypeName = nullptr;
		bool isMember = false;
		if(token.text == "ADD_TYPELIB_MEMBER")
			isMember = true;
		else if(token.text == "ADD_TYPELIB_MEMBER_OBJECT")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kObject;
		}
		else if(token.text == "ADD_TYPELIB_MEMBER_BOOL")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kBool;
		}
		else if(token.text == "ADD_TYPELIB_MEMBER_ENUM")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kEnum;
		}
		else if(token.text == "ADD_TYPELIB_MEMBER_FLOAT")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kFloat;
		}
		else if(token.text == "ADD_TYPELIB_MEMBER_INT")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kInt;
		}
		else if(token.text == "ADD_TYPELIB_MEMBER_STRING")
		{
			isMember = true;
			fixedTypeName = CCL::TypeNames::kString;
		}
		else if(const MemberMacroAlias* alias = scanner.findMemberMacroAlias (token.text))
		{
			isMember = true;
			fixedTypeName = alias->typeName.str ();
		}

		if(isMember == false)
			continue;

		// No flushRecentDoxyComment() here (unlike other push sites) - a leading doc comment must
		// stay pending until this member's own closing ")" pops it (below, which sets the pushed
		// element's data to the real tag name first) rather than being flushed now against the
		// still-current, non-documentable enclosing member list.

		parser.pushElement (kTypeLibMember, false);

		// ADD_TYPELIB_MEMBER (name, typeName) - both args captured.
		// ADD_TYPELIB_MEMBER_* shortcut / ADD_TYPELIB_MEMBER_OBJECT (name[, ignoredTypeArg]) - only
		// 'name' is captured here, 'typeName' (fixedTypeName) was already fixed above.

		enum Arg { kArgName = 0, kArgTypeName = 1 };

		ScannedMember member (fixedTypeName);
		int scanningPart = kArgName;

		while(parser.nextToken (reader, token))
		{
			if(token.type == SourceToken::kOperator)
			{
				if(token.text == ")")
				{
					if(targetEntry && member.tagName.isEmpty () == false)
						targetEntry->members.append (member);

					parser.getLastNonScopeElement ()->data = member.tagName;
					parser.popElement ();
					break;
				}
				else if(token.text == ",")
					scanningPart++;
				else if(token.text == "::" && scanningPart == kArgName)
					member.tagName.append (token.text); // preserve qualified constant names, e.g. StyleID::BIV::kFoo
			}
			else if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
			{
				if(scanningPart == kArgName)
					member.tagName.append (token.text);
				else if(scanningPart == kArgTypeName && member.typeName.isEmpty ())
					member.typeName = token.text;
			}
		}
	}
	return false; // EOF before END_TYPELIB_MEMBERS
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::parseTypeLibEntry (SourceLexer& reader, SourceToken& token, bool withParent) const
{
	// DEFINE_TYPELIB_ENTRY(_LOCAL) (UniqueName, entryName, library)
	// DEFINE_TYPELIB_ENTRY_WITH_PARENT (UniqueName, Parent, entryName, library), e.g.
	// DEFINE_TYPELIB_ENTRY_WITH_PARENT (Button, VisualStyle, "Button", VISUALSTYLES_TYPELIB) - self-
	// contained, captures the object and pops on its own closing paren. 'entryName' sits at a
	// different index for each macro (1 vs 2), told apart via 'withParent' - captured regardless of
	// token type, since real call sites pass it as either a string literal or a #define'd tag
	// constant (resolved later by postScan()'s #define pass, same as parseTypeLibElement's tagName).
	// 'Parent' (WITH_PARENT only) and the trailing 'library' argument are never captured.
	// BEGIN_TYPELIB_MEMBERS for this entry is handled separately, by parseTypeLibMembers (shared
	// with the class-backed case).

	const int kArgUniqueName = 0;
	const int kArgTagName = withParent ? 2 : 1;

	parser.pushElement (kTypeLibEntry, false);

	AutoPtr<ScannedEntry> entry;
	int scanningPart = kArgUniqueName;

	auto getPartString = [&] () -> MutableCString*
	{
		if(entry != nullptr)
		{
			if(scanningPart == kArgUniqueName)
				return &entry->uniqueName;
			else if(scanningPart == kArgTagName)
				return &entry->tagName;
		}
		return nullptr;
	};

	while(parser.nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
		{
			if(MutableCString* string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(entry == nullptr && scanningPart == kArgUniqueName)
					entry = NEW ScannedEntry;
			}
			else if(token.text == "::")
			{
				if(MutableCString* string = getPartString ())
					string->append (token.text); // preserve qualified constant names, e.g. StyleID::BIV::kFoo
			}
			else if(token.text == ",")
			{
				if(scanningPart == kArgUniqueName && entry)
					parser.getLastNonScopeElement ()->data = entry->uniqueName;

				scanningPart++;
			}
			else if(token.text == ")")
			{
				if(entry && entry->uniqueName.isEmpty () == false)
				{
					scanner.entries.append (entry);
					scanner.lastDefinedEntry = entry;
				}
				parser.popElement ();
				return true;
			}
		}
	}
	return false; // EOF before closing ')'
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::parseTypeLibEnumeration (SourceLexer& reader, SourceToken& token, bool withParent) const
{
	// DEFINE_TYPELIB_ENUMERATION (ownerTagName, memberName, enumDefName, library)
	// DEFINE_TYPELIB_ENUMERATION_PARENT (ownerTagName, memberName, enumDefName, parentOwnerTagName, parentMemberName, library)
	// The trailing 'library' argument sits at a different index for each macro (3 vs 5) and is never
	// captured - told apart via 'withParent'. Never a documentable/scoping element type, so no
	// elementStack push needed.

	enum Arg { kArgOwnerTagName = 0, kArgMemberName = 1, kArgEnumDefName = 2,
		kArgParentOwnerTagName = 3, kArgParentMemberName = 4 };

	ScannedEnum scannedEnum;
	bool started = false;
	int scanningPart = kArgOwnerTagName;

	auto getPartString = [&] () -> MutableCString*
	{
		if(started)
		{
			switch(scanningPart)
			{
			case kArgOwnerTagName :
				return &scannedEnum.ownerTagName;
			case kArgMemberName :
				return &scannedEnum.memberName;
			case kArgEnumDefName :
				return &scannedEnum.enumDefName;
			case kArgParentOwnerTagName :
				if(withParent)
					return &scannedEnum.parentOwnerTagName;
				break; // plain DEFINE_TYPELIB_ENUMERATION's trailing 'library' argument
			case kArgParentMemberName :
				if(withParent)
					return &scannedEnum.parentMemberName;
				break; // DEFINE_TYPELIB_ENUMERATION_PARENT's trailing 'library' argument
			}
		}
		return nullptr;
	};

	while(parser.nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
		{
			if(MutableCString* string = getPartString ())
				string->append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				started = true;
			}
			else if(token.text == ",")
			{
				scanningPart++;
			}
			else if(token.text == ")")
			{
				if(started)
					scanner.enums.append (scannedEnum);
				return true;
			}
			else if(token.text == "::" )
			{
				if(MutableCString* string = getPartString ())
					string->append (token.text);
			}
		}
	}
	return false; // EOF before closing ')'
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::MacroHandler::parseTypeLibRelatedClass (SourceLexer& reader, SourceToken& token) const
{
	// DEFINE_TYPELIB_RELATED_CLASS (UniqueName, RelatedClass)
	// 'UniqueName' is captured (not assumed to be whatever was scanned last) and used to look up the
	// target element, since - unlike BEGIN_TYPELIB_MEMBERS - the real macro expansion doesn't require
	// this call to immediately follow the element's own DEFINE_TYPELIB_CLASS(_WITH_PARENT) line. Never
	// a documentable/scoping element type, so no elementStack push needed.

	enum Arg { kArgUniqueName = 0, kArgRelatedClass = 1 };

	MutableCString uniqueName;
	MutableCString relatedClass;
	int scanningPart = kArgUniqueName;

	while(parser.nextToken (reader, token))
	{
		if(token.type == SourceToken::kOperator)
		{
			if(token.text == ",")
				scanningPart++;
			else if(token.text == "::" && scanningPart == kArgRelatedClass)
				relatedClass.append (token.text);
			else if(token.text == ")")
			{
				if(relatedClass.isEmpty () == false)
				{
					if(ScannedEntry* target = scanner.findTypeLibEntry (uniqueName))
					{
						target->relatedClass = relatedClass;
						target->relatedClassScopes.removeAll ();
						parser.getLookupScopes (target->relatedClassScopes);
					}
				}
				return true;
			}
		}
		else if(token.type == SourceToken::kIdentifier)
		{
			if(scanningPart == kArgUniqueName)
				uniqueName.append (token.text);
			else if(scanningPart == kArgRelatedClass)
				relatedClass.append (token.text);
		}
	}
	return false; // EOF before closing ')'
}

//************************************************************************************************
// TypeLibDocuScanner
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (TypeLibDocuScanner, kTypeLibClass, "typeLibClass")
DEFINE_STRINGID_MEMBER_ (TypeLibDocuScanner, kTypeLibEntry, "typeLibEntry")
DEFINE_STRINGID_MEMBER_ (TypeLibDocuScanner, kTypeLibMemberList, "typeLibMemberList")
DEFINE_STRINGID_MEMBER_ (TypeLibDocuScanner, kTypeLibMember, "typeLibMember")

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibDocuScanner::TypeLibDocuScanner ()
: lastDefinedEntry (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TypeLibDocuScanner::applyToModel (ClassRepository& repository)
{
	bool anyChanges = false;
	if(applyEntryDocs (repository))
		anyChanges = true;
	if(applyEnumDocs (repository))
		anyChanges = true;

	if(anyChanges)
		repository.signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::isQualifiedIdentifier (CStringRef text)
{
	// A (possibly namespace-qualified) identifier, e.g. "kColor" or "CCL::TypeNames::kColor" - same
	// identifier-character rules as SourceLexer itself.
	
	if(text.isEmpty ())
		return false;

	CStringPtr str = text.str ();
	for(int i = 0; i < text.length (); i++)
	{
		char c = str [i];
		if(c == ':' && i + 1 < text.length () && str [i + 1] == ':')
		{
			i++;
			continue;
		}
		bool atStart = (i == 0) || (str [i - 1] == ':');
		if(atStart ? !SourceLexer::isIdentStart (c) : !SourceLexer::isIdentChar (c))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TypeLibDocuScanner::registerMemberMacroAlias (const ScannedFunctionDefine& candidate, UrlRef path)
{
	Vector<MutableCString> forwardedArgs;
	if(!MacroCallScanner::matchSingleParamForward (forwardedArgs, candidate, "ADD_TYPELIB_MEMBER_WITH_TYPE", 3))
		return;

	const MutableCString& typeArg = forwardedArgs.at (1);
	MutableCString typeName;
	if(typeArg.length () >= 2 && typeArg.firstChar () == '"' && typeArg.lastChar () == '"')
	{
		typeName = typeArg.subString (1, typeArg.length () - 2); // fixed string literal, e.g. "color"
	}
	else if(isQualifiedIdentifier (typeArg))
	{
		// Every real ADD_TYPELIB_MEMBER_* shortcut (typelibmacros.h, guitypelibmacros.h) forwards a
		// named constant here (e.g. CCL::TypeNames::kColor), not a literal - there's no preprocessor
		// or linker available to resolve its actual value, so the identifier text stands in as-is.

		typeName = typeArg;
	}
	else
		return; // third argument must be a fixed string literal or a qualified constant name

	String fileName;
	path.getName (fileName);
	Logging::info ("registered member macro alias '%(1)' found in '%(2)'", String (candidate.name), fileName);

	for(MemberMacroAlias& existing : memberMacroAliases)
	{
		if(existing.name == candidate.name)
		{
			existing.typeName = typeName; // last definition wins, matching #define redefinition
			return;
		}
	}

	MemberMacroAlias alias;
	alias.name = candidate.name;
	alias.typeName = typeName;
	memberMacroAliases.append (alias);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const TypeLibDocuScanner::MemberMacroAlias* TypeLibDocuScanner::findMemberMacroAlias (CStringRef macroName) const
{
	// Look up a previously registered alias by macro name; returns nullptr if 'macroName' isn't one.

	for(const MemberMacroAlias& alias : memberMacroAliases)
		if(alias.name == macroName)
			return &alias;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibDocuScanner::ScannedEntry* TypeLibDocuScanner::findTypeLibEntry (CStringRef uniqueName) const
{
	// Look up a previously scanned entry by its linking identifier (see ScannedEntry::uniqueName -
	// also DEFINE_TYPELIB_RELATED_CLASS's own 'UniqueName' argument); returns nullptr if none was
	// found (yet).

	for(const AutoPtr<ScannedEntry>& entry : entries)
		if(entry->uniqueName == uniqueName)
			return entry;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::isElementSnippet (DocuSnippet* snippet) const
{
	// Excludes entry-family snippet types, so a permissive class-level doc match for a class-backed
	// entry (any documentable snippet type, not just kTypeLibClass's own - see postScan()) can't
	// accidentally pick up a same-named plain entry's, or a member's, snippet instead.

	if(snippet->elementType == kTypeLibEntry || snippet->elementType == kTypeLibMember)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationScanner::DocuSnippet* TypeLibDocuScanner::findClassDocu (CStringRef className, const Vector<MutableCString>& lookupScopes) const
{
	// Exact scoped name match only, so a same-named class in another namespace is never picked.

	if(className.startsWith ("::"))
	{
		MutableCString globalName (className.subString (2));
		for(const AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
			if(isElementSnippet (docuSnippet) && docuSnippet->scopedName == globalName)
				return docuSnippet;
		return nullptr;
	}

	for(const MutableCString& scope : lookupScopes)
	{
		MutableCString qualifiedName (scope);
		if(qualifiedName.isEmpty () == false)
			qualifiedName.append ("::");
		qualifiedName.append (className);

		for(const AutoPtr<DocuSnippet>& docuSnippet : docuSnippets)
			if(isElementSnippet (docuSnippet) && docuSnippet->scopedName == qualifiedName)
				return docuSnippet;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::applyEntryDocs (ClassRepository& repository, EntryKind kind) const
{
	bool anyChanges = false;

	ForEach (repository.getClasses (), Class, modelClass)
		ListForEach (entries, AutoPtr<ScannedEntry>, entry)
			bool classBacked = entry->elementClass.isEmpty () == false;
			bool kindMatches = kind == EntryKind::kAny
				|| (kind == EntryKind::kClassBackedOnly && classBacked)
				|| (kind == EntryKind::kPlainOnly && classBacked == false);

			if(kindMatches && entry->tagName == modelClass->getName ())
			{
				if(applyDocu (*modelClass, entry->docu))
					anyChanges = true;

				bool anyMemberChanges = false;
				ObjectArray members;
				modelClass->getMembers (members, false);
				ForEach (members, Member, modelMember)
					for(const ScannedMember& entryMember : entry->members)
					{
						if(entryMember.tagName == modelMember->getName ())
						{
							if(applyDocu (*modelMember, entryMember.docu))
								anyMemberChanges = true;

							break; // only the first scanned member with this tag counts
						}
					}
				EndFor
				if(anyMemberChanges)
				{
					modelClass->deferChanged ();
					anyChanges = true;
				}

				break; // tag names are unique within a matching kind, so one entry match is enough
			}
		EndFor
	EndFor

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::applyEnumDocs (ClassRepository& repository) const
{
	bool anyChanges = false;

	ForEach (repository.getEnumerations (), Enumeration, enumeration)
		EnumInfo* info = nullptr;
		for(const ScannedEnum& typeLibEnum : enums)
			if(typeLibEnum.info)
			{
				MutableCString enumName = MutableCString (typeLibEnum.ownerTagName).append (".").append (typeLibEnum.memberName);
				if(enumName == enumeration->getName ())
				{
					info = typeLibEnum.info;
					break;
				}
			}

		if(info)
		{
			bool anyEnumeratorChanges = false;
			ObjectArray enumerators;
			enumeration->getEnumerators (enumerators, true);
			ForEach (enumerators, Enumerator, enumerator)
				ListForEach (info->items, EnumInfo::ScannedEnumerator, item)
					if(item.docu && item.name == enumerator->getName ())
					{
						if(applyDocu (*enumerator, item.docu))
							anyEnumeratorChanges = true;
					}
				EndFor
			EndFor
			if(anyEnumeratorChanges)
			{
				enumeration->deferChanged ();
				anyChanges = true;
			}
		}
	EndFor

	return anyChanges;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TypeLibDocuScanner::postScan ()
{
	SuperClass::postScan ();

	// Resolve #define references.

	for(Define& define : defines)
	{
		ListForEach (entries, AutoPtr<ScannedEntry>, entry)
			define.resolve (entry->tagName);
			for(ScannedMember& member : entry->members)
			{
				define.resolve (member.tagName);
				define.resolve (member.typeName);
			}
		EndFor
		for(ScannedEnum& typeLibEnum : enums)
		{
			define.resolve (typeLibEnum.ownerTagName);
			define.resolve (typeLibEnum.memberName);
			define.resolve (typeLibEnum.enumDefName);
			define.resolve (typeLibEnum.parentOwnerTagName);
			define.resolve (typeLibEnum.parentMemberName);
		}
	}

	// Find docu: a class-backed entry's own class doc comment if documented - or, for a plain
	// (non-class-backed) entry, a comment written directly on its own macro invocation.

	ListForEach (entries, AutoPtr<ScannedEntry>, entry)
		if(entry->elementClass.isEmpty () == false)
		{
			entry->docu = findClassDocu (entry->elementClass, entry->elementClassScopes);
		}
		else
		{
			entry->docu = findDocuByTargetName (kTypeLibEntry, entry->uniqueName);
		}

		// Else fall back to the related class named via DEFINE_TYPELIB_RELATED_CLASS, if any -
		// covers a thin entry wrapping an existing, already-documented class (a plain entry can use
		// this too, not just a class-backed one).

		if(entry->docu == nullptr && entry->relatedClass.isEmpty () == false)
			entry->docu = findClassDocu (entry->relatedClass, entry->relatedClassScopes);

		// Connect docu for members.
		for(ScannedMember& member : entry->members)
		{
			MutableCString scopedMember (entry->uniqueName);
			scopedMember.append ("::");
			scopedMember.append (member.tagName);

			member.docu = findScopedDocu (kTypeLibMember, scopedMember);
		}
	EndFor

	// Rewrite a \see-style link from a plain class name (an entry's own elementClass or
	// relatedClass) to that entry's tag name instead, so a cross-reference between two entries
	// points at their public tag (e.g. "Style") rather than the backing C++ class ("StyleElement").

	ListForEach (entries, AutoPtr<ScannedEntry>, entry)
		if(entry->docu != nullptr)
		{
			for(int i = 0; i < entry->docu->links.count (); i++)
			{
				String& link = entry->docu->links.at (i);

				ListForEach (entries, AutoPtr<ScannedEntry>, other)
					if(other->matchesClassName (link))
					{
						link = String (other->tagName);
						break;
					}
				EndFor
			}
		}
	EndFor

	// Resolve enum info: match directly against a same-named BEGIN_ENUMINFO block.

	for(ScannedEnum& typeLibEnum : enums)
	{
		ListForEach (enumsInfos, AutoPtr<EnumInfo>, enumInfo)
			if(typeLibEnum.enumDefName == enumInfo->name)
			{
				typeLibEnum.info = enumInfo;
				break;
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::beforeScan (UrlRef folder, IProgressNotify* progress)
{
	if(!SuperClass::beforeScan (folder, progress))
		return false;

	AliasCollector collector (*this);
	return SourceFileTraverser::traverse (folder, SourceFileParser::getFilter (), collector, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token)
{
	return MacroHandler (*this, parser).handleMacros (reader, token);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::isDocumentableElementType (CStringRef type) const
{
	return SuperClass::isDocumentableElementType (type)
		|| type == kTypeLibMember
		|| type == kTypeLibEntry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibDocuScanner::isScopingElementType (CStringRef type) const
{
	return SuperClass::isScopingElementType (type)
		|| type == kTypeLibMemberList;
}

//************************************************************************************************
// GuiDocuScanner
//************************************************************************************************

void GuiDocuScanner::applyToModel (ClassRepository& repository)
{
	// Skin Elements
	if(repository.getTitle ().contains (CCLSTR ("skin"), false))
		applyToSkinModel (repository);

	// Visual Styles
	else if(repository.getTitle ().contains (CCLSTR ("visual"), false))
		applyToVisualStyleModel (repository);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::isMatchingFolder (UrlRef folder)
{
	return folder.getPath ().contains (CCLSTR ("/gui"), false) ||
		folder.getPath ().contains (CCLSTR ("/ccl/meta"), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::parseStyleDef (SourceLexer& reader, SourceFileParser& parser, SourceToken& token)
{
	// BEGIN_STYLEDEF (Name) { SkinName, CppName }, { SkinName, CppName }, ... END_STYLEDEF

	// Owns the whole token range itself, like SourceFileParser's parseEnumInfo()/parseClassMethods()/
	// parseClassProperties() - no elementStack needed here though, since this construct was never a
	// documentable/scoping element type (its items get docu linked separately, by name, in postScan()).

	AutoPtr<OptionList> styleDef;
	bool nameClosed = false; // true once the '(Name)' portion's closing ')' has been seen

	while(parser.nextToken (reader, token))
	{
		if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
		{
			if(token.type == SourceToken::kIdentifier && token.text == "END_STYLEDEF")
			{
				if(styleDef && styleDef->items.isEmpty () == false)
					optionLists.append (styleDef);
				return true;
			}
			else if(styleDef != nullptr && nameClosed == false)
				styleDef->name.append (token.text);
		}
		else if(token.type == SourceToken::kOperator)
		{
			if(token.text == "(")
			{
				if(styleDef == nullptr)
					styleDef = NEW OptionList;
			}
			else if(token.text == ")")
			{
				nameClosed = true;
			}
			else if(token.text == "::")
			{
				if(styleDef != nullptr && nameClosed == false)
					styleDef->name.append (token.text);
			}
			else if(token.text == "{")
			{
				OptionList::ScannedOption item;
				int itemScanningPart = 0;

				while(parser.nextToken (reader, token))
				{
					if(token.type == SourceToken::kIdentifier || token.type == SourceToken::kString)
					{
						MutableCString* string = nullptr;
						switch(itemScanningPart)
						{
						case 0: string = &item.skinName; break;
						case 1: string = &item.cppName; break;
						}
						if(string)
							string->append (token.text);
					}
					else if(token.type == SourceToken::kOperator)
					{
						if(token.text == "}")
						{
							if(styleDef)
								styleDef->items.append (item);
							break;
						}
						else if(token.text == ",")
							itemScanningPart++;
						else if(token.text == "::")
						{
							MutableCString* string = nullptr;
							switch(itemScanningPart)
							{
							case 0: string = &item.skinName; break;
							case 1: string = &item.cppName; break;
							}
							if(string)
								string->append (token.text);
						}
					}
				}
			}
		}
	}
	return false; // EOF before END_STYLEDEF
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GuiDocuScanner::applyToSkinModel (ClassRepository& repository) const
{
	bool anyChanges = false;
	if(applyEntryDocs (repository, EntryKind::kClassBackedOnly))
		anyChanges = true;
	if(applyEnumDocs (repository))
		anyChanges = true;

	if(anyChanges)
		repository.signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GuiDocuScanner::applyToVisualStyleModel (ClassRepository& repository) const
{
	bool anyChanges = applyEntryDocs (repository, EntryKind::kPlainOnly);

	if(anyChanges)
		repository.signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GuiDocuScanner::postScan ()
{
	SuperClass::postScan ();

	// Connect styledefs with docu for corresponding enum values.

	ListForEach (optionLists, AutoPtr<OptionList>, optionList)

		for(OptionList::ScannedOption& item : optionList->items)
		{
			for(EnumValueConstant& constant : enumConstants)
			{
				if(constant.docu.isValid ())
				{
					if(scopedCompare (constant.scopedName, item.cppName) > 0)
					{
						item.docu = constant.docu;
						break;
					}
				}
			}
			if(item.docu.isValid () == false)
			{
				ListForEach (docuSnippets, AutoPtr<DocuSnippet>, docuSnippet)
					if(docuSnippet->elementType == kEnumValue)
					{
						if(docuSnippet->scopedCompare (item.cppName) > 0)
						{
							item.docu = docuSnippet;
							break;
						}
					}
				EndFor
			}
		}
	EndFor

	// Some enums aren't resolved yet at this point: SuperClass::postScan() only matches a scanned
	// enum against a BEGIN_ENUMINFO block, but Skin defines its enum values via StyleDef/OptionList
	// instead. For each still-unresolved enum, build an EnumInfo from the matching OptionList so
	// TypeLibDocuScanner::applyEnumDocs() can use it the same way it uses a real one.

	for(ScannedEnum& typeLibEnum : enums)
	{
		if(typeLibEnum.info == nullptr)
		{
			ListForEach (optionLists, AutoPtr<OptionList>, optionList)
				if(typeLibEnum.enumDefName == optionList->name)
				{
					AutoPtr<EnumInfo> synthesized = NEW EnumInfo;
					for(const OptionList::ScannedOption& item : optionList->items)
					{
						EnumInfo::ScannedEnumerator enumItem;
						enumItem.name = item.skinName;
						enumItem.docu = item.docu;
						synthesized->items.append (enumItem);
					}
					typeLibEnum.info = synthesized;
					break;
				}
			EndFor
		}
	}

	// Find docu for entries and create links back to matching elements - both are now entries in the
	// same 'entries' list, so guard against matching one against itself.

	auto addCrossLink = [] (ScannedEntry* target, CStringRef linkTag)
	{
		if(target->docu == nullptr)
		{
			target->docu = AutoPtr<DocuSnippet> (NEW DocuSnippet (target->elementClass.isEmpty () == false ? kTypeLibClass : kTypeLibEntry));
			target->docu->linksOnly = true;
		}
		String link (linkTag);
		if(target->docu->links.contains (link) == false)
			target->docu->links.add (link);
	};

	ListForEach (entries, AutoPtr<ScannedEntry>, entry)
		ScannedEntry* entryPtr = entry;
		ListForEach (entries, AutoPtr<ScannedEntry>, element)
			ScannedEntry* elementPtr = element;
			if(elementPtr != entryPtr && element->tagName == entry->uniqueName)
			{
				addCrossLink (element, entry->tagName);
				addCrossLink (entry, element->tagName);
				break;
			}
		EndFor
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GuiDocuScanner::handleMacros (SourceLexer& reader, SourceFileParser& parser, SourceToken& token)
{
	if(token.type == SourceToken::kIdentifier && token.text == "BEGIN_STYLEDEF")
		return parseStyleDef (reader, parser, token);

	return SuperClass::handleMacros (reader, parser, token);
}

