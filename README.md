# MemoryAllocator
Basic C Memory Management Library for POSIX Systems

Custom_sbrk is a function utilized by the allocator, located in the files custom_unistd.h and memmanager.c. 
The creator of this function can be found at: https://github.com/tomekjaworski/SO2/tree/master/heap_sbrk-sim.

## Description
The allocator prefixes Malloc, Calloc, Realloc, and Free functions with heap_.
These functions include fences positioned directly before and after the memory block allocated to the user
These fences serve to aid in the detection of One-off errors, as each fence has specific and known content and length. 
A breach indicates incorrect usage of the allocated memory block by the user's code.

## How to use
Firstly, initialize the heap using the ```heap_setup()``` function. 
Then, memory allocation can begin.
Finally, ensure to clean up using ```heap_clean()```. 
See the example below:

```c
#include "heap.h"
int main() {
    heap_setup();
    int *number = (int *)heap_malloc(sizeof(int));
    if (!number) {
        return 1;
    }
    heap_free(number);
    heap_clean();
    return 0;
}
```
