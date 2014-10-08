#include <stdio.h>
#include <stdint.h>
#include <math.h>

const uint32_t sizeArray = 8192;//8192x4=32768

//Declare Bucket With Size: 32768 Bytes
uint32_t memory[8192] = {2048, 8192+10, 8192+10}; //0b100000 = 32
//Declare an array to contain all the head pointers for each bucket
static int buckets[11] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};

//buckets[0] 32 to 64 Bytes
//buckets[1] 64 to 127 Bytes
//buckets[2] 128 to 255 Bytes
//buckets[3] 256 to 511 Bytes
//buckets[4] 512 to 1023 Bytes
//buckets[5] 1024 to 2047 Bytes
//buckets[6] 2048 to 4095 Bytes
//buckets[7] 4096 to 8191 Bytes
//buckets[8] 8192 to 16383 Bytes
//buckets[9] 16384 to 32787 Bytes
//buckets[10] 32768 to whatever Bytes

//defines for reading bitfields
#define previousRead(ptr) ((4290772992 & ptr) >> 19) //0b11111111110000000000000000000000 = 4290772992, shift left by 22 and right by 17, 4 Byte addressable
#define nextRead(ptr) ((4190208 & ptr) >> 9)//0b1111111111000000000000 = 4190208, shift left by 12 and right by 5, 4 Byte addressable
#define sizeBlockRead(ptr) ((4094 & ptr) << 4)   //0b111111111110 = 4094
#define allocatedRead(ptr) (ptr & 1)

//defines for righting bitfields
#define previousWrite(ptr, value) ptr = (value << 19) + (4194303 & ptr) //0b1111111111111111111111 = 4194303
#define nextWrite(ptr, value) ptr = (value << 9) + (4290777087 & ptr) //0b11111111110000000000111111111111 = 4290777087
#define sizeBlockWrite(ptr, value) ptr = (value >> 4) + (4294963201 & ptr) //0b11111111111111111111000000000001 = 4294963201
//#define allocate(ptr) ptr = ptr & 4294967295 //0b11111111111111111111111111111111 = 4294967295
#define allocate(ptr) ptr = ptr | 1 //0b11111111111111111111111111111111 = 4294967295
#define unallocate(ptr) ptr = ptr & 4294967294 //0b11111111111111111111111111111110 = 4294967294


void half_init(){
    int i;
    //Declare Bucket With Size: 32768 Bytes
    memory[0] = 2048;
    memory[1] = 8902;
    memory[2] = 8902; //0b100000 = 32
    //Declare an array to contain all the head pointers for each bucket
    for (i = 0; i<10; i++)
        buckets[i] = -1;
    buckets[10] = 0;
    return;
}


void removeFromBucket(uint32_t location){
    if (memory[location+1] >= sizeArray && memory[location+2] >= sizeArray){
        buckets[memory[location+1]-sizeArray] = -1; //sizeArray + 10
    }
    else if (memory[location+1] >= sizeArray){
        buckets[memory[location+1]-sizeArray] = memory[location+2];
        memory[memory[location+2]+1] = memory[location+1];
    }
    else if (memory[location+2] >= sizeArray)
    {
        memory[memory[location+1]+2] = memory[location+2];
    }
    else{
        memory[memory[location+1]+2] = memory[location+2];
        memory[memory[location+2]+1] = memory[location+1];
    }
}

void addToBucket(uint32_t size, uint32_t location) {
    uint32_t i = 0;
    size = size >> 5;
    while(size > 0){
        i++;
        size = size >> 1;
    }
    //printf("i: %u\n", i );
    i--;
    //printf("i: %u\n", i );
    
    if (buckets[i] < 0){
        memory[location+2] = sizeArray+i;
    }
    else{
        memory[location+2] = buckets[i];
        memory[buckets[i]+1] = location;
    }
    memory[location+1] = sizeArray+i;
    buckets[i] = location;
}


