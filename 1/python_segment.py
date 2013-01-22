#!/usr/bin/env python
"""Collection of functions for CS 311: W13 Assignment 1"""

from functools import reduce
import getopt
import os
import re
import sys

__author__ = "Jordan Bayles (baylesj)"
__date__ = "Jan 21, 2013"
__credits__ = ["Meghan Gorman - discussed algorithms, questions, and LaTeX"]
__email__ = "baylesj@engr.orst.edu"

thousand_digit = int("73167176531330624919225119674426574742355349194934969835"
                     "20312774506326239578318016984801869478851843858615607891"
                     "12949495459501737958331952853208805511125406987471585238"
                     "63050715693290963295227443043557668966489504452445231617"
                     "31856403098711121722383113622298934233803081353362766142"
                     "82806444486645238749303589072962904915604407723907138105"
                     "15859307960866701724271218839987979087922749219016997208"
                     "88093776657273330010533678812202354218097512545405947522"
                     "43525849077116705560136048395864467063244157221553975369"
                     "78179778461740649551492908625693219784686224828397224137"
                     "56570560574902614079729686524145351004748216637048440319"
                     "98900088952434506585412275886668811642717147992444292823"
                     "08634656748139191231628245861786645835912456652947654568"
                     "28489128831426076900422421902267105562632111110937054421"
                     "75069416589604080719840385096245544436298123098787992724"
                     "42849091888458015616609791913387549920052406368991256071"
                     "76060588611646710940507754100225698315520005593572972571"
                     "636269561882670428252483600823257530420752963450")
ascii_offset = 64
long_cmd = ("help", "part1", "term=term", "course=course",
                    "part2", "part3", "part4")

short_cmd = "h1t:c:234"


def create_dir(path):
    """Create a directory at path"""
    try:
        os.makedirs(path)
    except:
        print("Directory '", path, "' already exists or could not be created")


def create_sym(source, link_name):
    """Create a symbolic link from link_path to source"""
    try:
        os.symlink(source, link_name)
    except:
        print("Exception occurred symlinking ", link_name, " to ", source)


def part_one(term, course):
    """Create directories and symlinks for term/course/"""
    dirs = ("assignments", "examples", "exams", "lecture_notes", "submissions")
    symlink_root = "/usr/local/classes/eecs/"
    symlink_dirs = (("public_html", "website"), ("handin", "handin"))

    root = os.getcwd()

    # Create Dirs
    for folder in dirs:
        path = os.path.join(root, term, course, folder)
        create_dir(path)

    # Create Symlinks
    for folders in symlink_dirs:
        source_name, link_name = folders
        link_path = os.path.join(root, term, course, link_name)
        source_path = os.path.join(symlink_root, term, course, source_name)
        create_sym(source_path, link_path)


def part_two(val, size):
    """Find the maximum subarray of size "size" in integer"""
    return(max([reduce(lambda x, y: x*y, [int(i) for i in list(str(val)[i:i+size])]) for i in range(len(str(val))-size+1)]))


def collect_words(names_file):
    """Collect CSV as words from names_file"""
    try:
        with open(names_file, 'r') as names:
            # Assume all names are on one line
            raw_names = names.readline()
            list_names = raw_names.split(",")
            return([name.strip('"\n') for name in list_names])
    except:
        return(0)


def word_score(word_list):
    """Find the individual word scores for words in a list"""
    word_scores = []
    for word in word_list:
        score = 0
        for letter in word:
            score += (ord(letter)-ascii_offset)
        word_scores.append(score)
    return(word_scores)


def word_score_pos(scores):
    """Find the word position score for each index in word_scores"""
    return([(idx + 1) * scores[idx] for idx in range(0, len(scores))])


def part_three(names_file):
    """Find the total word score from names_file"""
    # Open and initialize names file
    names = collect_words(names_file)
    # Sort names alphabetically
    names.sort()
    # Calculate name score (A=1, B=2,...)
    score = word_score_pos(word_score(names))
    # Sum name scores
    return(sum(score))


def gen_tri_sequence(cap):
    """Generate the triangle sequence up to the integer value cap"""
    # Generate triangle numbers
    idx = 0
    value = 0
    tri_sequence = []

    while(value < cap):
        value = (1 / 2) * idx * (idx + 1)
        tri_sequence.append(value)
        idx += 1
    return(tri_sequence)


def find_num_triangles(words):
    """Finds the total number of triangle words in words"""
    # Find the name scores
    scores = word_score(words)
    # Find max value
    cap = max(scores)
    # Gather triangle sequence
    tri_seq = gen_tri_sequence(cap)

    # Sum name scores that are triangle numbers
    triangle_numbers = []
    for idx, val in enumerate(scores):
        if val in tri_seq:
            # Note 1-1 idx correlation b/w words and scores
            triangle_numbers.append(words[idx])
    return(len(triangle_numbers))


def part_four(words_file):
    """Find the number of triangle words in words_file"""
    words = collect_words(words_file)
    return(find_num_triangles(words))


def usage():
    """How to use this program"""
    print("Usage\nShort Commands")
    for idx in range(len(short_cmd)):
        if re.match("[a-zA-Z0-9]", short_cmd[idx]):
            print(short_cmd[idx], end='')
            if idx+1 < len(short_cmd):
                if short_cmd[idx+1] == ":":
                    print("\tArgument Required", end='')
            print()

    print("\nLong Commands")
    for cmd in long_cmd:
        print(cmd)


def main():
    """Main entry point for assignment 1. Parses command line options"""
    # Getopt configuration
    try:
        opts, args = getopt.getopt(sys.argv[1:], short_cmd, long_cmd)
    except getopt.GetoptError as err:
        # print help and exit
        print(str(err))
        usage()
        sys.exit(2)
    output = None
    verbose = False

    # Analyze option output
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
        elif opt in ("-1", "--part1"):
            # Re-iterate through opts to capture dependencies
            try:
                for opt, arg in opts:
                    if opt in ("-c", "--course"):
                        course = arg
                    if opt in ("-t", "--term"):
                        term = arg
                part_one(term, course)
            except:
                    print("Missing arg. Course (-c) and term (-t) required")
        elif opt in ("-2", "--part2"):
            print(part_two(thousand_digit, 5))
        elif opt in ("-3", "--part3"):
            print(part_three("names.txt"))
        elif opt in ("-4", "--part4"):
            print(part_four("words.txt"))


if __name__ == "__main__":
    main()
