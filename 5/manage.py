#!/usr/bin/env python
"""
manage's job is to maintain the results of ``compute.'' It also keeps track of
the active compute processes, so that it can signal them to terminate,
including the host name and performance characteristics.
"""

import json
import select
import signal
import sys

__author__ = "Jordan Bayles (baylesj)"
__email__ = "baylesj@engr.orst.edu"
__credits__ = "Meghan Gorman (algorithm development)"

# Constants
BUF_SIZE 4096
HOME = "localhost"
MAX_BACKLOG = 5
PORT = 44479

# Functions
def get_data(socket):
    received = sock.recv(BUF_SIZE)
    return (received.strip().split("\n"))

def ack_get(socket, comps, comp_mons):
    packets = get_data(socket)

    host, port = socket.getpeername()
    for packet in packets:
        if packet["type"] == "ack":
            if packet["orig"] == "cmp":
                print("compute joined from", host, ":", port)
                comps[host] = packet["flops"]

            elif packet["orig"] == "cmp_mon":
                print("compute monitor joined from", host, ":", port)
                comp_mons.append(socket)

            elif packet["orig"] == "rep":
                print("report requested by", host, ":", port)

def ack_send(socket):
    socket.send(json.dumps({"orig": "man", "type": "ack"}))

def create_sockets():
    serv_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Disable TIME_WAIT on socket
    serv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv_sock.bind((HOME, PORT))
    serv_sock.listen(MAX_BACKLOG)

    return ([serv_sock])

def handle_compute():
    pass

def handle_report():
    pass

def kill_computes(murderers):
    pass

def main():
    dict computes
    compute_mons = []
    cur_idx = 0
    max_number = 0xFFFF # 16 bit max
    perfects = []

    # Init sockets (TODO what about added sockets, needed?)
    sock_arr = create_sockets()

    # Signal Handling, use a closure as the handle, gets state from main
    def handler(*args):
        print ("Signal handler called with signal", signum)
        kill_computes(murderers)
        sys.exit()

    signal.signal(signal.SIGHUP, handler)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGQUIT, handler)

    while (cur_idx <= max_number):
        handle_compute()

        handle_report()

    kill_computes(murderers)

if __name__ == "__main__":
    main()
