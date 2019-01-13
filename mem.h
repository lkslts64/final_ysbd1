#ifndef MEM_H
#define MEM_H

void SHT_ByteArrayToBlock(SecondaryBlock *block,void *arr) ;
void SHT_ByteArrayToRecord(void *arr,SecondaryRecord *rec) ;
void ByteArrayToRecord(void *arr,Record *rec) ;
void ByteArrayToBlock(Block *block,void *arr) ;

#endif
