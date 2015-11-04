// Dan Suciu
// ECE 368 Project 1
// 
// - Accepts 1 command line argument, being the Filename to be compressed in .txt format
// - Writes compressed file to Filename.txt.huff
//
// - Compile with:
// - gcc -Werror -Wall -O3 huff.c -o huff
//
// - Example of run:
// ./huff example.txt
//
// output is written to -> example.txt.huff
//
// Reference - Professor Lu's ECE 264 textbook

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "huff.h"

//global scope variables
unsigned char bits;
unsigned char countbits;
unsigned int numchar;
unsigned char bitspot;
unsigned char currentbyte;

int main(int argc, char * * argv)
{
	int * weights = malloc(sizeof(int) * 256);
	char * filename = argv[1];
	int count;
	int numNodes;
	numchar = 0;
	unsigned char sigbits[255] = {0};
	unsigned long bitencodings[255] = {0};
	Node * temp = NULL;
	Node * head = NULL;
	Node * root = NULL;

	if (argc > 2)
	{//error checking
		fprintf(stderr, "\nERROR! TOO MANY INPUTS, huff.c ACCEPTS 1 INPUT: FILENAME.txt\n\n");
		return  (ERROR);
	}

	FILE * fp = fopen(filename, "r");

	if (fp == NULL)
	{//error checking
		fprintf(stderr, "\nERROR! FILE COULD NOT BE OPENED.\n\n");
		return (ERROR);
	}

	//counting the frequencies of chars in the file
	weights = parse_file(weights, fp);
	fclose(fp);

	//generate linked list of non 0 weighted chars which are sorted in ascending order
	for (count = 0, numNodes = 0; count < 256; ++count)
	{
		if (weights[count] != 0)
		{//use all characters that showed up in the input file
			temp = create_node((char) count, weights[count]);
			if (head == NULL)
			{//if its the first node, set it as the head
				head = temp;
			} else
			{//insert the node into the list in ascending order
				head = insert_node(head, temp);
			}//keep track of the number of nodes in the list
			++numNodes;
		}
	}

	//adding pseudo-EOF node
	temp = create_node((char) 0, 1);
	head = insert_node(head, temp);
	++numNodes;

	//make huffman tree out of linked list
	root = create_tree(head, numNodes);

	//create sigbits and encodings from huffman tree
	bits = 0;
	countbits = 0;
	generate_encodings(root, sigbits, bitencodings);

	//file to read from
	FILE * fporig = fopen(filename, "r");
	rewind(fporig);
	//file to write to
	strcat(filename, ".huff");
	FILE * fphuff = fopen(filename, "w");

	if (fporig == NULL || fphuff == NULL)
	{//error checking
		fprintf(stderr, "\nERROR! FILE COULD NOT BE OPENED.\n\n");
		return (ERROR);
	}

	//write header
	bitspot = 0;
	currentbyte = 0;
	write_header(root, fphuff);
	//write an extra 0 so that the header reader can reach the exit condition
	write_bit(fphuff, 0);
	//pad with zeroes
	pad_file(fphuff);
	//write the number of characters
	fwrite(&numchar, sizeof(unsigned int), 1, fphuff);
	//write a newline which will mark the end of the header
	unsigned char newline = '\n';
	fwrite(&newline, sizeof(unsigned char), 1, fphuff);

	//write compressed data with pseudo EOF and necessary padding
	write_compression(sigbits, bitencodings, fphuff, fporig);

	//were done
	fclose(fporig);
	fclose(fphuff);
	destroy_tree(root);

	return (0);
}

void pad_file(FILE * fphuff)
{//pad the remaining bits with 0's until the byte is written
	while (bitspot != 0)
	{
		write_bit(fphuff, 0);
	}
	return;
}

void write_header(Node * root, FILE * fphuff)
{//write the header information recursively as a post-order tree traversal
	if (root != NULL)
	{
		if ((root -> left == NULL) && (root -> right == NULL))
		{//found a leaf node, header should always start with 1
			write_bit(fphuff, 1);
			write_bit_char(fphuff, root -> data);
			return;
		}
	}
	//post-order traversal of tree
	write_header(root -> left, fphuff);
	write_header(root -> right, fphuff);
	write_bit(fphuff, 0);
	return;
}

void write_bit_char(FILE * fphuff, char data)
{//write an ASCII character to the file 1 bit at a time
	unsigned int newdata = (unsigned int) data;
	//1000 0000
	unsigned char mask = 0x80;

	while (mask > 0)
	{//loop 8 times for the 8 bits
		if ((mask & newdata) == mask)
		{//if the furthest left bit is 1, pack a 1
			write_bit(fphuff, 1);
		} else
		{//if the furthest left bit is 0, pack a 0
			write_bit(fphuff, 0);
		}//shift mask to the right
		mask >>= 1;
	}
	return;
}

