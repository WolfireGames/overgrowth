#!/usr/bin/python

import numpy as np
import sys
import csv

if len(sys.argv) == 3:
    main_data = open(sys.argv[1])

    with open(sys.argv[2], mode='r') as infile:
        reader = csv.reader(infile)
        file_names = {int(rows[0]):rows[1] for rows in reader}

    raw_struct = np.loadtxt(main_data, delimiter=',', dtype={ 'names' : ('frame', 'user_def_ptr', 'file_ptr', 'line', 'duration nanoseconds' ),
                                    'formats' : ('i4', 'i8', 'i8', 'i4', 'i16' ) } )
    file_sorted = np.sort( raw_struct, order=["file_ptr", "line", "frame"] )
    
    split_ids = []
    last_val = None;
    index = 0
    for i in file_sorted:
        if last_val != None and last_val != (i[2],i[3]):
            split_ids.append(index)
        last_val = (i[2],i[3])
        index += 1

    split = np.split(file_sorted,split_ids)

    for i in split:
        print "{}:{},{},{},{}".format(file_names[int(i[0][2])],i[0][3],np.mean([row[4] for row in i]),np.median([row[4] for row in i]),len(i))
    
    #np.split(raw_struct, np.where(np.diff[
    
else:
    print "incorrect number of parameters"


