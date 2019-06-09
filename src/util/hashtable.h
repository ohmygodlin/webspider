/* Class hashtable
 * Default implementation: Bitmap
 * Create Date: 2013/06/03
*/
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include<sys/types.h>
const uint HASHTABLE_BIT_SIZE = 24;

class HashTable
{
public:
    HashTable (uint bit_size = HASHTABLE_BIT_SIZE);   // Default size: 2^24 = 16 * 1024 * 1024
    ~HashTable ();
    
    /* Test and put a string into the hashtable.
    * return true if the str not exist before, and being set.
    * return false if the str already exists, nothing need to be done.
    */
    bool test_put(uint code);
    
    // TODO add test function if needed to only test without put.
    
    static uint hash_code(char* str, uint bit_size, uint base = 0);
protected:
    uint m_bit_size;
    char* m_table;
};
#endif //HASHTABLE_H
