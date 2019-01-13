#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "ht.h"
#include "print.h"
#include "BF.h"
#include "sht.h"


int main(void)
{
	HT_info *ht;
	Record rec,record;
	
	
	BF_Init();

	HT_CreateIndex("HTfile",'i',"id",2,20);
	
	ht = HT_OpenIndex("HTfile");
	
	printf("\n--------INSERTING NOW-------\n");
	for (int i = 0; i < 100; i++)
	{
		rec.id=i;
		sprintf(rec.name,"kostis%d",i);
		sprintf(rec.surname,"lukas%d",i);
		sprintf(rec.address,"di%d",i);
		
		HT_InsertEntry(*ht,rec);
	}
	
	for (int i = 0; i < 100; i++)
		HT_GetAllEntries(*ht,(void *)&i);
	
	//delete 1 every 10 records
	for (int i = 0; i < 100; i+=10)
	{
		if ( HT_DeleteEntry(*ht,(void *)&i) < 0) {
			return -1;
		}
	}
	
	/////SECONDARY 
	SecondaryRecord srec;
	
	/////SECONDARY 1
	SHT_CreateSecondaryIndex("SHTfile","surname",strlen("surname"),20,"HTfile");
	SHT_info *sht = SHT_OpenSecondaryIndex("SHTfile");
	
	/////SECONDARY 2
	SHT_CreateSecondaryIndex("SHTfile2","address",strlen("address"),20,"HTfile");
	SHT_info *sht2 = SHT_OpenSecondaryIndex("SHTfile2");

	printf("\n--------SHT INSERTING NOW-------\n");
	for (int i = 100; i < 300; i++)
	{
		//insert ht
		record.id=i;
		sprintf(record.name,"kostis%d",i);
		sprintf(record.surname,"lukas%d",i);
		sprintf(record.address,"di%d",i);
		
		int blocknum = HT_InsertEntry(*ht,record);
		
		//insert sht
		srec.record.id=i;
		sprintf(srec.record.name,"kostis%d",i);
		sprintf(srec.record.surname,"lukas%d",i);
		sprintf(srec.record.address,"di%d",i);
		srec.block = blocknum;
		
		if ( SHT_SecondaryInsertEntry(*sht,srec) < 0 ){
			return -1;
		}
		
		//insert sht2
		if ( SHT_SecondaryInsertEntry(*sht2,srec) < 0 ){
			return -1;
		}
		
	}
	
	char  value[512];
	for (int i = 0; i < 300; i++)
	{
		sprintf(value,"lukas%d",i);
		SHT_SecondaryGetAllEntries(*sht,*ht,(void *)value);
		
		
		sprintf(value,"di%d",i);
		SHT_SecondaryGetAllEntries(*sht2,*ht,(void *)value);
		
	}
	
	

	
	HT_CloseIndex(ht);
	SHT_CloseSecondaryIndex(sht2);
	SHT_CloseSecondaryIndex(sht);
	
	if ( HashStatistics("HTfile") < 0) { 
		printf("Statistics Error");return -1;
	}
	
	if ( HashStatistics("SHTfile") < 0) { 
		printf("Statistics Error");return -1;
	}
	if ( HashStatistics("SHTfile2") < 0) { 
		printf("Statistics Error");return -1;
	}
	
	
	
	printf("\n--------THE END--------\n");
	
	return 0;
}
