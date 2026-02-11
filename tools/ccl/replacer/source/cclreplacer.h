//************************************************************************************************
//
// CCL Replacer
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
// Filename    : cclreplacer.h
// Description : Replacer Tool
//
//************************************************************************************************

#ifndef _cclreplacer_h
#define _cclreplacer_h

#include "ccl/extras/tools/toolhelp.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/storage/file.h"

namespace CCL {

//************************************************************************************************
// HeaderRecipe
//************************************************************************************************

class HeaderRecipe: public JsonStorableObject
{
public:
	HeaderRecipe ();

	struct Replacement
	{
		enum ReplaceMode { kReplace, kRemove, RemovePlusOne };
		ReplaceMode replaceMode = kReplace;
		AutoPtr<SearchDescription> searchDescription;
		Vector<String> replaceStrings;
	};

	PROPERTY_STRING (rootFolder, RootFolder)
	PROPERTY_VARIABLE (int, maxLineCount, MaxLineCount)
	const FileTypeFilter& getFileTypes () const { return fileTypes; }
	const Vector<String>& getCommentStyles () const { return commentStyles; }
	const Vector<Replacement>& getReplacements () const { return replacements; }

	bool startsWithCommentStyle (String& outStyle, StringRef candidate) const;
	const Replacement* matchesReplacements (StringRef candidate) const;

	// JsonStorableObject
	bool load (const Storage& storage) override;

protected:
	FileTypeFilter fileTypes;
	Vector<String> commentStyles;
	Vector<Replacement> replacements;
};

//************************************************************************************************
// ReplacerTool
//************************************************************************************************

class ReplacerTool: public CommandLineTool
{
public:
	ReplacerTool ();

	enum Mode
	{
		kHeaderMode
	};

	PROPERTY_VARIABLE (Mode, mode, Mode)
	PROPERTY_STRING (recipeFile, RecipeFile)

	bool run ();

protected:
	struct HeaderModeStats
	{
		int64 totalFileCount = 0;
		int64 modifiedFileCount = 0;
	};

	bool runHeadeMode ();
	bool runHeaderReplacementRecursive (HeaderModeStats& stats, UrlRef folder, const HeaderRecipe& recipe);
	bool replaceFileHeader (HeaderModeStats& stats, UrlRef path, const HeaderRecipe& recipe);
};

} // namespace CCL

#endif // _cclreplacer_h
