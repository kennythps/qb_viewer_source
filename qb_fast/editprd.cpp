#include "main.h"

/*
 * ... yea the code here is a lil bit... crappy?
 * everything in one big file ... and more or less in one big function
 * lol it wasn't planned that's the reason =)
 * and i don't want the thug2 community guys to understand this code easily...
 * they'd do so much shit... cheating.... whatever....
 */

extern unsigned long wgetlong(char *t, char**, int base);
char *NextEntry();
void CreateDirs(char *filename);

typedef struct
{
	unsigned int filesize;
	unsigned int version;
	unsigned int qbfiles;
} PRD_HEADER;	// ... PRE ... but ... yea ... lol

typedef struct
{
	unsigned int filesize;
	unsigned int compressed;
	unsigned int namelength;
	unsigned int magic;
	char		 name[128];
	unsigned char		 *data;
} QB_ENTRY;

#define PRE_THPS 0xABCD0002		// unused atm... :P
#define PRE_THUG 0xABCD0003

PRD_HEADER prdhead;
QB_ENTRY qbentry;
vector<QB_ENTRY> qbfiles;
char *prdname = NULL;
char *qbname = NULL;
char *pdata = NULL;
//char *qdata = NULL;
char *pnext = NULL;

void GetOffset();
int neededoffset = 0;
unsigned long GenerateCRC(char *buf, unsigned long size);
void PRE_AddEntry()
{
	char *pmini = pdata;
	char *psize = pdata;
	pnext = pdata;
	//printf("Old:\n");
	//printf("Size: %08x\nFiles: %i\n", prdhead.filesize, prdhead.qbfiles);

	// Save our registers:
	//printf("Getting Offset... ");
	if(prdhead.qbfiles)	GetOffset();	// If we have files, get the offset we need
	//printf("%08x\n", pnext - pdata);
	//printf("Copying head and name\n");
	psize = pnext;						// This is where the filesize goes, we must keep that in mind
	memcpy(pnext, &qbentry.filesize, 4);	pnext += 4;	// Put in our entry...
	memcpy(pnext, &qbentry.compressed, 4);	pnext += 4;
	memcpy(pnext, &qbentry.namelength, 4);	pnext += 4;
	qbentry.magic = GenerateCRC(qbentry.name, strlen(qbentry.name));
	memcpy(pnext, &qbentry.magic, 4);		pnext += 4;
	
	for(char *name = qbentry.name; *name; ++name,++pnext) pnext[0] = *name;
	pnext[0] = 0; pnext++;

	int esize = (qbentry.compressed) ? qbentry.compressed : qbentry.filesize;

	//printf("Copying data at %08x to %08x...\n", pnext - pdata, pnext-pdata + esize);
	memcpy(pnext, qbentry.data, esize);
	pnext += esize;

	pmini = pnext;		// pmini is the minimum where the next entry must be!
	prdhead.qbfiles++;	// added a file
	//printf("Fixing Offsets...\n");
	GetOffset();
//	printf("\t\tFileSize: %08x, %08x\n", esize, *((int*)psize));
	if(neededoffset) {
		pnext += neededoffset;
		printf("<------------------------- !!!!!!!!!!!!!!!!!\n");
	} else {
		while(pnext < pmini) {
			if(qbentry.compressed) {
				printf("ERROR: Wrong offset found for a compressed file!\n");
				exit(10);
			}
			qbentry.filesize++;
			memcpy(psize, &qbentry.filesize, 4);
			GetOffset();
		}
	}

	//printf("Size:: %08x - N: %08x - D: %08x\n", pnext-pdata, pnext, pdata);

	//prdhead.filesize += qbentry.filesize + 0x10 + qbentry.namelength;
	prdhead.filesize = 0x0C + pnext - pdata;
	//printf("Done... Writing %08x bytes...\n", prdhead.filesize);
	FILE *fn = fopen(prdname, "wb");
	fwrite(&prdhead, sizeof(PRD_HEADER), 1, fn);
	fwrite(pdata, 1, pnext-pdata, fn);
	fclose(fn);
	printf("Done...\n");
}

