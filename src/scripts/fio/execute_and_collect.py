#
#   INTEL CONFIDENTIAL
#
#   Copyright 2017 Intel Corporation
#
#   The source code contained or described herein and all documents related to the
#   source code ("Material") are owned by Intel Corporation or its suppliers or
#   licensors. Title to the Material remains with Intel Corporation or its
#   suppliers and licensors. The Material contains trade secrets and proprietary
#   and confidential information of Intel or its suppliers and licensors.
#   The Material is protected by worldwide copyright and trade secret laws and
#   treaty provisions. No part of the Material may be used, copied, reproduced,
#   modified, published, uploaded, posted, transmitted, distributed, or disclosed
#   in any way without Intel's prior express written permission.
#
#   No license under any patent, copyright, trade secret or other intellectual
#   property right is granted to or conferred upon you by disclosure or delivery
#   of the Materials, either expressly, by implication, inducement, estoppel or
#   otherwise. Any license under such intellectual property rights must be express
#   and approved by Intel in writing.
#

import argparse
import exitstatus as es
import os.path
import sys
from fnmatch import fnmatch
import shutil
import subprocess
import time

DEFAULT_FIO_RUNTIME = 60

FIO_CONFIGS_DIR = 'FioJobFiles'
FIO_CONFIGS_EXCLUDE = ['common-settings.fio']

TMP_DIR = 'tmp'

module_path = os.path.dirname(os.path.abspath(__file__))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for gathering xperf data - from fio io benchmark.')

    # parse input parameters
    parser.add_argument('--runtime', nargs='?', const=DEFAULT_FIO_RUNTIME, default=None,
                        help='fio runtime in seconds (default: 60)')

    args = parser.parse_args()
    runtime = DEFAULT_FIO_RUNTIME
    if args.runtime is not None:
        runtime = args.runtime
    is_filesystem_benchmark = True

    root = os.path.join(module_path, FIO_CONFIGS_DIR)
    if not os.path.isdir(root):
        print 'Cannot find fio configs folder [%s]' % root
        sys.exit(es.ExitStatus.failure)

    # analyze fio config files
    fio_config_files = []
    fio_config_names = []
    pattern = "*.fio"
    for path, subdirs, files in os.walk(root):
        for name in files:
            if fnmatch(name, pattern):
                if not any(name in excluded for excluded in FIO_CONFIGS_EXCLUDE):
                    if not (name in fio_config_names):
                        fio_config_files.append({'filename': name, 'path': os.path.join(path, name),
                                                'full_path': os.path.join(path, name)})
                        fio_config_names.append(name)

    # update fio config files with filename and runtime
    tmp_directory = os.path.join(module_path, TMP_DIR)
    if not os.path.exists(tmp_directory):
        os.mkdir(tmp_directory)
    for includes in FIO_CONFIGS_EXCLUDE:
        if os.path.exists(os.path.join(root, includes)):
            shutil.copy(os.path.join(root, includes), tmp_directory)
    for fio_config in fio_config_files:
        file_to_update = os.path.join(tmp_directory, fio_config['filename'])
        if os.path.exists(file_to_update):
            os.remove(file_to_update)
        shutil.copy(fio_config['full_path'], tmp_directory)
        with open(file_to_update, "a") as file_handle:
            if is_filesystem_benchmark:
                file_handle.write("\nruntime=%s\n" % runtime)
            else:
                file_handle.write("\nruntime=%s\n" % runtime)

    result_directory = 'results_%s' % time.strftime("%Y%m%d-%H%M%S")
    os.mkdir(result_directory)

    # execute fio and xperf
    all_files_to_process = len(fio_config_files)
    file_index = 0
    for fio_config in fio_config_files:
        file_index += 1
        print '\nprocess [%s][%d/%d]...' % (fio_config['filename'], file_index, all_files_to_process)

        execute_config = '%s/%s' % (TMP_DIR, fio_config['filename'])
        base_config_name = os.path.splitext(fio_config['filename'])[0]
        log_filename = "%s.log" % base_config_name
        execute_config_output = os.path.join(tmp_directory, log_filename)
        subprocess.Popen(['fio', '--output', execute_config_output, execute_config]).wait()

        result_folder = os.path.join(result_directory, base_config_name)
        os.mkdir(result_folder)

        shutil.copy(execute_config_output, result_folder)

    if os.path.exists(tmp_directory):
        shutil.rmtree(tmp_directory)

