############
Introduction
############

The CCL Cross-platform Framework provides an XML-based user interface definition language to describe the appearance of an application.

A skin package is used to deploy the relevant assets. It can either be a folder or a ZIP archive with the filename extension .skin, containing XML files, images (in formats like JPEG, PNG, WebP, SVG), fonts, etc. When the application launches, it looks for a skin package at a predefined location, parses its XML content and loads the primary form "ApplicationWindow".

Conceptually, the skin system separates the application's business logic from its graphical representation on screen. Both sides, the application code and the GUI, can address and connect to each other based on a naming scheme. The application code provides a tree of named components with parameters and properties. The skin data model in turn provides resources like images, styles, forms, etc. also associated with names.