void CompressQBEntry()
{
	QB_ENTRY qbc;
	memcpy(&qbc, &qbentry, sizeof(QB_ENTRY));	// Copy entry

	// Prepare memory
	qbc.data = (unsigned char*)malloc(qbc.filesize);
	memset(qbc.data, 0, qbc.filesize);

	char key = 0x07;

	//unsigned int up = 0;
	qbc.compressed = 0;
#define up qbc.compressed
	unsigned int pos = 0;
	unsigned char *data = qbentry.data;
	qbc.data[up++] = 0xFF;
	memcpy(&qbc.data[up], data, 8);
	//printf("File:\n");
	//for(int i = 0; i < 8; i++) printf("%c", data[i]);
	pos += 8; up += 8;
	// 8 uncompressed bytes there for start
	// up = compressed data position
	// pos = data position

	while(pos < qbentry.filesize)
	{
		// Add 8 byte set:
		unsigned char key = 0xFF;
		char set[16];
		unsigned int sp = 0;	// set pointer
		unsigned int sb = 0;	// instructions
		for(sb = 0; sb < 8 && pos < qbentry.filesize; sb++)
		{
			bool found = false;
			//char str[18];
			unsigned int start = 0;
			int cpylen = 18;
			// finddatapos = pos
			// look for 18 or less (min 3)
			// Search for an entry
			if(pos + cpylen > qbentry.filesize) cpylen = qbentry.filesize - pos;
			if(cpylen > 18) cpylen = 0;
			//printf("Look length: %i\n", cpylen);
			start = pos - 4095;
			if((int)start < 0) start = 0;
			cpylen = 3;
			unsigned int lastlen = cpylen;
			unsigned int cstart = start;
			while(start < (pos - cpylen+1)) {
				if(!memcmp(&data[pos], &data[start], cpylen)) {		// Found some...
					cstart = start;
					//while(!memcmp(&data[pos], &data[start], cpylen)) cpylen++;
					while(data[pos+cpylen] == data[start+cpylen] && cpylen < 18) cpylen++;
					found = true;
					if(cpylen >= 18) break;
				}
				start++;
				if(cpylen >= 18) break;
			}
			//cpylen--; // ...
			start = cstart;

			if(found) {
				// start	= start of where we copy
				// cpylen	= length
				// infogroup: xy zn
				// cpypos = uzxy, we just need to save zxy
				char n = cpylen-3;	// this is the N 4-bit-group
				char b1, b2;
				short z = (((short)start) & 0x0FFF) << 4;
				z = (z - 0x120) >> 4;	// 0zxy
				b1 = (char)(z & 0x00FF);
				z >>= 8;
				b2 = ((char)(z & 0x000F) << 4) + n;
				set[sp++] = b1;
				set[sp++] = b2;
				key ^= 1<<sb;	// Unset the key-bit for us
				pos += cpylen;
			} else {
				//printf("%c", data[pos]);
				set[sp++] = data[pos++];	// not found so just copy
			}
		}
		qbc.data[up++] = key;
		memcpy(&qbc.data[up], set, sp);
		up += sp;
	}
	float ratio = 1.0f - (((float)qbc.compressed) / ((float)qbc.filesize));
	ratio *= 100.0f;

	printf("Size: %i - Compressed: %i - Rate: %f%%\n", qbentry.filesize, qbc.compressed, ratio);
	qbentry.compressed = qbc.compressed;
	free(qbentry.data);
	qbentry.data = qbc.data;
}

