#!/usr/bin/env python
"""
Report's job is to report on the perfect numbers found, the number tested, and
the processes that are currently computing. If invoked with the "-k" switch, it
also is used to inform the manage process to shut down computation, i.e.
all compute instances listen to a signal socket so that when report is
ran with "-k", manage kills computes over this socket then exits itself. Report
queries the server for information, provides that to the caller, then exits.
"""

import json
import signal
import socket
import sys

__author__ = "Jordan Bayles (baylesj)"
__email__ = "baylesj@engr.orst.edu"
__credits__ = "Meghan Gorman (algorithm development)"

# Constants
BUF_SIZE 4096
HOME = "localhost"
PORT = 44479

def init_socket():
    sock_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock_fd.connect((HOST, PORT))

    get_data(sock_fd)
    ack(sock_fd)

    return sock_fd

def ack(socket):
    socket.send(json.dumps({"orig": "rep", "type": "ack"}))

def kill(socket)
    socket.send(json.dumps({"orig": "rep", "type": "kill"}))

def req(socket):
    socket.send(json.dumps({"orig": "rep", "type": "req"}))

def get_data(socket):
    recieved = sock.recv(BUF_SIZE)
    return (received.strip().split("\n"))

def ordered_to_kill():
    for arg in (sys.argv[1:]):
        if arg in "-k":
            return True
    return False

def main():
    def handler(*args):
        sys.exit()

    signal.signal(signal.SIGHUP, handler)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGQUIT, handler)

    sock_fd = init_socket()
    req(sock_fd)
    packets = get_data(sock_fd)

    for packet in packets:
        # {"clients":{"name": ops, ...}, "perfects":[]}
        obj = json.loads(packet)

        print("Compute clients (active)")
        for client in obj["clients"]:
            print("Hostname: ", client, "Performance: ", obj["clients"][client])

        print("Found numbers:")
        for num in obj["perfects"]:
            print(repr(num))

    if ordered_to_kill():
        kill()

if __name__ == "__main__":
    main()
