#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "ht.h"
#include "sht.h"
#include "mem.h"
#include "BF.h"

void print_HT(HT_info ht){
	printf("%d , %d , %c , %d , %s , %d \n",ht.offset,ht.fileDesc,ht.attrType,ht.attrLength,ht.attrName,ht.numBuckets);
}

void print_SHT(SHT_info sht){
	printf("%d , %d , %d , %s , %d , %s \n",sht.offset,sht.fileDesc,sht.attrLength,sht.attrName,sht.numBuckets,sht.fileName);
}

void print_Record(Record *rec) {
	printf("id = %d , name = %s , surname = %s , address = %s \n",rec->id,rec->name,rec->surname,rec->address);
}
void print_SHT_Record(SecondaryRecord *rec) {
	printf("id = %d , name = %s , surname = %s , address = %s \n",rec->record.id,rec->record.name,rec->record.surname,rec->record.address);
}
void print_Block(Block *bl) {
	printf( " recordsCounter = %d,maxRecords = %d,next = %d \n",bl->recordsCounter ,MAX_RECORDS,bl->next);
	for (int i = 0; i < MAX_RECORDS; i++ ) 
		print_Record(&bl->records[i]);
}
void print_SHT_Block(SecondaryBlock *sbl) {
	printf( " recordsCounter = %d,maxRecords = %d,next = %d \n",sbl->recordsCounter ,MAX_RECORDS,sbl->next);
	for (int i = 0; i < MAX_RECORDS; i++ ) 
		print_SHT_Record(&sbl->records[i]);
}

	


int SHT_Stats(SHT_info *sht)
{
	void *blockptr;
	void *blockptr2;
	int sumofrecs = 0;

	int block_num = 0;
	if ((block_num = BF_GetBlockCounter(sht->fileDesc)) < 0) {
		printf("Error\n"); BF_PrintError("cannot GetBlockCounter file");
	}
	printf("Index file  has %d blocks\n",block_num -1 );
	int i,offset = 0,min = MAX_RECORDS + 1,max = 0,mean_recs = 0,block_sum = 0,mean_blocks = 0,of_bucks = 0,of_blocks = 0;
	block_num  = 0;
	int flag = 0;
	SecondaryBlock bl;
	for (i = 0; i < sht->numBuckets; i++ ) {
		if (BF_ReadBlock(sht->fileDesc,0,&blockptr) < 0) {
			printf("Error\n"); BF_PrintError("cannot read file");
			return -1;
		}
		memcpy(&block_num,blockptr + sht->offset + offset,sizeof(int)); 
		BF_WriteBlock(sht->fileDesc,0);
		offset += sizeof(int);
		if ( !block_num ) {
			printf("Bucket %d doesn't point to any block \n",i);
			continue;
		}
		while (1) {
			if (BF_ReadBlock(sht->fileDesc,block_num,&blockptr2) < 0) {
				BF_PrintError("Block pointer in bucket doesnt exists\n");return -1;
			}
			block_sum++;
			SHT_ByteArrayToBlock(&bl,blockptr2);
			sumofrecs += bl.recordsCounter;
			//print_SHT_Block(&bl);
			if (bl.recordsCounter < min) {
				min = bl.recordsCounter;
			}
			if (bl.recordsCounter > max) {
				max = bl.recordsCounter;
			}
			if ( bl.recordsCounter == MAX_RECORDS) {
				of_blocks++;
				if ( !flag ) {
					of_bucks++;flag = 1;
				}
			}
			mean_recs += bl.recordsCounter;
			mean_blocks ++;
			if (bl.next <= 0) {
				BF_WriteBlock(sht->fileDesc,block_num); 
				break;
			}
			BF_WriteBlock(sht->fileDesc,block_num); 
			block_num = bl.next;
		}
		printf("Bucket %d : min_records = %d , max_records = %d , mean_records = %d , overflow_blocks = %d \n",i,min,max,mean_recs / block_sum,of_blocks);
		flag = 0;
		min = MAX_RECORDS + 1;
		max = -1;
		mean_recs = 0;
		of_blocks = 0;
		block_sum = 0;
	}
	printf("Mean value of blocks in buckets is %d and overflow buckets are %d \n",mean_blocks / sht->numBuckets , of_bucks);
	printf("-------- sum of recs = %d\n",sumofrecs);
	BF_WriteBlock(sht->fileDesc,0);
	SHT_CloseSecondaryIndex(sht);
	return 0;
}

