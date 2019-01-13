#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "BF.h"
#include "ht.h"
#include "mem.h"
#include "hash_func.h"
#include "print.h"

int HT_CreateIndex( char *fileName,char attrType,char* attrName,int attrLength,int buckets)
{
	
	int fd;
	void* block;
	
	//BF_Init();
	
	printf("Creating file with name: %s\n", fileName);
	if (BF_CreateFile(fileName) < 0) { BF_PrintError("Error creating file");return -1; }
	
	if ((fd = BF_OpenFile(fileName)) < 0) { BF_PrintError("Error opening file");return -1; }
	
	if (BF_AllocateBlock(fd) < 0) { BF_PrintError("Error allocating block");return -1; }
	
	//printf("File %d has %d blocks\n", fd, BF_GetBlockCounter(fd));
	
	if (BF_ReadBlock(fd, 0, &block) < 0) { BF_PrintError("Error reading block");return -1; }
	
	//INITIALIZATION
	memset(block,0,BLOCK_SIZE);
	
	int offset = 0;
	int metadata_size = 4*sizeof(int) + sizeof(char) + strlen(attrName)+1;
	
	memcpy( block , (char*)&metadata_size , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset , (char*)&fd , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset , (char*)&attrType , sizeof(char) ); 	offset += sizeof(char);
	memcpy( block+offset , (char*)&attrLength , sizeof(int) );	offset += sizeof(int);
	memcpy( block+offset , attrName , strlen(attrName)+1 );		offset += strlen(attrName)+1;
	memcpy( block+offset , (char*)&buckets , sizeof(int) );		offset += sizeof(int);
	
	if (BF_WriteBlock(fd, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return -1; }
	
	return 0;
}

HT_info* HT_OpenIndex( char *fileName ) {
	void *block;
	int fd;
	if ((fd = BF_OpenFile(fileName)) < 0) {
		printf("Error\n"); BF_PrintError("cannot open file");
		return NULL;
	}
	if (BF_ReadBlock(fd,0,&block) < 0) {
		printf("Error\n"); BF_PrintError("cannot read file");
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		return NULL;
	}
	HT_info *ht_info = malloc(sizeof(HT_info));
	int count = 0;
	memcpy(&(ht_info->offset),block + count,sizeof(int));		
	count += sizeof(int);
	memcpy(&(ht_info->fileDesc),(char *)&fd,sizeof(int));
	count += sizeof(int);
	memcpy(&(ht_info->attrType),block + count,sizeof(char));
	if (ht_info-> attrType != 'c' && ht_info-> attrType != 'i' ) {
		BF_WriteBlock(fd,0);
		free(ht_info);
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		return NULL;
	}
	count += sizeof(char);
	memcpy(&(ht_info->attrLength),block + count,sizeof(int));
	count += sizeof(int);
	if ( ht_info->attrLength > ADDRESS_SIZE) {
		free(ht_info);
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		return NULL;
	}
	ht_info->attrName = malloc((ht_info->attrLength+1) * sizeof(char));
	memcpy((ht_info->attrName),block + count,ht_info->attrLength + 1);	//+1 because we want to include '\0' char
	count += ht_info->attrLength + 1;
	memcpy(&(ht_info->numBuckets),block + count,sizeof(int));	
	if ( ht_info->fileDesc < 0 || ht_info-> attrLength < 0 || 
		( strncmp(ht_info->attrName,"id",strlen("id")) != 0 && strncmp(ht_info->attrName,"name",strlen("name")) != 0 && 
	    strncmp(ht_info->attrName,"surname",strlen("surname")) != 0 && strncmp(ht_info->attrName,"address",strlen("address")) != 0)  || 
		ht_info->numBuckets < 0) {
		printf("This is not an index file\n"); free(ht_info->attrName);free(ht_info);
		BF_WriteBlock(fd,0);
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		return NULL;
	}
	BF_WriteBlock(fd,0);
	//printf("\nFile %s is open\n",fileName);
	return ht_info;
}

int HT_CloseIndex( HT_info* header_info )
{
	int fd = header_info->fileDesc;
	//free memory
	free(header_info->attrName);
	free(header_info);
	
	//printf("Closing File\n");
	if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return -1; }
	
	return 0;
}

