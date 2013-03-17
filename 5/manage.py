#!/usr/bin/env python
"""
manage's job is to maintain the results of ``compute.'' It also keeps track of
the active compute processes, so that it can signal them to terminate,
including the host name and performance characteristics.
"""

import json
import sys
import socket

from math import sqrt
from select import select
from signal import signal, SIGHUP, SIGINT, SIGQUIT

__author__ = "Jordan Bayles (baylesj)"
__email__ = "baylesj@engr.orst.edu"
__credits__ = "Meghan Gorman (algorithm development)"

# Constants
BUF_SIZE = 4096
HOME = "localhost"
MAX_BACKLOG = 5
PORT = 44479
RUN_TIME = 15

# Functions
def get_packets(sock):
    received = sock.recv(BUF_SIZE).decode('utf-8')
    return (received.strip().split("\n"))

def ack_get(sock, comps, comp_mons):
    packets = get_packets(sock)

    host, port = sock.getpeername()
    for packet in packets:
        if not packet:
            continue

        obj = json.loads(packet)
        if obj["type"] == "ack":
            if obj["orig"] == "cmp":
                print("compute joined from", host, ":", port)
                comps[host] = obj["flops"]

            elif obj["orig"] == "cmp_mon":
                print("compute monitor joined from", host, ":", port)
                comp_mons.append(sock)

            elif obj["orig"] == "rep":
                print("report requested by", host, ":", port)

def ack_send(sock):
    sock.send(json.dumps({"orig": "man", "type": "ack"}).encode('utf-8'))

def create_socks():
    serv_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Disable TIME_WAIT on socket
    serv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv_sock.bind((HOME, PORT))
    serv_sock.listen(MAX_BACKLOG)

    return (serv_sock)

def get_range(cur_val, max_val, flops):
    total_flops = flops * RUN_TIME
    ops = 0
    idx = cur_val + 1
    while(ops < total_flops):
        # Constant time operations from 1..sqrt(num)
        ops += sqrt(i)
        idx += 1
    return (cur_val + 1, min(idx, max))

def handle_compute(obj, packet, sock, computes, perfects, cur_val, max_val):
    host, port = sock.getpeername()
    if obj["type"] == "req":
        flops = float(obj["flops"])
        computes[host] = flops
        min_val, max_val = get_range(cur_val, max_val, flops)
        sock.send(json.dumps({"orig": "man", "type": "rng",
                              "min": min_val, "max": max_val}).encode('utf-8'))

    elif obj["type"] == "add":
        perfects.add(int(obj["val"]))

    return cur_val

def handle_report(obj, packet, sock, computes, perfects, compute_mons):
    if obj["type"] == "req":
        out_pkt = {"orig": "man", "type": "dat"}

        out_pkt["clients"] = dict()
        for compute in computes:
            out_pkt["clients"][compute] = computes[compute]

        out_pkt["perfs"] = []
        for num in perfects:
            out_pkt["perfs"].append(num)

        sock.send(json.dumps(out_pkt).encode('utf-8'))

    elif obj["type"] == "kill":
        kill_all(compute_mons)

def kill_all(compute_mons):
    killed = 0
    to_kill = len(compute_mons)

    while killed < to_kill:
        r, w, x = select([], compute_mons, [])
        for victim in w:
            victim.send(json.dump({"orig": "man", "type": "kill"}).encode('utf-8'))
            killed += 1

    sys.exit(0)

def add_process(listen, socks, computes, compute_mons):
    new, addr = listen.accept()
    socks.append(new)
    ack_send(new)
    ack_get(new, computes, compute_mons)

def main():
    computes = dict()
    compute_mons = []
    perfects = set()

    cur_val = 1
    max_val = 0xFFFF # 16 bit max

    listen_sock = create_socks()
    sock_array = [listen_sock]

    # Signal Handling, use a closure as the handle, gets state from main
    def handler(signum, frame):
        print ("Signal handler called with signal", signum)
        kill_all(compute_mons)

    signal(SIGHUP, handler)
    signal(SIGINT, handler)
    signal(SIGQUIT, handler)

    while (cur_val <= max_val):
        r, w, x = select(sock_array, [], [])

        for sock in r:
            if sock == listen_sock:
                add_process(listen_sock, sock_array, computes, compute_mons)
                continue

            packets = get_packets(sock)
            if not packets:
                sock.close()
                sock_array.remove(sock)
                continue

            for packet in packets:
                if not packet:
                    continue

                obj = json.loads(packet)
                if obj["orig"] == "cmp":
                    handle_compute(obj, packet, sock, computes, perfects, cur_val, max_val)
                if obj["orig"] == "rep":
                    handle_report(obj, packet, sock, computes, perfects, compute_mons)


    kill_computes(compute_mons)

if __name__ == "__main__":
    main()
