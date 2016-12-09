import sys
import numpy

def median(lst):
    return numpy.median(numpy.array(lst))

fname = sys.argv[1]

rttdict = dict()

with open(fname) as f: 
    for v in f:
        ts = v.split(",")[0].split(".")
        rtt = v.split(",")[1].split(".")   
        
        ts_sec = ts[0]
        rtt_ms = rtt[1] * 1000
        if ts_sec not in rttdict:
            rttdict[ts_sec] = []
       
        rttdict[ts_sec].append(rtt_ms)

for key, value in rttdict:
    print median(value)
