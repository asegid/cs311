#!/usr/bin/env python

# Imports
import getopt
import os
import re
import sys

# Constants
thousand_digit = ("73167176531330624919225119674426574742355349194934"
                  "96983520312774506326239578318016984801869478851843"
                  "85861560789112949495459501737958331952853208805511"
                  "12540698747158523863050715693290963295227443043557"
                  "66896648950445244523161731856403098711121722383113"
                  "62229893423380308135336276614282806444486645238749"
                  "30358907296290491560440772390713810515859307960866"
                  "70172427121883998797908792274921901699720888093776"
                  "65727333001053367881220235421809751254540594752243"
                  "52584907711670556013604839586446706324415722155397"
                  "53697817977846174064955149290862569321978468622482"
                  "83972241375657056057490261407972968652414535100474"
                  "82166370484403199890008895243450658541227588666881"
                  "16427171479924442928230863465674813919123162824586"
                  "17866458359124566529476545682848912883142607690042"
                  "24219022671055626321111109370544217506941658960408"
                  "07198403850962455444362981230987879927244284909188"
                  "84580156166097919133875499200524063689912560717606"
                  "05886116467109405077541002256983155200055935729725"
                  "71636269561882670428252483600823257530420752963450")

# Getopt related constants
short_cmd = "h1t:c:234"
long_cmd = ("help", "part1", "term=term", "course=course",
                    "part2", "part3", "part4")

# Note that this may not work for unicode
ascii_offset = 64


# Functions
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


def part_two(integer, size):
    """Find the maximum subarray of size "size" in integer"""
    return(max([integer[i:i+size] for i in range(0, len(integer)-size+1)]))


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
