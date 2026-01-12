#!/bin/bash
#
# Reads the repository configuration and returns the list of the directories
#
# Usage       : ./get_repo_paths.sh <scope>
#               The script will search the file repo.json upwards and
#               and return the paths in the <scope> array in that file
#
#************************************************************************************************

scriptdir=$(dirname "$0")
scope="$1"
config_name="repo.json"

if [ -z "${scope}" ]; then
	>&2 echo "Usage: ./get_repo_paths.sh <scope>"
	exit 1
fi

config_path=$(${scriptdir}/find_upwards.sh "${config_name}")
config_file="${config_path}/${config_name}"

if [ ! -f "${config_file}" ]; then
	>&2 echo "The file repo.json is not found!"
	exit 1
fi

paths=$(grep "$scope" "${config_file}" | grep -oE '\"([^\"]+)?\"' | sed 's/"//g' | tail -n +2)

echo ${paths}

exit 0