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
import os
import sys
import bitmath
import exitstatus as es
from fnmatch import fnmatch

module_path = os.path.dirname(os.path.abspath(__file__))


def get_read_stats(in_data):
    """
    Retrieve data from fio 'read' stat line
    example (read : io=1017.9MB, bw=104211KB/s, iops=26052, runt= 10001msec)
    :param in_data: line to parse
    :return: list with: bandwith, iops
    """
    result_data = in_data[in_data.find(':') + 1:]
    stats = result_data.split(',')

    result_bw = 0
    result_bw_str = ''
    result_iops = 0

    for stat in stats:
        if 'BW' in stat:
            result_bw_str = stat.split('=')[1]
        elif 'IOPS' in stat:
            result_iops_str = stat.split('=')[1]

    if 'KiB' in result_bw_str:
        result_bw = float(result_bw_str[:result_bw_str.find('K')])
        result_bw *= 1024
    elif 'MiB' in result_bw_str:
        result_bw = float(result_bw_str[:result_bw_str.find('M')])
        result_bw *= 1024 * 1024

    if 'k' in result_iops_str:
        result_iops = float(result_iops_str[:result_iops_str.find('k')])
        result_iops *= 1000

    return result_bw, result_iops

def get_latency_ms(in_data):
    """
    Retrieve data from fio 'lat' stat line
    example (lat (usec): min=113, max=3318, avg=151.96, stdev=26.04)
    :param in_data: line to parse
    :return: average latency
    """
    return get_latency(in_data) * 1000

def get_latency(in_data):
    """
    Retrieve data from fio 'lat' stat line
    example (lat (usec): min=113, max=3318, avg=151.96, stdev=26.04)
    :param in_data: line to parse
    :return: average latency
    """
    result_data = in_data[in_data.find(':') + 1:]
    stats = result_data.split(',')

    result_avg = 0
    for stat in stats:
        if 'avg' in stat:
            result_avg = float(stat.split('=')[1])
    return result_avg


def get_cpu(in_data):
    """
    Retrieve data from fio 'cpu' stat line
    example (cpu          : usr=0.00%, sys=0.00%, ctx=0, majf=0, minf=0)
    :param in_data: line to parse
    :return: cpu usage
    """
    result_data = in_data[in_data.find(':') + 1:]
    stats = result_data.split(',')

    result_cpu = 0
    for stat in stats:
        if 'sys' in stat:
            result_cpu_str = stat.split('=')[1]
            result_cpu = float(result_cpu_str[:len(result_cpu_str) - 1])
    return result_cpu


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for analyzing fio benchmark results.')

    # parse input parameters
    parser.add_argument('--folder', nargs='?', const=None, default=None, required=True,
                        help='fio result directory')
    args = parser.parse_args()

    if args.folder is not None:
        root = os.path.join(module_path, args.folder)
        if not os.path.isdir(root):
            print 'Cannot result folder [%s]' % root
            sys.exit(es.ExitStatus.failure)
    else:
        print 'Folder not specified'
        sys.exit(es.ExitStatus.failure)

    # analyze fio config files
    fio_config_files = []
    pattern = "*.log"
    for path, subdirs, files in os.walk(root):
        for name in files:
            if fnmatch(name, pattern):
                fio_config_files.append({'filename': name, 'path': os.path.join(path, name), 'full_path': os.path.join(path, name)})

    for fio_config in fio_config_files:
        with open(fio_config['full_path'], "r") as fio_result_file:
            fio_config['bwr'] = 0
            fio_config['bww'] = 0
            fio_config['lat'] = 0
            for line in fio_result_file:

                if 'read:' in line:
                    bw, iops = get_read_stats(line)
                    fio_config['bwr'] = bw
                    fio_config['iops'] = iops
                if 'write:' in line:
                    bw, iops = get_read_stats(line)
                    fio_config['bww'] = bw
                    fio_config['iops'] = iops
                elif 'lat (usec): min' in line:
                    fio_config['lat'] = get_latency(line)
                elif 'lat (msec): min' in line:
                    fio_config['lat'] = get_latency_ms(line)
                elif 'cpu' in line:
                    fio_config['cpu'] = get_cpu(line)

    print '-' * 80
    sorted_by_cpu = sorted(fio_config_files, key=lambda cpu: cpu['cpu'], reverse=True)
    for fio_config in sorted_by_cpu:
        print 'cpu = [%s]: %s - bwr[%s]; bww[%s]; iops[%s]; lat[%s]' % (
            fio_config['cpu'], fio_config['filename'], bitmath.Byte(bytes=fio_config['bwr']).best_prefix(), bitmath.Byte(bytes=fio_config['bww']).best_prefix(), fio_config['iops'], fio_config['lat'])

    print '-' * 80
    sorted_by_lat = sorted(fio_config_files, key=lambda lat: lat['lat'])
    for fio_config in sorted_by_lat:
        print 'latency = [%s us]: %s - bwr[%s]; bww[%s]; iops[%s]; cpu[%s]' % (
            fio_config['lat'], fio_config['filename'], fio_config['bwr'], fio_config['bww'], fio_config['iops'],
            fio_config['cpu'])

    print '-' * 80
    sorted_by_bw = sorted(fio_config_files, key=lambda bw: bw['bwr'], reverse=True)
    for fio_config in sorted_by_bw:
        print 'bwr = [%s]: %s - iops[%s]; lat[%s], cpu[%s]' % (
            bitmath.Byte(bytes=fio_config['bwr']).best_prefix(), fio_config['filename'], fio_config['iops'], fio_config['lat'],
            fio_config['cpu'])
                
