#include "main.h"

// This is the standard fileheader
char file_header[3] = {0x01, 0x23, 0x16};
// This is the start of the descriptiontable (it includes the first +)
char dt_start[4] = {0x01, 0x24, 0x01, 0x2b};
char dt_next[2] = {0x00, 0x2b}; // this indicates a new word

char head[3];	// store the read head here
unsigned int size = 0;	// store the filesize here
unsigned char *qb = NULL;	// data
unsigned int pos = 0;		// parsing position
int dstart = 0;		// start of our table
bool found = false;	// used when looking for ID table

vector<unsigned char*> dt_table;	// text-table
vector<unsigned int> dt_id;			// id-table
char *TextOf(unsigned int id);		// get the text of an ID
//int i = 0;

// Get the next INT (or ID)
#define GetID() {ip = (unsigned int*)&qb[pos]; pos+=4;}
#define GetINT GetID
char undef_id[128] = "<UID 0x%x>";	// used to indicate an undefined ID 

unsigned int *ip = NULL;		// integer pointer

int CFLAGS = 0;
int DFLAGS = 0;
bool no_switch_offsets = false;

unsigned long GenerateCRC(char * buf, unsigned long size);
unsigned long GenerateCRC2(char * buf, unsigned long size);

int main(int argc, char **argv)
{
	int sl = 0;
	if(argc < 3) goto usage;
	if(argv[1][0] != '-') goto usage;
	switch(argv[1][1]) {
	case 'c':
		//if(argc < 4) goto usage;
		if(argc > 3) {
			if(strcmp(argv[3], "-thug1") == 0) {
				no_switch_offsets = true;
			}
		}
		return Compile(argv[2]);
		break;
	case 'd':
		if(argc > 3) {
			if(strcmp(argv[3], "-debug") == 0) {
				qb_debuginfo = true;
			}
			if(strcmp(argv[3], "-thug1") == 0) {
				no_switch_offsets = true;
			}
		}
		return Decompile(argv[2]);
		break;
	case 't':
		printf("THIS FUNCTION IS DEPRECATED...\n");
		return CreateTableFile(argv[2]);
		printf("THIS FUNCTION IS DEPRECATED...\n");
	case 'p':
		return EditPRD(argc - 3, &argv[2]);
		break;
	default: goto usage;
	}

	return 0;
usage:
	printf("Usage: %s -[c,d,t] filename\nc = compile\nd = decompile\nt = create table file\n", argv[0]);
	return 1;
}

char *TextOf(unsigned int id)
{
	int i = 0;
	for(; i < (int)dt_id.size(); i++) {
		if(dt_id[i] == id) return (char*)dt_table[i];
	}
	sprintf(undef_id, "<UID: 0x%x>", id);
	return undef_id;
}
    