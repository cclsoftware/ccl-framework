//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/controls/textmodel.h
// Description : Text Models
//
//************************************************************************************************

#ifndef _ccl_textmodel_h
#define _ccl_textmodel_h

#include "ccl/base/object.h"

#include "ccl/gui/graphics/markupsupport.h"

#include "ccl/public/gui/framework/itextmodel.h"

namespace CCL {

//************************************************************************************************
// TextModel
//************************************************************************************************

class TextModel: public Object,
				 public AbstractTextModel
{
public:
	DECLARE_CLASS_ABSTRACT (TextModel, Object)

	CLASS_INTERFACE2 (ITextModel, IMutableTextModel, Object)
};

//************************************************************************************************
// StringTextModel
//************************************************************************************************

class StringTextModel: public TextModel
{
public:
	// IMutableTextModel
	void CCL_API toDisplayString (String& string) const override;
	int CCL_API insertText (int textIndex, StringRef text, IMutableTextModel::EditOptions options) override;
	int CCL_API removeText (int textIndex, int length, IMutableTextModel::EditOptions options) override;
	tbool CCL_API undo () override;
	tbool CCL_API redo () override;
	void CCL_API fromParamString (StringRef string) override;
	void CCL_API toParamString (String& string) const override;
	void CCL_API addObserver (IObserver* observer) override;

protected:
	static bool verifyRemoveTextIndex (int& index, int& length);
	StringRef getDataString () const;
	String& getDataString ();
	virtual void onChanged ();

private:
	Vector<String> undoStack;
	int undoIndex = -1;
	bool wasObserved = false;

	void saveUndoState ();
};

//************************************************************************************************
// MarkupTextModel
//************************************************************************************************

class MarkupTextModel: public StringTextModel
{
public:
	// IMutableTextModel
	void CCL_API toDisplayString (String& string) const override;
	void CCL_API updateLayout (ITextLayout& textLayout) const override;
	int CCL_API insertText (int textIndex, StringRef text, IMutableTextModel::EditOptions options) override;
	int CCL_API removeText (int textIndex, int length, IMutableTextModel::EditOptions options) override;

private:
	mutable AutoPtr<MarkupParser> markupParser;
	mutable bool markupDirty = false;

	MarkupParser& getMarkupParser () const;

	// StringTextModel
	void onChanged () override;
};

//************************************************************************************************
// PasswordTextModel
//************************************************************************************************

class PasswordTextModel: public StringTextModel
{
public:
	// IMutableTextModel
	void CCL_API toDisplayString (String& string) const override;
	void CCL_API copyText (String& text, int textIndex = 0, int length = -1) const override;

public:
	static const String kPasswordReplacementString;
};

//************************************************************************************************
// ITextModelModifier
//************************************************************************************************

interface ITextModelModifier: IUnknown
{
	virtual void modifyDisplayString (String& displayString) const = 0;

	virtual void modifyLayout (ITextLayout& textLayout) const = 0;

	DECLARE_IID (ITextModelModifier)
};

//************************************************************************************************
// ProxyTextModel
/** Redirects every call to an internal text model and allows an ITextModelModifier to modify
 its outputs. */
//************************************************************************************************

class ProxyTextModel: public TextModel
{
public:
	ProxyTextModel (ITextModelModifier* modifier, IMutableTextModel* internalModel);
	~ProxyTextModel ();

	// IMutableTextModel
	void CCL_API toDisplayString (String& string) const override;
	void CCL_API updateLayout (ITextLayout& textLayout) const override;
	void CCL_API copyText (String& text, int textIndex = 0, int length = -1) const override;
	int CCL_API insertText (int textIndex, StringRef text, IMutableTextModel::EditOptions options) override;
	int CCL_API removeText (int textIndex, int length, IMutableTextModel::EditOptions options) override;
	tbool CCL_API undo () override;
	tbool CCL_API redo () override;
	tbool CCL_API onTextInteraction (const ITextLayout& layout, const InteractionInfo& info) override;
	void CCL_API fromParamString (StringRef string) override;
	void CCL_API toParamString (String& string) const override;
	void CCL_API addObserver (IObserver* observer) override;

	// Object
	CLASS_INTERFACES (Object)

protected:
	SharedPtr<IMutableTextModel> internalModel;
	ITextModelModifier* modifier;
	bool wasObserved;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_textmodel_h
