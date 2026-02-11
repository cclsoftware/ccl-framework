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
// Filename    : cclgenerator.cpp
// Description : Generator Tool
//
//************************************************************************************************

#include "cclgenerator.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/stringtemplate.h"

#include "ccl/extras/modeling/cplusplus.h"
#include "ccl/extras/modeling/classrepository.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/logging.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPORT_BINARY_RESOURCE (BuiltInLanguages) // builtinlanguages.cpp

//************************************************************************************************
// LanguageTypeFilter
/** Map meta type to language specific type. */
//************************************************************************************************

class LanguageTypeFilter: public StringTemplateFilter
{
public:
	LanguageTypeFilter (const LanguageConfig& _config);

	// StringTemplateFilter
	StringID getID () const override;
	void apply (Variant& value, const Attributes* context) override;

protected:
	MutableCString id; // language identifier + "type"
	const LanguageConfig& config;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguageTypeFilter::LanguageTypeFilter (const LanguageConfig& _config)
: config (_config)
{
	id.append (_config.getLanguageID ());
	id.append (CSTR ("type"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID LanguageTypeFilter::getID () const
{
	return id;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguageTypeFilter::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
	{
		ASSERT (false)
		return;
	}

	if(value == MetaFileFormat::kValueTypeBool)
		value = config.getBoolType ();
	else if(value == MetaFileFormat::kValueTypeInt)
		value = config.getIntType ();
	else if(value == MetaFileFormat::kValueTypeBigInt)
		value = config.getBigIntType ();
	else if(value == MetaFileFormat::kValueTypeFloat)
		value = config.getFloatType ();
	else if(value == MetaFileFormat::kValueTypeDouble)
		value = config.getDoubleType ();
	else if(value == MetaFileFormat::kValueTypeBigInt)
		value = config.getBigIntType ();
	else if(value == MetaFileFormat::kValueTypeString)
		value = config.getStringType ();
	else
	{
		ASSERT (false)
	}
}

//************************************************************************************************
// LanguageValueFilter
/** Map value with respect to type type to language conform value. */
//************************************************************************************************

class LanguageValueFilter: public StringTemplateFilter
{
public:
	LanguageValueFilter (const LanguageConfig& _config);

	// StringTemplateFilter
	StringID getID () const override;
	void apply (Variant& value, const Attributes* context) override;

protected:
	MutableCString id; // language identifier + "value"
	const LanguageConfig& config;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguageValueFilter::LanguageValueFilter (const LanguageConfig& _config)
: config (_config)
{
	id.append (_config.getLanguageID ());
	id.append (CSTR ("value"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID LanguageValueFilter::getID () const
{
	return id;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguageValueFilter::apply (Variant& value, const Attributes* context)
{
	// Handle expressions. These can be anything and should
	// never be exported in quotes.

	Variant expressionAttr;
	context->getAttribute (expressionAttr, MetaFileFormat::kAttrExpression);
	if(expressionAttr.asBool ())
		return;

	// Expect value to always be a string, even for numerical values.

	ASSERT (value.isString ())

	String valueType = context->getString (MetaFileFormat::kAttrType);
	if(valueType == MetaFileFormat::kValueTypeString)
	{
		String valueString = value.asString ();
		String quotedString = String () << "\"" << valueString << "\"";
		value.fromString (quotedString);
		return;
	}
	else if(valueType == MetaFileFormat::kValueTypeBool)
	{
		if(value.asBool ())
			value.fromString (config.getBoolValueTrue ());
		else
			value.fromString (config.getBoolValueFalse ());
		return;
	}

	// Other types do not require extra steps. Export
	// value string as is.
}

//************************************************************************************************
// IdentifierFilter
/** Map to language agnostic but any language compatible identifier. */
//************************************************************************************************

class IdentifierFilter: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override;
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID IdentifierFilter::getID () const
{
	return CSTR ("identifier");
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void IdentifierFilter::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
	{
		ASSERT (false)
		return;
	}

	// Generated identifier should be language agnostic but C++ 
	// ValidName format should work for most languages.

	String validName = Model::Cpp::ValidName (value.toString ());
	value.fromString (validName);
}

//************************************************************************************************
// SentenceFilter
/** Make value string start with capital letter and end with a period. */
//************************************************************************************************

class SentenceFilter: public StringTemplateFilter
{
public:
	// StringTemplateFilter
	StringID getID () const override;
	void apply (Variant& value, const Attributes* context) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID SentenceFilter::getID () const
{
	return CSTR ("sentence");
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void SentenceFilter::apply (Variant& value, const Attributes* context)
{
	if(!value.isString ())
	{
		ASSERT (false)
		return;
	}

	String formatted = value.toString ();
	if(formatted.isEmpty ())
		return;

	String firstLetter = formatted.subString (0, 1);
	formatted.remove (0, 1);
	formatted.prepend (firstLetter.capitalize ());
	if(!formatted.endsWith ("."))
		formatted << ".";

	value.fromString (formatted);
}

//************************************************************************************************
// DeconstifyFilter
/** Strip "k" from a string identifying as a CCL constant name. */
//************************************************************************************************

class DeconstifyFilter: public StringTemplateFilter
{
public:
	DeconstifyFilter ();

	// StringTemplateFilter
	StringID getID () const override;
	void apply (Variant& value, const Attributes* context) override;

protected:
	AutoPtr<IRegularExpression> constantPattern;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DeconstifyFilter::DeconstifyFilter ()
{
	// Assumed CCL constant name pattern: string starts with 
	// lower case 'k' followed by an uppercase character.

	constantPattern = System::CreateRegularExpression ();
	constantPattern->construct ("^k[A-Z]");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID DeconstifyFilter::getID () const
{
	return CSTR ("deconstify");
};

//////////////////////////////////////////////////////////////////////////////////////////////////

void DeconstifyFilter::apply (Variant& value, const Attributes* context)
{
	if(value.isString () == false)
		return;

	ASSERT (constantPattern != nullptr)
	if(constantPattern == nullptr)
		return;

	String valueString = value.toString ();
	if(constantPattern->isPartialMatch (valueString) == false)
		return;

	String modifiedValue = valueString.subString (1);
	value.fromString (modifiedValue);
}

//************************************************************************************************
// FourCCFunction
/** Compute four character code (int). */
//************************************************************************************************

class FourCCFunction: public MetaModelFunction
{
public:
	DECLARE_STRINGID_MEMBER (kFunctionID)

	// MetaModelFunction
	bool run (Variant& result, String& hint, const Vector<Variant>& args) const override;

protected:
	static int fourcc (char a, char b, char c, char d);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (FourCCFunction, kFunctionID, MetaFileFormat::kFunctionIDFourCC)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FourCCFunction::run (Variant& result, String& comment, const Vector<Variant>& args) const
{
	if(args.count () != 1)
		return false;

	// Arg must provide four characters, example: "divx".
	String code = args.at (0);
	if(code.isEmpty () || code.length () != 4)
		return false;

	int cc = fourcc (code.at (0), code.at (1), code.at (2), code.at (3));
	result = Variant (cc);
	comment = String () << "FourCC ('" << code << "')";

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FourCCFunction::fourcc (char a, char b, char c, char d)
{
	return ((a * 256 + b) * 256 + c) * 256 + d;
}

//************************************************************************************************
// MetaFile
//************************************************************************************************

const FileType& MetaFile::getFileType ()
{
	return JsonArchive::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModel::Root& MetaFile::getModelRoot ()
{
	return modelRoot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MetaFile::load (IStream& stream)
{
	JsonArchive archive (stream, JsonArchive::kKeepDuplicateKeys);
	return archive.loadObject (nullptr, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaFile::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	modelRoot.load (a);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaFile::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	modelRoot.save (a);

	return true;
}

//************************************************************************************************
// ModelProcessor
//************************************************************************************************

void MetaModelProcessor::run (MetaModel::Root& root)
{
	traverse (root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModelProcessor::traverse (MetaModel::Root& root)
{
	// Group recursion.
	ForEach (root.getGroups (), MetaModel::Group, group)
		traverse (*group);
	EndFor

	ForEach (root.getConstants (), MetaModel::Constant, constant)
		resolveFunction (*constant);
	EndFor

	ForEach (root.getDefinitions (), MetaModel::Definition, definition)
		resolveFunction (*definition);
	EndFor

	ForEach (root.getEnums (), MetaModel::Enumeration, enumeration)
		int enumeratorIndex = 0;
		ForEach (enumeration->getEnumerators (), MetaModel::Enumerator, enumerator)
			resolveFunction (*enumerator);
			if(enumeration->isAutoValue ())
				setAutoValue (*enumerator, enumeratorIndex);
			enumeratorIndex++;
		EndFor
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModelProcessor::resolveFunction (MetaModel::Assignment& assignment)
{
	// Resolve assignment that requires a computed value, i.e.
	// the model only specifies a specific function to run.

	MetaModel::ValueFunction* valueFunction = assignment.getValueFunction ();
	if(valueFunction == nullptr)
		return;

	MutableCString functionId (valueFunction->getName ());
	AutoPtr<MetaModelFunction> function = createFunction (functionId);
	if(function == nullptr)
		return;

	Variant result;
	String hint;
	if(!function->run (result, hint, valueFunction->getArgs ()))
		return;

	assignment.setValue (result.toString ());
	assignment.setBrief (String () << hint << ", " << assignment.getBrief ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaModelProcessor::setAutoValue (MetaModel::Enumerator& enumerator, int value)
{
	if(!enumerator.getValue ().isEmpty ())
		return;

	String valueString;
	valueString << value;

	enumerator.setValue (valueString);
	enumerator.setValueType (MetaFileFormat::kValueTypeInt);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaModelFunction* MetaModelProcessor::createFunction (StringID functionId)
{
	if(functionId == FourCCFunction::kFunctionID)
		return NEW FourCCFunction;

	// Unexpected function.
	ASSERT (false)
	return nullptr;
}

//************************************************************************************************
// GeneratorTool
//************************************************************************************************

GeneratorTool::GeneratorTool (StringRef appName)
: appName (appName),
  mode (Mode::kGenerate)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::run ()
{
	Url inputPath, outputPath;
	inputPath.fromDisplayString (inputFile);
	makeAbsolute (inputPath);
	outputPath.fromDisplayString (outputFile);
	makeAbsolute (outputPath);

	if(mode == Mode::kParse)
	{
		if(parse (outputPath, inputPath))
		{
			Logging::info ("Wrote output file '%(1)'", UrlDisplayString (outputPath));
			return true;
		}
		else
			return false;
	}
	else
	{
		Url templatePath;
		templatePath.fromDisplayString (templateFile);
		makeAbsolute (templatePath);
		return generate (outputPath, inputPath, templatePath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::loadInput (MetaFile& inputFile, UrlRef inputPath)
{
	const FileType& fileType = inputPath.getFileType ();
	if(fileType == MetaFile::getFileType ())
		return inputFile.loadFromFile (inputPath);
	else
	{
		// convert other formats on the fly
		TempFile tempFile (LegalFileName (appName) << "." << MetaFile::getFileType ().getExtension ());
		if(parse (tempFile.getPath (), inputPath))
			return inputFile.loadFromFile (tempFile.getPath ());
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::generate (UrlRef outputPath, UrlRef inputPath, UrlRef templatePath)
{
	// Meta model input file.
	MetaFile metaFile;
	if(!loadInput (metaFile, inputPath))
	{
		Logging::error ("Failed to load meta model file '%(1)'", UrlDisplayString (inputPath));
		return false;
	}
	else
	{
		Logging::info ("Using meta model file '%(1)'", UrlDisplayString (inputPath));
	}

	StringTemplateEnvironment environment;
	environment.setOption (StringTemplate::kOptionTrimBlocks, true);

	// Set a templates working folder to enable use of {% include %}.
	Url templatesFolder = templatePath;
	templatesFolder.ascend ();
	environment.setTemplatesFolder (templatesFolder);

	auto registerFilter = [&environment] (StringTemplateFilter* filter)
	{
		environment.registerFilter (filter);
		Logging::debug ("Added string template filter '%(1)'", String (filter->getID ()));
	};

	registerFilter (NEW IdentifierFilter);
	registerFilter (NEW SentenceFilter);
	registerFilter (NEW DeconstifyFilter);

	LanguageConfigRegistry& registry = LanguageConfigRegistry::instance ();
	registry.loadBuiltIns ();
	if(registry.countLanguages () == 0)
		Logging::warning ("No language configs found, language template filters may not work");
	
	// Add filters to convert meta model data types and values to language specific format.
	IterForEach (LanguageConfigRegistry::instance ().newIterator (), LanguageConfig, config)
		registerFilter (NEW LanguageTypeFilter (*config)); // "cpptype", "javatype", ...
		registerFilter (NEW LanguageValueFilter (*config)); // "cppvalue", "javavalue", ...
	EndFor

	// Template "frame" file.
	AutoPtr<StringTemplate> stringTemplate = environment.loadTemplate (templatePath);
	if(stringTemplate == nullptr)
	{
		Logging::error ("Failed to open template file '%(1)'", UrlDisplayString (templatePath));
		return false;
	}
	else
	{
		Logging::info ("Using template file '%(1)'", UrlDisplayString (templatePath));
	}

	// Output file intended to use line endings from output string
	// to preserve line endings from template file.

	TextFile outputFile (outputPath, Text::kASCII, Text::kUnknownLineFormat);
	if(!outputFile.isValid ())
	{
		Logging::error ("Failed to create output file '%(1)'", UrlDisplayString (outputPath));
		return false;
	}

	MetaModel::Root& model = metaFile.getModelRoot ();

	MetaModelProcessor::run (model);

	Attributes templateData;

	// Save model to attributes.
	model.save (templateData);

	// Add additional meta attributes
	templateData.set ("appname", appName);

	String inputFileName;
	inputPath.getName (inputFileName);
	templateData.set ("inputfile", inputFileName);

	String outputFileName;
	outputPath.getName (outputFileName);
	templateData.set ("outputfile", outputFileName);
	
	String renderedTemplate = stringTemplate->render (templateData);
	outputFile->writeString (renderedTemplate);

	Logging::info ("Wrote output file '%(1)'", UrlDisplayString (outputPath));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::parse (UrlRef outputPath, UrlRef inputPath)
{
	MetaFile outputFile;
	bool parseResult = false;

	Logging::info ("Parsing input file '%(1)", UrlDisplayString (inputPath));

	const FileType& fileType = inputPath.getFileType ();
	if(fileType == Model::Cpp::HeaderFile () || fileType == Model::Cpp::SourceFile ())
		parseResult = parseCpp (outputFile, inputPath);
	else if(fileType == Model::ClassRepository::getFileType ())
		parseResult = parseClassModel (outputFile, inputPath);
	else
	{
		Logging::error ("Unsupported input file type '%(1)'", fileType.getExtension ());
		return false;
	}

	if(!parseResult)
		return false;

	MetaModel::Root& model = outputFile.getModelRoot ();
	if(model.hasData ())
	{
		if(!outputFile.saveToFile (outputPath))
		{
			Logging::error ("Failed to create output file '%(1)'", UrlDisplayString (outputPath));
			return false;
		}
	}
	else
		Logging::info ("No data found.");

	// Do not log output file here, may be temporary file.
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::parseCpp (MetaFile& outputFile, UrlRef inputPath)
{
	// Parse define statements from source code. Limited to #define statements
	// that alias a string literal, like #define TAG_SKIN "Skin".

	TextFile sourceFile (inputPath, TextFile::kOpen);
	if(!sourceFile.isValid ())
	{
		Logging::error ("Failed to open input file '%(1)'", UrlDisplayString (inputPath));
		return false;
	}

	const String kDefine ("#define");
	const String kQuote ("\"");

	MetaModel::Root& model = outputFile.getModelRoot ();

	String line;
	while(sourceFile->readLine (line))
	{
		StringParser parser (line);
		parser.skipWhitepace ();
		if(!parser.readToken (kDefine))
			continue;

		String key, value;
		parser.skipWhitepace ();
		if(!parser.readUntilWhitespace (key))
			continue;
		if(!parser.skipUntil (kQuote))
			continue;
		parser.readUntil (value, kQuote);

		if(!key.isEmpty () && !value.isEmpty ())
		{
			MetaModel::Definition* definition = NEW MetaModel::Definition;
			definition->setName (MutableCString (key));

			Variant valueVar;
			valueVar.fromString (value);
			definition->setValue (value);
			definition->setValueType (MetaFileFormat::kValueTypeString);

			model.add (definition);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GeneratorTool::parseClassModel (MetaFile& outputFile, UrlRef inputPath)
{
	// Convert classmodel enumerations to meta model enumerations.

	Model::ClassRepository repo;
	if(!repo.loadFromFile (inputPath))
	{
		Logging::error ("Failed to load input file '%(1)'", UrlDisplayString (inputPath));
		return false;
	}

	MetaModel::Root& model = outputFile.getModelRoot ();

	for(auto* enumElement : iterate_as<Model::Enumeration> (repo.getEnumerations ()))
	{
		ObjectArray enumerators;
		enumElement->getEnumerators (enumerators, false);
		if(enumerators.isEmpty ())
			continue;

		MutableCString enumName = enumElement->getName ();
		MetaModel::Enumeration* enumeration = NEW MetaModel::Enumeration;

		// Uses enum name as is, can be "SomeClass" but also composite "SomeClass.someEnum".

		enumeration->setName (enumName);

		StringRef brief = enumElement->getDocumentation ().getBriefDescription ();
		if(!brief.isEmpty ())
			enumeration->setBrief (brief);

		StringRef details = enumElement->getDocumentation ().getDetailedDescription ();
		if(!details.isEmpty ())
			enumeration->setDetails (details);

		for(auto* enumeratorElement : iterate_as<Model::Enumerator> (enumerators))
		{
			MetaModel::Enumerator* enumerator = NEW MetaModel::Enumerator;

			// Name
			MutableCString name = enumeratorElement->getName ();
			enumerator->setName (name);

			// Value, classmodel stores int.
			VariantRef intValue = enumeratorElement->getValue ();
			ASSERT (intValue.isInt ())
			enumerator->setValue (intValue.toString ());
			enumerator->setValueType (MetaFileFormat::kValueTypeInt);

			// Documentation: brief
			StringRef brief = enumeratorElement->getDocumentation ().getBriefDescription ();
			if(!brief.isEmpty ())
				enumerator->setBrief (brief);

			// Documentation: details
			StringRef details = enumeratorElement->getDocumentation ().getDetailedDescription ();
			if(!details.isEmpty ())
				enumerator->setDetails (details);

			enumeration->addEnumerator (enumerator);
		}
		model.add (enumeration);
	}

	return true;
}

//************************************************************************************************
// LanguageConfigRegistry
//************************************************************************************************

LanguageConfigRegistry::LanguageConfigRegistry ()
{
	configs.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LanguageConfig* LanguageConfigRegistry::findConfig (StringID languageId) const
{
	auto matchesId = [&] (const LanguageConfig& c) { return c.getLanguageID () == languageId; };
	return configs.findIf<LanguageConfig> (matchesId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguageConfigRegistry::loadBuiltIns ()
{
	MemoryStream stream (BuiltInLanguages_Ptr, BuiltInLanguages_Size);
	Attributes attributes;
	stream.rewind ();
	if(JsonArchive (stream).loadAttributes (nullptr, attributes))
	{
		if(attributes.contains ("languages"))
		{
			auto* it = attributes.newQueueIterator ("languages", ccl_typeid<Attributes> ());
			IterForEach (it, Attributes, a)
			if(LanguageConfig* config = LanguageConfig::createFromAttributes (*a))
				add (config);
			EndFor
		}
	}
	else
		Logging::error ("Failed to load built-in languages");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LanguageConfigRegistry::countLanguages () const
{
	return configs.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* LanguageConfigRegistry::newIterator () const
{
	return configs.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LanguageConfigRegistry::add (LanguageConfig* config)
{
	// Overwrite case: config replaces existing config.
	LanguageConfig* existing = findConfig (config->getLanguageID ());
	if(existing != nullptr)
	{
		configs.remove (existing);
		safe_release (existing);
		Logging::debug ("Removed language config '%(1)'", String (config->getLanguageID ()));
	}

	// Transfer ownership.
	configs.add (config);
	Logging::debug ("Adding language config '%(1)'", String (config->getLanguageID ()));
}
