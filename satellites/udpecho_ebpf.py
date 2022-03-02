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
import argparse

# Parse arguments
argParser = argparse.ArgumentParser(description=
"""Echo UDP packets, preserve IP- and UDP Headers.
Run without optional arguments, it will respond on any UDP port.""")

argParser.add_argument("interface",
                       help="interface on which to listen")

portGroup = argParser.add_mutually_exclusive_group()
portGroup.add_argument("-p", "--port", type=int,
                       help="port on which to listen")
portGroup.add_argument("-r", "--range", type=int, nargs=2,
                       help="port range on which to listen")
args = argParser.parse_args()

cflags=[]
if args.port:
    cflags.append("-DPORT={}".format(args.port))
if args.range:
    cflags.append("-DPORTMIN={}".format(args.range[0]))
    cflags.append("-DPORTMAX={}".format(args.range[1]))

# Load the eBPF filter
udpecho_source = open("udpecho_ebpf.c")
udpecho = udpecho_source.read()
udpecho_source.close() 

ipr = IPRoute()

try:
    ifIndex = ipr.link_lookup(ifname=args.interface)[0]
except:
    print("Error: Interface " + args.interface + " not found.")
    exit()

b = BPF(text=udpecho, cflags=cflags)
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
