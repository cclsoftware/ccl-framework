//********************************************************************************************manager//
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
// Filename    : ccl/platform/shared/skia/skiafontmanager.h
// Description : Skia Font Manager Factory
//
//************************************************************************************************

#ifndef _ccl_skia_fontmanager_h
#define _ccl_skia_fontmanager_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"

#include "ccl/platform/shared/skia/skiaglue.h"

namespace CCL {

//************************************************************************************************
// SkiaFontManagerFactory
//************************************************************************************************

class SkiaFontManagerFactory
{
public:
	static sk_sp<SkFontMgr> createFontManager ();
};

//************************************************************************************************
// SkiaFontManager
//************************************************************************************************

class SkiaFontManager: public SkFontMgr
{
public:
	SkiaFontManager ();
	~SkiaFontManager ();

	void addTypeface (sk_sp<SkTypeface> typeface, CStringRef name);

protected:
	sk_sp<SkFontMgr> fontManager;

	class StyleSet: public SkFontStyleSet
	{
	public:
		PROPERTY_MUTABLE_CSTRING (name, Name)

		void add (sk_sp<SkTypeface> typeface);

		// SkFontStyleSet
		int count () override;
		void getStyle (int index, SkFontStyle* style, SkString* styleName) override;
		sk_sp<SkTypeface> createTypeface (int index) override;
		sk_sp<SkTypeface> matchStyle (const SkFontStyle& pattern) override;

	private:
		Vector<sk_sp<SkTypeface>> faces;
	};

	Vector<sk_sp<StyleSet>> userFonts;
	mutable Vector<sk_sp<StyleSet>> fontCache;
	mutable Vector<MutableCString> missingFontsCache;

	// SkFontMgr
	int onCountFamilies () const override;
	void onGetFamilyName (int index, SkString* familyName) const override;
	sk_sp<SkFontStyleSet> onCreateStyleSet (int index) const override;
	sk_sp<SkFontStyleSet> onMatchFamily (const char familyName[]) const override;
	sk_sp<SkTypeface> onMatchFamilyStyle (const char familyName[], const SkFontStyle& style) const override;
	sk_sp<SkTypeface> onMatchFamilyStyleCharacter (const char familyName[], const SkFontStyle& style, const char* bcp47[], int bcp47Count, SkUnichar character) const override;
	sk_sp<SkTypeface> onMakeFromStreamIndex (std::unique_ptr<SkStreamAsset> stream, int ttcIndex) const override;
	sk_sp<SkTypeface> onMakeFromStreamArgs (std::unique_ptr<SkStreamAsset> stream, const SkFontArguments& args) const override;
	sk_sp<SkTypeface> onMakeFromData (sk_sp<SkData> data, int ttcIndex) const override;
	sk_sp<SkTypeface> onMakeFromFile (const char path[], int ttcIndex) const override;
	sk_sp<SkTypeface> onLegacyMakeTypeface (const char familyName[], SkFontStyle style) const override;
};

} // namespace CCL

#endif // _ccl_skia_fontmanager_h
