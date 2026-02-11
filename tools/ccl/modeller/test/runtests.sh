#!/bin/bash

#************************************************************************************************
# Modeller Tool
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
# Description : Modeller functional tests script.
#
# Usage       : ./runtests.sh
#
#************************************************************************************************

# ///////////////////////////////////////////////////////////////////////////
# // Globals
# ///////////////////////////////////////////////////////////////////////////

scriptdir=$(dirname "$0")
modeller=$(${scriptdir}/../../../../tools/bin/findtool.sh ccl modeller)
console_log="./output/console.log"
succeeded=0 # Number of succeeded tests
failed=0 # Number of failed tests

# ///////////////////////////////////////////////////////////////////////////

log ()
{
	# Print a log message. Add arbitrary
	# to separate from other output.
	#
	# $1 - message to print
	#
	# Example:
	#
	#	log "hello ${somevar}"
	#

	msg=$1
	echo "[*] ${msg}"
}

# ///////////////////////////////////////////////////////////////////////////

compare_output ()
{
	# Compare test result to reference file.
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

	reference=$1
	output=$2

	diff --strip-trailing-cr --brief <(cat $reference) <(cat $output) >/dev/null
	echo $?
}

# ///////////////////////////////////////////////////////////////////////////

cleanup_output ()
{
	# Cleanup output folder.
	rm "./output/*.*" 2> /dev/null
}

# ///////////////////////////////////////////////////////////////////////////

copy_input ()
{
	# Copy input file to output folder.
	#
	# $1 - file to copy inside input/ folder
	# $2 - target filename in output/ folder
	#

	infile=$1
	outfile=$2
	cp "./input/${infile}" "./output/${outfile}"
}

# ///////////////////////////////////////////////////////////////////////////

run_test ()
{
	# Run modeller tool with command line arguments.
	# Make return value contribute to test results.
	#
	# $1 - modeller scan mode
	# $2 - modeller file 1 argument
	# $3 - modeller file 2 argument
	# $4...$6 - optional modeller arguments
	#

	mode=$1
	file1=$2
	file2=$3
	arg1=$4
	arg2=$5
	arg3=$6

	${modeller} ${mode} "${file1}" "${file2}" ${arg1} ${arg2} ${arg3} > "$console_log"
	modeller_result=$?

	# Gracefully check whether modeller returned an error
	# code. This may happen for an unsupported command line.
	if [ $modeller_result -ne 0 ]
	then
		let failed=failed+1
		log "Test '$testcase': FAIL [modeller error]"
	fi
}

# ///////////////////////////////////////////////////////////////////////////

verify_equal ()
{
	# Compare reference file to generated output file.
	#
	# $1 - reference file, expected value
	# $2 - output file
	#

	expected=$1
	actual=$2

	result=$(compare_output "./ref/${expected}" "./output/${actual}")

	# Print test result
	if [ $result -eq 1 ]
	then
		log "Test '$testcase': FAIL [output files mismatch]"
		let failed=failed+1
	else
		log "Test '$testcase': OK"
		let succeeded=succeeded+1
	fi
}

# ///////////////////////////////////////////////////////////////////////////

verify_exists ()
{
	# Check that the passed in file exists.
	# $1 - file expected to exist

	expected_file=$1

	if [ ! -e "${expected_file}" ]
	then
		log "Test '$testcase': FAIL [output file does not exist]"
		let failed=failed+1
	else
		log "Test '$testcase': OK"
		let succeeded=succeeded+1
	fi
}

# ///////////////////////////////////////////////////////////////////////////

verify_contains ()
{
	# Check that  passed-in file exists.
	# $1 - file expected to exist
	# $2 - search string

	file_to_check=$1
	search_string=$2

	if grep -q "${search_string}" "${file_to_check}"
	then
		log "Test '$testcase': OK"
		let succeeded=succeeded+1
	else
		log "Test '$testcase': FAIL [search string not found]"
		let failed=failed+1
	fi
}

# ///////////////////////////////////////////////////////////////////////////

main ()
{
	# Run series of test cases.

	log "Using binary ${modeller}"

	############################################
	# Action: -update
	# Note: input file is modified by tool,
	# test must use a copy.
	############################################

	# Update documented class model from prototype class
	# model, do not generate documentation stubs.
	testcase="update_model_no_options"
	cleanup_output
	copy_input "Documented.classModel" "Update.classModel"
	run_test "-update" "./output/Update.classModel" "input/Proto.classModel"
	verify_equal "Update.classModel.ref" "Update.classModel"

	############################################
	# Action: -scan
	# Note: input file is modified by tool,
	# test must use a copy.
	############################################

	# Scan skin element documentation from source file.
	testcase="scan_source_no_options"
	cleanup_output
	copy_input "Skin.classModel" "Scan.classModel"
	run_test "-scan" "./input/" "./output/Scan.classModel"
	verify_equal "Scan.classModel.ref" "Scan.classModel"

	############################################
	# Action: -export
	# Note: Does not check for class model
	# content
	############################################

	testcase="export_classmodel"
	cleanup_output
	run_test "-export" "Cross-platform System Framework" "./output/Exported.classModel"
	verify_exists "./output/Exported.classModel"

	############################################
	# Action: -list
	############################################

	testcase="list_type_libraries"
	cleanup_output
	run_test "-list"
	verify_contains ${console_log} "Found type library"

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
