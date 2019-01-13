#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "BF.h"
#include "mem.h"
#include "hash_func.h"
#include "sht.h"
#include "ht.h"
#include "print.h"

int SHT_CreateSecondaryIndex( char *sfileName, char* attrName, int attrLength, int buckets , char *fileName)
{
	int fd;
	void* block;
	
	printf("Creating file with name: %s\n", sfileName);
	if (BF_CreateFile(sfileName) < 0) { BF_PrintError("Error creating file");return -1; }
	
	//printf("Opening file with name: %s\n", sfileName);
	if ((fd = BF_OpenFile(sfileName)) < 0) { BF_PrintError("Error opening file");return -1; }
	
	//printf("Allocating Block\n");
	if (BF_AllocateBlock(fd) < 0) { BF_PrintError("Error allocating block");return -1; }
	
	//printf("Reading Block\n");
	if (BF_ReadBlock(fd, 0, &block) < 0) { BF_PrintError("Error reading block");return -1; }
	
	//INITIALIZATION
	memset(block,0,BLOCK_SIZE);
	
	int offset = 0;
	int metadata_size = 4*sizeof(int) + strlen(attrName)+1 + strlen(fileName)+1 ;
	
	memcpy( block 		, (char*)&metadata_size  , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset	, (char*)&fd 		 , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset	, (char*)&attrLength	 , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset	, attrName		 , strlen(attrName)+1 );	offset += strlen(attrName)+1;
	memcpy( block+offset	, (char*)&buckets	 , sizeof(int) );		offset += sizeof(int);
	memcpy( block+offset	, fileName               , strlen(fileName)+1);		offset += strlen(fileName)+1;
	
	//printf("Writing Block\n");
	if (BF_WriteBlock(fd, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	////// put all the existing records from ht, into sht!!!
	HT_info* ht;
	void *blockptr;
	ht = HT_OpenIndex(fileName);
	for (int i = 0; i < ht->numBuckets; i++)//copy every bucket
	{
		if (BF_ReadBlock(ht->fileDesc, 0, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
		int next;
		memcpy( &next , blockptr + ht->offset + i*4 , sizeof(int) );//get first block's number
		
		if (BF_WriteBlock(ht->fileDesc, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
		
		while (next)//read all blocks of the bucket
		{
			Block bl;
			
			if (BF_ReadBlock(ht->fileDesc, next, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
			ByteArrayToBlock(&bl,blockptr);
			if (BF_WriteBlock(ht->fileDesc, next) < 0) { BF_PrintError("Error writing block back");return -1; }
			
			for (int j = 0; j < bl.recordsCounter; j++)//insert every record of this block, into sht
			{
				
				SecondaryRecord sr;
				sr.record.id = bl.records[j].id;
				strcpy(sr.record.name,bl.records[j].name);
				strcpy(sr.record.surname,bl.records[j].surname);
				strcpy(sr.record.address,bl.records[j].address);
				sr.block = next;
				
				if ( InsertV2( fd , attrName , metadata_size, buckets , sr ) < 0 ){return -1;}
				
			}
			next=bl.next;
		}
	}
	HT_CloseIndex(ht);
	
	
	//printf("Closing File\n");
	if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return -1; }
	
	return 0;
}

int InsertV2(int sFileDescriptor, char *attrName , int metadata_size, int numBuckets , SecondaryRecord record )
{
	void* blockptr;
	int bucket_num,block_num,next;
	char recordsCounter;
	
	if (BF_ReadBlock(sFileDescriptor, 0, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
	
	//find the key for hashing
	if ( !strncmp(attrName,"id",strlen("id")) )
		bucket_num = universal_hash_int(record.record.id,numBuckets);
	else if ( !strncmp(attrName,"name",strlen("name")) )
		bucket_num = universal_hash_string(record.record.name,numBuckets);
	else if ( !strncmp(attrName,"surname",strlen("surname")) )
		bucket_num = universal_hash_string(record.record.surname,numBuckets);
	else if ( !strncmp(attrName,"address",strlen("address")) )
		bucket_num = universal_hash_string(record.record.address,numBuckets);
	else 
		{printf("ERROR ! NOT CORRECT attrName\n");return -1;}
	
	bucket_num *= sizeof(int);//normalize
	bucket_num += metadata_size;
	memcpy(&block_num,blockptr+bucket_num,sizeof(int));
	
	if(block_num<=0) //no existing block, allocate one
	{
		//printf("Allocating Block (not existing)\n");
		if (BF_AllocateBlock(sFileDescriptor) < 0) { BF_PrintError("Error allocating block");return -1; }
		block_num = BF_GetBlockCounter(sFileDescriptor) - 1;
		memcpy(blockptr+bucket_num,&block_num,sizeof(int));//insert value into hash table
	}
	
	if (BF_WriteBlock(sFileDescriptor, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	//printf("Reading Block %d\n",block_num);
	void *blockptr2;
	if (BF_ReadBlock(sFileDescriptor, block_num, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
	
	memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
	memcpy(&next,blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char),sizeof(int));//get info for next;
	
	while (recordsCounter == MAX_RECORDS) //full of records, check next
	{
		if(next<=0)//allocate block
		{
			//printf("Allocating Block(full)\n");
			if (BF_AllocateBlock(sFileDescriptor) < 0) { BF_PrintError("Error allocating block");return -1; }
			int new_block_num = BF_GetBlockCounter(sFileDescriptor) - 1;
			
			//insert new_block_num into full block
			memcpy(blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char), &new_block_num, sizeof(int));
			//write the full block
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(sFileDescriptor, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			
			//read new block
			block_num = new_block_num;
			//printf("Reading Block %d\n",block_num);
			if (BF_ReadBlock(sFileDescriptor, block_num, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
			
			break;
		}
		else
		{
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(sFileDescriptor, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			//printf("Reading Block %d\n",next);
			if (BF_ReadBlock(sFileDescriptor, next, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
			block_num=next;
			memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
			memcpy(&next,blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char),sizeof(int));//get info for next;
		}
	}
	
	//insert record into block
	memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
	recordsCounter++;
	memcpy(blockptr2,&recordsCounter,sizeof(char));
	int offset = sizeof(char);
	offset+=(recordsCounter-1)*sizeof(SecondaryRecord);//skip previous records
	memcpy(blockptr2+offset,&(record.record.id),sizeof(int));
	offset+=sizeof(int);
	memcpy(blockptr2+offset,record.record.name,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr2+offset,record.record.surname,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr2+offset,record.record.address,ADDRESS_SIZE);
	offset+=ADDRESS_SIZE;
	//////blockID
	//int BlockId = getBlockId(header_info , record.record.id);
	//We can also do this instead of the above line (assuming in record there is a value block number given as parameter).
	int BlockId = record.block;
	if( BlockId<0 ) { return -1; }
	memcpy(blockptr2+offset, &BlockId ,sizeof(int));
	offset+=sizeof(int);
	
	//write block
	//printf("Writing Block %d\n",block_num);
	if (BF_WriteBlock(sFileDescriptor, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	//return block_num;
	return 0;
}

SHT_info* SHT_OpenSecondaryIndex ( char *sfileName ){
	void *block;
	int fd;
	if ((fd = BF_OpenFile(sfileName)) < 0) {
		printf("Error\n"); BF_PrintError("cannot open file");
		return NULL;
	}
	if (BF_ReadBlock(fd,0,&block) < 0) {
		printf("Error\n"); BF_PrintError("cannot read file");
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		return NULL;
	}
	SHT_info *sht_info = malloc(sizeof(SHT_info));
	int count = 0;
	memcpy(&(sht_info->offset),block + count,sizeof(int));		
	count += sizeof(int);
	memcpy(&(sht_info->fileDesc),(char *)&fd,sizeof(int));
	count += sizeof(int);
	memcpy(&(sht_info->attrLength),block + count,sizeof(int));
	count += sizeof(int);
	if ( sht_info->attrLength > ADDRESS_SIZE) {
		free(sht_info);
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL; }
		BF_WriteBlock(fd,0);
		return NULL;
	}
	sht_info->attrName = malloc((sht_info->attrLength+1) * sizeof(char));
	memcpy((sht_info->attrName),block + count,sht_info->attrLength + 1);	//+1 because we want to include '\0' char
	count += sht_info->attrLength + 1;
	memcpy(&(sht_info->numBuckets),block + count,sizeof(int));	
	count += sizeof(int);
	

	int loop=0;
	do
	{
		memcpy( sht_info->fileName + loop , block + count ,1);
		count++;loop++;
	}while( (*(sht_info->fileName + loop - 1))!='\0' );
	
	
	if ( sht_info->fileDesc < 0 || sht_info-> attrLength < 0 || 
		(strncmp(sht_info->attrName,"name",strlen("name")) != 0 && strncmp(sht_info->attrName,"surname",strlen("surname")) != 0 &&
		strncmp(sht_info->attrName,"address",strlen("address")) != 0)  || sht_info->numBuckets < 0 || sht_info->fileName[0] == '\0')
	{
		printf("This is not an index file\n"); free(sht_info->attrName);free(sht_info);
		if (BF_CloseFile(fd) < 0) { BF_PrintError("Error closing file");return NULL;}
		BF_WriteBlock(fd,0);
		return NULL;
	}
	//print_SHT(*sht_info);
	BF_WriteBlock(fd,0);
	//printf("File %s is open\n",sfileName);
	return sht_info;
}


int SHT_CloseSecondaryIndex( SHT_info* header_info )
{
	printf("Closing File\n");
	if (BF_CloseFile(header_info->fileDesc) < 0) { BF_PrintError("Error closing file");return -1; }
	
	//free memory
	free(header_info->attrName);
	free(header_info);
	
	return 0;
}

int SHT_SecondaryInsertEntry( SHT_info header_info, SecondaryRecord record)
{
	void* blockptr;
	int bucket_num,block_num,next;
	char recordsCounter;
	if (BF_ReadBlock(header_info.fileDesc, 0, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
	
	//find the key for hashing
	if ( !strncmp(header_info.attrName,"id",strlen("id")) )
		bucket_num = universal_hash_int(record.record.id,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"name",strlen("name")) )
		bucket_num = universal_hash_string(record.record.name,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"surname",strlen("surname")) )
		bucket_num = universal_hash_string(record.record.surname,header_info.numBuckets);
	else if ( !strncmp(header_info.attrName,"address",strlen("address")) )
		bucket_num = universal_hash_string(record.record.address,header_info.numBuckets);
	else 
		{printf("ERROR ! NOT CORRECT attrName\n");return -1;}
	
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
	void *blockptr2;
	if (BF_ReadBlock(header_info.fileDesc, block_num, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
	
	memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
	memcpy(&next,blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char),sizeof(int));//get info for next;
	
	while (recordsCounter == MAX_RECORDS) //full of records, check next
	{
		if(next<=0)//allocate block
		{
			//printf("Allocating Block(full)\n");
			if (BF_AllocateBlock(header_info.fileDesc) < 0) { BF_PrintError("Error allocating block");return -1; }
			int new_block_num = BF_GetBlockCounter(header_info.fileDesc) - 1;
			
			//insert new_block_num into full block
			memcpy(blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char), &new_block_num, sizeof(int));
			//write the full block
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			
			//read new block
			block_num = new_block_num;
			//printf("Reading Block %d\n",block_num);
			if (BF_ReadBlock(header_info.fileDesc, block_num, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
			
			break;
		}
		else
		{
			//printf("Writing Block(full)\n");
			if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
			//printf("Reading Block %d\n",next);
			if (BF_ReadBlock(header_info.fileDesc, next, &blockptr2) < 0) { BF_PrintError("Error reading block");return -1; }
			block_num=next;
			memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
			memcpy(&next,blockptr2+sizeof(char)+MAX_RECORDS*sizeof(SecondaryRecord)+sizeof(char),sizeof(int));//get info for next;
		}
	}
	
	//insert record into block
	memcpy(&recordsCounter,blockptr2,sizeof(char));//get info for records;
	recordsCounter++;
	memcpy(blockptr2,&recordsCounter,sizeof(char));
	int offset = sizeof(char);
	offset+=(recordsCounter-1)*sizeof(SecondaryRecord);//skip previous records
	memcpy(blockptr2+offset,&(record.record.id),sizeof(int));
	offset+=sizeof(int);
	memcpy(blockptr2+offset,record.record.name,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr2+offset,record.record.surname,NAMES_SIZE);
	offset+=NAMES_SIZE;
	memcpy(blockptr2+offset,record.record.address,ADDRESS_SIZE);
	offset+=ADDRESS_SIZE;
	//////blockID
	//int BlockId = getBlockId(header_info , record.record.id);
	//We can also do this instead of the above line (assuming in record there is a value block number given as parameter).
	int BlockId = record.block;
	if( BlockId<0 ) { return -1; }
	memcpy(blockptr2+offset, &BlockId ,sizeof(int));
	offset+=sizeof(int);
	
	//write block
	//printf("Writing Block %d\n",block_num);
	if (BF_WriteBlock(header_info.fileDesc, block_num) < 0) { BF_PrintError("Error writing block back");return -1; }
	
	//return block_num;
	return 0;
}


int getBlockId( SHT_info header_info ,int id )
{
	int next,value;
	Block bl;
	void* blockptr;
	HT_info* primary;
	
	//open the primary file to look for the record
	if ( (primary = HT_OpenIndex(header_info.fileName)) == NULL ) return -1;
	print_HT(*primary);
	//read the hash table
	if (BF_ReadBlock(primary->fileDesc, 0, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
	if (BF_WriteBlock(primary->fileDesc, 0) < 0) { BF_PrintError("Error writing block back");return -1; }
	printf("blockptr = %p and fd = %d \n",blockptr,primary->fileDesc);
	blockptr += primary->offset;
	value = universal_hash_int(id,primary->numBuckets);
	blockptr += value*4;
	memcpy(&next,blockptr,sizeof(int));
	printf ("next is =  %d and id = %d ,offset = %d ,bucket_num = %d \n", next,id,primary->offset,value);
	if (next <= 0 ) {
		HT_CloseIndex(primary);
		printf("Error at getBlockId .Returning...\n");return -1;
	}
	while (1)
	{
		//read next block and look for the record
		if (BF_ReadBlock(primary->fileDesc, next, &blockptr) < 0) { BF_PrintError("Error reading block");return -1; }
		ByteArrayToBlock(&bl,blockptr);
		if (BF_WriteBlock(primary->fileDesc, next) < 0) { BF_PrintError("Error writing block back");return -1; }
		
		for (int i = 0; i < bl.recordsCounter ; i++)
			if (bl.records[i].id == id)//found
			{
				HT_CloseIndex(primary);
				return next;
			}
		
		next = bl.next;
		if(next<=0)
		{
			printf("Error: blockid not found\n");
			HT_CloseIndex(primary);
			return -1;
		}
	}
}


int SHT_SecondaryGetAllEntries ( SHT_info header_info_sht,HT_info header_info_ht,void *value) {
	int block_num,bucket_num;
	void *block;
	bucket_num = universal_hash_string((char *)value,header_info_sht.numBuckets);
	bucket_num *= sizeof(int);
	bucket_num += header_info_sht.offset;
	if ( BF_ReadBlock(header_info_sht.fileDesc,0,&block) < 0) {
		BF_PrintError("Error Reading\n");return -1;
	}
	memcpy(&block_num,block + bucket_num,sizeof(int));
	if ( !block_num ) {
		BF_WriteBlock(header_info_sht.fileDesc,0);
		printf("Entry with key = %s doesn't exist \n",(char *)value); return -1;
	}
	int  i, block_read = 0, rec_print = 0;
	SecondaryRecord  rec;
	SecondaryBlock bl;
	while(666) {
		if ( BF_ReadBlock(header_info_sht.fileDesc,block_num,&block) < 0 ) {
			BF_PrintError("Error Reading\n");return -1;
		}
		block_read++;
		SHT_ByteArrayToBlock(&bl,block);
		for (i = 0; i < bl.recordsCounter; i++ ) {
			rec = bl.records[i];
			if (strncmp(header_info_sht.attrName,"name",strlen("name")) == 0) {
				if (strcmp((char *)value,rec.record.name) == 0) {
						print_Record(&rec.record);rec_print++;
				}
			} else if (strncmp(header_info_sht.attrName,"surname",strlen("surname")) == 0) {
				if (strcmp((char *)value,rec.record.surname) == 0) {
						print_Record(&rec.record);rec_print++;
				}
			} else if (strncmp(header_info_sht.attrName,"address",strlen("address")) == 0) {
				if (strcmp((char *)value,rec.record.address) == 0) {
						print_Record(&rec.record);rec_print++;
				}
			}
		}
		if ( bl.next <= 0 ) {
			if ( !rec_print )  {
				printf("Entry with key = %s doesn't exist \n",(char *)value); return -1;
			}
			printf("Blocks Read : %d \n",block_read);
			BF_WriteBlock(header_info_sht.fileDesc,block_num);
			return rec_print;
		}
		BF_WriteBlock(header_info_sht.fileDesc,block_num);
		block_num = bl.next;
	}
}
//not used anymore.
Record *FindInHT(int ht_block_num,int id,HT_info header_info_ht) 
{
	void *block;
	Block bl;
	Record *rec = malloc(sizeof(Record));
	if ( BF_ReadBlock(header_info_ht.fileDesc,ht_block_num,&block) < 0 )
		return NULL;
	ByteArrayToBlock(&bl,block);
	for (int i = 0; i < bl.recordsCounter; i++) {
		*rec  = bl.records[i];
		if ( rec->id == id ) 
			return rec;
	}
	printf("Record with id = %d exist in SHT but not in HT .Maybe primary index  is not updated\n",id);
	return NULL;
}
