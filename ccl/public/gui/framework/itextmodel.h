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
// Filename    : ccl/public/gui/framework/itextmodel.h
// Description : Text Model Interface
//
//************************************************************************************************

#ifndef _ccl_itextmodel_h
#define _ccl_itextmodel_h

#include "ccl/public/gui/graphics/itextlayout.h"

namespace CCL {

interface IView;
struct GUIEvent;
interface ITextModel;

//************************************************************************************************
// ITextModelProvider
/** Text model provider interface.
	\ingroup gui */
//************************************************************************************************

interface ITextModelProvider: IUnknown
{
	virtual ITextModel* CCL_API getTextModel () = 0;

	virtual void CCL_API setTextModel (ITextModel* model, tbool update = false) = 0;

	DECLARE_IID (ITextModelProvider)
};

DEFINE_IID (ITextModelProvider, 0x694a2c48, 0x7e84, 0x434a, 0x85, 0xc4, 0x98, 0xfb, 0x5c, 0x80, 0x6d, 0x68)

//************************************************************************************************
// ITextModel
/** Text model interface for use in a TextBox and EditBox.
	\ingroup gui */
//************************************************************************************************

interface ITextModel: IUnknown
{
	/** Draw information. */
	struct DrawInfo
	{
		IView* view;
		IGraphics& graphics;
		const Rect& rect;
	};

	/** Interaction information. */
	struct InteractionInfo
	{
		IView* view;
		const GUIEvent& editEvent;
	};

	DEFINE_ENUM (EditOptions)
	{
		kMergeUndo = 1 << 0 ///< merge into previous undo step
	};

	/** Get plain string representation for display (without formatting). */
	virtual void CCL_API toDisplayString (String& string) const = 0;

	/** Update text layout with formatting. */
	virtual void CCL_API updateLayout (ITextLayout& layout) = 0;

	/**	Insert text into data model at given display text index.
		Returns the number of inserted characters and signals kChanged if successful. */
	virtual int CCL_API insertText (int textIndex, StringRef text, EditOptions options) = 0;

	/**	Remove text from data model at given display text index.
		Returns the number of removed characters and signals kChanged if successful.
		The length argument may be negative if the text should be removed backwards from the given index. */
	virtual int CCL_API removeText (int textIndex, int length, EditOptions options) = 0;

	/** Copy a range of text in a representation that is accepted as input for insertText. */
	virtual void CCL_API copyText (String& text, int textIndex = 0, int length = -1) const = 0;

	/**	Undo the last change.
		Returns true if the last change was caused by the text model and signals kChanged if successful. */
	virtual tbool CCL_API undo () = 0;

	/**	Redo the next change.
		Returns true if the next change was caused by the text model and signals kChanged if successful. */
	virtual tbool CCL_API redo () = 0;

	/** Draw additional background. */
	virtual tbool CCL_API drawBackground (const ITextLayout& layout, const DrawInfo& info) = 0;

	/** Text interaction notification. */
	virtual tbool CCL_API onTextInteraction (const ITextLayout& layout, const InteractionInfo& info) = 0;

	/** Get string representation for use in a parameter. */
	virtual void CCL_API toParamString (String& string) const = 0;

	/** Restore text from string representation of parameter. Signals kChanged if successful. */
	virtual void CCL_API fromParamString (StringRef string) = 0;

	DECLARE_STRINGID_MEMBER (kRequestLayoutUpdate) ///< request text layout update call from text control

	DECLARE_IID (ITextModel)
};

DEFINE_IID (ITextModel, 0xd1c7dcb2, 0x71d8, 0x44b0, 0xa7, 0x1d, 0x32, 0x17, 0xf3, 0xb9, 0x3, 0xae)
DEFINE_STRINGID_MEMBER (ITextModel, kRequestLayoutUpdate, "requestLayoutUpdate")

//************************************************************************************************
// AbstractTextModel
//************************************************************************************************

class AbstractTextModel: public ITextModel
{
public:
	// Note: toDisplayString() needs to be implemented by each TextModel

	// ITextModel
	void CCL_API updateLayout (ITextLayout& layout) override
	{}

	int CCL_API insertText (int textIndex, StringRef text, ITextModel::EditOptions) override
	{
		return 0;
	}

	int CCL_API removeText (int textIndex, int length, ITextModel::EditOptions) override
	{
		return 0;
	}

	void CCL_API copyText (String& text, int textIndex = 0, int length = -1) const override
	{
		toDisplayString (text);
		text = text.subString (textIndex, length);
	}

	tbool CCL_API undo () override
	{
		return false;
	}

	tbool CCL_API redo () override
	{
		return false;
	}

	tbool CCL_API drawBackground (const ITextLayout& layout, const DrawInfo& info) override
	{
		return false;
	}

	tbool CCL_API onTextInteraction (const ITextLayout& layout, const InteractionInfo& info) override
	{
		return false;
	}

	void CCL_API toParamString (String& string) const override
	{
		toDisplayString (string);
	}

	void CCL_API fromParamString (StringRef string) override
	{}
};

} // namespace CCL

#endif // _ccl_itextmodel_h
