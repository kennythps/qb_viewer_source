#include "main.h"

int CreateTableFile(char *filename)
{
	char tablefile[128];
	FILE *file = fopen(filename, "rb");
	if(!file) {
		printf("Couldn't open file!");
		goto eerr;
	}

	/*fread(head, 1, 3, file);
	if(memcmp(head, file_header, 3)) {
		printf("Not a valid qb-file!");
		goto eerr;
	}*/

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 3, SEEK_SET);
	qb = (unsigned char*)malloc(size);
	if(!qb) goto eerr;
	fread(qb, 1, size, file);
	fclose(file);

	pos = 0;
    
	dstart = 0;
	while(pos < size && !found) {
//		dt_start[0] = qb[pos];
		dt_start[1] = qb[pos+1];
		if(!memcmp(&qb[pos], dt_start, 4) && dt_start[1] != 0x16) {
			found = true;
			dstart = pos-1;
		}
		pos++;
	}
	if(!dstart) {
		printf("#/ Warning: This file might have a crappy table\n");
		dt_start[0] = 0x01;
		dt_start[1] = 0x2b;
		pos = 0;
		while(pos < size && !found) {
			if(!memcmp(&qb[pos], dt_start, 2)) {
				found = true;
				dstart = pos-1;
			}
			pos++;
		}
	}

	//pos += 2;
	pos = dstart + 2;
	sprintf(tablefile, "%s.tab", filename);
	file = fopen(tablefile, "wb");
	if(!file) {
		printf("Couldn't create table file!\n");
		goto eerr;
	}

	fwrite(&qb[pos], 1, size-pos, file);
	fclose(file);

	return 0;
eerr:
	if(qb) free(qb);
	if(file) fclose(file);
	return 1;
}
