//==============================================================================
//                                  buffer.h
//------------------------------------------------------------------------------
// Brief
//   Implements circular buffer to hold data elements of a fixed number of bytes
//
// Contents
//   - newBuffer
//   - freeBuffer
//   - isBufferEmpty
//   - isBufferFull
//   - popFromBuffer
//   - pushToBuffer
//
// Description
//   Declaration
//      buffer_t *b;
//      b = newBuffer(3, sizeof(int), B_FIFO & B_DROP);
//   Adding data
//      int howMuchILoveBuffers = 99;
//      int myMaximumLove[] = {0xFF, 0xF3, 0xF9, 0xF4};
//      pushToBuffer(b, &howMuchILoveBuffers, 1);
//      pushToBuffer(b, &myMaximumLove[0], 4);
//   Getting data
//      int howMuchYouLoveBuffers;
//      int yourMaximumLove[4];
//      popFromBuffer(b, &howMuchYouLoveBuffers, 1);
//      popFromBuffer(b, &yourMaximumLove[0], 4);
//
// Warnings
//  -Each buffer is stored in the heap.  If there is not enough RAM, calling the
//   'newBuffer()' function returns a NULL pointer instead of a pointer to a new
//   buffer. Always check that the return value is not NULL, e.g.:
//      buffer_t *b;
//      b = newBuffer(3, sizeof(int), B_FILO & B_DROP);
//      if ( b == NULL ) return -1;
//      ...
//  -Both pushToBuffer and popFromBuffer have the potential to access unmapped
//   memory, since only a pointer and an offset are used to read/write to memory
//
// Author
//   Daniel Wilkinson-Thompson (daniel@wilkinson-thompson.com)
//
// Date
//   2012-08-10
//
// Licence
//   Copyright (c) 2012 Daniel Wilkinson-Thompson. All rights reserved.
//------------------------------------------------------------------------------

#ifndef BUFFER_H
#define BUFFER_H

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
// -Bitwise AND constants to configure a new buffer, e.g.:
//      buffer_t *b
//      b = newBuffer(3, sizeof(int), B_FILO & B_DROP);
// -Constants B_FILO(=B_STACK) and B_FIFO(=B_QUEUE) are mutually exclusive
// -Constants B_DROP and B_OVERWRITE are mutually exclusive

// Implement stack (first-in, last-out (FILO) buffer)
#define B_STACK        0xFF
#define B_FILO         0xFF

// Implement queue (first-in, first-out (FIFO) buffer)
#define B_QUEUE        0x7F
#define B_FIFO         0x7F

// Drop oldest elements when buffer is full
// -Existing elements move down one slot, new element(s) pushed to head
#define B_OVERWRITE    0xFF

// Drop all incoming elements when buffer is full
// -Existing elements don't move
#define B_DROP         0xBF


//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------
typedef struct B_BUFFER {
    void *data;
    void *head;
    void *tail;
    unsigned int depth;
    unsigned char width;
    union B_BEHAVIOR {
        unsigned char byte;
        struct B_BITS {
            unsigned unused:6;
            unsigned overwrite:1;
            unsigned stack:1;
        } bits;
    } behavior;
} buffer_t;


//------------------------------------------------------------------------------
// Function prototypes
//------------------------------------------------------------------------------

// ------------------------- Generate a new buffer ----------------------------
// -The created buffer is stored in the heap, make sure there is sufficient
//  space in the heap (typically configured with options to linker)
// -Because the buffer is stored in the heap, it must be freed using the
//  freeBuffer() function described below.
// -Take care when freeing a buffer referenced by multiple pointers
// -Never use free(b), as b contains pointers to allocated memory that must be
//  freed first
// -A NULL return implies that the buffer was not properly initialized,
//  typically because there was not enough free memory in the heap
// -The last parameter, 'config', is set via a bitwise AND of constants, which
//  are defined in the 'Constants' section above
// -Constants B_FILO(=B_STACK) and B_FIFO(=B_QUEUE) are mutually exclusive
// -Constants B_DROP and B_OVERWRITE are mutually exclusive
// -Example usage:
//      buffer_t *b
//      b = newBuffer(3, sizeof(int), B_FILO & B_DROP);
buffer_t* newBuffer(unsigned int numberOfElements, unsigned char elementSizeInBytes, unsigned char config);

// --------------------------- Free the buffer -------------------------------
// -All pointers within b are set to NULL before b is freed
// -Take care when freeing a buffer referenced by multiple pointers
// -Never use free(b), as b contains pointers to allocated memory that must be
//  freed first
// -Example usage:
//      buffer_t *b;
//      b = newBuffer(256, 1, B_DROP & B_STACK);
//      ...
//      freeBuffer(b);
void freeBuffer(buffer_t *b);

// ------------------- Check whether the buffer is empty ----------------------
// -A return value of 1 implies the buffer is empty, zero implies not empty
// -Example usage:
//      buffer_t *b;
//      b = newBuffer(256, 1, B_DROP & B_STACK);
//      if ( isBufferEmpty(b) ) {
//          printf("Buffer is empty");
//      }
unsigned char isBufferEmpty(buffer_t *b);

// -------------------- Check whether the buffer is full -----------------------
// -A return value of 1 implies the buffer is full, zero implies not full
// -Example usage:
//      buffer_t *b;
//      b = newBuffer(256, 1, B_DROP & B_STACK);
//      ...
//      if ( isBufferFull(b) ) {
//          printf("Buffer is full");
//      }
unsigned char isBufferFull(buffer_t *b);

// ---------------------- Pop data from the buffer ----------------------------
// Pop l elements of size elementSizeInBytes from the buffer into memory,
// starting at the memory location pointed to by d
// -The return value is the number of elements that could not be popped
// -The return value is always zero using B_OVERWRITE
// -Example usage:
//      buffer_t *b;
//      int output[16];
//      unsigned int failedBytes;
//      b = newBuffer(256, 1, B_DROP & B_STACK);
//      ...
//      failedBytes = popFromBuffer(b, &output[0], 4);
unsigned int popFromBuffer(buffer_t *b, void *d, unsigned int l);

// ----------------------- Push data to the buffer ----------------------------
// Push l elements of size elementSizeInBytes to the buffer from memory,
// starting at the memory location pointed to by d
// -The return value is the number of elements that could not be pushed
// -The return value is always zero using B_OVERWRITE
// -Example usage:
//      buffer_t *b;
//      int input[] = {44, 33, 22, 11, 0};
//      unsigned int failedBytes;
//      b = newBuffer(256, 1, B_DROP & B_STACK);
//      failedBytes = pushToBuffer(b, &input[0], 4);
unsigned int pushToBuffer(buffer_t *b, void *d, unsigned int l);

#endif