/*
 * ============================================================================
 *
 *         Author:  Jordan Bayles (baylesj), baylesj@onid.orst.edu
 *        Company:  Oregon State University
 *        Created:  11/02/2012 08:37:30 PM
 *
 *    Environment:  vim + gdb + valgrind, gcc compiler on Arch Linux
 *    Description:  Binary Search tree implementation
 *
 * ============================================================================
 */

/*-----------------------------------------------------------------------------
Include statements
-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"
#include "bst.h"

/*-----------------------------------------------------------------------------
Literals, macros (#define statements)
-----------------------------------------------------------------------------*/
#define DEBUG

/*-----------------------------------------------------------------------------
Global variables
-----------------------------------------------------------------------------*/
struct Node {
	TYPE val;
	struct Node *left;
	struct Node *right;
};
struct BSTree {
	struct Node *root;
	int cnt;
};

/*-----------------------------------------------------------------------------
Functions
-----------------------------------------------------------------------------*/
/*
 function to initialize the binary search tree.
 param: tree
 pre:   tree is not null
 post:  tree size is 0
        root is null
 */
void initBSTree(struct BSTree *tree)
{
	tree->cnt = 0;
	tree->root = 0;
}

/*
 function to create a binary search tree.
 param: none
 pre:   none
 post:  tree->count = 0
        tree->root = 0;
 */
struct BSTree *newBSTree()
{
	struct BSTree *tree = malloc(sizeof(struct BSTree));
	assert(tree != NULL);
	initBSTree(tree);
	return (tree);
}


/*
 Helper function to remove a node from the tree.
 This is necessary due to the fact that the node may contain
 pointers to strings, etc. that (1) need to be freed and (2)
 are not freed by a simple free( node ).
 param: node - the node to free
 pre:   node is not null
 */
void _freeNode(struct Node *node)
{
	free(node);
}

/*
 function to free the nodes of a binary search tree
 param: node - the root node of the tree to be freed
 pre:   none
 post:  node and all descendants are deallocated
 */
void _freeBST(struct Node *node)
{
	if (node != NULL) {
		_freeBST(node->left);
		_freeBST(node->right);
		_freeNode(node);
	}
}


/*
 function to clear the nodes of a binary search tree
 param: tree - a binary search tree
 pre:   tree ! = null
 post:  the nodes of the tree are deallocated
        tree->root = 0;
        tree->cnt = 0
 */
void clearBSTree(struct BSTree *tree)
{
	assert(tree != NULL);
	_freeBST(tree->root);
	tree->root = 0;
	tree->cnt = 0;
}

/*
 function to deallocate a dynamically allocated binary search tree
 param: tree - the binary search tree
 pre:   tree != null;
 post:  all nodes and the tree structure itself are deallocated.
 */
void deleteBSTree(struct BSTree *tree)
{
	assert(tree != NULL);
	clearBSTree(tree);
	free(tree);
}

/*
 function to determine if  a binary search tree is empty.
 param: tree - the binary search tree
 pre:  tree is not null
 */
int isEmptyBSTree(struct BSTree *tree)
{
	assert(tree != NULL);
	return (tree->cnt == 0);
}


/*
 function to determine the size of a binary search tree
 param: tree - the binary search tree
 pre:   tree is not null
 */
int sizeBSTree(struct BSTree *tree)
{
	assert(tree != NULL);
	return (tree->cnt);
}


/*
 Function that creates a new node and fills its value with input
 param:  val - the value to be added to the new node
 */
struct Node *_initNode(TYPE val)
{

/* Create a new node to store the value of val */
	struct Node *val_node = malloc(sizeof(struct Node));
	assert(val_node != NULL);
	val_node->val = val;

/* Explicit is better than implicit, ensure NULL value */
	val_node->left = NULL;
	val_node->right = NULL;
	return (val_node);
}


/*
 Recursive helper function to add a node to the binary search tree.
 param:  cur - the current root node
         val - the value to be added to the binary search tree
 pre:    val is not null
 */
struct Node *_addNode(struct Node *cur, TYPE val)
{
	if (cur == NULL) {
		cur = _initNode(val);
	}

	else {

		/* Note: a binary search tree is often considered an ordered set, i.e.
		 * no duplicates are allowed. However, after in class discussion, it is
		 * suggested that duplicates should be allowed, thus this implementation
		 * allows duplicate values to be inserted, on the RIGHT
		 */
		switch (compare(val, cur->val)) {

			/* val < cur->val */
		case (-1):
			cur->left = _addNode(cur->left, val);
			break;

			/* val = cur->val */
		case (0):	// DELIBERATE FALLTHROUGH
			/* val > cur->val */
		case (1):
			cur->right = _addNode(cur->right, val);
			break;
		default:

			/* Should not reach this case under normal circumstances */
			assert(NULL);
			break;
		}
	}

/* Original root should still be the root of the tree with val added */
	return (cur);
}


