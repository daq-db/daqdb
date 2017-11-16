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

import os
import sys
import shutil
import argparse

IOEngine = "ioengine=sync"
IOEngine_DAX = "ioengine=pmemblk"

FilenameHowTo = ["#when executing on a file system you must specify size",
                 "#ex. filesystem",
                 "filename=/repo/fog/FogKV/src/scripts/fio/test_file",
                 "size=5g",
                 "#",
                 "#ex. raw device",
                 "#filename=/dev/sda"]

CommonSettings = ["runtime=10",
                  "time_based",
                  "#ramp_time=30",
                  "#refill_buffers",
                  "norandommap",
                  "randrepeat=0",
                  "overwrite=0",
                  "direct=1",
                  "group_reporting",
                  "rw=randrw",
                  "thread",
                  "thinktime=0"]

CommonJobFilePart = ["[global]",
                     "include common-settings.fio\n",
                     "[job]"]

WLAccess = ["rand", "seq"]
RW = ["100r", "66r", "0r"]
QD = ["1", "2", "4", "8", "16", "32"]
Threads = ["1", "2", "4", "8", "16", "$ncpus"]
BS = ["512", "1k", "2k", "4k", "8k", "16k", "32k"]

JFRoot = "FioJobFiles"


def GenJobFile(fPath, rPercenate, aType, thCount, bSize, qDepth):
    jobFileNameParts = (rPercenate, thCount + "th", qDepth + "qd", aType, bSize)
    jf = "_".join(jobFileNameParts)
    jf += ".fio"
    with open(fPath + "/" + jf, "w") as jfFile:
        specificPart = ["percentage_random=" + ("0" if aType == "seq" else "100"),
                        "rwmixread=" + rPercenate[:-1],
                        "bs=" + bSize,
                        "numjobs=" + thCount,
                        "iodepth=" + qDepth]

        jfFile.write("\n".join(CommonJobFilePart))
        jfFile.write("\n")
        jfFile.write("\n".join(specificPart))
        jfFile.write("\n")
        jfFile.write("\n")
        jfFile.write("\n".join(FilenameHowTo))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--dax",
                        action="store_true",
                        help="Generate jobfiles")

    args = parser.parse_args()

    ioengine = IOEngine
    if args.dax:
        JFRoot += "_DAX"
        ioengine = IOEngine_DAX

    os.mkdir(JFRoot)
    with open(JFRoot + "/common-settings.fio", "w") as fioCommon:
        fioCommon.write("\n".join(CommonSettings))
        fioCommon.write("\n")
        fioCommon.write(ioengine)

    for access in WLAccess:
        acDir = JFRoot + "/" + access
        os.mkdir(acDir)
        for rw in RW:
            rwDir = acDir + "/" + rw
            os.mkdir(rwDir)

            path = rwDir + "/thScaling"
            os.mkdir(path)
            for tCount in Threads:
                GenJobFile(path, rw, access, tCount, "4k", "1")

            path = rwDir + "/qdScaling"
            os.mkdir(path)
            for qdepth in QD:
                GenJobFile(path, rw, access, "1", "4k", qdepth)

            path = rwDir + "/bsScaling"
            os.mkdir(path)
            for bsize in BS:
                GenJobFile(path, rw, access, "1", bsize, "1")
