#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>

#pragma warning(disable : 4996)

// well... i don't use M$ compilers so it won't affect
// the speed that much
using namespace std;	// yeayea...

extern int Compile(char *filename);
extern int Decompile(char *filename);

// This is the standard fileheader
extern char file_header[3];
// This is the start of the descriptiontable (it includes the first +)
extern char dt_start[4];
extern char dt_next[2]; // this indicates a new word

extern char head[3];		// store the read head here
extern unsigned int size;			// store the filesize here
extern unsigned char *qb;	// data CC: pointer to current data
extern unsigned char *cqb;	// CC: actual QB data
extern unsigned char *iqb;	// CC: includefile qb data

extern unsigned int pos;				// parsing position
extern int dstart;			// start of our table
extern bool found;			// used when looking for ID table

extern vector<unsigned char*> dt_table;	// text-table
extern vector<unsigned int> dt_id;			// id-table
extern char *TextOf(unsigned int id);		// get the text of an ID
extern unsigned int IdOf(char *name);
//extern int i;

// Get the next INT (or ID)
#define GetID() {ip = (unsigned int*)&qb[pos]; pos+=4;}
#define GetINT GetID
extern char undef_id[128];	// used to indicate an undefined ID 

extern unsigned int *ip;		// integer pointer

extern int Compile(char *filename, char *tabfile);
extern int Decompile(char *filename);
extern int CreateTableFile(char *filename);
extern int EditPRD(int argc, char **argv);

extern int CFLAGS;
extern int DFLAGS;

extern bool qb_debuginfo;

#define CF_WALL		(1)

#define DF_ALL			(0)
#define DF_WITH_REDUNDANT	(1)

namespace conv {
	template<typename T>
	std::string toString(T data)
	{
		std::ostringstream ss;
		ss << data;
		return ss.str();
	}
}

typedef struct {
		unsigned int off;	// debug

		unsigned char inst;
		unsigned char opcode[64];
		unsigned int len;

		int tabs;			// add to global tabs
		bool nl;			// new line
		bool pnl;			// in a new line: pre-newline
		int pnltabs;

		unsigned int name;

		string pre;
		string text;	// text
		string post;
} QB_INSTR;

typedef struct {
	unsigned int id;
	string name;
} QB_NAME;

extern bool no_switch_offsets;
      