#ifndef SHT_H
#define SHT_H

SHT_info* SHT_OpenSecondaryIndex ( char *sfileName );
int SHT_SecondaryGetAllEntries ( SHT_info header_info_sht,HT_info header_info_ht,void *value);
Record *FindInHT(int ht_block_num,int id,HT_info header_info_ht);

int SHT_CreateSecondaryIndex( char *sfileName, char* attrName, int attrLength, int buckets , char *fileName);
int SHT_CloseSecondaryIndex( SHT_info* header_info );
int SHT_SecondaryInsertEntry( SHT_info header_info, SecondaryRecord record);
int getBlockId( SHT_info header_info ,int id );

/////////
int InsertV2(int sFileDescriptor , char *attrName  , int metadata_size, int numBuckets  , SecondaryRecord record );//for prefetching in createSecondary

#endif
