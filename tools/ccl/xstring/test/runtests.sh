#!/bin/bash

#************************************************************************************************
# CCL String Extractor
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
# Filename    : runtests.sh
# Description : Run xstring functional tests.
#
# Usage       : ./runtests.sh
#
#************************************************************************************************

scriptdir=$(dirname "$0")
xstring=$(${scriptdir}/../../../../tools/bin/findtool.sh ccl xstring)
succeeded=0 # Number of succeeded tests
failed=0 # Number of failed tests

# ///////////////////////////////////////////////////////////////////////////

# Public: print a log message. Add
# some arbitrary prefix to separate
# script logs from any other output.
#
# $1 - message to print
#
# Example:
#
#	log "hello ${somevar}"
#
log ()
{
	msg=$1
	echo "[*] ${msg}"
}

# ///////////////////////////////////////////////////////////////////////////

# Public: compare test result to reference file.
#
# $1 - path to reference file
# $2 - path to output file
# $3 - test name to include in output
#
# Returns the comparison result as 0: success, 1: failure.
#
# Example:
#
#   compare_output "file.ref" "file.out" "mytest"
#
compare_output ()
{
  reference=$1
  output=$2
  testname=$3

  # Ignore comments for now. They show the string source
  # file but differ between release and debug builds.
  diff --brief <(grep -v '^#' $reference) <(grep -v '^#' $output) >/dev/null
  echo $?
}

# ///////////////////////////////////////////////////////////////////////////

# Public: run test
#
# Create a po file from a content folder, compare the output
# to a reference file.
#
# $1 - xstring input mode argument
# $2 - path to content folder, basename for output file
# $3 - path to model file (optional)
#
# Example:
#
#   run_test "-menu" "menu"
#
run_test ()
{
	mode=$1
	testcase=$2
	model=$3

  # Cleanup output file from any previous run.
  # xstring may fail, preserving an existing
  # file, leading to false positives.
  rm "${testcase}.po" 2> /dev/null

	# Run xstring.
	${xstring} ${mode} ./input -po ./${testcase}.po ./${model} -v >/dev/null
	xstring_result=$?

	# Gracefully check whether xstring returned an error
	# code. This may happen for an unsupported command line.
	if [ $xstring_result -ne 0 ]
	then
		let failed=failed+1
		log "Test '$testcase': FAIL [xstring error]"
	else
		# Compare reference file to generated output file.
		result=$(compare_output "${testcase}.po.ref" "${testcase}.po" $testcase)

		# Print test result
		if [ $result -eq 1 ]
		then
			log "Test '$testcase': FAIL [output mismatch]"
			let failed=failed+1
		else
			log "Test '$testcase': OK"
			let succeeded=succeeded+1
		fi
	fi
}

# ///////////////////////////////////////////////////////////////////////////

main ()
{
  log "Using binary ${xstring}"

  ############################################
  # Exclusive mode tests.
  ############################################

  run_test "-menu" "menu"
  run_test "-skin" "skin"
  run_test "-code" "code"
  run_test "-tutorial" "tutorial"
  run_test "-metainfo" "metainfo"
  run_test "-template" "template"
  run_test "-custom" "custom" "input/custom.json"

  ############################################
  # Auto mode tests
  ############################################

  run_test "-auto" "auto" "input/custom.json"

  ############################################
  # Model replacement or inheritance tests.
  ############################################

  # Replace built-in format (new model).
  run_test "-skin" "skin_custom" "input/skincustom.json"

  # Modify built-in format (inheriting model).
  run_test "-template" "inherited_builtin" "input/inherited.json"

  # Introduce custom format (inheriting model).
  run_test "-custom" "inherited_custom" "input/inherited.json"

  # Model supports multiple file extensions.
  run_test "-custom" "inherited_multi" "input/inheritedmulti.json"

  ############################################
  # Other features test.
  ############################################

  # Root can have conditions attached.
  run_test "-custom" "rootcondition" "input/rootcondition.json"

  ############################################
  # Test result summary
  ############################################

  log "----------------------------"
  log "Total succeeded tests: ${succeeded}"
  log "Total failed tests: ${failed}"
  log "----------------------------"

  # Reminder: evaluate exit code in caller (Jenkins).
  if [ $failed -ne 0 ]
  then
    exit 1
  fi
}

# ///////////////////////////////////////////////////////////////////////////

main
