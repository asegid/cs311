
#!/usr/bin/env python
"""
Report's job is to report on the perfect numbers found, the number tested, and
the processes that are currently computing. If invoked with the "-k" switch, it
also is used to inform the manage process to shut down computation, i.e.
all compute instances listen to a signal socket so that when report is
ran with "-k", manage kills computes over this socket then exits itself. Report
queries the server for information, provides that to the caller, then exits.
"""

# Imports (built in)
# Imports (third party)
# Imports (personal)

__author__ = "Jordan Bayles (baylesj)"
__email__ = "baylesj@engr.orst.edu"
__credits__ = "Meghan Gorman (algorithm development)"


def main():
    pass

if __name__ == "__main__":
    main()