void write_compression(unsigned char sigbits[255], unsigned long bitencodings[255], FILE * fphuff, FILE * fporig)
{//write the huffman compression to the output file
	//compression variables
	int readchar;
	int numsignificant;
	unsigned long encoding;
	//1000 0000 0000 0000 0000 0000 0000 0000
	unsigned long mask = 0x80000000;

	bitspot = 0;
	currentbyte = 0;

	//write file 
	while (!feof(fporig))
	{//read every character in the file until the EOF is found
		readchar = fgetc(fporig);
		if (readchar != EOF) //keeps the standard EOF file from being written, we must write our own pseudo EOF
		{//save encoding, save amount of digits, shift the most significant bit to the far left
			encoding = bitencodings[(int) readchar];
			numsignificant = sigbits[(int) readchar];
			encoding <<= (32 - numsignificant);
			while (numsignificant > 0)
			{//loop until we have used all of the significant bits in our encoding
				if ((mask & encoding) == mask)
				{//if the furthest left bit is 1, pack a 1
					write_bit(fphuff, 1);
				} else
				{//if the furthest left bit is 0, pack a 0
					write_bit(fphuff, 0);
				}
				//set up the next bit to write, decrement the number of bits still to write
				encoding <<= 1;
				--numsignificant;
			}
		}
	}
	//write the pseudo EOF character, using ASCII NULL = 0000 0000
	encoding = bitencodings[0];
	numsignificant = sigbits[0];
	encoding <<= (32 - numsignificant);
	//same as above, except dedicated to writing pseudo EOF
	while (numsignificant > 0)
	{
		if ((mask & encoding) == mask)
		{
			write_bit(fphuff, 1);
		} else
		{
			write_bit(fphuff, 0);
		}
		encoding <<= 1;
		--numsignificant;
	}
	//finish off the byte with 0's and write it
	pad_file(fphuff);
	return;
}

void write_bit(FILE * fphuff, unsigned char bit)
{//packs 1 bit at a time and writes to the file when a byte has been filled up
	if(!bitspot)
	{//if we just wrote a byte to the file, reset the buffer byte
		currentbyte = 0;
	}
	//shift the bit to write to the next unoccupied location, then add it to the buffer byte
	unsigned char temp = bit << (7 - bitspot);
	currentbyte |= temp;

	if (bitspot == 7)
	{//if we have filled out byte, write it to the file
		fwrite(&currentbyte, sizeof(unsigned char), 1, fphuff);
	}
	//if we just wrote a full byte to the file, reset the next bit position
	bitspot = (bitspot + 1) % 8;
	return;
}

void generate_encodings(Node * root, unsigned char sigbits[255], unsigned long bitencodings[255])
{//generates encodings for each leaf node and saves them and the number of significant bits for each
	if (root != NULL)
	{
		if (root -> left == NULL && root -> right == NULL)
		{//we found a leaf, save current encoding and amount of significant bits
			sigbits[(int) root -> data] = countbits;
			bitencodings[(int) root -> data] = bits;
		} else
		{//we are at a parent of some kind
			if (root -> left != NULL)
			{//parent has left child, move left and twiddle a 0 into the encoding
				bits <<= 1;
				++countbits;
				generate_encodings(root -> left, sigbits, bitencodings);
				bits >>= 1; //back from recursive call, remove the bit twiddled above
				--countbits;
			} 
			if (root -> right != NULL)
			{//parent has right child, move right and twiddle a 1 into the encoding
				bits <<= 1;
				bits += 1;
				++countbits;
				generate_encodings(root -> right, sigbits, bitencodings);
				bits >>= 1; //back from recursive call, remove the bit twiddled above
				--countbits;
			}
		}
	}
	return;
}

int * parse_file(int * weights, FILE * fp)
{ //parses file and creates the array of weights per ASCII character (extended ASCII table)
	int charConversion;

	while ((charConversion = fgetc(fp)) != EOF)
	{//loop until we read in the EOF character
		++weights[charConversion];
		++numchar;
	}
	return (weights);
}

Node * create_node(char data, int weight)
{//allocates space for and creates a new node struct
	Node * newNode = malloc(sizeof(Node));
	newNode -> data = data;
	newNode -> weight = weight;
	newNode -> left = NULL;
	newNode -> right = NULL;
	newNode -> next = NULL;
	return (newNode);
}

Node * insert_node(Node * head, Node * temp)
{//inserts a node into the correct place in the linked list in ascending weight order
	if (head == NULL)
	{
		head = temp;
		return (head);
	}

	if (head -> weight < temp -> weight)
	{//if the node we are at has a lesser weight, move to the next node
		head -> next = insert_node(head -> next, temp);
	} else if (head -> weight >= temp -> weight)
	{//if the node we are at has a greater or equal weight, place it in the list here
		temp -> next = head;
		return(temp);
	}
	return (head);
}

Node * create_tree(Node * head, int numNodes)
{//creates the huffman tree from the linked list of nodes in ascending weight order
	Node * tempL = NULL;
	Node * tempR = NULL;
	Node * newNode = NULL;
	Node * root = NULL;

	for (;numNodes > 1; --numNodes)
	{//loop until we have used all of the nodes in the linked list
		//pull out two smallest nodes
		tempL = head;
		tempR = head -> next;
		//move head pointer over 2 spots
		head = head -> next -> next;
		//create new tree node with a char value of NULL
		newNode = create_node(0, (tempL -> weight) + (tempR -> weight));
		//remove new leaf nodes connection to linked list
		tempL -> next = NULL;
		tempR -> next = NULL;
		//put leaves in appropriate spots
		newNode -> left = tempL;
		newNode -> right = tempR;
		//insert parent node back into linked list in ascending sorted position
		head = insert_node(head, newNode);
	}
	root = head;
	return (root);
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
