
#ifndef PRINT_H
#define PRINT_H
void print_SHT(SHT_info);
void print_HT(HT_info ht);
void print_Record(Record *rec) ;
void print_Block(Block *bl);
int HashStatistics(char *filename);
int SHT_Stats(SHT_info *sht);
int HT_Stats(HT_info *ht);

void print_SHT_Record(SecondaryRecord *srec);
void print_SHT_Block(SecondaryBlock *sbl);


#endif
