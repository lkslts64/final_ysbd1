#ifndef DEFS_H
#define DEFS_H

#define FILENAME_SIZE 30

#define NAMES_SIZE   20
#define ADDRESS_SIZE 40
#define MAX_RECORDS  6

typedef struct {
	int offset; //metadata size
	int fileDesc; /* αναγνωριστικός αριθμός ανοίγματος αρχείου από το επίπεδο block */
	char attrType; /* ο τύπος του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο, 'c' ή'i' */
	char *attrName; /* το όνομα του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
	int attrLength; /* το μέγεθος του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
	int numBuckets; /* το πλήθος των “κάδων” του αρχείου κατακερματισμού */
}HT_info;

typedef struct {
	int offset;				 // metadata size
	int fileDesc;			 /* αναγνωριστικός αριθμός ανοίγματος αρχείου από το επίπεδο block */
	int attrLength;          /* το μέγεθος του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
	char* attrName;          /* το όνομα του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
	int numBuckets;     /* το πλήθος των “κάδων” του αρχείου κατακερματισμού */
	char fileName[FILENAME_SIZE];			 /* όνομα αρχείου με το πρωτεύον ευρετήριο στο id */
} SHT_info;



typedef struct{
	int id;
	char name[NAMES_SIZE];
	char surname[NAMES_SIZE];
	char address[ADDRESS_SIZE];
}Record;

typedef struct{
	char recordsCounter;
	Record records[MAX_RECORDS];
	char maxRecords;
	int next;
}Block;

typedef struct {
	Record record;
	int block;
}SecondaryRecord;

typedef struct{
	char recordsCounter;
	SecondaryRecord records[MAX_RECORDS];
	char maxRecords;
	int next;
}SecondaryBlock;
#endif
