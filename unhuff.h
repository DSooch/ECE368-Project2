#ifndef __unhuff_h_
#define __unhuff_h_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Return/Error Codes */
#define OK               (  0 )  // No errors, everything as should be
#define ERROR            ( -1 ) // Generic error

/* Structure Definitions */
typedef struct Node_temp
{
	char data;
	struct Node_temp * next;
	struct Node_temp * left;
	struct Node_temp * right;
} Node;

/* Function Prototypes */
void read_bit(FILE * fporig);
Node * read_header(FILE * fporig);
void destroy_tree(Node * root);
Node * create_node(char data);
Node * add_list(Node * head, Node * newNode);
Node * merge_two(Node * head);
void write_decompression(Node * root, FILE * fporig, FILE * fpunhuff);

#endif  // __unhuff_h_