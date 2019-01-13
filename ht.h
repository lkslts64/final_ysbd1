#ifndef HT_H
#define HT_H



void print_HT(HT_info ht);

int HT_CreateIndex( char *fileName, /* όνομα αρχείου */
					char attrType, /* τύπος πεδίου-κλειδιού: 'c', 'i' */
					char* attrName, /* όνομα πεδίου-κλειδιού */
					int attrLength, /* μήκος πεδίου-κλειδιού */
					int buckets /* αριθμός κάδων κατακερματισμού*/);

HT_info* HT_OpenIndex( char *fileName /* όνομα αρχείου */ );

int HT_CloseIndex( HT_info* header_info );

int HT_InsertEntry( HT_info header_info, /* επικεφαλίδα του αρχείου*/
					Record record /* δομή που προσδιορίζει την εγγραφή */ );

int HT_DeleteEntry( HT_info header_info, /* επικεφαλίδα του αρχείου*/
					void *value /* τιμή του πεδίου-κλειδιού προς διαγραφή */);
					
int HT_GetAllEntries( 	HT_info header_info, /* επικεφαλίδα του αρχείου */
						void *value /* τιμή του πεδίου-κλειδιού προς αναζήτηση */);

#endif