int EditPRD(int argc, char **argv)
{
	memset(&prdhead, 0, sizeof(PRD_HEADER));
	if(!strcmp(argv[0], "new") && argc == 1) {
		FILE *fn = fopen(argv[1], "wb");
		if(!fn) { printf("Couldn't open file '%s'!\n", argv[1]); exit(2); }
		prdhead.filesize = 12;
		prdhead.version = PRE_THUG;
		prdhead.qbfiles = 0;
		fwrite(&prdhead, sizeof(PRD_HEADER), 1, fn);
		fclose(fn);
	} else if((!strcmp(argv[0], "add") || !strcmp(argv[0], "addx")) && argc == 3)	{ // PRD FILE MAGIC
		prdname = argv[1];
		qbname = argv[2];
		qbentry.magic = wgetlong(argv[3], NULL, 16);

		FILE *fn = fopen(qbname, "rb");
		if(!fn) { printf("Couldn't open QB file '%s' for reading!\n", qbname); exit(2); }
		fseek(fn, 0, SEEK_END);
		qbentry.filesize = ftell(fn);
		fseek(fn, 0, SEEK_SET);
		qbentry.data = (unsigned char*)malloc(qbentry.filesize);
		fread(qbentry.data, 1, qbentry.filesize, fn);
		fclose(fn);
		qbentry.compressed = 0;
		qbentry.namelength = strlen(qbname)+1;
		strcpy(qbentry.name, qbname);
		/*printf("QB Entry:\n");
		printf("%s: %x\n%i - %i - %08x\n", qbentry.name, qbentry.namelength, qbentry.filesize, qbentry.compressed, qbentry.magic);*/
		char *swap = (char*)&qbentry.magic;
		swap[0] ^= swap[3] ^= swap[0] ^= swap[3];
		swap[1] ^= swap[2] ^= swap[1] ^= swap[2];
		// <-- Finished loading the qbentry
		printf("Adding '%s'\n", qbentry.name);

		if(!strcmp(argv[0], "add")) CompressQBEntry();

		fn = fopen(prdname, "rb");
		if(!fn) { printf("Couldn't open file '%s' for reading!\n", prdname); exit(2); }
		fread(&prdhead, sizeof(PRD_HEADER), 1, fn);
		fseek(fn, 0, SEEK_END);
		if(prdhead.filesize != ftell(fn)) { printf("Oh, a filesize mistake...\n"); prdhead.filesize = ftell(fn); }
		fseek(fn, sizeof(PRD_HEADER), SEEK_SET);
		pdata = (char*)malloc(prdhead.filesize + qbentry.filesize + 128);	// allocate the needed size + bytes to move
		//printf("Allocated data: %08x\n", prdhead.filesize + qbentry.filesize + 128);
		memset(pdata, 0, prdhead.filesize + qbentry.filesize + 128); // NULL now so we have less work later
		fread(pdata, 1, prdhead.filesize - sizeof(PRD_HEADER), fn);	// read data without the header
		fclose(fn);

		// <-- prdhead	= header
		// <-- pdata	= data
		// <-- qbentry	= entry

		//printf("Adding entry...\n");
		PRE_AddEntry();

		return 0;
	} else if(!strcmp(argv[0], "x") && argc == 1) { // FILE
		printf("Loading '%s'... ", argv[1]);
		FILE *fn = fopen(argv[1], "rb");
		fseek(fn, 0, SEEK_END);
			int length = ftell(fn);
			printf("%i bytes read - ", length);
			pdata = (char*)malloc(length);
		fseek(fn, 0, SEEK_SET);
		fread(pdata, 1, length, fn);
		fclose(fn);
		memcpy(&prdhead, pdata, sizeof(PRD_HEADER));
		pdata += sizeof(PRD_HEADER);

		printf("%i bytes wanted\n", prdhead.filesize);
		if(prdhead.filesize != length) {
			printf("Filesizes don't match, this isn't an originally-QBE-compressed PRE file!\n");
			exit(1);
		}

		if(prdhead.version != PRE_THUG) {
			printf("This is THUG PRE File - QBE can only decompress QBE-Compressed THUG PREs atm!\n");
			exit(1);
		}

		/* M'Kay:
		 * prdhead is read and
		 * pdata = first entry data
		 * FileSize - Compressed - NameLength - ID - name/0 - data**
		 */
		for(unsigned int i = 0; i < prdhead.qbfiles; i++)
		{
			char *next = NextEntry();	// Save the next entry address :)
			int filesize = 0;
			int csize = 0;
			int length = 0;
			int xtracted = 0;
			memcpy(&filesize, pdata, 4); pdata += 4;
			memcpy(&csize, pdata, 4); pdata += 4;
			memcpy(&length, pdata, 4); pdata += 4;
			pdata += 4;
			//FILE *fex = fopen(pdata, "wb");
			char *filename = pdata;
			printf("Extracting '%s'...\n", filename, filesize);
			while(*pdata) pdata++;
			pdata++; // Skip the NULL terminator
			int ucs = 0;	// uncompressed-index
			std::vector<char> ucomp; // I hate those...

			while(ucs < filesize) {
				char bKey = *pdata; pdata++;
				char key[8] = {
					bKey & 1, (bKey >> 1) & 1, (bKey >> 2) & 1, (bKey >> 3) & 1,
					(bKey >> 4) & 1, (bKey >> 5) & 1, (bKey >> 6) & 1, (bKey >> 7) & 1
				};
				for(int i = 0; i < 8; i++)
				{
					if(key[i]) {
						ucomp.push_back(*pdata);
						pdata++; ucs++;
					} else {
						int ch1 = ((int)pdata[1]) & 0x000000FF;	// Byte 2
						int ch0 = ((int)pdata[0]) & 0x000000FF; // Byte 1
						int nToCpy = (ch1 & 0x0F) + 3;
						unsigned int off = 0x12 + ((ucs>>0xC)<<0xC) + ((ch1&0xF0)<<4) + ch0;
						while(off >= ucomp.size()) off -= 0x1000;
						if(off >= ucomp.size()) {
								printf("\nERROR: UCS: 0x%08x - OFF: 0x%08x\n", ucomp.size(), off);
								exit(1);
							}
						while(nToCpy > 0) {
							//ucomp[ucs] = *src;
							if(off >= ucomp.size()) {
								printf("\nUCS: 0x%08x - OFF: 0x%08x\n", ucomp.size(), off);
								exit(1);
							}
							ucomp.push_back(ucomp[off]);
							ucs++; off++;
							nToCpy--;
						}
						pdata += 2;
					}
				}
			}
			//printf("0x%0x extracted\n", ucomp.size());
			char *dirs = (char*)malloc(strlen(filename));
			strcpy(dirs, filename);
			CreateDirs(dirs);
			FILE *fex = fopen(filename, "wb");
			fwrite(&ucomp[0], 1, filesize, fex);
			fclose(fex);
			ucomp.clear();
			pdata = next;
		}
	} else if(!strcmp(argv[0], "jdc") && argc == 1)	{ // FILE
		// ... this is just a test... nobody needs to be able to read that lol
		printf("Unpacking '%s'...\n", argv[1]);
		FILE *fn = fopen(argv[1], "rb");
		fseek(fn, 0, SEEK_END);
		 int length = ftell(fn);
		 printf("Allocating %i bytes...\n", length);
		 pdata = (char*)malloc(length);
		fseek(fn, 0, SEEK_SET);
		printf("Reading...\n");
		 fread(pdata, 1, length, fn);
		fclose(fn);

		char *ptr = pdata;
		char *pend = pdata + length;
		printf("Allocating...\n");
		char *ucomp = (char*)malloc(length*16);
		int ucs = 0;

		printf("Decompressing...\n");
		while(ptr < pend) {
			char bKey = *ptr; ptr++;
			char key[8] = {
				bKey & 1, (bKey >> 1) & 1, (bKey >> 2) & 1, (bKey >> 3) & 1,
				(bKey >> 4) & 1, (bKey >> 5) & 1, (bKey >> 6) & 1, (bKey >> 7) & 1
			};
			for(int i = 0; i < 8; i++)
			{// Working decompression routine

// Behind all this shit:
// nToCpy = last 4 bits +3 = (n2 & 0F) + 3
// start: ucs = xyza >>c <<c = x000
// n1n2 = xyzN
// v2 = zN w/o N << 8 = z00
// +ch0 = zxy ++ 0x12
// so format = XYZN -- Uxyz = ucs
// start = UZXY, num = N+3

				if(key[i]) {
					ucomp[ucs] = *ptr;
					ptr++; ucs++;
				} else {
					int ch1 = ((int)ptr[1]) & 0x000000FF;
					int ch0 = ((int)*ptr) & 0x000000FF;
					int nToCpy = (ch1 & 0x0F) + 3;
					int off = 0x12 + ((ucs>>0xC)<<0xC) + ((ch1&0xF0)<<4) + ch0;
					if(off > ucs) off -= 0x1000;
					/*int v1 = ((ucs>>0x0C)<<0x0C);
					int v2 = ((ch1>>4)<<8) + ch0;
					int off = v1 + v2 + 0x12;
					if(off > ucs) v1 -= 0x1000;
					char *src = (char*)(ucomp + (v1 + v2 + 0x12));*/
					char *src = (char*)(ucomp + off);
					while(nToCpy > 0) {
						ucomp[ucs] = *src;
						//printf("%c", *src);
						ucs++;
						src++;
						nToCpy--;
					}
					ptr += 2;
				}
			}
		}
		fn = fopen("uncomp.bin", "wb");
		fwrite(ucomp, 1, ucs, fn);
		fclose(fn);
		free(pdata);
		free(ucomp);
		return 0;
	} else if(!strcmp(argv[0], "list") && argc == 1)	{ // FILE
		FILE *fn = fopen(argv[1], "rb");
		fread(&prdhead, 1, sizeof(PRD_HEADER), fn);
		fseek(fn, 0, SEEK_END);
		 int flen = ftell(fn);
		 pdata = (char*)malloc(flen);
		fseek(fn, 0, SEEK_SET);
		 fread(pdata, 1, flen, fn);
		fclose(fn);
		char *ptr = pdata;
		//unsigned int offset1 = 0x1C;
		unsigned int counter = prdhead.qbfiles;
		unsigned int length = 0;
		unsigned int fsize = 0;
		unsigned int csize = 0;
		unsigned int magic = 0;
		char *swap = (char*)&magic;
		//unsigned int offset2 = 0;
		ptr += sizeof(PRD_HEADER);

		while(counter) {
			fsize = csize = 0;
			memcpy(&length, ptr+8, 4);	// +12 = magic, +16 = name
			memcpy(&magic, ptr+12, 4);
			swap[0] ^= swap[3] ^= swap[0] ^= swap[3];
			swap[1] ^= swap[2] ^= swap[1] ^= swap[2];
			printf("%s 0x%08x\n", ptr+16, magic);
			memcpy(&fsize, ptr, 4);
			memcpy(&csize, ptr+4, 4);
			if(csize) fsize = csize;
			fsize = ((fsize+3) & 0xFFFFFFFC) + length;
			ptr += fsize + 0x10;

			counter--;
		}
	} else {
		printf("Usage:\n");
		printf("\tnew filename.prd\n");
		printf("\tadd filename.prd filename.qb magic\n\n");
		printf("magic = magic value of the QB, use HEX numbers!\n");
		printf("Example:\n\t");
		printf("-p add qb_scripts.prd scripts\\game\\menu\\gamemenu_pause.qb .\\gamemenu_pause.qb 0x3f142ab9\n");
		return 1;
	}
	return 0;
}
/*
well they use arithmetic shift right and ... normal shift left...
but this is of sporadic's prerip, hmm does a lot of shit :P lol
yea it's hard to see sense in asm code i see it cuz i do lots of asm coding
even wrote my own scriptlanguage, (which isn't interpreted but translated to machine code...)
hehe that's cool
okay some old stuff lol:
					__asm {
						mov eax, v1
						sar eax, 0x0C
						shl eax, 0x0C
						mov v1, eax
					}
					int v1 = (*ptr) + ((ptr[1]>>4)<<8);
					int v2 = ((ucs>>0xC)<<0xC);
					int off = v1 + v2 + 0x12;
					if(off > ucs) v1 -= 0x1000;
					char *src = (char*)(ucomp + v1 + v2 + 0x12);
*/


