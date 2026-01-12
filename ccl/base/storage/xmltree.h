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
// Filename    : ccl/base/storage/xmltree.h
// Description : XML Tree Model
//
//************************************************************************************************

#ifndef _ccl_xmltree_h
#define _ccl_xmltree_h

#include "ccl/base/objectnode.h"
#include "ccl/base/collections/stringdictionary.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/storage/xmlpihandler.h"

#include "ccl/public/storage/ixmltree.h"
#include "ccl/public/text/xmlcontentparser.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

class Attributes;

//************************************************************************************************
// XmlNode
//************************************************************************************************

class XmlNode: public ObjectNode,
			   public IXmlNode
{
public:
	DECLARE_CLASS (XmlNode, ObjectNode)
	DECLARE_PROPERTY_NAMES (XmlNode)
	DECLARE_METHOD_NAMES (XmlNode)

	XmlNode (StringRef name = nullptr);

	using ObjectNode::getParentNode;
	XmlNode* getParentNode () const;

	XmlNode* findNode (StringRef name) const;

	MutableCString getNameCString () const;
	XmlNode* findNodeCString (StringID name) const;

	const StringDictionary& getAttributes () const;
	void setAttributes (const IStringDictionary& attributes);

	StringRef CCL_API getAttribute (StringRef key) const override; ///< IXmlNode
	void CCL_API setAttribute (StringRef key, StringRef value) override; ///< IXmlNode

	MutableCString getAttributeCString (StringID key) const;
	void setAttributeCString (StringID key, StringID value);

	/** Get text of child node with given name. */
	String CCL_API getElementString (StringRef name) const override; ///< IXmlNode

	/** Add simple child node with given name and text (no attributes). */
	XmlNode* addElementString (StringRef name, StringRef value);

	/** Comment before XML tag. */
	PROPERTY_STRING (comment, Comment)

	/** Text inside XML tag. */
	void CCL_API setText (StringRef text) override; ///< IXmlNode
	StringRef CCL_API getText () const override; ///< IXmlNode
	String& getText ();
	
	/** Line number. */
	PROPERTY_VARIABLE (int, lineNumber, LineNumber)

	void dump (bool deep = false, int indent = 0);

	// IXmlNode
	IXmlNode& CCL_API newChildNode (StringRef name) override;

	CLASS_INTERFACE (IXmlNode, ObjectNode)

protected:
	static StringDictionary emptyAttributes;

	AutoPtr<StringDictionary> attributes;
	String text;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// XmlTree
//************************************************************************************************

class XmlTree: public StorableObject
{
public:
	DECLARE_CLASS (XmlTree, StorableObject)
	DECLARE_PROPERTY_NAMES (XmlTree)

	XmlTree ();

	PROPERTY_SHARED_AUTO (XmlNode, root, Root)
	PROPERTY_BOOL (storeText, StoreText)
	PROPERTY_STRING (errorMessage, ErrorMessage)

	// StorableObject
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

protected:
	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// XmlTreeParser
//************************************************************************************************

class XmlTreeParser: public XmlContentParser
{
public:
	XmlTreeParser ();
	~XmlTreeParser ();

	PROPERTY_BOOL (textEnabled, TextEnabled)
	PROPERTY_BOOL (ignoreWhitespace, IgnoreWhitespace)

	XmlNode* getRoot ();

	// XmlContentParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;

protected:
	XmlNode* root;
	XmlNode* current;

	virtual XmlNode* createNode (StringRef name);
};

//************************************************************************************************
// XmlTreeParserWithPI
//************************************************************************************************

class XmlTreeParserWithPI: public XmlTreeParser,
						   public XmlProcessingInstructionHandler
{
public:
	// XmlTreeParser
	tresult CCL_API startElement (StringRef name, const IStringDictionary& attributes) override;
	tresult CCL_API endElement (StringRef name) override;
	tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;
	tresult CCL_API processingInstruction (StringRef target, StringRef data) override;
};

//************************************************************************************************
// XmlTreeExtractor
//************************************************************************************************

class XmlTreeExtractor: public XmlTreeParser						 
{
public:
	XmlTreeExtractor ();
	void addSupportedTag (CCL::StringRef tag) {supportedTags.add(tag);}

	// XmlTreeExtractor
	CCL::tresult CCL_API startElement (CCL::StringRef name, const CCL::IStringDictionary& attributes) override;
	CCL::tresult CCL_API endElement (CCL::StringRef name) override;
	CCL::tresult CCL_API characterData (const uchar* data, int length, tbool isCDATA) override;

private:
	CCL::Vector<CCL::String> supportedTags; 
	int skippingDepth;
};

//************************************************************************************************
// XmlTreeWriter
//************************************************************************************************

class XmlTreeWriter
{
public:
	XmlTreeWriter ();
	~XmlTreeWriter ();

	PROPERTY_BOOL (textEnabled, TextEnabled)
	void setLineFormat (TextLineFormat lineFormat);

	bool writeDocument (IStream& stream, const XmlNode& root, TextEncoding encoding = Text::kUTF8);
	bool writeDocument (UrlRef path, const XmlNode& root, TextEncoding encoding = Text::kUTF8);

protected:
	IXmlWriter& writer;

	bool writeNode (const XmlNode& node);
	bool hasNodeText (const XmlNode& node) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void CCL_API XmlNode::setText (StringRef t) { text = t; }
inline String& XmlNode::getText () { return text; }
inline StringRef CCL_API XmlNode::getText () const { return text; }

} // namespace CCL

#endif // _ccl_xmltree_h
