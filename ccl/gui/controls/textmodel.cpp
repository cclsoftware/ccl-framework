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
// Filename    : ccl/gui/controls/textmodel.cpp
// Description : Text Models
//
//************************************************************************************************

#include "ccl/gui/controls/textmodel.h"

#include "ccl/gui/graphics/textlayoutbuilder.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// TextModel
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (TextModel, Object)

//************************************************************************************************
// StringTextModel
//************************************************************************************************

void CCL_API StringTextModel::toDisplayString (String& string) const
{
	string = getDataString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StringTextModel::insertText (int textIndex, StringRef text, EditOptions options)
{
	if(textIndex < 0 || textIndex > getDataString ().length ())
	{
		ASSERT (false)
		return 0;
	}

	if(!(options & kMergeUndo))
		saveUndoState ();

	getDataString ().insert (textIndex, text);
	onChanged ();

	return text.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StringTextModel::removeText (int textIndex, int length, EditOptions options)
{
	if(!verifyRemoveTextIndex (textIndex, length))
		return 0;

	if(!(options & kMergeUndo))
		saveUndoState ();

	getDataString ().remove (textIndex, length);
	onChanged ();

	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringTextModel::undo ()
{
	if(undoIndex <= 0)
		return false;

	--undoIndex;
	onChanged ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringTextModel::redo ()
{
	if(undoIndex >= undoStack.count () - 1)
		return false;

	++undoIndex;
	onChanged ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringTextModel::fromParamString (StringRef string)
{
	if(getDataString () != string)
	{
		undoStack.setCount (0);
		undoIndex = -1;
		getDataString () = string;
		onChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringTextModel::toParamString (String& string) const
{
	string = getDataString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringTextModel::addObserver (IObserver* observer)
{
	wasObserved = true;
	Object::addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/bool StringTextModel::verifyRemoveTextIndex (int& index, int& length)
{
	if(length < 0)
	{
		length = -length;
		index -= length;
	}

	if(index < 0)
	{
		ASSERT (false)
		length += index;
		index = 0;
	}

	return length > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringTextModel::getDataString () const
{
	return const_cast<StringTextModel*> (this)->getDataString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& StringTextModel::getDataString ()
{
	if(undoIndex < 0)
	{
		undoIndex = 0;
		undoStack.add (String::kEmpty);
	}

	return undoStack.at (undoIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTextModel::onChanged ()
{
	if(wasObserved) // skip useless lookup in SignalHandler if nobody cares (e.g. inside TextBox constructor)
		signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringTextModel::saveUndoState ()
{
	// copy data string since setCount potentially reallocates memory and invalidates the reference
	String dataString (getDataString ());
	undoStack.setCount (++undoIndex);
	undoStack.add (dataString);
}

//************************************************************************************************
// MarkupTextModel
//************************************************************************************************

void CCL_API MarkupTextModel::toDisplayString (String& string) const
{
	string = getMarkupParser ().getPlainText ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkupTextModel::updateLayout (ITextLayout& textLayout) const
{
	textLayout.clearFormatting ();

	TextLayoutBuilder builder (textLayout);
	getMarkupParser ().applyFormatting (builder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MarkupTextModel::insertText (int textIndex, StringRef text, EditOptions options)
{
	String escapedText (text);
	MarkupTags::escapePlainText (escapedText);
	int markupIndex = getMarkupParser ().getMarkupPosition (textIndex, true);
	StringTextModel::insertText (markupIndex, escapedText, options);

	return text.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MarkupTextModel::removeText (int textIndex, int length, EditOptions options)
{
	if(!verifyRemoveTextIndex (textIndex, length))
		return 0;

	int markupStart = getMarkupParser ().getMarkupPosition (textIndex, false);
	int markupEnd = getMarkupParser ().getMarkupPosition (textIndex + length, true);
	int markupLength = markupEnd - markupStart;
	StringTextModel::removeText (markupStart, markupLength, options);

	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkupParser& MarkupTextModel::getMarkupParser () const
{
	if(markupParser == nullptr)
		markupParser = NEW MarkupParser (getDataString ());
	else if(markupDirty)
		markupParser->parse (getDataString ());

	markupDirty = false;

	return *markupParser;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MarkupTextModel::onChanged ()
{
	markupDirty = true;
	StringTextModel::onChanged ();
}

//************************************************************************************************
// PasswordTextModel
//************************************************************************************************

void CCL_API PasswordTextModel::toDisplayString (String& string) const
{
	String blindText (kPasswordReplacementString, getDataString ().length ());
	string = blindText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PasswordTextModel::copyText (String& text, int textIndex, int length) const
{
	// not allowed
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const String PasswordTextModel::kPasswordReplacementString (Text::kUTF8, u8"\u25CF");

//************************************************************************************************
// ProxyTextModel
//************************************************************************************************

ProxyTextModel::ProxyTextModel (ITextModelModifier* modifier, IMutableTextModel* internalModel)
: modifier (modifier),
  internalModel (internalModel),
  wasObserved (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ProxyTextModel::~ProxyTextModel ()
{
	if(wasObserved)
	{
		UnknownPtr<ISubject> subject (internalModel);
		if(subject)
			subject->removeObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::toDisplayString (String& string) const
{
	internalModel->toDisplayString (string);
	modifier->modifyDisplayString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::updateLayout (ITextLayout& textLayout) const
{
	internalModel->updateLayout (textLayout);
	modifier->modifyLayout (textLayout);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::copyText (String& text, int textIndex, int length) const
{
	internalModel->copyText (text, textIndex, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ProxyTextModel::insertText (int textIndex, StringRef text, EditOptions options)
{
	return internalModel->insertText (textIndex, text, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ProxyTextModel::removeText (int textIndex, int length, EditOptions options)
{
	return internalModel->removeText (textIndex, length, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProxyTextModel::undo ()
{
	return internalModel->undo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProxyTextModel::redo ()
{
	return internalModel->redo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ProxyTextModel::onTextInteraction (const ITextLayout& layout, const InteractionInfo& info)
{
	return internalModel->onTextInteraction (layout, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::fromParamString (StringRef string)
{
	internalModel->fromParamString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::toParamString (String& string) const
{
	internalModel->toParamString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::addObserver (IObserver* observer)
{
	if(!wasObserved)
	{
		wasObserved = true;
		UnknownPtr<ISubject> subject (internalModel);
		if(subject)
			subject->addObserver (this);
	}

	Object::addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ProxyTextModel::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<ITextBackgroundRenderer> ())
		return internalModel->queryInterface (iid, ptr);
	
	QUERY_INTERFACE (ITextModel)
	QUERY_INTERFACE (IMutableTextModel)

	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ProxyTextModel::notify (ISubject* subject, MessageRef msg)
{
	if(wasObserved)
		signal (msg);
}
