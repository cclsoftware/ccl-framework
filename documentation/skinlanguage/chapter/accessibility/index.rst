.. include:: ../../reference/skin-elements-classmodel.ref.rst

#############
Accessibility
#############

CCL has built-in support for accessibility tools like screen readers to consume information about user interface elements in CCL-based applications.

All elements defined in Skin XML are accessible by default. The ``accessibility`` attribute can be used to disable accessibility support per element.

The ``title`` of a skin element is used as the readable name or title of the user interface element.
If the ``title`` is not set, the element's ``tooltip`` is used instead.

=====
Title
=====

Some elements display a title that shouldn't be used for accessibility tools.

The ``accessibility.title`` attribute overrides the title announced by accessibility tools, without any
effect on what the element displays:

.. code-block:: xml

    <Toggle name="t" title="Title" tooltip="Tooltip" accessibility.title="The Title"/>

To avoid duplicated text, the attribute may contain the same ``@keyword[argument]`` text variables as
a ``tooltip``, including ``@title[]`` and ``@tooltip[]`` for the element's own title and composed
tooltip:

.. code-block:: xml

    <Toggle name="t" title="Title" tooltip="The Title" accessibility.title="@tooltip[]"/>

The resulting order of precedence for the announced title is ``accessibility.title``, the
``accessibility.label`` relation, ``title``, and finally ``tooltip``. If the resolved
``accessibility.title`` is empty, the element falls back to that default title.

.. note::
    With ``accessibility.proxy`` the title is taken from the proxied element, so
    ``accessibility.title`` belongs on the proxied element rather than on the proxying one.

=========
Relations
=========

In some cases, multiple skin elements are logically related to each other.

For example, a |xml.class.label| might describe the purpose of a |xml.class.valuebox|. 
In this case, accessibility tools should be informed that the Label's title can be utilized as a description for the ValueBox.

Relations can be expressed using the following skin attributes:

.. list-table::
    :widths: 25 75
    :header-rows: 1

    * - Attribute
      - Remarks
    * - ``accessibility.id``
      - ID that uniquely identifies a skin element. Defaults to the element's ``name``.
    * - ``accessibility.proxy``
      - Denotes a provider that should be used instead of the element's default provider.
    * - ``accessibility.label``
      - Denotes a provider for the label.
    * - ``accessibility.value``
      - Denotes a provider for the value.

==============================================
Toggle: exposing a one-shot action as a Button
==============================================

A |xml.class.toggle| exposes "adjustable" and "toggle" capabilities to accessibility tools, in addition
to being reported as a button. This is correct for a persistent on/off switch, but misleading for a
one-shot action (e.g. entering or leaving a mode) where the element disappears or is replaced afterward.

Set ``accessibility.button="true"`` on such a |xml.class.toggle| to expose it as a plain button instead.
The toggle itself and its interaction with the parameter stay exactly as authored; only what
accessibility tools are told about it changes.

**Example**

.. code-block:: xml

    <Toggle name="editMode" title="Enter Edit Mode" accessibility.button="true"/>

