#!/usr/bin/python3

"""
Inspired from
- https://github.com/zaheersm/ebpf-trekking/tree/master/treks/ping_reply
- https://github.com/badboy/ebpf-icmp-ping/
- https://github.com/path-network/bpf-echo/ (BSD-License)
"""

from bcc import BPF
from pyroute2 import IPRoute
import time

udpecho_source = open("udpecho_ebpf.c")
udpecho = udpecho_source.read()
udpecho_source.close() 

ipr = IPRoute()
ifIndex = ipr.link_lookup(ifname="eth1")[0]

b = BPF(text=udpecho)
bpfFunction = b.load_func("udpecho", BPF.SCHED_CLS)
ipr.tc("add", "ingress", ifIndex, "ffff:")

ipr.tc("add-filter",
       "bpf",
       ifIndex,
       ":1",
       fd=bpfFunction.fd,
       name=bpfFunction.name,
       parent="ffff:",
       action="drop",
       classid=1)

try:
    print ("udpecho running...")
    while True:
        time.sleep(60)
except KeyboardInterrupt:
    print ("udpecho terminating...")
finally:
    ipr.tc("del", "ingress", ifIndex, "ffff:")
