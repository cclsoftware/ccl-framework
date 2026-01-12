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
// Filename    : ccl/gui/dialogs/commandeditor.cpp
// Description : Key Command Editor
//
//************************************************************************************************

#include "ccl/gui/dialogs/commandeditor.h"

#include "ccl/gui/help/keyglyphpainter.h"
#include "ccl/gui/controls/textbox.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/systemservices.h"

#include "ccl/app/params.h"

#define EDIT_INPLACE 1
#define CONFLICT_POPUP 0
#define CHECK_CMD_DUPLICATES DEBUG

#if CONFLICT_POPUP
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/gui/popup/popupselector.h"
#endif

using namespace CCL;

//************************************************************************************************
// CommandEditor::KeyParam
//************************************************************************************************

class CommandEditor::KeyParam: public Parameter
{
public:
	DECLARE_CLASS (KeyParam, Parameter)

	KeyParam (StringID name = nullptr);

	const KeyEvent& getKey () const;
	void setKey (const KeyEvent& keyEvent, tbool update);

	// IParameter
	Variant CCL_API getValue () const override;
	void CCL_API setValue (VariantRef value, tbool update) override;
	void CCL_API getString (String& string, VariantRef value) const override;

protected:
	AutoPtr<Boxed::KeyEvent> key;
};

//************************************************************************************************
// CommandEditor::KeyEditBox
//************************************************************************************************

class CommandEditor::KeyEditBox: public TextBox
{
public:
	typedef TextBox SuperClass;

	KeyEditBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	bool setKey (const KeyEvent& event, tbool update);

	// View
	bool onKeyDown (const KeyEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;

	// TextBox
	StringRef getText () override { return String::kEmpty; }
};

//************************************************************************************************
// CommandEditor::InplaceKeyEditBox
//************************************************************************************************

class CommandEditor::InplaceKeyEditBox: public CommandEditor::KeyEditBox
{
public:
	typedef KeyEditBox SuperClass;

	InplaceKeyEditBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);

	// View
	void onMove (const Point& delta) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
};

//************************************************************************************************
// CommandEditor::BindingsListModel
//************************************************************************************************

class CommandEditor::BindingsListModel: public Object,
										public ItemViewObserver<AbstractItemModel>
{
public:
	BindingsListModel () : target (nullptr)
	{
		keyItems.objectCleanup (true);
	}

	PROPERTY_POINTER (CommandEditor, target, Target)

	class KeyItem: public Object
	{
	public:
	
		PROPERTY_STRING (title, Title)
		PROPERTY_BOOL (enabled, Enabled)
		PROPERTY_OBJECT (KeyEvent, key, Key)
	};

	int getSelectedIndex ()
	{
		if(IItemView* listView = getItemView ())
		{
			ForEachItem (listView->getSelection (), index)
				return index.getIndex ();
			EndFor
		}
		return -1;
	}
	
	void addItem (KeyItem* item)
	{
		keyItems.add (item);
	}
	
	void removeAll ()
	{
		keyItems.removeAll ();
	}
	
	bool isEmpty () const
	{
		return keyItems.isEmpty ();
	}
		
	KeyItem* getKeyItem (ItemIndexRef index)
	{
		return ((KeyItem*)keyItems.at (index.getIndex ()));
	}
	
	// AbstractItemModel
	int CCL_API countFlatItems () override
	{
		return keyItems.count ();
	}
	
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override
	{
		if(KeyItem* item = getKeyItem (index.getIndex ()))
		{
			title = item->getTitle ();
			return true;
		}
		return false;
	}

	tbool CCL_API onItemFocused (ItemIndexRef index) override
	{
		if(getKeyItem (index) && target)
			target->onKeyItemFocused (index.getIndex ());
		return true;
	}

	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override
	{
		KeyItem* item = getKeyItem (index);
		if(item && column == 0)
		{
			Rect rect (info.rect);
			rect.left += 3;
			KeyGlyphPainter (info.style.font, info.style.textBrush).drawKeyGlyphs (info.graphics, rect, item->getKey (), Alignment::kLeft|Alignment::kVCenter);
		}
		return true;
	}
	
	CLASS_INTERFACE (IItemModel, Object)
	
protected:
	
	ObjectArray keyItems;
};


