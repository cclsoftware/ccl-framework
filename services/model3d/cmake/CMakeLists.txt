cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project ("3D Model Importer")

include (vendor)

find_package (ccl REQUIRED COMPONENTS cclbase)

find_package (modelimporter3d)
