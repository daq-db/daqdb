FioJobFileGen script generates a set of job files with worklods that scale over thread count, queue depth and io size.
The job files are placed in a directory structure that looks as follows:
    FioJobFiles\ # or FioJobFiles_DAX if dax option specified
        rand\
            0r\
                bsScaling\
                    # job files specifying workloads that differ in io size
                qdScaling\
                    # job files specifying workloads that differ in queue depth
                thScaling\
                    # job files specifying workloads that differ in number of threads
            66r\
                # as above
            100r\
                # as above
        seq\
            # as above
        common-settings.fio

 Job file naming convention:
    <read-percentage>r_<thread-count>th_<queue-depth>qd_[rand|seq]_<io-size>.fio
    ex.
        0r_1th_1qd_rand_1k.fio # 0% read (only writes), 1 thread, queue depth 1, full random, io size 1k
        0r_16th_1qd_rand_4k.fio # 0% read (only writes), 16 threads, queue depth 1, full random, io size 4k
        100r_$ncpusth_1qd_seq_4k.fio # 100% read , thread number equal to logical processors count, queue depth 1, full sequential, io size 4k

common-settings.fio - this file is used to store workload settings that are common for each job file.
In order for the include to work properly one needs place this file in the fio.exe working directory.
ex.
    FioJobFiles\fio.exe seq\100r\thScaling\100r_$ncpusth_1qd_seq_4k.fio # OK!
    FioJobFiles\seq\100r\thScaling\fio.exe 100r_$ncpusth_1qd_seq_4k.fio # NOT OK! fio will not be able to include common-settings.fio and will try to use defaults

IMPORTANT!
    The script does not add 'filename' parameter, this needs to be done manually

    Raw device ex:
        filename=\\.\PhysicalDriveX # X is a proper drive index: 0, 1, ...
    Filesystem ex:
        filename=E\:\\test_file
        size=5g # size of the created file, required in filesystem scenario
