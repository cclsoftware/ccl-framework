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
// Filename    : ccl/gui/dialogs/useroptionmodel.h
// Description : User Option Model
//
//************************************************************************************************

#ifndef _ccl_useroptionmodel_h
#define _ccl_useroptionmodel_h

#include "ccl/base/objectnode.h"
#include "ccl/gui/views/view.h"

#include "ccl/public/gui/iuseroption.h"

namespace CCL {

class OptionRoot;
class OptionPage;
class OptionCategory;
class Parameter;

//************************************************************************************************
// OptionItem
//************************************************************************************************

class OptionItem: public ObjectNode
{
public:
	DECLARE_CLASS (OptionItem, ObjectNode)

	OptionItem (StringRef title = nullptr);

	StringRef getTitle () const		{ return getName (); }
	void setTitle (StringRef title)	{ setName (title); }
	
	virtual OptionRoot* getRoot ();
	OptionItem* getItem (int index) const;

	virtual IImage* getIcon () const;
	
	virtual void opened ();
	virtual void closed ();
	virtual bool needsApply () const;
	virtual void apply ();

	virtual View* createView (const Rect& bounds);
};

//************************************************************************************************
// OptionRoot
//************************************************************************************************

class OptionRoot: public OptionItem
{
public:
	DECLARE_CLASS (OptionRoot, OptionItem)

	OptionRoot (StringRef title = nullptr);

	static void categorize (String& category, String& page, StringRef title);

	PROPERTY_STRING (helpid, HelpIdentifier)

	void build (IUserOptionList& optionList);
	void restoreSelected (IUserOptionList& optionList);
	void storeSelected (IUserOptionList& optionList);

	void select (OptionCategory* category);
	OptionCategory* getSelected () const;
	int getSelectedIndex () const;

	OptionPage* getSelectedPage () const;
	void selectPage (OptionPage* page);

	View* getOptionView () const;

	// OptionItem
	OptionRoot* getRoot () override;
	bool needsApply () const override;
	void apply () override;
	View* createView (const Rect& bounds) override;

protected:
	OptionCategory* selected;
	ObservedPtr<View> optionView;

	OptionCategory& getCategory (StringRef title);
	void updateView ();
};

//************************************************************************************************
// OptionCategory
//************************************************************************************************

class OptionCategory: public OptionItem
{
public:
	DECLARE_CLASS (OptionCategory, OptionItem)

	OptionCategory (StringRef title = nullptr);
	~OptionCategory ();

	OptionPage& getPage (StringRef title);
	
	void select (OptionPage* page);
	OptionPage* getSelected () const;

	// OptionItem
	IImage* getIcon () const override;
	View* createView (const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Parameter* tabParam;
};

//************************************************************************************************
// OptionPage
//************************************************************************************************

class OptionPage: public OptionItem
{
public:
	DECLARE_CLASS (OptionPage, OptionItem)
	
	OptionPage (StringRef title = nullptr);
	~OptionPage ();

	OptionCategory* getCategory ();

	void addOption (IUserOption* option);
	IUserOption* getFirstOption () const;
	bool contains (IUserOption* option) const;

	// OptionItem
	IImage* getIcon () const override;
	void opened () override;
	void closed () override;
	bool needsApply () const override;
	void apply () override;
	View* createView (const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Vector<IUserOption*> options;

	View* createOptionView (IUserOption* option);
};

} // namespace CCL

#endif // _ccl_useroptionmodel_h
