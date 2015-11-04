#ifndef __huff_h_
#define __huff_h_

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
	int weight;
	struct Node_temp * left;
	struct Node_temp * right;
	struct Node_temp * next;
} Node;

/* Function Prototypes */
Node * create_node(char data, int weight);
void destroy_tree(Node * root);
int * parse_file(int * weights, FILE * fp);
Node * insert_node(Node * head, Node * temp);
Node * create_tree(Node * head, int numNodes);
void generate_encodings(Node * root, unsigned char encodings[255], unsigned long bitencodings[255]);
void write_compression(unsigned char encodings[255], unsigned long bitencodings[255], FILE * fphuff, FILE * fporig);
void write_header(Node * root, FILE * fphuff);
void write_bit(FILE * fphuff, unsigned char bit);
void write_bit_char(FILE * fphuff, char data);
void pad_file(FILE * fphuff);

#endif  // __huff_h_