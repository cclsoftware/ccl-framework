#!/bin/bash
#
# Normalize po files to be sorted and have no line wraps
#
# Usage       : ./normalizeall.sh <optional translations path>
#               When this script is called without a path, it will scan all translation
#               directories in the repository and submodules.
#
#************************************************************************************************

scriptdir=$(dirname "$0")
config="$1"
repo_root=""

languages="de es fr it ja ko pl pt tr zh"

pip install polib

if [ -z ${config} ]; then
	repo_root=$("${scriptdir}/get_repo_root.sh")
	if [ -z ${repo_root} ]; then
		>&2 echo "Cannot normalize without repository root information!"
		exit 1
	fi
	translations=$("${scriptdir}/get_repo_paths.sh" translations)
	echo "Normalizing in ${translations}"
else
	translations=${config}	
fi

for translation in ${translations}; do
	if [ -z "${config}" ]; then
		# path found in repo config is relative to repo root
		translation_path="${repo_root}/${translation}"
	else
		translation_path="${translation}"
	fi
	for lang in ${languages}; do
		echo "-----------------------------------------"
		echo "-- Normalizing ${translation_path} ${lang} --"
		echo "-----------------------------------------"
		${scriptdir}/normalize.sh "${translation_path}" ${lang}
	done
done

exit 0
