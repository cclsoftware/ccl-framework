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
// Filename    : ccl/gui/theme/visualstyle.h
// Description : VisualStyle class
//
//************************************************************************************************

#ifndef _ccl_visualstyle_h
#define _ccl_visualstyle_h

#include "ccl/base/object.h"

#include "ccl/gui/theme/colorreference.h"

#include "ccl/public/base/itrigger.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/ivisualstyle.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Style XML Definition
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	<Style name="MyStyle">
		<Color name="forecolor" color="black"/>
		<Color name="backcolor" color="#855660"/>
		<Color name="hilitecolor" color="yellow"/>
		<Color name="textcolor" color="#000000"/>

		<Metric name="strokewidth" value="3"/>

		<Font name="textfont" face="Arial" size="10" style="italic"/>

		<Align name="textalign" align="left vcenter"/>

		<Image name="background" image="sliderBack"/>
	</Style>

	...or...

	<Style name="MyStyle"
		   forecolor="black" backcolor="#855660" hilitecolor="yellow" textcolor="#000000"
		   strokewidth="3"
		   textface="Arial" textsize="10" textstyle="italic"
		   textalign="left vcenter"
		   background="sliderBack"
		   />
*/

namespace Boxed {

//************************************************************************************************
// Boxed::Font
//************************************************************************************************

class Font: public Object,
			public IFont
{
public:
	DECLARE_CLASS (Boxed::Font, Object)

	Font (CCL::FontRef font = CCL::Font ());

	// IFont
	void CCL_API assign (CCL::FontRef font) override;
	void CCL_API copyTo (CCL::Font& font) const override;

	CLASS_INTERFACE (IFont, Object)

protected:
	CCL::Font font;
};

} // namespace Boxed

interface IVisualStyleClient;

//************************************************************************************************
// VisualStyle
//************************************************************************************************

class VisualStyle: public Object,
				   public IVisualStyle
{
public:
	DECLARE_CLASS (VisualStyle, Object)

	VisualStyle (StringID name = nullptr);
	VisualStyle (const VisualStyle& other);
	~VisualStyle ();

	static const VisualStyle emptyStyle;
	static constexpr bool kStyleCaseSensitive = false; ///< case-sensitivity of visual style items

	StringID CCL_API getName () const override; // IVisualStyle
	void setName (StringID name);

	void merge (const VisualStyle& other);
	void removeAll ();
	void setInherited (VisualStyle* inherited);

	void setTrigger (ITriggerPrototype* trigger);
	ITriggerPrototype* getTrigger (bool inherited = true) const;

	void addColorSchemeReference (StringID nameInStyle, ColorScheme& scheme, StringID nameInScheme);

	// add / remove clients that will be notified when style changes; does not take ownership of client
	virtual void use (IVisualStyleClient* client);
	virtual void unuse (IVisualStyleClient* client);

	// IVisualStyle
	ColorRef CCL_API getColor (StringID name, ColorRef defaultColor = Colors::kBlack) const override;
	void CCL_API setColor (StringID name, ColorRef color) override;

	FontRef CCL_API getFont (StringID name, FontRef defaultFont = Font::getDefaultFont ()) const override;
	void CCL_API setFont (StringID name, FontRef font) override;

	Metric CCL_API getMetric (StringID name, Metric defaultValue = 0) const override;
	void CCL_API setMetric (StringID name, Metric value) override;
	
	CString CCL_API getString (StringID name, StringID defaultValue = CString::kEmpty) const override;
	void CCL_API setString (StringID name, StringID value) override;

	Options CCL_API getOptions (StringID name, Options defaultOptions = 0) const override;
	void CCL_API setOptions (StringID name, Options options) override;

	IImage* CCL_API getImage (StringID name) const override;
	void CCL_API setImage (StringID name, IImage* image) override;

	IGradient* CCL_API getGradient (StringID name) const override;
	void CCL_API setGradient (StringID name, IGradient* gradient) override;

	tbool CCL_API hasReferences (IColorScheme& scheme) const override;
	tbool CCL_API copyFrom (const IVisualStyle& other) override;

	const IVisualStyle* CCL_API getInherited () const override;
	const IVisualStyle* CCL_API getOriginal () const override;

	using IVisualStyle::getMetric;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IVisualStyle, Object)

