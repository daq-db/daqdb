#
# INTEL CONFIDENTIAL
#
#    Copyright 2017 Intel Corporation
#
#    The source code contained or described herein and all documents related to the
#    source code ("Material") are owned by Intel Corporation or its suppliers or
#    licensors. Title to the Material remains with Intel Corporation or its
#    suppliers and licensors. The Material contains trade secrets and proprietary
#    and confidential information of Intel or its suppliers and licensors.
#    The Material is protected by worldwide copyright and trade secret laws and
#    treaty provisions. No part of the Material may be used, copied, reproduced,
#    modified, published, uploaded, posted, transmitted, distributed, or disclosed
#    in any way without Intel's prior express written permission.
#
#    No license under any patent, copyright, trade secret or other intellectual
#    property right is granted to or conferred upon you by disclosure or delivery
#    of the Materials, either expressly, by implication, inducement, estoppel or
#    otherwise. Any license under such intellectual property rights must be express
#    and approved by Intel in writing.
#

Description:

	FioJobFileGen.py
		Generates fio configuration files.
	execute_and_collect.py
		Tool for analyzing fio benchmark results - generated sorted tables with latency and bandwith
	execute_and_collect.py
		Script executes fio configurations from a given folder

Usage:
	python ./sort_results.py --folder results_20171102-154541/
	python ./execute_and_collect.py --runtime 10

Python depedencies:
	python 2.7
	exitstatus
	fnmatch
	bitmath