//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("CommandEditor")
	XSTRING (modified, "(modified)")
	XSTRING (KeyIsAlreadyAssignedTo, "Key is already assigned to:\n%(1)")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum CommandEditorTags
	{
		kKey = 200,
		kAssignKey,
		kRemoveKey,
		kShowCommand,
		kCommandTitle,
		kKeyUsedInfo,
		kHasKeyUsedInfo,
		kModification,
		kReset
	};
}

//************************************************************************************************
// CommandEditor::KeyParam
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CommandEditor::KeyParam, Parameter)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandEditor::KeyParam::KeyParam (StringID name)
: Parameter (name),
  key (NEW Boxed::KeyEvent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const KeyEvent& CommandEditor::KeyParam::getKey () const
{
	return *key;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::KeyParam::setKey (const KeyEvent& keyEvent, tbool update)
{
	if(!(keyEvent == *key))
	{
		*key = keyEvent;
		deferChanged ();

		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API CommandEditor::KeyParam::getValue () const
{
	Variant v (ccl_as_unknown (key));
	v.share ();
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditor::KeyParam::setValue (VariantRef value, tbool update)
{
	if(Boxed::KeyEvent* key = unknown_cast<Boxed::KeyEvent> (value.asUnknown ()))
		setKey (*key, update);
	else
		setKey (KeyEvent (), update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditor::KeyParam::getString (String& string, const Variant& value) const
{
	if(Boxed::KeyEvent* key = unknown_cast<Boxed::KeyEvent> (value.asUnknown ()))
		key->CCL::KeyEvent::toString (string, true);
}

//************************************************************************************************
// CommandEditor::KeyEditBox
//************************************************************************************************

CommandEditor::KeyEditBox::KeyEditBox (const Rect& size, IParameter* param, StyleRef style)
: TextBox (size, param, style)
{
	wantsFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandEditor::KeyEditBox::setKey (const KeyEvent& event, tbool update)
{
	if(param)
	{
		AutoPtr<Boxed::KeyEvent> key (NEW Boxed::KeyEvent (event));
		key->character = Unicode::toUppercase (key->character);
		Variant v (ccl_as_unknown (key));
		v.share ();
		param->setValue (v, update);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandEditor::KeyEditBox::onKeyDown (const KeyEvent& event)
{
	if(!isValidCommandKey (event)) // swallow reserved keys
		return true;

	setKey (event, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::KeyEditBox::draw (const UpdateRgn& updateRgn)
{
	Rect rect;
	getClientRect (rect);
	GraphicsPort graphics (this);

	if(isFocused ())
		graphics.drawRect (rect, Pen (getTheme ().getThemeColor (ThemeElements::kSelectionColor)));

	if(param)
	{
		Font font = getVisualStyle ().getTextFont ();
		SolidBrush brush = getVisualStyle ().getTextBrush ();
		
		CommandEditor::KeyParam* keyParam = unknown_cast<CommandEditor::KeyParam> (param);
		if(keyParam)
			KeyGlyphPainter (font, brush).drawKeyGlyphs (graphics, rect, keyParam->getKey (), Alignment::kCenter);
		else
		{
			String text;
			param->toString (text);
			graphics.drawString (rect, text, font, brush, Alignment::kCenter);
		}
	}
}

//************************************************************************************************
// CommandEditor::InplaceKeyEditBox
//************************************************************************************************

CommandEditor::InplaceKeyEditBox::InplaceKeyEditBox (const Rect& size, IParameter* param, StyleRef style)
: KeyEditBox (size, param,style)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::InplaceKeyEditBox::onMove (const Point& delta)
{
	killFocus ();
	SuperClass::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandEditor::InplaceKeyEditBox::onFocus (const FocusEvent& event)
{
	if(event.eventType == FocusEvent::kKillFocus)
	{
		if(IEditControlHost* host = GetViewInterfaceUpwards<IEditControlHost> (this))
			host->onEditControlLostFocus (this);
	}
	else
	{
		// prevent siblings from stealing us the focus
		ForEachViewFast (*parent, sibling)
			if(sibling != this)
				sibling->ignoresFocus (true);
		EndFor
	}
	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandEditor::InplaceKeyEditBox::onKeyDown (const KeyEvent& event)
{
	SuperClass::onKeyDown (event);

	if(isValidCommandKey (event))
	{
		if(CommandEditor* editor = unknown_cast<CommandEditor> (param->getController ()))
		{
			bool isUsedKey = editor->findParameter ("hasKeyUsedInfo")->getValue ().asBool ();
			if(!isUsedKey)
			{
				// automatically assign if no conflict
				editor->assignKey ();
				setKey (event, false); // editor resets param in assignKey
			}
			else
			{
				#if CONFLICT_POPUP // could replace the separate controls panel
				if(IView* view = getTheme ().createView ("CCL/CommandEditorConflictPopup", *editor))
				{
					// show conflict popup
					PopupSizeInfo sizeInfo (this, PopupSizeInfo::kLeft|PopupSizeInfo::kBottom);
					sizeInfo.canFlipParentEdge (true);

					SimplePopupSelectorClient popupClient;

					PopupSelector popupSelector;
					popupSelector.setTheme (&getTheme ());
					popupSelector.popup (view, &popupClient, sizeInfo);
				}
				#endif
			}
		}
	}
	return true;
}

//************************************************************************************************
// CommandEditor
//************************************************************************************************

bool CommandEditor::isValidCommandKey (const KeyEvent& key)
{
	static const VirtualKey reservedKeys[] = {VKey::kCommand, VKey::kShift, VKey::kOption, VKey::kControl};
	
	if(key.vKey != VKey::kUnknown)
		for(int i = 0; i < ARRAY_COUNT (reservedKeys); i++)
			if(key.vKey == reservedKeys[i])
				return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (CommandEditor, CommandSelector)
DEFINE_CLASS_UID (CommandEditor, 0x211bb2f0, 0xad36, 0x44a8, 0x9f, 0xf1, 0x42, 0xf2, 0x2e, 0x6, 0xbc, 0xcb)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandEditor::CommandEditor ()
: bindingsList (NEW BindingsListModel),
  keyParam (nullptr),
  editSchemeModified (false)
{
	bindingsList->setTarget (this);

	commandTree->setKeyColumnEnabled (true);

	paramList.add (keyParam = NEW KeyParam (CSTR ("key")), Tag::kKey);
	paramList.addParam (CSTR ("assignKey"), Tag::kAssignKey);
	paramList.addParam (CSTR ("removeKey"), Tag::kRemoveKey);
	paramList.addParam (CSTR ("showCommand"), Tag::kShowCommand);
	paramList.addString (CSTR ("commandTitle"), Tag::kCommandTitle);
	paramList.addString (CSTR ("keyUsedInfo"), Tag::kKeyUsedInfo);
	paramList.addParam (CSTR ("hasKeyUsedInfo"), Tag::kHasKeyUsedInfo);
	paramList.addString (CSTR ("modification"), Tag::kModification);
	paramList.addParam (CSTR ("reset"), Tag::kReset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandEditor::~CommandEditor ()
{
	if(bindingsList)
		bindingsList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CommandEditor::getObject (StringID name, UIDRef classID)
{
	if(name == "bindingsList")
		return ccl_as_unknown (bindingsList);
	else
		return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API CommandEditor::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "KeyEditBox")
		return NEW KeyEditBox (bounds, paramList.byTag (Tag::kKey));
	if(name== "InplaceKeyEditBox")
		return NEW InplaceKeyEditBox (bounds, paramList.byTag (Tag::kKey));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::getState (CommandFile& file) const
{
	file.setName (editSchemeName);
	file.setModified (editSchemeModified);

	ForEach (commandTree->getCategories (), CommandCategory, category)
		ForEach (*category, KnownCommand, k)
			Command* c = NEW Command (k->getCategory (), k->getName ());
			c->copyKeys (*k);
			file->add (c);
		EndFor
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::setState (CommandFile& file)
{
	editSchemeName = file.getName ();
	setModified (file.isModified ());

	// remove old bindings first
	ForEach (commandTree->getCategories (), CommandCategory, category)
		ForEach (*category, KnownCommand, k)
			k->removeKeys ();
		EndFor
	EndFor

	// add new bindings
	ForEach (commandTree->getCategories (), CommandCategory, category)
		ForEach (*category, KnownCommand, k)
			Command* c = (Command*)file->findEqual (*k);
			if(c)
				k->copyKeys (*c);
		EndFor
	EndFor

	// display updates
	commandTree->signal (Message (kChanged));
	bindingsList->signal (Message (kChanged));
	setFocusCommand (nullptr);
	updateParamStates ();
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditor::init (const CommandDescription& _initialCommand)
{
	initialCommand = _initialCommand;

	tbool modified = false;
	editSchemeName = CommandTable::instance ().getActiveSchemeName (&modified);
	setModified (modified != 0);
	
	if(!hasInitialCommand ())
	{
		const CommandMsgEx cmd = CommandTable::instance ().getLastCommand ();
		initialCommand.category = cmd.category;
		initialCommand.name = cmd.name;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandEditor::apply ()
{
	CommandFile file;
	getState (file);
	CommandTable::instance ().load (file, CommandTable::kReplaceAll);
	CommandTable::instance ().signal (Message (CommandTable::kCommandsLoaded));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditor::load (UrlRef path)
{
	CommandFile file;
	if(!file.loadFromFile (path))
		return false;

	#if CHECK_CMD_DUPLICATES
	// check each key of each command against each other command
	IterForEach (file->newIterator (), Command, c)
		IterForEach (c->getBindings (), Boxed::KeyEvent, key)
			IterForEach (file->newIterator (), Command, other)
				if(other < c && other->isKeyAssigned (*key, false))
				{
					// note: this can give a false alarm for "Control" on windows
					String keyString;
					key->toString (keyString);
					Debugger::printf ("Duplicate key command: Key \"%s\":  [%s|%s] conflicts with [%s|%s]\n", MutableCString (keyString).str (), 
								c->getCategory ().str (), c->getName ().str (),
								other->getCategory ().str (), other->getName ().str ());
					ASSERT (0)
				}
			EndFor
		EndFor
	EndFor
	#endif
	
	setState (file);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditor::save (UrlRef path) const
{
	CommandFile file;
	getState (file);
	return file.saveToFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API CommandEditor::newCategoryIterator () const
{
	return commandTree->getCategories ().newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandEditor::run ()
{
	init (CommandDescription ());
	bool ok = runDialog ("CommandEditorDialog");
	if(ok)
		apply ();
	return ok ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandEditor::run (CommandDescription& command)
{
	initialCommand = command;

	return run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KnownCommand* CommandEditor::findCommand (const KeyEvent& key)
{
	ForEach (commandTree->getCategories (), CommandCategory, category)
		ForEach (*category, KnownCommand, command)
			if(command->isKeyAssigned (key, false))
				return command;
		EndFor
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::setFocusCommand (KnownCommand* command)
{
	if(focusCommand != command)
		keyParam->setKey (KeyEvent (), false);

	focusCommand = command;

	bindingsList->removeAll ();
	String title;

	if(command)
	{
		title = command->getTitle ();

		IterForEach (command->getBindings (), Boxed::KeyEvent, key)
			String keyString;
			key->CCL::KeyEvent::toString (keyString, true);

			BindingsListModel::KeyItem* item = NEW BindingsListModel::KeyItem;
			item->setKey (*key);
			item->setTitle (keyString);
			item->setEnabled (true);
			bindingsList->addItem (item);
		EndFor
	}

	bindingsList->signal (Message (kChanged));

	if(!bindingsList->isEmpty ())
		if(IItemView* listView = bindingsList->getItemView ())
			listView->setFocusItem (ItemIndex (0));

	paramList.byTag (Tag::kCommandTitle)->fromString (title);
	updateParamStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::assignKey ()
{
	const KeyEvent& key = keyParam->getKey ();
	if(key.isValid () && focusCommand)
	{
		// remove key from another command
		if(KnownCommand* loosingCommand = findCommand (key))
			if(loosingCommand != focusCommand)
			{
				loosingCommand->removeKey (key);
				invalidateCommandItem (*loosingCommand);

				ASSERT (!findCommand (key)) // there can be only one
			}

		// replace selected key // todo: focus item, if selected
		int keyIndex = bindingsList->getSelectedIndex ();
		if(keyIndex >= 0)
			focusCommand->removeKeyAt (keyIndex);

		focusCommand->assignKey (key);
		invalidateCommandItem (*focusCommand);

		keyParam->setKey (KeyEvent (), false);

		setFocusCommand (focusCommand);
		setModified (true);
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::removeKey ()
{
	// remove selected key in bindingsList
	int index = bindingsList->getSelectedIndex ();
	if(index >= 0 && focusCommand)
	{
		focusCommand->removeKeyAt (index);

		keyParam->setKey (KeyEvent (), false);
		invalidateCommandItem (*focusCommand);

		setFocusCommand (focusCommand);
		setModified (true);
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::setModified (bool state)
{
	editSchemeModified = state;

	String string;
	if(state)
		string = XSTR (modified);
	paramList.byTag (Tag::kModification)->fromString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::showCommandForKey ()
{
	const KeyEvent key = keyParam->getKey ();
	if(KnownCommand* existing = findCommand (key))
	{
		CommandDescription description (existing->getCategory (), existing->getName ());
		selectCommand (description);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::updateParamStates ()
{
	String assignedMessage;
	const KeyEvent key = keyParam->getKey ();

	paramList.byTag (Tag::kAssignKey)->enable (focusCommand && key.isValid () && !focusCommand->isKeyAssigned (key, false));
	paramList.byTag (Tag::kRemoveKey)->enable (focusCommand && bindingsList->getSelectedIndex () >= 0);

	if(key.isValid ())
		if(!focusCommand || !focusCommand->isKeyAssigned (key, false))
			if(KnownCommand* existing = findCommand (key))
				assignedMessage.appendFormat (XSTR (KeyIsAlreadyAssignedTo), existing->getTitle ());

	paramList.byTag (Tag::kKeyUsedInfo)->fromString (assignedMessage);
	paramList.byTag (Tag::kHasKeyUsedInfo)->setValue (!assignedMessage.isEmpty ());
	paramList.byTag (Tag::kShowCommand)->enable (key.isValid ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::reset ()
{
	ResourceUrl url (System::GetMainModuleRef (), "commands.xml");
	load (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandEditor::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kKey:
		onKeyEntered ();
		return true;

	case Tag::kAssignKey:
		assignKey ();
		return true;

	case Tag::kRemoveKey:
		removeKey ();
		return true;

	case Tag::kShowCommand:
		showCommandForKey ();
		return true;

	case Tag::kReset:
		reset ();
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::onKeyItemFocused (int index)
{
	updateParamStates ();
#if 0
	if(focusCommand)
	{
		int i = 0;
		IterForEach (focusCommand->getBindings (), Boxed::KeyEvent, key)
			if(i == index)
			{
				keyParam->setKey (*key, false);
				return;
			}
			i++;
		EndFor
	}
	keyParam->setKey (KeyEvent (), false);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandEditor::onKeyEntered ()
{
	updateParamStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CommandEditor::onEditKeyColumn (CommandItem& item, const IItemModel::EditInfo& info)
{
	#if EDIT_INPLACE
	if(IItemView* itemView = commandTree->getItemView ())
	{
		if(auto mouseEvent = info.editEvent.as<MouseEvent> ())
		{
			View* parent = unknown_cast<View> (info.view);
			if(parent && (!parent->isEmpty () || parent->detectDoubleClick (*mouseEvent)))
			{
				ITheme& theme = parent->getTheme ();

				IView* view = theme.createView ("CCL/CommandEditorInplaceView", asUnknown ());
				if(view)
				{
					KeyEvent* key = item.getCommand ()->getDefaultKey ();
					keyParam->setKey (key ? *key : KeyEvent (), false);

					view->setSize (info.rect);
					itemView->setEditControl (view);
				}
				return true;
			}
		}
	}
	#endif
	return false;
}

