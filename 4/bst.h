/*
  File: bst.h
  Interface definition of the binary search tree data structure.
*/
#ifndef __BST_H
#define __BST_H

#include <stdint.h>

/* Defines the type to be stored in the data structure. */
#define TYPE      uint32_t

int compare(TYPE left, TYPE right);
void print_type(TYPE curval);

struct BSTree;

/* Initialize binary search tree structure. */
void initBSTree(struct BSTree *tree);

/* Allocate and initialize search tree structure. */
struct BSTree *newBSTree();

/* Deallocate nodes in BST. */
void clearBSTree(struct BSTree *tree);

/* Deallocate nodes in BST and deallocate the BST structure. */
void deleteBSTree(struct BSTree *tree);

/*-- BST Bag interface --*/
int isEmptyBSTree(struct BSTree *tree);
int sizeBSTree(struct BSTree *tree);

void addBSTree(struct BSTree *tree, TYPE val);
int containsBSTree(struct BSTree *tree, TYPE val);
void removeBSTree(struct BSTree *tree, TYPE val);
void printTree(struct BSTree *tree);
#endif