/*
the ... kinda first version:
but it does lots of shit, i mean, oh and it's from thug2 not spos prerip...:
prerip does wrong offset calculation so if i just align with the games alignment routine...
prerip can't rip it and that's GOOD so nobody can steal the files ;)
here the old thin... u see, the offset vars are somehow useless
		length = (unsigned int) *((short *)(ptr[8]));
		fsize = *((unsigned int*)(ptr));
		offset2 = (fsize + 3) & 0xFFFFFFFC;
		offset1 = ((offset1 + length + 0x1f) & 0xFFFFFFF0) + offset2;
		offset2 = *((unsigned int*)(ptr + 4));
		if(offset2) fsize = offset2;
		fsize = ((fsize + 3) & 0xFFFFFFFC) + length;
		ptr += fsize + 0x10;
*/

char *NextEntry()
{
	char *ptr = pdata;
	unsigned int length = 0;
	unsigned int fsize = 0;
	unsigned int csize = 0;
	memcpy(&length, ptr+8, 4);
	memcpy(&fsize, ptr, 4);
	memcpy(&csize, ptr+4, 4);
	if(csize) fsize = csize;
	fsize = ((fsize+3) & 0xFFFFFFFC) + length;
	ptr += fsize + 0x10;
	return ptr;
}

void CreateDirs(char *filename)
{
	char cmd[512];
	char dir[1024];
	char *tok = strtok(filename, "\\/");
	/*printf("'%s'\n", tok);
	tok = strtok(NULL, "\\/");
	printf("'%s'\n", tok);
	tok = strtok(NULL, "\\/");
	printf("%i\n", tok);
	exit(1);*/
	memset(dir, 0, 1024);
	system("echo Start > .__output.txt");
	while(tok != NULL)
	{
		strcat(dir, tok);
		strcat(dir, "\\");
		sprintf(cmd, "mkdir \"%s\" 2> .__output.txt", dir);
		system(cmd);
		sprintf(cmd, "rmdir \"%s\" 2> .__output.txt", dir);
		tok = strtok(NULL, "\\/");
		if(tok == NULL) system(cmd);
	}
	system("del .__output.txt");
}

