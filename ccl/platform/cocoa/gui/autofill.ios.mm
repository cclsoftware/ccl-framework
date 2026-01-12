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
// Filename    : ccl/platform/cocoa/gui/autofill.ios.mm
// Description : Autofill Support, iOS implementation
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/autofill.ios.h"

#include "ccl/gui/views/view.h"
#include "ccl/gui/windows/nativewindow.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"

#include "ccl/public/cclexports.h"
#include "ccl/public/base/ccldefpush.h"
#include <UIKit/UIKit.h>

using namespace CCL;

//************************************************************************************************
// AutofillDelegate
//************************************************************************************************

@interface CCL_ISOLATED (AutofillDelegate) : NSObject<UITextFieldDelegate>
{
	SharedPtr<IAutofillClient> client;
}

- (instancetype)init:(IAutofillClient*)client;
- (void)textFieldEditingChanged:(UITextField*)textField;

@end

namespace CCL {
namespace iOS {

//************************************************************************************************
// iOS::UIAutofillElement
//************************************************************************************************

class UIAutofillElement: public Object
{
public:
	UIAutofillElement (IAutofillClient* client);
	~UIAutofillElement ();

	void configure ();
	IAutofillClient* getClient () const { return client; }

protected:
	SharedPtr<IAutofillClient> client;
	NSObj<UITextField> view;
	NSObj<CCL_ISOLATED (AutofillDelegate)> delegate;
};

//************************************************************************************************
// iOS::UIAutofillManager
//************************************************************************************************

class UIAutofillManager: public AutofillManager
{
public:
	UIAutofillManager ();

	// AutofillManager
	void addClient (IAutofillClient* client) override;
	void removeClient (IAutofillClient* client) override;
	void updateClient (IAutofillClient* client) override;

protected:
	ObjectList elements;

	UIAutofillElement* findElement (IAutofillClient* client) const;
};

} // namespace iOS
} // namespace CCL

using namespace iOS;

//************************************************************************************************
// AutofillDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (AutofillDelegate)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (instancetype)init:(IAutofillClient*)_client
{
	if(self = [super init])
		client = _client;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)textFieldEditingChanged:(UITextField*)textField
{
	String filledText;
	filledText.appendNativeString ([textField text]);
	client->receiveAutofillText (filledText);
}

@end

//************************************************************************************************
// iOS::UIAutofill
//************************************************************************************************

void UIAutofill::setContentType (UITextField* textField, const IAutofillClient& client)
{
	if(!textField)
		return;

	// Content type
	UITextContentType type = nil;
	switch(client.getAutofillClientType ())
	{
	case Styles::kAutofillTypeUsername :
		type = UITextContentTypeUsername;
		break;
	case Styles::kAutofillTypePassword :
		type = UITextContentTypePassword;
		break;
	case Styles::kAutofillTypeNewPassword :
		type = UITextContentTypeNewPassword;
		break;
	case Styles::kAutofillTypeEmail :
		type = UITextContentTypeEmailAddress;
		break;
	case Styles::kAutofillTypeCountry :
		type = UITextContentTypeCountryName;
		break;
	case Styles::kAutofillTypeLastName :
		type = UITextContentTypeFamilyName;
		break;
	case Styles::kAutofillTypeFirstName :
		type = UITextContentTypeGivenName;
		break;
	}
	[textField setTextContentType:type];
}

//************************************************************************************************
// iOS::UIAutofillManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (AutofillManager, UIAutofillManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAutofillManager::UIAutofillManager ()
{
	elements.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutofillManager::addClient (IAutofillClient* client)
{
	if(!client)
		return;

	UIAutofillElement* uiElement = NEW UIAutofillElement (client);
	elements.add (uiElement);
	uiElement->configure ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutofillManager::removeClient (IAutofillClient* client)
{
	if(!client)
		return;

	UIAutofillElement* uiElement = findElement (client);
	if(uiElement)
	{
		elements.remove (uiElement);
		uiElement->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutofillManager::updateClient (IAutofillClient* client)
{
	if(!client)
		return;

	UIAutofillElement* uiElement = findElement (client);
	if(uiElement)
		uiElement->configure ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAutofillElement* UIAutofillManager::findElement (IAutofillClient* client) const
{
	for(auto* element : iterate_as<UIAutofillElement> (elements))
		if(element->getClient () == client)
			return element;

	return nullptr;
}

//************************************************************************************************
// iOS::UIAutofillElement
//************************************************************************************************

UIAutofillElement::UIAutofillElement (IAutofillClient* _client)
: client (_client)
{
	view = [[UITextField alloc] initWithFrame:CGRectZero];
	delegate = [[CCL_ISOLATED (AutofillDelegate) alloc] init:client];
	[view addTarget:delegate action:@selector(textFieldEditingChanged:) forControlEvents:UIControlEventEditingChanged];
	[view setDelegate:delegate];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIAutofillElement::~UIAutofillElement ()
{
	[view removeFromSuperview];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UIAutofillElement::configure ()
{
	if(!client)
		return;
	UIAutofill::setContentType (view, *client);

	// Attach to super view
	View* frameworkView = client->getAutofillClientView ();
	ASSERT (frameworkView)
	if(!frameworkView)
		return;
	IOSWindow* window = IOSWindow::cast (frameworkView->getWindow ());
	if(!window)
		return;
	NativeView* nativeView = window->getNativeView ();
	if(!nativeView)
		return;
	[nativeView->getView () addSubview:view];

	// Resize to minimum allowed size
	Point p;
	Rect size;
	frameworkView->getVisibleClient (size);
	size.offset (frameworkView->clientToWindow (p));
	size.setWidth (1);
	size.setHeight (1);
	CGRect frame;
	MacOS::toCGRect (frame, size);
	[view setFrame:frame];

	// Make almost invisible
	[view setAlpha:0.01];
}
