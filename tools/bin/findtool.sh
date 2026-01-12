#!/bin/bash
#
# Tools
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
# Filename    : findtool.sh
# Description : Finds a precompiled tool
#
# Usage       : ./findtool.sh <category> <toolname>
# Example     : ./findtool.sh ccl package
#
#************************************************************************************************

scriptdir=$(dirname "$0")
category="$1"
name="$2"

postfix=""
arch=""
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	platform=linux
	arch=$(uname -m)
elif [[ "$OSTYPE" == "darwin"* ]]; then
	platform=mac
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
	platform=win
	postfix=".exe"
else
	>&2 echo "Unknown platform"
	exit 1
fi

path="${scriptdir}/${platform}/${category}/${arch}/${name}${postfix}"

if [ ! -f "${path}" ]; then
	path="${scriptdir}/${platform}/${category}/${arch}/${category}${name}${postfix}"
fi

if [ ! -f "${path}" ]; then
	path=$(which "${name}" 2>/dev/null)
fi

if [ ! -f "${path}" ]; then
	>&2 echo "Tool \"${name}\" not found!"
	exit 1
fi

echo ${path}
exit 0
