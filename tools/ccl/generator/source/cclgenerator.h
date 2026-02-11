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
// Filename    : cclgenerator.h
// Description : Generator Tool
//
//************************************************************************************************

#ifndef _cclgenerator_h
#define _cclgenerator_h

#include "cclgeneratormodel.h"

#include "ccl/extras/tools/toolhelp.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/singleton.h"

namespace CCL {

//************************************************************************************************
// MetaFile
//************************************************************************************************

class MetaFile: public JsonStorableObject
{
public:
	static const FileType& getFileType ();

	MetaModel::Root& getModelRoot ();

	// JsonStorableObject
	tbool CCL_API load (IStream& stream) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	MetaModel::Root modelRoot; ///< model root element
};

//************************************************************************************************
// MetaModelFunction
/** A model processor function called in response to model 'function' attribute.
 Closely related to the meta model file format. Each 'function' supported by
 the meta model is expected to be associated with a processor function class. */
//************************************************************************************************

class MetaModelFunction: public Unknown
{
public:
	/** Calculate variant type result, generate a processing comment. */
	virtual bool run (Variant& result, String& comment, const Vector<Variant>& args) const = 0;
};

//************************************************************************************************
// MetaModelProcessor
/** Model post processing class, may alter model.
 Use to compute additional model values or perform integrity checks. */
//************************************************************************************************

class MetaModelProcessor
{
public:
	static void run (MetaModel::Root& root);

protected:
	static void traverse (MetaModel::Root& object);
	static void resolveFunction (MetaModel::Assignment& assignment);
	static void setAutoValue (MetaModel::Enumerator& enumerator, int value);

	// Function factory.
	static MetaModelFunction* createFunction (StringID functionId);
};

//************************************************************************************************
// GeneratorTool
//************************************************************************************************

class GeneratorTool: public CommandLineTool
{
public:
	GeneratorTool (StringRef appName);

	enum class Mode
	{
		kGenerate,
		kParse
	};

	PROPERTY_VARIABLE (Mode, mode, Mode)
	PROPERTY_STRING (inputFile, InputFile)
	PROPERTY_STRING (outputFile, OutputFile)
	PROPERTY_STRING (templateFile, TemplateFile)

	bool run ();

protected:
	String appName;

	bool loadInput (MetaFile& inputFile, UrlRef inputPath);
	bool generate (UrlRef outputPath, UrlRef inputPath, UrlRef templatePath);
	bool parse (UrlRef outputPath, UrlRef inputPath);
	bool parseCpp (MetaFile& outputFile, UrlRef inputPath);
	bool parseClassModel (MetaFile& outputFile, UrlRef inputPath);
};

//************************************************************************************************
// LanguageConfigRegistry
/* TODO, future: allow users to specificy additional configs. A user provided config
 should be able to replace a built-in config. */
//************************************************************************************************

class LanguageConfigRegistry: public StaticSingleton<LanguageConfigRegistry>
{
public:
	LanguageConfigRegistry ();

	LanguageConfig* findConfig (StringID languageId) const;
	void loadBuiltIns ();
	int countLanguages () const;
	CCL::Iterator* newIterator () const;
	void add (LanguageConfig* config);

protected:
	ObjectArray configs;
};

} // namespace CCL

#endif // _cclgenerator_h
