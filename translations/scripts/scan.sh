#!/bin/bash
#
# Scan source files for translatable strings and creates pot files
#
# Usage       : ./scan.sh <optional translations path>
#               When this script is called without a path, it will scan all translation
#               directories in the repository and submodules.
#               The scan configurations are expected to the in a /modules/config.txt file
#               in the given or found locations.
#
#************************************************************************************************

scriptdir=$(dirname "$0")
config="$1"

if [ -z ${config} ]; then
	repo_root=$("${scriptdir}/get_repo_root.sh")
	if [ -z ${repo_root} ]; then
		>&2 echo "Cannot scan without repository root information!"
		exit 1
	fi
	translations=$("${scriptdir}/get_repo_paths.sh" translations)
	echo "Scanning in: ${translations}"
	for translation in ${translations}; do
		${scriptdir}/makepot.sh "${repo_root}/${translation}/modules/config.txt"
	done
else
	${scriptdir}/makepot.sh "${config}/modules/config.txt"
fi

exit 0
