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
// Filename    : ccl/base/storage/xmltree.cpp
// Description : XML Tree Model
//
//************************************************************************************************

#define DEBUG_LOG 1 // keep it enabled for XmlNode::dump() to work in debug builds!
#define USE_CDATA 0

#include "ccl/base/storage/xmltree.h"
#include "ccl/base/storage/storage.h"

#if USE_CDATA
#include "ccl/public/base/istream.h"
#include "ccl/public/system/ifileutilities.h"
#endif

#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// XmlNode
//************************************************************************************************

DEFINE_CLASS (XmlNode, ObjectNode)
DEFINE_CLASS_NAMESPACE (XmlNode, NAMESPACE_CCL)

StringDictionary XmlNode::emptyAttributes;

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode::XmlNode (StringRef name)
: ObjectNode (name),
  lineNumber (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlNode::getParentNode () const
{
	return getParentNode<XmlNode> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString XmlNode::getNameCString () const
{
	return MutableCString (getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlNode::findNodeCString (StringID name) const
{
	return findNode (String (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlNode::findNode (StringRef name) const
{
	return findChildNode<XmlNode> (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StringDictionary& XmlNode::getAttributes () const
{
	if(attributes)
		return *attributes;

	return emptyAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNode::setAttributes (const IStringDictionary& _attributes)
{
	if(attributes)
		attributes->copyFrom (_attributes);
	else if(_attributes.countEntries () > 0)
		attributes = NEW StringDictionary (_attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API XmlNode::getAttribute (StringRef key) const
{
	if(attributes)
		return attributes->lookupValue (key);

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlNode::setAttribute (StringRef key, StringRef value)
{
	if(!attributes)
		attributes = NEW StringDictionary;

	attributes->setEntry (key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString XmlNode::getAttributeCString (StringID key) const
{
	return MutableCString (getAttribute (String (key)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNode::setAttributeCString (StringID key, StringID value)
{
	setAttribute (String (key), String (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API XmlNode::getElementString (StringRef name) const
{
	if(XmlNode* node = findNode (name))
		return node->getText ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlNode::addElementString (StringRef name, StringRef value)
{
	XmlNode* node = NEW XmlNode (name);
	node->setText (value);
	addChild (node);
	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IXmlNode& CCL_API XmlNode::newChildNode (StringRef name)
{
	XmlNode* node = NEW XmlNode (name);
	addChild (node);
	return *node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlNode::dump (bool deep, int indent)
{
	Debugger::IndentGuard indentGuard (indent);

	Debugger::print (String () << Debugger::getIndent () << getName ());
	if(!text.isEmpty ())
	{
		Debugger::println (String () << " \"" << String (text).trimWhitespace () << "\"");
	}
	else
	{
		Debugger::println ("");
	}

	if(attributes)
	{
		for(int i = 0; i < attributes->countEntries (); i++)
		{
			StringRef key = attributes->getKeyAt (i);
			StringRef value = attributes->getValueAt (i);
			Debugger::print (String () << Debugger::getIndent () << "\t");
			Debugger::print (key);
			Debugger::print (" = ");
			Debugger::println (value);
		}
	}

	if(deep)
	{
		ForEach (*this, XmlNode, child)
			child->dump (true, indent + 1);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (XmlNode)
	DEFINE_PROPERTY_NAME ("name")
	DEFINE_PROPERTY_NAME ("parent")
	DEFINE_PROPERTY_NAME ("text")
	DEFINE_PROPERTY_NAME ("comment")
END_PROPERTY_NAMES (XmlNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlNode::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "name")
	{
		setName (var.asString ());
		return true;
	}
	if(propertyId == "text")
	{
		setText (var.asString ());
		return true;
	}
	if(propertyId == "comment")
	{
		setComment (var.asString ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlNode::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "name")
	{
		var = getName ();
		var.share ();
		return true;
	}
	if(propertyId == "parent")
	{
		var.takeShared (ccl_as_unknown (getParentNode ()));
		return true;
	}
	if(propertyId == "text")
	{
		var = getText ();
		var.share ();
		return true;
	}
	if(propertyId == "comment")
	{
		var = getComment ();
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (XmlNode)
	DEFINE_METHOD_ARGR ("newNode", "title=''", "XmlNode")
	DEFINE_METHOD_ARGS ("setAttribute", "key, value")
	DEFINE_METHOD_ARGR ("getAttribute", "key", "string")
	DEFINE_METHOD_ARGS ("addChild", "node")	
	DEFINE_METHOD_ARGR ("findNode", "name", "XmlNode")
	DEFINE_METHOD_ARGR ("newIterator", "", "Iterator")	
END_METHOD_NAMES (XmlNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlNode::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "newNode")
	{
		String name;
		if(msg.getArgCount () > 0)
			name = msg[0].asString ();
		returnValue.takeShared (AutoPtr<IObject> (NEW XmlNode (name)));
		return true;
	}
	else if(msg == "setAttribute")
	{
		setAttribute (msg[0].asString (), VariantString (msg[1]));
		return true;
	}
	else if(msg == "getAttribute")
	{
		returnValue = getAttribute (msg[0].asString ());
		returnValue.share ();
		return true;
	}
	else if(msg == "addChild")
	{
		XmlNode* node = unknown_cast<XmlNode> (msg[0].asUnknown ());
		ASSERT (node != nullptr)
		if(node)
		{
			node->retain ();
			addChild (node);
		}
		return true;
	}
	else if(msg == "findNode")
	{
		returnValue.takeShared (ccl_as_unknown (findNode (msg[0].asString ())));
		return true;
	}
	else if(msg == "newIterator")
	{
		returnValue.takeShared (AutoPtr<IObject> (NEW HoldingIterator (&getChildren (), newIterator ())));
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// XmlTree
//************************************************************************************************

DEFINE_CLASS (XmlTree, StorableObject)
DEFINE_CLASS_NAMESPACE (XmlTree, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlTree::XmlTree ()
: root (NEW XmlNode),
  storeText (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlTree::save (IStream& stream) const
{
	ASSERT (getRoot ())
	if(!getRoot ())
		return false;

	XmlTreeWriter writer;
	writer.setTextEnabled (isStoreText ());
	return writer.writeDocument (stream, *getRoot ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlTree::load (IStream& stream)
{
	XmlTreeParser parser;
	parser.setTextEnabled (isStoreText ());
	if(!parser.parse (stream))
	{
		errorMessage = parser.getErrorMessage ();
		return false;
	}
	setRoot (parser.getRoot ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (XmlTree)
	DEFINE_PROPERTY_NAME ("root")
	DEFINE_PROPERTY_NAME ("errorMessage")
END_PROPERTY_NAMES (XmlTree)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlTree::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "root")
	{
		setRoot (unknown_cast<XmlNode> (var.asUnknown ()));
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XmlTree::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "root")
	{
		var.takeShared (ccl_as_unknown (getRoot ()));
		return true;
	}
	if(propertyId == "errorMessage")
	{
		var = errorMessage;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// XmlTreeParser
//************************************************************************************************

XmlTreeParser::XmlTreeParser ()
: root (nullptr),
  current (nullptr),
  textEnabled (false),
  ignoreWhitespace (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlTreeParser::~XmlTreeParser ()
{
	if(root)
		root->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlTreeParser::getRoot ()
{
	return root;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlNode* XmlTreeParser::createNode (StringRef name)
{
	return NEW XmlNode (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParser::startElement (StringRef name, const CCL::IStringDictionary& attributes)
{
	XmlNode* child = createNode (name);
	ASSERT (child != nullptr)
	if(!child)
		return kResultFailed;

	child->setLineNumber (xmlParser->getCurrentLineNumber ());
	
	if(current)
		current->addChild (child);
	else
	{
		ASSERT (root == nullptr)
		if(root)
		{
			child->release ();
			return kResultFailed;
		}

		root = child;
	}

	child->setAttributes (attributes);
	current = child;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParser::endElement (StringRef name)
{
	if(current)
		current = (XmlNode*)current->getParent ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParser::characterData (const uchar* data, int length, tbool isCDATA)
{
	if(!current || !isTextEnabled ())
		return kResultOk;

	if(isIgnoreWhitespace ())
	{
		String dataString;
		dataString.append (data, length);
		dataString.trimWhitespace ();
		if(!dataString.isEmpty ())
			current->getText ().append (dataString);
	}
	else
		current->getText ().append (data, length);

	return kResultOk;
}

//************************************************************************************************
// XmlTreeParserWithPI
//************************************************************************************************

tresult CCL_API XmlTreeParserWithPI::processingInstruction (StringRef target, StringRef data)
{
	XmlProcessingInstructionHandler::handleInstruction (target, data);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParserWithPI::startElement (StringRef name, const IStringDictionary& attributes)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::startElement (name, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParserWithPI::endElement (StringRef name)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::endElement (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlTreeParserWithPI::characterData (const uchar* data, int length, tbool isCDATA)
{
	if(skipping)
		return kResultOk;

	return XmlTreeParser::characterData (data, length, isCDATA);
}

//************************************************************************************************
// XmlTreeExtractor
//************************************************************************************************

XmlTreeExtractor::XmlTreeExtractor ()
: skippingDepth (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API XmlTreeExtractor::startElement (CCL::StringRef name, const CCL::IStringDictionary& attributes)
{
	if(skippingDepth > 0)
	{
		skippingDepth++;
		return kResultOk;
	}
	
	if(supportedTags.contains (name) == false)
	{
		ASSERT (skippingDepth == 0)
		skippingDepth = 1;
		return kResultOk;
	}

	return XmlTreeParser::startElement (name, attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API XmlTreeExtractor::endElement (CCL::StringRef name)
{	
	if(skippingDepth > 0)
	{
		skippingDepth--;
		return kResultOk;
	}

	return XmlTreeParser::endElement (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API XmlTreeExtractor::characterData (const uchar* data, int length, tbool isCDATA)
{
	if(skippingDepth > 0)
		return kResultOk;
	return XmlTreeParser::characterData (data, length, isCDATA);
}

//************************************************************************************************
// XmlTreeWriter
//************************************************************************************************

XmlTreeWriter::XmlTreeWriter ()
: writer (*System::CreateXmlWriter ()),
  textEnabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlTreeWriter::~XmlTreeWriter ()
{
	writer.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlTreeWriter::setLineFormat (TextLineFormat lineFormat)
{
	writer.setDocumentLineFormat (lineFormat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlTreeWriter::writeDocument (UrlRef path, const XmlNode& root, TextEncoding encoding)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	if(stream == nullptr)
		return false;

	return writeDocument (*stream, root, encoding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlTreeWriter::writeDocument (IStream& stream, const XmlNode& root, TextEncoding encoding)
{
	if(writer.beginDocument (stream, encoding) != kResultOk)
		return false;

	if(!writeNode (root))
		return false;

	if(writer.endDocument () != kResultOk)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlTreeWriter::hasNodeText (const XmlNode& node) const
{
	if(!isTextEnabled ())
		return false;
	return node.getText ().isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlTreeWriter::writeNode (const XmlNode& node)
{
	if(!node.getComment ().isEmpty ())
		writer.writeComment (node.getComment ());
	
	bool hasAttributes = node.getAttributes ().countEntries () > 0;
	bool hasChildren = node.countChildren () > 0;
	bool hasText = hasNodeText (node);

	if(hasText && !hasChildren && !hasAttributes) // <name>text</name>
	{
		if(writer.writeElement (node.getName (), node.getText ()) != kResultOk)
			return false;
	}
	else if(!hasChildren && !hasText) // <name [attr="..."]/>
	{
		if(writer.writeElement (node.getName (), &node.getAttributes ()) != kResultOk)
			return false;
	}
	else // <name [attr="..."]> CRLF [text CRLF] </name>
	{
		if(writer.startElement (node.getName (), &node.getAttributes ()) != kResultOk)
			return false;

		if(hasText)
		{
			#if USE_CDATA
			AutoPtr<IStream> charData = System::GetFileUtilities ().createStringStream (node.getText (), Text::kUTF16);
			ASSERT (charData != 0)
			if(charData && writer.characterData (*charData) != kResultOk)
				return false;
			#else
			if(writer.writeLine (node.getText ()) != kResultOk)
				return false;
			#endif
		}

		ForEach (node, XmlNode, child)
			if(!writeNode (*child))
				return false;
		EndFor

		if(writer.endElement (node.getName ()) != kResultOk)
			return false;
	}
	return true;
}
