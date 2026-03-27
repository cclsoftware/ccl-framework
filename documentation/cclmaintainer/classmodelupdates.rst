##################
Classmodel Updates
##################

============
Introduction
============

Classmodels of CCL framework classes are generated using the CCL Modeller command-line tool. Classes are scanned from the TypeLibrary entries of the linked CCL modules, which requires the CCL Modeller to be re-built in order to generate classmodels reflecting the current repository state.

==================
Update using CMake
==================

To simplify classmodel updates, the CCL Modeller CMake implementation provides a custom target, which builds CCL Modeller and runs it with the required arguments to generate currently supported classmodels.

In order to update the classmodels
    * Configure a CMake project containing CCL Modeller (e.g. *build-all*)
    * Build target **classmodelexport**