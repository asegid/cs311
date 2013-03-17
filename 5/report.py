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
BUF_SIZE = 4096
HOME = "localhost"
PORT = 44479

def init_socket():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOME, PORT))

    get_packets(sock)
    ack(sock)

    return sock

def ack(sock):
    print("Requesting acknowledgment from server")
    sock.send((json.dumps({"orig": "rep", "type": "ack"})+"\n").encode('utf-8'))

def kill(sock):
    print("Sending kill order to server")
    sock.send((json.dumps({"orig": "rep", "type": "kill"})+"\n").encode('utf-8'))

def req(sock):
    print("Requesting the current status of server")
    sock.send((json.dumps({"orig": "rep", "type": "req"})+"\n").encode('utf-8'))

def get_packets(sock):
    received = sock.recv(BUF_SIZE).decode('utf-8')
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

    sock = init_socket()
    req(sock)
    packets = get_packets(sock)

    for packet in packets:
        obj = json.loads(packet)

        print("Compute clients (active)")
        for client in obj["clients"]:
            print("Hostname: ", client, "Performance: ", obj["clients"][client])

        print("Found numbers:")
        for num in obj["perfs"]:
            print(repr(num))

    if ordered_to_kill():
        kill(sock)

if __name__ == "__main__":
    main()