protected:

	class Item: public Unknown,
				public IVisualStyleItem
	{
	public:
		Item (StringID name): name (name) {}
		Item () {}
		
		PROPERTY_MUTABLE_CSTRING (name, Name)
		virtual operator Variant () const = 0;

		// IVisualStyleItem
		StringID CCL_API getItemName () const override { return name; }
		void CCL_API getItemValue (Variant& value) const override { value = this->operator Variant (); }

		CLASS_INTERFACE (IVisualStyleItem, Unknown)
	};

	class ColorItem: public Item
	{
	public:
		ColorItem (StringID name, Color color): Item (name), color (color) {}
		ColorItem () {}
		ColorItem (const ColorItem& other): Item (other.name), color (other.color) {}		
		void operator = (const ColorItem& other) { name = other.name; color = other.color;}
		bool operator == (const ColorItem& other) const { return name == other.name; }

		PROPERTY_OBJECT (Color, color, Color)
		operator Variant () const override { String s; Colors::toString (color, s); return Variant (s, true); }
	};

	class FontItem: public Item
	{
	public:
		FontItem (StringID name, const Font& font): Item (name), font (font) {}
		FontItem () {}
		FontItem (const FontItem& other): Item (other.name), font (other.font)  {}
		void operator = (const FontItem& other) { name = other.name; font = other.font;}
		bool operator == (const FontItem& other) const { return name == other.name; }

		PROPERTY_OBJECT (Font, font, Font)
		operator Variant () const override { AutoPtr<Boxed::Font> bf = NEW Boxed::Font (font); return Variant (ccl_as_unknown (bf), true); }
	};

	class MetricItem: public Item
	{
	public:
		MetricItem (StringID name, Metric value): Item (name), value (value) {}
		MetricItem (): value (0) {}
		MetricItem (const MetricItem& other): Item (other.name), value (other.value)  {}
		void operator = (const MetricItem& other) { name = other.name; value = other.value;}
		bool operator == (const MetricItem& other) const { return name == other.name; }

		PROPERTY_VARIABLE (Metric, value, Value)
		operator Variant () const override { return value; }
	};

	class StringItem: public Item
	{
	public:
		StringItem (StringID name, StringID value): Item (name), value (value) {}
		StringItem () {}
		StringItem (const StringItem& other): Item (other.name), value (other.value)  {}
		void operator = (const StringItem& other) { name = other.name; value = other.value;}
		bool operator == (const StringItem& other) const { return name == other.name; }

		PROPERTY_MUTABLE_CSTRING (value, Value)
		operator Variant () const override { return String (value); }
	};

	class OptionsItem: public Item
	{
	public:
		OptionsItem (StringID name, Options options): Item (name), options (options) {}
		OptionsItem (): options (0) {}
		OptionsItem (const OptionsItem& other): Item (other.name), options (other.options) {}		
		void operator = (const OptionsItem& other) { name = other.name; options = other.options;}
		bool operator == (const OptionsItem& other) const { return name == other.name; }

		PROPERTY_VARIABLE (Options, options, Options)
		operator Variant () const override { return options; }
	};

	class ImageItem: public Item
	{
	public:
		ImageItem (StringID name, IImage* image): Item (name), image (image) {}
		ImageItem () {}
		ImageItem (const ImageItem& item): Item (item.name), image (item.image) {}
		void operator = (const ImageItem& other) { name = other.name; image = other.image;}
		bool operator == (const ImageItem& other) { return name == other.name; }

		PROPERTY_SHARED_POINTER (IImage, image, Image)
		operator Variant () const override { return static_cast<IImage*> (image); }
	};

	class GradientItem: public Item
	{
	public:
		GradientItem (StringID name, IGradient* gradient): Item (name), gradient (gradient) {}
		GradientItem () {}
		void operator = (const GradientItem& other) { name = other.name; gradient = other.gradient;}
		bool operator == (const GradientItem& other) const { return name == other.name; }

		PROPERTY_SHARED_POINTER (IGradient, gradient, Gradient)
		operator Variant () const override { return static_cast<IGradient*> (gradient); }
	};

	template <class ItemType>
	class ItemVector: public Object,
					  public Vector<ItemType>,
					  public IArrayObject
	{
	public:
		ItemVector (int capacity, int delta)
		: Vector<ItemType> (capacity, delta)
		{}

		// IObject
		tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
		{
			ItemType* item = lookup (*this, propertyId);
			if(item)
			{
				var = *item;
				return true;
			}
			return Object::getProperty (var, propertyId);
		}

		// IArrayObject
		int CCL_API getArrayLength () const override { return Vector<ItemType>::count (); }

		tbool CCL_API getArrayElement (Variant& var, int index) const override
		{
			if(index >= Vector<ItemType>::count ())
				return false;

			var = Variant (static_cast<IVisualStyleItem*> (&Vector<ItemType>::at (index)), true);
			return true;
		}

		CLASS_INTERFACE (IArrayObject, Object)
	};

	MutableCString name;
	ItemVector<ColorItem> colors;
	ItemVector<FontItem> fonts;
	ItemVector<MetricItem> metrics;
	ItemVector<StringItem> strings;
	ItemVector<OptionsItem> options;
	ItemVector<ImageItem> images;
	ItemVector<GradientItem> gradients;
	SharedPtr<ITriggerPrototype> trigger;
	SharedPtr<VisualStyle> inherited;

	struct ColorStyleReference: ColorSchemeReference
	{
		MutableCString nameInStyle;
	};

	Vector<ColorScheme*> colorSchemeObserverList;
	Vector<ColorStyleReference*> colorSchemeReferences;

	void removeColorSchemeReferences ();

	template<typename T>
	static T* lookup (const Vector<T>& items, StringID name);

	template<typename T>
	void merge (Vector<T>& destination, const Vector<T>& source) const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// IVisualStyleClient interface
//************************************************************************************************

interface IVisualStyleClient: IUnknown
{
	virtual void onVisualStyleChanged () = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// VisualStyle inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
T* VisualStyle::lookup (const Vector<T>& items, StringID name)
{
	T* ptr = items.getItems ();
	for(int i = 0; i < items.count (); i++)
	{
		if(ptr[i].getName ().compare (name, kStyleCaseSensitive) == Text::kEqual)
			return &ptr[i];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void VisualStyle::merge (Vector<T>& destination, const Vector<T>& source) const
{
	VectorForEach (source, T, sourceItem)
		T* existingItem = lookup (destination, sourceItem.getName ());
		if(existingItem)
			destination.remove (*existingItem);

		destination.add (sourceItem);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_visualstyle_h