/*
 function to add a value to the binary search tree
 param: tree - the binary search tree
        val - the value to be added to the tree
 pre:   tree is not null
        val is not null
 post:  tree size increased by 1
        tree now contains the value, val
 */
void addBSTree(struct BSTree *tree, TYPE val)
{
	assert(tree != NULL);
	tree->root = _addNode(tree->root, val);
	tree->cnt++;
}

/*
 function to determine if the binary search tree contains a particular element
 param: tree - the binary search tree
        val - the value to search for in the tree
 pre:   tree is not null
        val is not null
 post:  none
 */
int containsBSTree(struct BSTree *tree, TYPE val)
{
	assert(tree != NULL);
	struct Node *cur = tree->root;
	while (cur != NULL) {
		switch (compare(val, cur->val)) {
		case (-1):
			cur = cur->left;
			break;
		case (0):
			return (1);
			break;
		case (1):
			cur = cur->right;
			break;
		default:

			/* Should not reach this case under normal circumstances */
			assert(NULL);
			break;
		}
	}
	return (0);
}


/*
 helper function to find the left most child of a node's subtree
 (defined as the smallest element in the subtree)
 return the value of the left most child of cur
 param: cur - the current node
 pre:   cur is not null
 post:  none
 */
TYPE _leftMost(struct Node * cur)
{
	assert(cur != NULL);
	struct Node *leftest = cur;

/* Traverse to the leftmost child of the subtree beginning with root node at
 * curr
 */
	while (leftest->left != NULL) {
		leftest = leftest->left;
	}
	return (leftest->val);
}


/*
 recursive helper function to remove the left most child of a node
 HINT: this function returns cur if its left child is NOT NULL. Otherwise,
 it returns the right child of cur and free cur.
 Note:  If you do this iteratively, the above hint does not apply.
 param: cur - the current node
 pre:   cur is not null
 post:  the left most node of cur is not in the tree
 */
struct Node *_removeLeftMost(struct Node *cur)
{
	assert(cur != NULL);

/* cur is the parent of the leftmost child */
	if (cur->left == NULL) {
		struct Node *tmp = cur;
		_freeNode(cur);
		return (tmp->right);
	}

	else {
		cur->left = _removeLeftMost(cur->left);
		return (cur);
	}
}


/*
 recursive helper function to remove a node from the tree
 HINT: You have to use the compare() function to compare values.
 param: cur - the current node
        val - the value to be removed from the tree
 pre:   val is in the tree - handled by calling function (removeBSTree)
        cur is not null
        val is not null
 */
struct Node *_removeNode(struct Node *cur, TYPE val)
{
	assert(cur != NULL);
	switch (compare(val, cur->val)) {
	case (-1):
		cur->left = _removeNode(cur->left, val);
		break;
	case (1):
		cur->right = _removeNode(cur->right, val);
		break;
	case (0):
		{
			struct Node *tmp = NULL;

			/* Found a match, so remove this node */
			if (cur->left == NULL) {
				tmp = cur->right;
				_freeNode(cur);
			}

			else if (cur->right == NULL) {
				tmp = cur->left;
				_freeNode(cur);
			}

			else {

				/* Get the leftmost child's value of the right subtree, and
				 * remove the leftmost child before shifting it into place.
				 */
				tmp = _initNode(_leftMost(cur->right));
				_removeLeftMost(cur->right);

				/* Set pointer data for tmp */
				tmp->right = cur->right;
				tmp->left = cur->left;
			}
			return (tmp);
		}
		break;
	default:

		/* Should not reach this case under normal circumstances */
		assert(NULL);
		break;
	}
	return (NULL);
}


/*
 function to remove a value from the binary search tree
 param: tree - the binary search tree
        val - the value to be removed from the tree
 pre:   tree is not null
        val is not null
        val is in the tree
 pose:  tree size is reduced by 1
 */
void removeBSTree(struct BSTree *tree, TYPE val)
{
	if (containsBSTree(tree, val)) {
		tree->root = _removeNode(tree->root, val);
		tree->cnt--;
	}
}


/*-----------------------------------------------------------------------------
Debugging Functions
-----------------------------------------------------------------------------*/
#ifdef DEBUG
#include <stdio.h>
void printNode(struct Node *cur)
{
	if (cur == NULL) {
		return;
	}
	printf("(");
	printNode(cur->left);

/* print_type prints the value of the TYPE */
	print_type(cur->val);
	printNode(cur->right);
	printf(")");
}

void printTree(struct BSTree *tree)
{
	if (tree != NULL) {
		printNode(tree->root);
	}
}


#endif				/*  */
