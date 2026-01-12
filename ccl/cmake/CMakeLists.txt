#************************************************************************************************
#
# This file is part of Crystal Class Library (R)
# Copyright (c) 2025 CCL Software Licensing GmbH.
# All Rights Reserved.
#
# Licensed for use under either:
#  1. a Commercial License provided by CCL Software Licensing GmbH, or
#  2. GNU Affero General Public License v3.0 (AGPLv3).
# 
# You must choose and comply with one of the above licensing options.
# For more information, please visit ccl.dev.
#
# Filename    : CMakeLists.txt
#[*****************************************************************************************[.rst:
# CMakeLists.txt
# -------
#
# CCL framework CMake targets
# 
# Targets
# ^^^^^^^
#
# This project contains following targets:
#
# ``cclapp``
# ``cclbase``
# ``cclgui``
# ``cclnet``
# ``cclsecurity``
# ``cclsystem``
# ``ccltext``
# ``ccltest``
#
#]**********************************************************************************************]

cmake_minimum_required (VERSION 3.30 FATAL_ERROR)

project (ccl)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

set (corelib_ENABLE_CORETEST ON CACHE BOOL "Mandatory for this project" FORCE)
	
include (vendor)
find_package (ccl REQUIRED COMPONENTS cclapp cclbase ccltext cclsecurity cclsystem cclnet cclgui cclstatic ccltest)