int HT_Stats(HT_info *ht) 
{
	void *blockptr;
	void *blockptr2;
	int sumofrecs = 0;
	int block_num = 0;
	if ((block_num = BF_GetBlockCounter(ht->fileDesc)) < 0) {
		printf("Error\n"); BF_PrintError("cannot GetBlockCounter file");
	}
	printf("Index file  has %d blocks\n",block_num - 1);
	int i,offset = 0,min = MAX_RECORDS + 1,max = 0,mean_recs = 0,block_sum = 0,mean_blocks = 0,of_bucks = 0,of_blocks = 0;
	block_num  = 0;
	int flag = 0;
	Block bl;
	for (i = 0; i < ht->numBuckets; i++ ) {
		if (BF_ReadBlock(ht->fileDesc,0,&blockptr) < 0) {
			printf("Error\n"); BF_PrintError("cannot read file");
			return -1;
		}
		memcpy(&block_num,blockptr + ht->offset + offset,sizeof(int)); 
		BF_WriteBlock(ht->fileDesc,0);
		offset += sizeof(int);
		if ( !block_num ) {
			printf("Bucket %d doesn't point to any block \n",i);
			continue;
		}
		while (1) {
			if (BF_ReadBlock(ht->fileDesc,block_num,&blockptr2) < 0) {
				BF_PrintError("Block pointer in bucket doesnt exists\n");return -1;
			}
			block_sum++;
			ByteArrayToBlock(&bl,blockptr2);
			sumofrecs += bl.recordsCounter;
			if (bl.recordsCounter < min) {
				min = bl.recordsCounter;
			}
			if (bl.recordsCounter > max) {
				max = bl.recordsCounter;
			}
			if ( bl.recordsCounter == MAX_RECORDS) {
				of_blocks++;
				if ( !flag ) {
					of_bucks++;flag = 1;
				}
			}
			mean_recs += bl.recordsCounter;
			mean_blocks ++;
			if (bl.next <= 0) {
				BF_WriteBlock(ht->fileDesc,block_num); 
				break;
			}
			BF_WriteBlock(ht->fileDesc,block_num); 
			block_num = bl.next;
		}
		printf("Bucket %d : min_records = %d , max_records = %d , mean_records = %d , overflow_blocks = %d \n",i,min,max,mean_recs / block_sum,of_blocks);
		flag = 0;
		min = MAX_RECORDS + 1;
		max = -1;
		mean_recs = 0;
		of_blocks = 0;
		block_sum = 0;
	}
	printf("Mean value of blocks in buckets is %d and overflow buckets are %d \n",mean_blocks / ht->numBuckets , of_bucks);
	printf("-------- sum of recs = %d\n",sumofrecs);
	BF_WriteBlock(ht->fileDesc,0);
	HT_CloseIndex(ht);
	return 0;
}

int HashStatistics(char *filename)
{
	HT_info *ht;
	SHT_info *sht;
	if ((ht = HT_OpenIndex(filename)) == NULL) {
		if ((sht = SHT_OpenSecondaryIndex(filename)) == NULL){
			return -1;
		}
		else{
			SHT_Stats(sht);
		}
	}	
	else if((sht = SHT_OpenSecondaryIndex(filename)) == NULL) {
		HT_Stats(ht);
		
	}
	else {
		printf("Error\n"); BF_PrintError("cannot open file");
		return -1;
	}
	return 0;
}
