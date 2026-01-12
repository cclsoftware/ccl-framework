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
// Filename    : ccl/gui/skin/skinparser.cpp
// Description : Skin XML Parser
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skinparser.h"

#include "ccl/gui/skin/skinmodel.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// SkinXmlAttributes
//************************************************************************************************

class SkinXmlAttributes: public SkinAttributes
{
public:
	SkinXmlAttributes (const IStringDictionary& attributes);

	// SkinAttributes
	String getString (StringID name) const override;
	bool setString (StringID name, StringRef value) override;
	int count () const override;
	MutableCString getNameAt (int index) const override;
	String getStringAt (int index) const override;

protected:
	const IStringDictionary& attributes;
};

} // namespace CCL

using namespace CCL;
using namespace SkinElements;

//************************************************************************************************
// SkinParser
//************************************************************************************************

SkinParser::SkinParser (ISkinContext* context)
: firstTag (true)
{
	current = model = NEW SkinModel (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinParser::~SkinParser ()
{
	if(model)
		model->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel* SkinParser::parseSkin (UrlRef url)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (url, IStream::kOpenMode);
	fileName = MutableCString (url.getPath ());
	return stream ? parseSkin (*stream) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel* SkinParser::parseSkin (IStream& stream)
{
	if(parse (stream) == false)
	{
		String message (xmlParser->getErrorMessage ());
		message << "\nXML file: ";
		message << fileName;
		Alert::error (message);
		return nullptr;
	}
		
	return return_shared (model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel* SkinParser::getModel () 
{ 
	return model; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinParser::getFirstError (String& message) const
{
	message = xmlParser->getErrorMessage ();
	return !message.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinParser::setFileName (CStringRef _fileName)
{
	fileName = _fileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkinParser::startElement (StringRef name, const IStringDictionary& attributes)
{
	if(skipping)
		return kResultOk;

	MutableCString asciiName (name);

	if(firstTag)
	{
		firstTag = false;
		bool isSkin = asciiName.compare (TAG_SKIN, MetaElement::kTagsCaseSensitive) == Text::kEqual;

		if(isSkin) // set root attributes
			model->setAttributes (SkinXmlAttributes (attributes));
		
		return isSkin ? kResultOk : kResultFalse;
	}

	Element* e = MetaElement::createElement (asciiName);
	if(e == nullptr) // unknown xml tag
	{
		e = NEW Element;
		CCL_DEBUGGER ("Unknown Skin XML tag encountered!")
	}

	ASSERT (!fileName.isEmpty ())
	e->setFileName (fileName);
	e->setLineNumber (xmlParser->getCurrentLineNumber ());

	e->setParent (current);    // uplink before loading attributes
	e->setAttributes (SkinXmlAttributes (attributes));

	ASSERT (current != nullptr)
	if(current)
		current->addChild (e); // requires name to be set because of sorting
	current = e;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkinParser::endElement (StringRef name)
{
	if(skipping)
		return kResultOk;

	if(current)
		current->loadFinished ();

	current = current ? current->getParent () : nullptr;
	if(!current)
		current = model;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SkinParser::processingInstruction (StringRef target, StringRef data)
{
	XmlProcessingInstructionHandler::handleInstruction (target, data);
	return kResultOk;
}

//************************************************************************************************
// SkinXmlAttributes
//************************************************************************************************

SkinXmlAttributes::SkinXmlAttributes (const IStringDictionary& attributes)
: attributes (attributes)
{
	// apply case sensitivity of skin attributes to dictionary
	const_cast<IStringDictionary&> (attributes).setCaseSensitive (kAttrCaseSensitive);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinXmlAttributes::getString (StringID name) const
{
	return String (attributes.lookupValue (String (name)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinXmlAttributes::setString (StringID name, StringRef value)
{
	CCL_DEBUGGER ("Attributes are read-only!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SkinXmlAttributes::count () const
{
	return attributes.countEntries ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString SkinXmlAttributes::getNameAt (int index) const
{
	return MutableCString (attributes.getKeyAt (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinXmlAttributes::getStringAt (int index) const
{
	return String (attributes.getValueAt (index));
}
