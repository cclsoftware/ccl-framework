.. include:: ../../reference/skin-elements-classmodel.ref.rst

#############
Accessibility
#############

CCL has built-in support for accessibility tools like screen readers to consume information about user interface elements in CCL-based applications.

All elements defined in Skin XML are accessible by default. The ``accessibility`` attribute can be used to disable accessibility support per element.

The ``title`` of a skin element is used as the readable name or title of the user interface element.
If the ``title`` is not set, the element's ``tooltip`` is used instead.

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
