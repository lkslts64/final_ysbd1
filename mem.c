#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"

void ByteArrayToRecord(void *arr,Record *rec) {
		
	int offset = 0;
	memcpy(&(rec->id),arr + offset,sizeof(int));
	offset += sizeof(int);
	memcpy(&(rec->name),arr + offset,sizeof(rec->name));
	offset += sizeof(rec->name);
	memcpy(&(rec->surname),arr + offset,sizeof(rec->surname));
	offset += sizeof(rec->surname);
	memcpy(&(rec->address),arr + offset,sizeof(rec->address));
	offset += sizeof(rec->address);
}

void ByteArrayToBlock(Block *block,void *arr) {
	int offset = 0;
	memcpy(&(block->recordsCounter),arr + offset,sizeof(char));
	offset += sizeof(char);
	for (int i = 0; i < MAX_RECORDS; i++ ) {
		ByteArrayToRecord(arr+offset,&(block->records[i]));
		offset += sizeof(Record);
	}
	memcpy(&(block->maxRecords),arr + offset,sizeof(block->maxRecords));
	offset += sizeof(block->maxRecords);
	memcpy(&(block->next),arr + offset,sizeof(block->next));
	offset += sizeof(block->next);
}

void SHT_ByteArrayToRecord(void *arr,SecondaryRecord *rec) {
		
	int offset = 0;
	ByteArrayToRecord(arr,&rec->record);
	offset += sizeof(Record);
	memcpy(&(rec->block),arr + offset,sizeof(rec->block));
}

void SHT_ByteArrayToBlock(SecondaryBlock *block,void *arr) {
	int offset = 0;
	memcpy(&(block->recordsCounter),arr + offset,sizeof(char));
	offset += sizeof(char);
	for (int i = 0; i < MAX_RECORDS; i++ ) {
		SHT_ByteArrayToRecord(arr+offset,&(block->records[i]));
		offset += sizeof(SecondaryRecord);
	}
	memcpy(&(block->maxRecords),arr + offset,sizeof(block->maxRecords));
	offset += sizeof(block->maxRecords);
	memcpy(&(block->next),arr + offset,sizeof(block->next));
	offset += sizeof(block->next);
}