void GetOffset()
{
	char *ptr = pdata;
	//unsigned int offset1 = 0x1C;
	unsigned int counter = prdhead.qbfiles;
	unsigned int length = 0;
	unsigned int fsize = 0;
	unsigned int csize = 0;
	//unsigned int offset2 = 0;

	while(counter) {
		fsize = csize = 0;
		memcpy(&length, ptr+8, 4);
		memcpy(&fsize, ptr, 4);
		memcpy(&csize, ptr+4, 4);
		if(csize) fsize = csize;
		fsize = ((fsize+3) & 0xFFFFFFFC) + length;
		ptr += fsize + 0x10;

		counter--;
	}
	pnext = ptr;
	/*
	and this?
	yea this...
	it's the game's routine in plain asm, works as well
	but why shouldn't i "compress" it to understandable c :P
	and it does so much shit lol (and i don't mean the pushes and pops lol)
	__asm {
		push eax
		push edi
		push ecx
		push ebp
		push esi
		push edx
	}
	__asm {		
		mov eax, pdata // Data
		//add eax, 0x0C
		mov edi, 0x1C
		mov ecx, prdhead.qbfiles
		mov ebp, ecx
nextentry:
		movzx EDX, WORD PTR DS:[EAX+8]		// Load Stringlengt
		mov ECX, DWORD PTR DS:[EAX]			// Load Filesize
		lea EDI, DWORD PTR DS:[EDX+EDI+0x1F]	// Data offset (next round it's also increased by the entry size ...)
		lea ESI, DWORD PTR DS:[ECX+3]		// FileSize + 3
		AND ESI, 0xFFFFFFFC
		AND EDI, 0xFFFFFFF0
		ADD EDI, ESI
		MOV ESI, DWORD PTR DS:[EAX+4]
		TEST ESI, ESI
		JE SHORT uncompressed
			MOV ECX, ESI	// compressed <- so set the csize to skip
uncompressed:
		ADD ECX, 3
		AND ECX, 0xFFFFFFFC
		ADD ECX, EDX		// Datalength+3|cut|+namelength (min 3 over for next aligned

		LEA EAX, DWORD PTR DS:[EAX+ECX+0x10]
		DEC EBP
		JNZ nextentry
		MOV pnext, EAX		// pnext is now where we expect the file to be
	}
	// Restore our registers
	__asm {
		pop edx
		pop esi
		pop ebp
		pop ecx
		pop edi
		pop eax
	}*/
}