//Declare Half_Alloc Function
void *half_alloc(unsigned int n){
    uint32_t i = 0;
    uint32_t differnce;
    uint32_t size = n+4;
    uint32_t temp = size;
    uint32_t GP;
    uint32_t mod = 0;
    
    
    if (n > 32763)
        return NULL;
	//smallest bucket size is 32 so start there
    temp = temp >> 5;
    //find the power of two (or bucket size in which size is less then

    
    while(temp > 0){
        //If all the buckets are empty return null
        if (i == 10)
            return NULL;
        i++;
        temp = temp >> 1;
    }
    //If larger bucket is empty keep going larger till you find a non empty bucket
    while(buckets[i] == -1){
        if (i == 10)
            return NULL;
        i++;
    }
    //Take the first block of the top of the bucket larger the the size to be allocated
    
	// Check to see if amount of unallocated memory left after allocation has occurred is greater then 8 bytes (64 bits) (smallest appropriate block size)
    temp = sizeBlockRead(memory[buckets[i]]);
    
    mod = size%32;
    if (mod == 0)
        differnce = 0;
    else
        differnce = 32-mod;
    size += differnce;
    if(temp - size == 0){
        temp = buckets[i];
        removeFromBucket(temp);
        return (void *)&memory[temp+1];
    }
    printf("Allocating %u\n", size);
    
    GP = buckets[i] + size/4;
    
    //Update size of old header
    sizeBlockWrite(memory[buckets[i]], size);
    
    //Create new header
    mod = nextRead(memory[buckets[i]]);
    if (mod == buckets[i]){
        nextWrite(memory[GP], GP);
    }
    else{
        nextWrite(memory[GP], nextRead(memory[buckets[i]]));
    }
    
    previousWrite(memory[GP], buckets[i]);
    size = temp - size;
    sizeBlockWrite(memory[GP], size);
    unallocate(memory[GP]);
    //Update the next pointer of the old header
    nextWrite(memory[buckets[i]], GP);
    //Update the linked list, removing block to allocate
    temp = buckets[i];
    //Allocate the block
    allocate(memory[temp]);
    //Add the new header to linked list
    removeFromBucket(temp);
    addToBucket(size, GP);
    printf("Memory Address %u",&memory[temp+1] );
    return &memory[temp+1];
}

// Declare Half_Free Function
void half_free(void *mem_free){
	uint32_t *mem_f = (uint32_t *) mem_free;
    uint32_t next, previous, newSize, size, location;
    uint8_t nextA, previousA;
    
    if (mem_free == NULL)
        return;
    
    printf("%u", *mem_f);
    --mem_f;
    printf("%u", *mem_f);
    
    size = sizeBlockRead(*mem_f);
    previous = previousRead(*mem_f);
    next = nextRead(*mem_f);
    
    if (&memory[previous] ==  mem_f && &memory[next] == mem_f){
        location = 1;
        nextA = 1;
        previousA = 1;
    }
    else if (&memory[previous] ==  mem_f){
        location = 0;
        nextA = allocatedRead(memory[next]);
        previousA = 1;
    }
    else if (&memory[next] == mem_f){
        location = nextRead(memory[previous]);
        nextA = 1;
        previousA = allocatedRead(memory[previous]);
    }
    else{
        location = nextRead(memory[previous]);
        nextA = allocatedRead(memory[next]);
        previousA = allocatedRead(memory[previous]);
    }
    
    location = previousRead(memory[next]);
    
    //check for coalesce with next and previous
    if (!nextA && !previousA){
        printf("coalesce both\n");
        newSize = sizeBlockRead(memory[previous])+size+sizeBlockRead(memory[next]);
        previousWrite(memory[nextRead(memory[next])], previous);
        nextWrite(memory[previous], nextRead(memory[next]));
        sizeBlockWrite(memory[previous], newSize);
        
        removeFromBucket(location);
        removeFromBucket(previous);
        removeFromBucket(next);
        
        unallocate(previous);
        addToBucket(newSize, previous);
    }
    //check for coalesce with next only
    else if (!nextA){
        printf("coalesce next\n");
        newSize = size+sizeBlockRead(memory[next]);
        previousWrite(memory[nextRead(memory[next])], *mem_f);
        nextWrite(*mem_f, nextRead(memory[next]));
        sizeBlockWrite(*mem_f, newSize);
        removeFromBucket(location);
        removeFromBucket(next);
        unallocate(*mem_f);
        //printf("%u \n",allocatedRead(memory[location]));
        addToBucket(newSize, location);
    }
    else if (!previousA){
        printf("Coalesce Previous\n");
        newSize = size+sizeBlockRead(memory[previous]);
        previousWrite(memory[next], previous);
        nextWrite(memory[previous], next);
        sizeBlockWrite(memory[previous], newSize);
        removeFromBucket(location);
        removeFromBucket(next);
        unallocate(previous);
        addToBucket(newSize, previous);
    }
    else{
        printf("Coalesce Nothing\n");
        unallocate(*mem_f);
        printf("add to bucket\n");
        addToBucket(size, location);
    }
    
}
