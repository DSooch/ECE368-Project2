// Dan Suciu
// ECE 368 Project 1
// 
// - Accepts 1 command line argument, being the Filename to be decompressed in .txt.huff format
// - Writes decompressed file to Filename.txt.huff.unhuff
//
// - Compile with:
// - gcc -Werror -Wall -O3 unhuff.c -o unhuff
//
// - Example of run:
// ./unhuff example.txt.huff
//
// output is written to -> example.txt.huff.unhuff
//
// Reference - Professor Lu's ECE 264 textbook

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h> 
#include <math.h>
#include "unhuff.h"

//global scope variables
unsigned char bitspot;
unsigned char currentbyte;
unsigned char bit;
unsigned int numchar;

int main(int argc, char * * argv)
{
	char * filename = argv[1];
	Node * root;

	if (argc > 2)
	{//error checking
		fprintf(stderr, "\nERROR! TOO MANY INPUTS, unhuff.c ACCEPTS 1 INPUT: FILENAME.txt.huff\n\n");
		return (ERROR);
	}
	//open input file for reading
	FILE * fporig = fopen(filename, "r");
	//open output file for writing with .unhuff on the end
	strcat(filename, ".unhuff");
	FILE * fpunhuff = fopen(filename, "w");
	if (fporig == NULL || fpunhuff == NULL)
	{//error checking
		fprintf(stderr, "\nERROR! FILE COULD NOT BE OPENED.\n\n");
		return (ERROR);
	}

	//read the header and build the tree
	bit = 0;
	bitspot = 0;
	currentbyte = 0;
	root = read_header(fporig);
	//read in the number of characters
	fread(&numchar, sizeof(unsigned int), 1, fporig);
	//read in the newline character, marking the end of the header
	unsigned char newline;
	fread(&newline, sizeof(unsigned char), 1, fporig);
	if (newline != '\n')
	{//error checking
		fprintf(stderr, "\nERROR! LAST ELEMENT IN HEADER SHOULD BE NEWLINE.\n\n");
		return (ERROR);
	}
	//write the decompressed information to filename.txt.huff.unhuff
	bit = 0;
	bitspot = 0;
	currentbyte = 0;
	write_decompression(root, fporig, fpunhuff);

	//were done baby
	fclose(fporig);
	fclose(fpunhuff);
	destroy_tree(root);

	return (0);
}

void write_decompression(Node * root, FILE * fporig, FILE * fpunhuff)
{//takes one bit at a time from the .huff file, traverses the tree, and prints the character of leaf nodes reached to the .unhuff file
	Node * temp;
	while (numchar > 0)
	{//start at the top of the tree after character is written until we have decompressed all characters compressed
		temp = root;
		while ((temp -> left != NULL) && (temp -> right != NULL))
		{//we havent found a leaf node, keep traversing the tree bit by bit
			read_bit(fporig);
			if (bit == 0)
			{//read a 0, move left
				temp = temp -> left;
			} else
			{//read a 1, move right
				temp = temp -> right;
			}
		}//exit means we have found a leaf node, print character to file
		fprintf(fpunhuff, "%c", temp -> data);
		--numchar;
	}//exit means we have written back every character compressed
	destroy_tree(root);
	return;
}

Node * read_header(FILE * fporig)
{//reads the header from the beginning of the .huff file and builds the huffman tree
	int donecheck = 0;
	int count;
	unsigned char readchar;
	Node * root = NULL;
	Node * newNode = NULL;
	Node * head = NULL;

	while (!donecheck)
	{//while were not done, keep reading header
		read_bit(fporig);
		if (bit)
		{//found a leaf in the header, read in the 8-bit unsigned char
			readchar = 0;
			read_bit(fporig);
			readchar |= bit; //put in first bit
			for (count = 0; count < 7; ++count)
			{//loop 7 times to save the next 7 bits
				readchar <<= 1;
				read_bit(fporig);
				readchar |= bit;
			}
			//create a list of nodes where the last two are the lowest in the tree
			newNode = create_node(readchar);
			head = add_list(head, newNode);
		} else
		{//we are at a non-leaf node
			if (head == NULL)
			{
				fprintf(stderr, "\nERROR! FIRST BIT IN HEADER SHOULD NOT BE 0. WILL SEGFAULT.\n\n");
			}
			if (head -> next == NULL)
			{//the list has become the tree
				root = head;
				donecheck = 1;
			} else
			{//have atleast 2 nodes in the list, merge the 2 furthest to the right
				head = merge_two(head);
			}
		}
	}
	//done reading the header, root points to our tree, last 0 bits are padding
	return (root);
}

Node * merge_two(Node * head)
{//merge the last two nodes in the list into one tree node
	if (head -> next -> next == NULL)
	{//we are at the left child to merge, head -> next is the right child
		Node * newParent = create_node(0);
		newParent -> left = head;
		newParent -> right = head -> next;
		newParent -> left -> next = NULL;
		return (newParent);
	}
	//head -> next becomes the new parent
	head -> next = merge_two(head -> next);
	return (head);
}

Node * add_list(Node * head, Node * newNode)
{//recursively adds a new node to the end of the linked list
	if (head == NULL)
	{
		head = newNode;
		return (head);
	}

	head -> next = add_list(head -> next, newNode);
	return (head);
}

void read_bit(FILE * fporig)
{//reads one bit at a time from the current byte and sends it to bit
	//0000 0001
	unsigned char mask = 0x01;

	if (!bitspot)
	{//if bitspot was reset (%8 == 0), read in a new byte
		fread(&currentbyte, sizeof(unsigned char), 1, fporig);
	}

	//currentbyte's next bit to read, starting from the far left, is saved and isolated in temp
	unsigned char temp = currentbyte >> (7 - bitspot);
	temp &= mask;

	//increase bitspot, and reset at 8
	bitspot = (bitspot + 1) % 8;
	//send bit read
	bit = temp;
	return;
}

Node * create_node(char data)
{//allocates space for and creates a new node struct
	Node * newNode = malloc(sizeof(Node));
	newNode -> data = data;
	newNode -> next = NULL;
	newNode -> left = NULL;
	newNode -> right = NULL;
	return (newNode);
}

void destroy_tree(Node * root)
{//recursively frees all memory associated with the binary tree
	if (root == NULL)
	{
		return;
	}

	destroy_tree(root -> left);
	destroy_tree(root -> right);

	free(root);

	return;
}
