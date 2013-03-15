#!/usr/bin/env python
"""
manage's job is to maintain the results of ``compute.'' It also keeps track of
the active compute processes, so that it can signal them to terminate,
including the host name and performance characteristics.
"""

# Imports (built in)
import json
import select
import signal
import sys

# Imports (third party)
# Imports (personal)

__author__ = "Jordan Bayles (baylesj)"
__email__ = "baylesj@engr.orst.edu"
__credits__ = "Meghan Gorman (algorithm development)"

# Constants
BUF_SIZE 4096
HOME = "localhost"
MAX_BACKLOG = 5
PORT = 44479

# Functions

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
    computes = []
    cur_idx = 0
    murderers = []
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
