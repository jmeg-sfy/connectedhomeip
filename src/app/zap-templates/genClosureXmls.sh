#!/usr/bin/env bash

#
# Copyright (c) 2021-2022 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#



set -e # Stop at the first error
#set -x # To see and debug all commands

## Color helper ##
# Reset
Color_Off='\033[0m'       # Text Reset
Nc='\033[0m'              # Text Reset

# Regular Colors
Black='\033[0;30m'        # Black
Red='\033[0;31m'          # Red
Green='\033[0;32m'        # Green
Yellow='\033[0;33m'       # Yellow
Blue='\033[0;34m'         # Blue
Purple='\033[0;35m'       # Purple
Cyan='\033[0;36m'         # Cyan
White='\033[0;37m'        # White

# Background
On_Black='\033[40m'       # Black
On_Red='\033[41m'         # Red
On_Green='\033[42m'       # Green
On_Yellow='\033[43m'      # Yellow
On_Blue='\033[44m'        # Blue
On_Purple='\033[45m'      # Purple
On_Cyan='\033[46m'        # Cyan
On_White='\033[47m'       # White

baseName=$(basename -- "$0";)

echo "====== GenClosureXmls Beging ======"

cliUsage() {
	echo "Usage: $baseName [optional parameters]"
	echo ""
	echo "Generate XMLs"
	echo ""
	echo "Script parameters:"
	echo "  --help                         Print this help"
	echo ""
	echo "All these parameters should be preset"
	echo "but those specified on the command line have a higher priority."
}

errReport() {
	echo "Error on line $1 Parser=$parser"
	if [[ $parser == "ko" ]]; then
	echo -e "${Red}Error parsing argument${Nc}"
	cliUsage
	fi
}

#pushd "chip" #enter @chip root directory
#trap 'echo \"EXIT restore directory\";popd' EXIT # executed at EXIT
trap 'errReport $LINENO'  ERR                    # executed at ERR

here=${0%/*}

echo "The script you are running has basename $( basename -- "$0"; ), dirname $( dirname -- "$0"; )";
echo "Script is located $here"

# Whether to display help
help=no_help
parser=ok

# Parse arguments
while [ $# -gt 0 ]; do
	if [[ $1 == *"--"* ]]; then
		param="${1/--/}"
		if [[ $2 != *"--"* ]] && [[ ! -z "$2" ]]; then
			declare $param="$2"
		else
			# for variables without value define this value as "empty"
			declare $param="empty"
		fi
		if [[ $2 != *"--"* ]]; then
			# shift only when variable has some value
			shift
		fi
	fi
	parser=ko
	shift
	parser=ok
done

if [ "$help" != "no_help" ]; then
	cliUsage
	exit 0
else
	echo "Usage SKIPPED"
fi



###########################
# Declare functions       #
###########################

###########################
# Global Variables        #
###########################

declare -A CLUSTERS
declare -A CLUSTER

# Preset a particular CLUSTER_TYPE w/ its list of PIDs (USED to generate the corresponding DACs + their CD)
# Each PID gives a DAC
# Each CLUSTER_TYPE gives a CD w/ allowing the bundle of PIDs


CLUSTER=([cluster_id]="0x0CD1" [cluster_short_lower]="1st" [cluster_short_upper]="1ST" )
holder=$(declare -p CLUSTER)
CLUSTERS["1"]=${holder}

CLUSTER=([cluster_id]="0x0CD2" [cluster_short_lower]="2nd" [cluster_short_upper]="2ND" )
holder=$(declare -p CLUSTER)
CLUSTERS["2"]=${holder}

CLUSTER=([cluster_id]="0x0CD3" [cluster_short_lower]="3rd" [cluster_short_upper]="3RD" )
holder=$(declare -p CLUSTER)
CLUSTERS["3"]=${holder}


echo -e "${On_Cyan}===> Step: Generate DACs & CDs:${Nc} per CLUSTER_TYPE"
{
	# Loop toward the declared CLUSTERS type
	# In order to generate the XMLs with their corresponding Identifier
	for SELECT in "${!CLUSTERS[@]}"; do
		printf "A: $SELECT - ${CLUSTERS["$SELECT"]}\n"
		eval "${CLUSTERS["$SELECT"]}" #Eval trick to spaw the CLUSTER_TYPE

		# Extract the information for the array
		# Use read to bash split pids_list into array
		#read -ra pid_pids <<< "$pids_list"
		cluster_short_lower=${CLUSTER[cluster_short_lower]}
		cluster_short_upper=${CLUSTER[cluster_short_upper]}
		cluster_id=${CLUSTER[cluster_id]}
		echo "CLUSTER_TYPE=${cluster_short_id} is composed of ${cluster_short_upper} ${cluster_short_lower}"

		file_name="closure-${cluster_short_lower}-dimension-cluster.xml"
		output_file="zcl/data-model/chip/${file_name}"
		echo "FileName=${file_name}"
		cp zcl/data-model/chip/closure-dimension-clusters.xml ${output_file}

		# Replacement fields within the new cluster
		sed -i -e "s/\[cluster_short_lower\]/${cluster_short_lower}/g" ${output_file}
		sed -i -e "s/\[cluster_short_upper\]/${cluster_short_upper}/g" ${output_file}
		sed -i -e "s/\[cluster_id\]/${cluster_id}/g"                   ${output_file}

	done
}

echo "====== GenClosureXmls Ending ======"

###########################
# Ending: Dump some infos #
###########################

dirs -l -v
echo "Generated files under ${dst_dir}"
ls -l $dst_dir

echo -e "FYI ${On_Purple}Tips: Chip-tool's commissioning \$PATH ${Nc}"

exit 0

