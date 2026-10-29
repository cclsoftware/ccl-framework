.. include:: ../../reference/skin-elements-classmodel.ref.rst

.. _visual_styles:

######
Styles
######

A visual style is the key element defining the appearance of controls. This concept borrows some aspects from CSS used in HTML rendering. A visual style is defined once and can be shared among multiple controls. When no specific style is given a default "Standard" style is used.
(e.g. a button without an explicit style will use the "Standard.Button" style)

A style is a named collection of properties such as colors, fonts, and metrics, declared as child elements (e.g. ``<Color name="textcolor" .../>``). See :doc:`../colors/index` for the color value syntax.

Instead of defining standard styles for every app - basic design packages can be imported. For a neutral cross-platform design these packages are :ref:`neutralbase and neutraldesign<neutralbase_and_neutraldesign>`.

********
Advanced
********

Most styles are just a single named collection of properties, referenced directly by name - the following only matters once styles start combining or overriding each other.

================
Composite Styles
================

Composite styles (e.g. style="a b") are merged from the given styles in the order of appearance. (Later styles overwrite earlier ones). Inherited properties are always overwritten by direct properties. (i.e. if there is a style "a" derived from "A" and a style "b" derived from "B" and they appear in a style array in the order [a b], then B.x is overwritten by a.x even though a appears before b.) This is because the composite style [a b] is derived from another ad-hoc built composite style [A B].

Example::

	<Style name="A">
		<Metric name="u" value="5"/>
	</Style>

	<Style name="a" inherit="A">
		<Metric name="x" value="1"/>
		<Metric name="t" value="9"/>
	</Style>

	<Style name="B">
		<Metric name="x" value="2"/>
	</Style>

	<Style name="b" inherit="B">
		<Metric name="t" value="8"/>
	</Style>

	style="a b" -> x is 1, t is 8, u is 5