/*
here i "summed up" the decompression routine...
the actual one works fine now and this can be summed as:
byte1 | byte2
 Y Z     X N
 N = num of bytes to copy
 and Offset & 0xF000 + x<<8 + byte1
 (UXYZ)
 is the start from where we copy
 remember, the code down there does exactly this.... in a strange way yea, but it does lol
char *esip = NULL;
					char cl = *(ptr+1);
					// ESI = ucs! then				// ref:/skiped asm part here we have our ucs var!
					// EAX = ucs! (as long) so EDX UNUSED!!!
					int off = ucs;	// <- EAX
					char bl = cl >> 4;
					int edx = (int)bl;	// FIRST EDX REF :abs
					int ebx = *ptr;		// FIRST EBX REF :abs
					__asm {
						mov eax, off
						sar eax, 0x0C
						shl eax, 0x0C
						mov off, eax
					}	// sAr and sHl difference ... dunno if there is any important here
					edx <<= 8;
					edx += ebx;
					ebx = edx + off + 0x12;
					if(ucs < ebx) off -= 0x1000;	// if ucs is smaller then our copypointer
					cl = (cl & 0x0F) + 3;
					int ecx = (int)cl;
					if(ecx)		// We have something here *hide* >.>
					{
						esip = &ucomp[off];
						ebx = ecx;
						while(ebx > 0)
						{
							ucomp[ucs] = *esip;
							printf("%c", *esip);
							ucs++;
							esip++;
							ebx--;
						}
						ptr += 2;
					}*/

unsigned long CRC_TABLE[256] = // CRC polynomial 0xedb88320
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};
 
unsigned long GenerateCRC(char * buf, unsigned long size)
{
    unsigned long rc = 0xFFFFFFFF;
    for(unsigned long i = 0; i < size; ++i)
    {
        unsigned char ch = buf[i];
        if(ch >= 'A' && ch <= 'Z') {
			ch += 'a' - 'A';
        } else if(ch=='/') {
            ch='\\';
        }
		rc = CRC_TABLE[(rc^ch)&0x0FF] ^ (rc >> 8);
	}
 
    return rc;
}