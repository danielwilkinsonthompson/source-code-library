//==============================================================================
//                                  buffer.c
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
//   - popByte (private)
//   - pushByte (private)
//   - increment (private)
//   - decrement (private)
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
//   memory, since only a pointer and an offset are used to read/write memory
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

#ifndef BUFFER_C
#define BUFFER_C

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "buffer.h"
#include <stdlib.h>

//------------------------------------------------------------------------------
// Private function prototypes
//------------------------------------------------------------------------------
unsigned char popByte(buffer_t *b);
void pushByte(buffer_t *b, unsigned char d);
void increment(buffer_t *b, void **ht);
void decrement(buffer_t *b, void **ht);

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
// Generate buffer
buffer_t* newBuffer(unsigned int numberOfElements, unsigned char elementSizeInBytes, unsigned char behavior) {
    
    buffer_t *b;
    b = malloc(sizeof(buffer_t));
    
    // Allocate memory for buffer wrapper
    // -If there is not enough free RAM in the heap, return a NULL pointer
    if ( !(b) ) {
        b = NULL;
        return NULL;
    }

    // Allocate memory for buffer data
    // -If there is not enough free RAM in the heap, free all allocated RAM and
    //  return a NULL pointer
    // -Strictly speaking ((numberOfElements+1)*elementSizeInBytes) is always
    //  more data storage than we need (numberOfElements*elementSizeInBytes+1),
    //  but this simplifies checking whether the buffer is full.
    b->data = calloc(numberOfElements + 1, elementSizeInBytes);
    if ( !(b->data) ) {
        free(b);
        b = NULL;
        return NULL;
    }

    // Initialize buffer
    b->behavior.byte = behavior;
    b->head = b->data;
    b->tail = b->data;
    b->width = elementSizeInBytes;
    b->depth = numberOfElements + 1;
    return b;
}

// Free buffer
void freeBuffer(buffer_t *b) {
    
    // Deallocate data buffer
    free(b->data);
    
    // Set all pointers to NULL
    //  -Just in case something nasty happens during deallocation of b
    b->data = NULL;
    b->head = NULL;
    b->tail = NULL;
    
    // Deallocate buffer_t variable
    free(b);
    b = NULL;
}

// Buffer empty check
unsigned char isBufferEmpty(buffer_t *b) {
    return (b->head == b->tail);
}

// Buffer full check
unsigned char isBufferFull(buffer_t *b) {
    return ( (b->tail == b->head + 1) || ((b->tail == b->data) && ( b->head >= (b->data + (b->depth - 1) * (b->width)) )) );
}

// Increment head/tail pointer
void increment(buffer_t *b, void **ht){
    
    // Check whether head/tail pointer is at the last element
    if (*ht < b->data + (b->depth - 1) * (b->width)) {
        *ht = *ht + 1;
    }
    
    // If ht the last element, wrap to the first element
    else {
        *ht = b->data;
    }
}

// Decrement head/tail pointer
void decrement(buffer_t *b, void **ht){
    
    // Check whether head/tail pointer is at the first element
    if (*ht > b->data) {
        *ht = *ht-1;
    }
    
    // If ht is at the first element, wrap to the last element
    else {
        *ht = b->data + (b->depth-1) * (b->width);
    }
}

// Byte-size pop function
unsigned char popByte(buffer_t *b){
    unsigned char d;

    // FILO: Push to head, pop from head
    if (b->behavior.bits.stack) {
        
        // Decrement head first as it is currently pointing to a free slot
        decrement(b, &(b->head));
        d =  *( (unsigned char*)b->head );
    }
    
    // FIFO: Push to head, pop from tail
    else {
        d =  *( (unsigned char*)b->tail );
        increment(b, &(b->tail));
    }

    return d;
}

// Arbitrary-size pop function
unsigned int popFromBuffer(buffer_t *b, void *d, unsigned int l){
    unsigned int elementIndex, byteIndex;

    for (elementIndex = 0; elementIndex < l; elementIndex++) {
        for (byteIndex = 0; byteIndex < b->width; byteIndex++) {
            if (!isBufferEmpty(b)){
                
                // Stacks swap bytes of multi-byte elements, so swap back
                // on pop operation
                if (b->behavior.bits.stack){
                    *( (unsigned char*)(d + ((elementIndex + 1) * b->width) - 1 - byteIndex) ) = popByte(b);
                }
                
                // Queue does not swap bytes, so no need to swap on pop
                else {
                    *( (unsigned char*)(d + (elementIndex * b->width) + byteIndex) ) = popByte(b);
                }
            }
            else {
                // Push any bytes back to buffer that form an incomplete element
                // -Ideally this should never run, but added just in case
                unsigned int failedbytes;
                for (failedbytes=byteIndex; failedbytes > 0; failedbytes--){
                    
                    // Careful not to swap bytes here...
                    pushByte(b, *( (unsigned char*)(d + elementIndex * b->width + failedbytes ) ));
                }
                
                // Return a count of failed pop operations
                // -Include partial pops in counter
                return l - elementIndex;
            }
        }
    }
    return 0;
}

// Byte-size push function
void pushByte(buffer_t *b, unsigned char d){
    
    // If we are overwriting a full buffer, increment tail pointer so that the
    // head doesn't move past the tail
    if ( (isBufferFull(b)) && (b->behavior.bits.overwrite) ) {
        increment(b, &(b->tail));
    }
    
    // Regardless of FIFO or FILO, always push to head
    *((unsigned char*)b->head) = d;
    increment(b, &(b->head));
}

// Arbitrary-size push function
unsigned int pushToBuffer(buffer_t *b, void *d, unsigned int l) {
    unsigned int elementIndex, byteIndex;
    
    // Loop through all elements
    for (elementIndex = 0; elementIndex < l; elementIndex++) {

        // Loop through all bytes of each element
        for (byteIndex = 0; byteIndex < b->width; byteIndex++) {
        
            // Only push to buffer if it is not full or overwriting is allowed
            if ( (!isBufferFull(b)) || (b->behavior.bits.overwrite) ) {
                pushByte(b, *( (unsigned char*)(d + elementIndex * (b->width) + byteIndex) ));
            }
            
            // If buffer is full, return a count of those elments not pushed
            else {
                
                // Pop all bytes of incomplete elements
                // -This should never run, but added just in case
                unsigned int failedbytes;
                for (failedbytes = byteIndex; failedbytes > 0; failedbytes--) {
                   
                    // If it is a queue pop comes from tail, so decrement head
                    decrement(b, &(b->head));
                }
                
                // Return a count of failed push operations
                // -Include partial pushes in count
                return l - elementIndex;
            }
        }
    }
    return 0;
}

#endif