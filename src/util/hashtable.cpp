#include<stdio.h>
#include<string.h>
#include "hashtable.h"

HashTable::HashTable(uint bit_size)
{
    if (bit_size <= 3)
        bit_size = HASHTABLE_BIT_SIZE;
    this->m_bit_size = bit_size;
    this->m_table = new char[1 << (bit_size - 3)];
}

HashTable::~HashTable()
{
    if (this->m_table)
    {
        delete[] this->m_table;
        this->m_table = NULL;
    }
}

// DJB Hash Function
uint HashTable::hash_code(char* str, uint bit_size, uint base)
{
    uint result = base; // Magic num?
    long size = (1 << bit_size) - 1;
printf("hash_code for %s, %x", str, size);
    while (*str)
    {
        result = (result << 5) + result + (*str ++);
        result &= size;
    }
printf(", %d, %d is %d\n", bit_size, base, result);
    return result;
}

bool HashTable::test_put(uint code)
{
//    uint code = hash_code(str, this->m_bit_size);
    uint pos = code >> 3;
    uint bit = 1 << (code & 7);
    if (this->m_table[pos] & bit)
        return false;
    this->m_table[pos] |= bit;
    return true;
}