int HT_InsertEntry( HT_info header_info,Record record )
{
	void* blockptr;
	int bucket_num,block_num,next;
	char recordsCounter;
	
	//printf("Reading Block 0\n");
	if (BF_ReadBlock(header_info.fileDesc, 0, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
	
	//find the key for hashing
	if ( !strncmp(header_info.attrName,"id",strlen("id")) )
		bucket_num = universal_hash_int(record.id,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"name",strlen("name")) )
		bucket_num = universal_hash_string(record.name,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"surname",strlen("surname")) )
		bucket_num = universal_hash_string(record.surname,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"address",strlen("address")) )
		bucket_num = universal_hash_string(record.address,header_info.numBuckets);
	else{
		printf("Error with attrName\n");return -1;
	}
	bucket_num *= sizeof(int);//normalize
	bucket_num += header_info.offset;
	memcpy(&block_num,blockptr+bucket_num,sizeof(int));
	
	if(block_num<=0) //no existing block, allocate one
	{
		//printf("Allocating Block (not existing)\n");
		if (BF_AllocateBlock(header_info.fileDesc) < 0) { BF_PrintError("Error allocating block");return -1; }
		block_num = BF_GetBlockCounter(header_info.fileDesc) - 1;
		memcpy(blockptr+bucket_num,&block_num,sizeof(int));//insert value into hash table
	}
	
	if (BF_WriteBlock(header_info.fileDesc, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	//printf("Reading Block %d\n",block_num);
	if (BF_ReadBlock(header_info.fileDesc, block_num, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
	
	memcpy(&recordsCounter,blockptr,sizeof(char));//get info for records;
	memcpy(&next,blockptr+sizeof(char)+MAX_RECORDS*sizeof(Record)+sizeof(char),sizeof(int));//get info for next;
	
	while (recordsCounter == MAX_RECORDS) //full of records, check next
	{
		if(next<=0)//allocate block
		{
			//printf("Allocating Block(full)\n");
			if (BF_AllocateBlock(header_info.fileDesc) < 0) { BF_PrintError("Error allocating block");return -1; }
			int new_block_num = BF_GetBlockCounter(header_info.fileDesc) - 1;
			
			//insert new_block_num into full block
			memcpy(blockptr+sizeof(char)+MAX_RECORDS*sizeof(Record)+sizeof(char), &new_block_num, sizeof(int));
			//write the full block
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			
			//read new block
			block_num = new_block_num;
			//printf("Reading Block %d\n",block_num);
			if (BF_ReadBlock(header_info.fileDesc, block_num, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
			
			break;
		}
		else
		{
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			//printf("Reading Block %d\n",next);
			if (BF_ReadBlock(header_info.fileDesc, next, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
			block_num=next;
			memcpy(&recordsCounter,blockptr,sizeof(char));//get info for records;
			memcpy(&next,blockptr+sizeof(char)+MAX_RECORDS*sizeof(Record)+sizeof(char),sizeof(int));//get info for next;
		}
	}
	
	//insert record into block
	memcpy(&recordsCounter,blockptr,sizeof(char));//get info for records;
	recordsCounter++;
	memcpy(blockptr,&recordsCounter,sizeof(char));
	int offset = sizeof(char);
	offset+=(recordsCounter-1)*sizeof(Record);//skip previous records
	memcpy(blockptr+offset,&(record.id),sizeof(int));
	offset+=sizeof(int);
	memcpy(blockptr+offset,record.name,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr+offset,record.surname,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr+offset,record.address,ADDRESS_SIZE);
	offset+=ADDRESS_SIZE;
	
	//write block
	//printf("Writing Block %d\n",block_num);
	if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	
	return block_num;
}

int HT_DeleteEntry( HT_info header_info,void *value )
{
	int block_num,bucket_num;
	void *block;
	if ( header_info.attrType == 'i' ) {
		bucket_num = universal_hash_int((*(int *)value),header_info.numBuckets);
	} else if ( header_info.attrType == 'c' ) {
		bucket_num = universal_hash_string((char *)value,header_info.numBuckets);
	}
	else {
		return -1;
	}
	bucket_num *= sizeof(int);	//normalize
	bucket_num += header_info.offset;
	if ( BF_ReadBlock(header_info.fileDesc,0,&block) < 0 ) {
		BF_PrintError("Error reading block");return -1; 
	}

	memcpy(&block_num,block +bucket_num,sizeof(int));
	if ( !block_num ) {
		BF_WriteBlock(header_info.fileDesc,0);
		printf("Entry with key = %d doesn't exist \n",*(int *)value); return -1;
	}
	int  i;
	Record  rec;
	Block bl;
	while(666) {
		if ( BF_ReadBlock(header_info.fileDesc,block_num,&block) < 0 ) {
			BF_PrintError("Error reading block");return -1; 
		}
		ByteArrayToBlock(&bl,block);
		for (i = 0; i < bl.recordsCounter; i++ ) {
			rec = bl.records[i];
			if (strncmp(header_info.attrName,"id",strlen("id")) == 0) {
				if ( rec.id == (*(int *)value) ) {
					goto delete;
				}
			} else if (strncmp(header_info.attrName,"name",strlen("name")) == 0) {
				if (strcmp((char *)value,rec.name)== 0) {
					goto delete;
				}
			} else if (strncmp(header_info.attrName,"surname",strlen("surname")) == 0) {
				if (strcmp((char *)value,rec.surname)== 0) {
					goto delete;
				}
			} else if (strncmp(header_info.attrName,"address",strlen("address")) == 0) {
				if (strcmp((char *)value,rec.address) == 0) {
					goto delete;
				}
			}
		}
		if ( bl.next <= 0 ) {
			BF_WriteBlock(header_info.fileDesc,block_num);
			printf("Entry with key = %d doesn't exist \n",*(int *)value); return -1;
		}
		block_num = bl.next;
		BF_WriteBlock(header_info.fileDesc,block_num);
	}
	char last_rec;
	delete:
		last_rec = bl.recordsCounter - 1;
		bl.recordsCounter--;
		//set record's bytes for deletion to zero. 
		//Copy last record's bytes to the newly created empty slot and set last record's byte to zero.
		memset(block+sizeof(char)+i*sizeof(Record) , '\0' , sizeof(Record));
		memcpy(block,&bl.recordsCounter,sizeof(char));
		if ( i == last_rec)  {
			BF_WriteBlock(header_info.fileDesc,block_num);
			return 0;
		}
		memcpy(block+sizeof(char)+i*sizeof(Record) , block+sizeof(char)+last_rec*sizeof(Record) , sizeof(Record));
		memset(block+sizeof(char)+last_rec*sizeof(Record) , '\0' , sizeof(Record));
		BF_WriteBlock(header_info.fileDesc,block_num);
		return 0;
}

int HT_GetAllEntries( HT_info header_info,void *value )
{
	int block_num,bucket_num;
	void *block;
	if ( header_info.attrType == 'i' ) {
		bucket_num = universal_hash_int((*(int *)value),header_info.numBuckets);
	} else if ( header_info.attrType == 'c' ) {
		bucket_num = universal_hash_string((char *)value,header_info.numBuckets);
	}
	else {
		return -1;
	}
	bucket_num *= sizeof(int);
	bucket_num += header_info.offset;
	if ( BF_ReadBlock(header_info.fileDesc,0,&block) < 0 ) {
		BF_PrintError("Error reading block");return -1; 
	}
	memcpy(&block_num,block + bucket_num,sizeof(int));
	if ( !block_num )  {
		BF_WriteBlock(header_info.fileDesc,0);
		printf("Entry with key = %d doesn't exist \n",*(int *)value); return -1;
	}
	int  i, block_read = 0, rec_print = 0;
	Record  rec;
	Block bl;
	while(666) {
		if ( BF_ReadBlock(header_info.fileDesc,block_num,&block) < 0 ) {
			BF_PrintError("Error reading block");return -1; 
		}
		block_read++;
		ByteArrayToBlock(&bl,block);
		for (i = 0; i < bl.recordsCounter; i++ ) {
			rec = bl.records[i];
			if (strcmp(header_info.attrName,"id")== 0) {
				if ( rec.id == (*(int *)value) ) {
					print_Record(&rec);rec_print++;
				}
			} else if (strncmp(header_info.attrName,"name",strlen("name")) == 0) {
				if (strcmp((char *)value,rec.name)== 0) {
					print_Record(&rec);rec_print++;
				}
			} else if (strncmp(header_info.attrName,"surname",strlen("surname")) == 0) {
				if (strcmp((char *)value,rec.surname) == 0) {
					print_Record(&rec);rec_print++;
				}
			} else if (strncmp(header_info.attrName,"address",strlen("address")) == 0) {
				if (strcmp((char *)value,rec.address) == 0) {
					print_Record(&rec);rec_print++;
				}
			}
		}
		if ( bl.next <= 0 ) {
			if ( !rec_print )  {
				printf("Entry with key = %d doesn't exist \n",*(int *)value); return -1;
			}
			printf("Blocks Readed : %d \n",block_read);
			BF_WriteBlock(header_info.fileDesc,block_num);
			return rec_print;
		}
		BF_WriteBlock(header_info.fileDesc,block_num);
		block_num = bl.next;
	}
	return rec_print;
}
