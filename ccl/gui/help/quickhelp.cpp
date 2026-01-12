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
// Filename    : ccl/gui/help/quickhelp.cpp
// Description : Quick Help
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/help/quickhelp.h"
#include "ccl/gui/controls/label.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/clipboard.h"

#include "ccl/base/storage/xmltree.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/viewfinder.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/systemservices.h"

#define LIMIT_HELPID_LENGTH 1
#define MAX_HELPID_LENGTH 75

using namespace CCL;

const Configuration::BoolValue showHelpIds ("GUI.QuickHelp", "showHelpIds", false);

//************************************************************************************************
// QuickHelp
//************************************************************************************************

int QuickHelp::hashData (const String& key, int size)
{
	return key.getHashCode () % size;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (QuickHelp, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuickHelp::showsHelpIds ()
{
	return showHelpIds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuickHelp::QuickHelp ()
: table (1024, hashData),
  currentView (nullptr),
  recentKeyStart (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuickHelp::loadMadcapFile (UrlRef stream)
{
	XmlTreeParser parser;
	parser.setTextEnabled (true);
	if(parser.parse (stream))
		if(XmlNode* root = parser.getRoot ())
			if(XmlNode* body = root->findNode ("body"))
			{
				String key;
				Data current;
				IterForEach (body->newIterator (), XmlNode, node)
				CStringRef element = node->getNameCString ();
				if(element == "h1" || element == "h2" || element == "h3" || element == "h4" || element == "h5" || element == "h6")
				{
					StringRef xref = node->getAttribute ("MadCap:xrefTargetName");
					if(!xref.isEmpty ())
					{
						if(!key.isEmpty ())
						{
							current.text.trimWhitespace ();
							table.add (key, current);
							current.clear ();
						}
						if(xref == "/end")
							key.empty ();
						else
						{
							key = xref;
							#if LIMIT_HELPID_LENGTH
							key.truncate (MAX_HELPID_LENGTH);
							#endif
						}
					}
					if(!key.isEmpty ())
					{
						String text = node->getText ();
						text.trimWhitespace ();
						if(!text.isEmpty ())
						{
							if(!current.title.isEmpty ())
								current.title.append (String::getLineEnd ());	
							current.title.append (text);
						}
					}				
				}
				else if(!key.isEmpty () && element == "p")
				{
					String text = node->getText ();
					text.trimWhitespace ();
					if(!text.isEmpty ())
					{
						if(!current.text.isEmpty ())
							current.text.append (String::getLineEnd ());							
						current.text.append (text);
					}
				}
				EndFor
				if(!key.isEmpty ())
					table.add (key, current);
				return true;
			}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuickHelp::setByKey (StringRef key)
{
	current = nullptr;
	currentKey = key;
	#if LIMIT_HELPID_LENGTH
	currentKey.truncate (MAX_HELPID_LENGTH);
	#endif
	currentKey.replace(" ", "_"); // MadCap flare replaces spaces with underscores

	updateRecentKey (currentKey);

	static const String kSeparator (CCLSTR (";"));
	if(currentKey.contains (kSeparator)) // multiple alternatives
	{
		ForEachStringToken (currentKey, kSeparator, token)
			const Data& entry = table.lookup (token);
			if(!(entry.title.isEmpty () && entry.text.isEmpty ()))
			{
				current = entry;
				return true;
			}
		EndFor
	}
	else
		current = table.lookup (currentKey);

	return !(current.title.isEmpty () && current.text.isEmpty ()) || showHelpIds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* QuickHelp::findView (Point& pos)
{
	if(pos != currentPosition)
	{
		currentPosition = pos;
		if(Window* window = unknown_cast<Window> (Desktop.findWindow (pos)))
		{
			window->screenToClient (pos);
			if(View* view = window->findView (pos, true))
			{
				// give siblings underneath a chance if this view has no help id, e.g. if it blocks via Styles::kNoHelpId
				while(view && view->getHelpIdentifier ().isEmpty ())
					view = unknown_cast<View> (ViewFinder (view).findNextView (window, pos));

				if(view != currentView)
				{
					currentView = view;
					return view;
				}
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API QuickHelp::createImage (const Point& size, const IVisualStyle& style)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API QuickHelp::createView (const Rect& size, const IVisualStyle& style)
{
	if(current.title.isEmpty () && current.text.isEmpty () && !showHelpIds)
		return nullptr;
	
	bool vertical = style.getMetric<bool> ("vertical", true);
	
	String text;

	if(showHelpIds)
	{
		text.append ("{");
		text.append (currentKey);
		text.append ("} ");
	}

	if(!current.title.isEmpty ())
	{
		text.append ("[color=$heading][size=$heading][b=$heading][i=$heading][u=$heading]");
		text.append (current.title);
		if(!vertical)
			text.append (": ");
		text.append ("[/u][/i][/b][/size][/color]");
		if(vertical)
			text.append (String::getLineEnd ());
	}
	
	text.append (current.text);
	
	StyleFlags flags (0, Styles::kLabelMarkupEnabled);
	if(vertical)
		flags.setCustomStyle (Styles::kLabelMultiLine);
	Rect rect (size);	
	Label* label = NEW Label (rect, flags, text);
	label->setVisualStyle (unknown_cast<VisualStyle> (&style));
	return label;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API QuickHelp::createText ()
{
	if(current.title.isEmpty () && current.text.isEmpty ())
		return nullptr;
	
	String result;
	result.append (current.title);
	result.append (" : ");
	result.append (current.text);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuickHelp::updateRecentKey (StringRef key)
{
	if(showHelpIds && recentKeys.at (0) != key)
	{
		// discard last added recent key if time was to short (avoid flooding the history on mouse moves, e.g. towards info view for context menu)
		int64 now = System::GetSystemTicks ();
		if(now - recentKeyStart < 400)
			recentKeys.removeFirst ();

		recentKeyStart = now;

		// add or move to head
		if(!recentKeys.moveToHead (key))
			recentKeys.prepend (key);

		// remove oldest
		while(recentKeys.count () > kMaxRecentKeys + 1) // keep one more, might have to remove a short one in appendContextMenu
			recentKeys.removeLast ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API QuickHelp::appendContextMenu (IContextMenu& contextMenu)
{
	if(showHelpIds)
	{
		// (not translated, for development only)
		contextMenu.addHeaderItem (CCLSTR ("Copy Recent Help Identifier"));

		// discard last added recent key if time was to short 
		int64 now = System::GetSystemTicks ();
		if(now - recentKeyStart < 800)
			recentKeys.removeFirst ();

		int count = 0;
		for(auto key : recentKeys)
		{
			// offer alternatives separately
			ForEachStringToken (*key, ";", id)
				contextMenu.addCommandItem (CommandWithTitle ("Help", "Copy", id),
					makeCommandDelegate ([=] (const CommandMsg& msg, VariantRef data)
					{
						if(!msg.checkOnly ())
							Clipboard::instance ().setText (data.asString ());

						return true;
					}, id));

				if(++count >= kMaxRecentKeys)
					break;
			EndFor
			contextMenu.addSeparatorItem ();
		}
	}
	return kResultFalse;
}
