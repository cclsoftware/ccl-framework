.. include:: ../../reference/skin-elements-classmodel.ref.rst

.. _visual_styles:

######
Styles
######

A key element defining the appearance of controls is a visual style. This concept borrows some aspects from CSS used in HTML rendering. A visual style is defined once and can be shared among multiple controls. When no specific style is given a default "Standard" style is used.
(I.e. A button without an explicit style will use the "Standard.Button" style)

Instead of defining standard styles for every app - basic design packages can be imported. For a neutral cross-platform design these packages are :ref:`neutralbase and neutraldesign<neutralbase_and_neutraldesign>`.

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
