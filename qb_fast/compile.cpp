#include "main.h"
#include <iostream>
#include <fstream>
#include <sstream>

/* WHY THE FUCK DOES M$ VS ASK "Do you want to ____NORMALIZE!!!___ the endings to CRLF!!!??
 * THAT'S NOT NORMAL
 * THAT'S FUCKING GAY!
 */

// ATM: Switch at line 100..
// qb_view
// compare dump with qb file

/* REMEMBER TO __asm (bswap) THE UNKNOWN ID BYTES (if any ...)
 * otherwise you'd be totally screwed
 * THEY'RE WRITTEN BSWAPped TO MAKE THEM EASYER TO LOOK UP IN HEX EDITORS...
 * you don't needa swap them while searching then...
 * format would be: AaBbCcDd
 * and you'd have to look in hex for DdCcBbAa
 */
#define ISHEX ((qb[pos] >= 'a' && qb[pos] <= 'f') || \
		(qb[pos] >= 'A' && qb[pos] <= 'F') || \
		(qb[pos] >= '0' && qb[pos] <= '9'))

vector<unsigned char> data;
extern vector<QB_INSTR> inst;
extern vector<QB_NAME> names;
extern vector<unsigned int> labels;

char lastWord[512];
char lastString[2048];
void GetWord();
int GetNumber();
int GetHex();
void GetString();
void SkipWhite();
void CC();
unsigned int GetUID(char *name);
void Match(char c);
bool IsNum();
unsigned long wgetlong(char *t, char**, int base);

unsigned int line;

typedef struct {
	unsigned int pos;
	unsigned int target;
	int size;			// 2 or 4
} QB_OFFSET;

vector<QB_OFFSET> data_offsets;
vector<QB_OFFSET> data_positions;

bool nodump = false;
void DUMP() {
	if(!nodump) {
		FILE *file = fopen("temp.dump", "wb");
		fwrite(&data[0], 1, data.size(), file);
		fclose(file);
	}
}

void Prep() {
	if(qb[pos] == '/') {
		while(qb[pos] && qb[pos++] != '\n');
		line++;
		SkipWhite();
	} else {
		GetWord();
		if(strcmp(lastWord, "addx") == 0) {
			//int id = GetNumber();
			/*__asm {
				mov eax, id
				bswap eax
				mov id, eax
			}*/
			pos += 2; // 0x
			int id = wgetlong((char*)(qb+pos), NULL, 16);
			while(ISHEX) pos++;
			SkipWhite();
			QB_NAME name;
			name.id = id;
			GetString();
			name.name = lastString;
			names.push_back(name);
			//printf("%08x = '%s'\n", name.id, name.name.c_str());
		} else {
			std::cout << "Error at line " << line << std::endl;
			exit(1);
		}
	}
}

void Percent() {
	GetWord();
	if(strcmp(lastWord, "include") == 0) {
		GetString();
		int s = size;
		int p = pos;
		int l = line;
		printf("Including: '%s'\n...", lastString);
		unsigned char *old = qb;
		FILE *file = fopen(lastString, "rb");
		if(!file) {
			printf("Couldn't open file!");
			exit(1);
		}
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		qb = (unsigned char*)malloc(size);
		if(!qb) exit(2);
		fread(qb, 1, size, file);
		fclose(file);
		pos = 0;
		line = 1;
		CC();
		free(qb);
		qb = old;
		line = l;
		pos = p;
		size = s;
		printf("done.\n");
		SkipWhite();
	} else if(strcmp(lastWord, "GLOBAL") == 0) {
		data.push_back(0x2D);
		Match('%');
	} else if(strcmp(lastWord, "i") == 0) {
		//sprintf(tmp, "%%i(%u,%08x)", *iptmp, *iptmp);
		data.push_back(0x17);
		Match('(');
		int num = 0;
		unsigned char *p = (unsigned char*)&num;
		if(qb[pos] != ',') {
			num = GetNumber();
			Match(',');
			while(qb[pos] && qb[pos] != ')') pos++;
		} else {
			Match(',');
			num = GetHex();
		}
		Match(')');
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
	} else if(strcmp(lastWord, "f") == 0) {
		data.push_back(0x1A);
		Match('(');
		float a = 0.0f;
		unsigned char *p = (unsigned char*)&a;

		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		Match(')');
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
	} else if(strcmp(lastWord, "s") == 0 || strcmp(lastWord, "sc") == 0) {
		if(strcmp(lastWord, "sc") == 0) data.push_back(0x1C);
		else data.push_back(0x1B);
		Match('(');
		while(qb[pos] && qb[pos] != ',') pos++;
		Match(',');
		GetString();
		Match(')');
		int n, l = strlen(lastString)+1;
		unsigned char *p = (unsigned char*)&l;
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		for(n = 0; n < l; n++) {
			data.push_back((unsigned char)lastString[n]);
		}
	} else if(strcmp(lastWord, "vec2") == 0) {
		// 1f float float
		data.push_back(0x1F);
		Match('(');
		float a = 0.0f;
		unsigned char *p = (unsigned char*)&a;

		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		Match(',');
		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		Match(')');
	} else if(strcmp(lastWord, "vec3") == 0) {
		// 1f float float
		data.push_back(0x1E);
		Match('(');
		float a = 0.0f;
		unsigned char *p = (unsigned char*)&a;

		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		Match(',');
		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		Match(',');
		a = (float)atof((char*)(qb+pos));
		if(qb[pos] == '-') pos++;
		while(IsNum()) pos++;
		if(qb[pos] != '.') {
			printf("Float needs a dot... add it you fool - line %i\n", line);
			exit(1);
		}
		pos++;
		while(IsNum()) pos++;
		SkipWhite();
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		Match(')');
	} else {
		printf("Unknown Percent command '%s' at line %i\n", lastWord, line);
		exit(1);
	}
}
int rIfs = 0;
int rSwitches = 0;
//int rWhiles = 0;

void Colon() {
	static QB_OFFSET doff;

	if(qb[pos+1] == '{') {
		if(qb[pos] == 's') data.push_back(0x03);
		else if(qb[pos] == 'a') data.push_back(0x05);
		else {
			printf("Unkown :*{ combo at line %i\n", line);
			exit(1);
		}
		pos += 2;
	} else if(qb[pos+1] == '}') {
		if(qb[pos] == 's') data.push_back(0x04);
		else if(qb[pos] == 'a') data.push_back(0x06);
		else {
			printf("Unkown :*} combo at line %i\n", line);
			exit(1);
		}
		pos += 2;
	} else if(qb[pos] == 'i') {
		pos++;
		data.push_back(0x01);
	} else if(qb[pos] == 'u') {
		pos++;
		//data.push_back(0x02);
		Match('(');
		unsigned int byte = wgetlong((char*)(qb+pos), NULL, 16);
		unsigned char c = (unsigned char)byte;
		data.push_back(c);
		while(ISHEX) pos++;
		Match(':');
		byte = wgetlong((char*)(qb+pos), NULL, 16);
		unsigned char *p = (unsigned char*)&byte;
		data.push_back(p[0]);
		data.push_back(p[1]);
		data.push_back(p[2]);
		data.push_back(p[3]);
		while(ISHEX) pos++;
		Match(')');
	} else if(strncmp((char*)&qb[pos], "OFFSET", 6) == 0) {
		pos += 6;
		SkipWhite();
		doff.pos = data.size();
		doff.size = 4;
		Match('(');
		doff.target = wgetlong((char*)(qb+pos), NULL, 0);
		while(IsNum()) pos++;
		Match(')');
		data_offsets.push_back(doff);
		data.push_back(0);
		data.push_back(0);
		data.push_back(0);
		data.push_back(0);
	} else if(strncmp((char*)&qb[pos], "POS", 3) == 0) {
		pos += 3;
		SkipWhite();
		Match('(');
		doff.target = wgetlong((char*)(qb+pos), NULL, 0);
		while(IsNum()) pos++;
		Match(')');
		doff.pos = data.size();
		doff.size = 0;
		data_positions.push_back(doff);
	} else if(strncmp((char*)&qb[pos], "BREAKTO", 7) == 0) {
		pos += 7;
		SkipWhite();
		data.push_back(0x2E);
		doff.pos = data.size();
		doff.size = 4;
		Match('(');
		doff.target = wgetlong((char*)(qb+pos), NULL, 0);
		while(IsNum()) pos++;
		Match(')');
		data_offsets.push_back(doff);
		data.push_back(0);
		data.push_back(0);
		data.push_back(0);
		data.push_back(0);
	} else if(strncmp((char*)&qb[pos], "end", 3) == 0) {
		pos = size;
		if(rIfs || rSwitches) {
			printf(":end within a switch(%i) or an if(%i) at line %i\n", rSwitches, rIfs, line);
			exit(1);
		}
	} else {
		printf("Unknown Colon command starting with '%c' at line %i\n", qb[pos], line);
		exit(1);
	}
	SkipWhite();
}
void DoIf2() {
	rIfs++;
	data.push_back(0x25);
	while(1) {
		CC();
		if(strncmp((char*)&qb[pos], "endif", 5) == 0) {
			pos += 5;
			SkipWhite();
			break;
		} else if(strncmp((char*)&qb[pos], "doElse", 6) == 0) {
			pos += 6;
			SkipWhite();

			data.push_back(0x26);
		}
	}
	data.push_back(0x28);
	rIfs--;
}
void DoIf() {
	rIfs++;
	data.push_back(0x47);
	unsigned int target = data.size();
	data.push_back(0);
	data.push_back(0);

	unsigned short sz = 0;
	unsigned int target2 = 0;	
	unsigned char *p = (unsigned char*)&sz;
	while(1) {
		CC();
		if(strncmp((char*)&qb[pos], "endif", 5) == 0) {
			pos += 5;
			SkipWhite();
			
			break;
		} else if(strncmp((char*)&qb[pos], "else", 4) == 0) {
			pos += 4;
			SkipWhite();

			data.push_back(0x48);
			target2 = data.size();
			data.push_back(0);
			data.push_back(0);
			sz = data.size() - target;
			p = (unsigned char*)&sz;
			data[target] = p[0];
			data[target+1] = p[1];
			target = target2;
		}
	}
	sz = data.size() - target+1;
	p = (unsigned char*)&sz;
	data[target] = p[0];
	data[target+1] = p[1];
	data.push_back(0x28);
	rIfs--;
}
void DoSwitch() {
	rSwitches++;
	data.push_back(0x3C);
	CC();					// compile the condition
	if(strncmp((char*)&qb[pos], "case", 4) && strncmp((char*)&qb[pos], "default", 4)) {
		pos += 4;
		printf("Switch without cases??? fuck you??? Line %i\n", line);
		printf("Or maybe just a foolish syntax error :P\n");
		exit(1);
	}

	unsigned int target = 0;
	vector<unsigned int> end_targets;
	unsigned short sz = 0;
	unsigned char *p = (unsigned char*)&sz;

	while(1) {
		if(strncmp((char*)&qb[pos], "case", 4) == 0) {
			pos += 4;
			SkipWhite();
			data.push_back(0x3E);
		} else if(strncmp((char*)&qb[pos], "default", 7) == 0) {
			pos += 7;
			SkipWhite();
			data.push_back(0x3F);	
		} else {
			printf("Unknown switch command starting with '%c' in line %i\n", qb[pos], line);
			exit(1);
		}
		if(!no_switch_offsets) {
			data.push_back(0x49);
			target = data.size();
			data.push_back(0);
			data.push_back(0);
		}
		CC();
		if(strncmp((char*)&qb[pos], "endcase", 7) == 0) {
			pos += 7;
			SkipWhite();
			data.push_back(0x49);
			end_targets.push_back(data.size());
			data.push_back(0);
			data.push_back(0);

			sz = data.size() - target;
			data[target] = p[0];
			data[target+1] = p[1];
		} else if(strncmp((char*)&qb[pos], "end_switch", 10) == 0) {
			pos += 10;
			SkipWhite();
			data.push_back(0x3D);
			break;
		}
	}
	sz = data.size() - target-1;
	data[target] = p[0];
	data[target+1] = p[1];
	while(end_targets.size()) {
		target = end_targets[end_targets.size()-1];

		sz = data.size() - target;
		data[target] = p[0];
		data[target+1] = p[1];

		end_targets.pop_back();
	}

	rSwitches--;
}
void CC() {
	while(pos < size) {
		if(qb[pos] == '#') {
			pos++;
			Prep();
		} else if(qb[pos] == '%') {
			pos++;
			Percent();
		} else if(qb[pos] == '$') {
			Match('$');
			GetWord();
			Match('$');
			data.push_back(0x16);
			int id = GetUID(lastWord);
			unsigned char *p = (unsigned char*)&id;
			data.push_back(p[0]);
			data.push_back(p[1]);
			data.push_back(p[2]);
			data.push_back(p[3]);
		} else if(qb[pos] == '=') {
			pos++;
			SkipWhite();
			data.push_back(0x07);
		} else if(qb[pos] == '=') {
			pos++;
			SkipWhite();
			data.push_back(0x07);
		} else if(qb[pos] == '+') {
			pos++;
			SkipWhite();
			data.push_back(0x0B);
		} else if(qb[pos] == '-' && qb[pos+1] != '>') {
			pos++;
			SkipWhite();
			data.push_back(0x0A);
		} else if(qb[pos] == '-' && qb[pos+1] == '>') {
			pos += 2;
			SkipWhite();
			data.push_back(0x08);
		} else if(qb[pos] == '*') {
			pos++;
			SkipWhite();
			data.push_back(0x0D);
		} else if(qb[pos] == '/') {
			pos++;
			SkipWhite();
			data.push_back(0x0C);
		} else if(qb[pos] == '<') {
			pos++;
			SkipWhite();
			data.push_back(0x12);
		} else if(qb[pos] == '>') {
			pos++;
			SkipWhite();
			data.push_back(0x14);
		} else if(qb[pos] == '(') {
			pos++;
			SkipWhite();
			data.push_back(0x0E);
		} else if(qb[pos] == ')') {
			pos++;
			SkipWhite();
			data.push_back(0x0F);
		} else if(qb[pos] == ':') {
			pos++;
			Colon();
		} else if(qb[pos] == ';') {
			pos++;
			SkipWhite();
			data.push_back(0x09);
		} else if(strncmp((char*)&qb[pos], "function", 8) == 0) {
			pos += 8;
			SkipWhite();
			data.push_back(0x23);
		} else if(strncmp((char*)&qb[pos], "endfunction", 11) == 0) {
			pos += 11;
			SkipWhite();
			data.push_back(0x24);
		} else if(strncmp((char*)&qb[pos], "while", 5) == 0) {
			pos += 5;
			SkipWhite();
			data.push_back(0x20);
		} else if(strncmp((char*)&qb[pos], "loop_to", 7) == 0) {
			pos += 7;
			SkipWhite();
			data.push_back(0x21);
		} else if(strncmp((char*)&qb[pos], "continue", 8) == 0) {
			pos += 8;
			SkipWhite();
			data.push_back(0x22);
		} else if(strncmp((char*)&qb[pos], "if", 2) == 0) {
			pos += 2;
			SkipWhite();
			DoIf();
		} else if(strncmp((char*)&qb[pos], "doIf", 4) == 0) {
			pos += 4;
			SkipWhite();
			DoIf2();
		} else if(strncmp((char*)&qb[pos], "NOT", 3) == 0) {
			pos += 3;
			SkipWhite();
			data.push_back(0x39);
		} else if(strncmp((char*)&qb[pos], "OR", 2) == 0) {
			pos += 2;
			SkipWhite();
			data.push_back(0x32);
		} else if(strncmp((char*)&qb[pos], "AND", 3) == 0) {
			pos += 3;
			SkipWhite();
			data.push_back(0x33);
		} else if(strncmp((char*)&qb[pos], "isNull", 6) == 0) {
			pos += 6;
			SkipWhite();
			data.push_back(0x2C);
		} else if(strncmp((char*)&qb[pos], "return", 6) == 0) {
			pos += 6;
			SkipWhite();
			data.push_back(0x29);
		} else if(strncmp((char*)&qb[pos], "endif", 5) == 0
			|| strncmp((char*)&qb[pos], "else", 4) == 0
			|| strncmp((char*)&qb[pos], "doElse", 6) == 0) {
			if(!rIfs) {
				printf("endif/else without if at line %i\n", line);
				exit(1);
			}
			//pos += 5;
			SkipWhite();
			return;
		} else if(strncmp((char*)&qb[pos], "switch", 6) == 0) {
			pos += 6;
			SkipWhite();
			if(no_switch_offsets) data.push_back(0x3C);
			else DoSwitch();
		} else if(no_switch_offsets
			&& (strncmp((char*)&qb[pos], "case", 4) == 0
				|| strncmp((char*)&qb[pos], "default", 7) == 0
				|| strncmp((char*)&qb[pos], "end_switch", 10) == 0
			)) {
			if(strncmp((char*)&qb[pos], "case", 4) == 0) {
				pos += 4;
				SkipWhite();
				data.push_back(0x3E);
			} else if(strncmp((char*)&qb[pos], "default", 7) == 0) {
				pos += 7;
				SkipWhite();
				data.push_back(0x3F);
			} else if(strncmp((char*)&qb[pos], "end_switch", 10) == 0) {
				pos += 10;
				SkipWhite();
				data.push_back(0x3D);
			}
		} else if(strncmp((char*)&qb[pos], "case", 4) == 0
			|| strncmp((char*)&qb[pos], "endcase", 7) == 0
			|| strncmp((char*)&qb[pos], "end_switch", 10) == 0) {
			if(!rSwitches) {
				printf("case/endcase/endswitch without switch at line %i\n", line);
				exit(1);
			}
			return;
		} else if(strncmp((char*)&qb[pos], "call", 4) == 0) {
			pos += 4;
			SkipWhite();
		} else if(strncmp((char*)&qb[pos], "arguments", 9) == 0) {
			pos += 9;
			SkipWhite();
//////////////////////////////////////////////////////////////////////////////
// THE FOLLOWING IS ... "datasyntax"...
// We don't know yet... how these selects work correctly...
// AND... we don't care lol ('course we do... but not now...)
		} else if(strncmp((char*)&qb[pos], "select", 6) == 0) {
			pos += 6;
			SkipWhite();
			//data.push_back(0x2F);
			Match('(');
			unsigned int count = 0;
			unsigned char *p = (unsigned char*)&count;

			count = wgetlong((char*)(qb+pos), NULL, 16);
			while(ISHEX) pos++;
			Match(',');
			data.push_back((unsigned char)count);

			count = wgetlong((char*)(qb+pos), NULL, 0);
			data.push_back(p[0]);
			data.push_back(p[1]);
			data.push_back(p[2]);
			data.push_back(p[3]);

			while(IsNum()) pos++;
			Match(',');

			unsigned char byte;
			while(count) {
				if(!IsNum()) {
					printf("Wrong select syntax...\n");
					exit(1);
				}
				byte = (unsigned char)wgetlong((char*)(qb+pos), NULL, 16);
				while(ISHEX) pos++;
				data.push_back(byte);
				SkipWhite();
				byte = (unsigned char)wgetlong((char*)(qb+pos), NULL, 16);
				while(ISHEX) pos++;
				data.push_back(byte);
				SkipWhite();
				count--;
			}
			Match(')');
		} else if(qb[pos] == '.') {
			data.push_back(0x42);
			pos++;
			SkipWhite();
		} else {
			printf("Unkown how to handle '%c' at line %i\n", qb[pos], line);
			exit(1);
		}
	}
}
int Compile(char *filename)
{
	FILE *file = fopen(filename, "rb");
	if(!file) {
		printf("Couldn't open file!");
		return 1;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	qb = (unsigned char*)malloc(size);
	if(!qb) return 2;
	fread(qb, 1, size, file);
	fclose(file);
	file = NULL;

	atexit(DUMP);
	pos = 0;
	line = 1;
	CC();

	unsigned int n;
	unsigned char *c;
	for(n = 0; n < names.size(); n++) {
		data.push_back(0x2B);
		c = (unsigned char*)&names[n].id;
		data.push_back(c[0]);
		data.push_back(c[1]);
		data.push_back(c[2]);
		data.push_back(c[3]);
		c = (unsigned char*)names[n].name.c_str();
		while(*c) { data.push_back(*c); c++; }
		data.push_back(0);
	}

	data.push_back(0);

	// HANDLE OFFSETS!!!
	unsigned int o;
	unsigned int p;
	unsigned short sz2;
	unsigned int sz4;
	unsigned char *w = (unsigned char*)&sz4;
	while(data_offsets.size()) {
		o = data_offsets.size()-1;

		for(p = 0; p < data_positions.size(); p++) {
			if(data_positions[p].target == data_offsets[o].target) break;
		}
		if(p >= data_positions.size()) {
			printf("Offset pointint to non-existing offset %i\n", data_offsets[o].target);
			exit(1);
		}
		if(data_offsets[o].size == 2) {
			w = (unsigned char*)&sz2;
			sz2 = data_positions[p].pos - (data_offsets[o].pos+2);
		} else if(data_offsets[o].size == 4) {
			w = (unsigned char*)&sz4;
			sz4 = data_positions[p].pos - (data_offsets[o].pos+4);
			data[data_offsets[o].pos+2] = w[2];
			data[data_offsets[o].pos+3] = w[3];
		} else {
			printf("INTERNAL: data_offset(%i) has size %i\n", data_offsets[o].target, data_offsets[o].size);
			exit(99);
		}
		data[data_offsets[o].pos] = w[0];
		data[data_offsets[o].pos+1] = w[1];
		
		data_offsets.pop_back();
	}
	/*printf("Handle offsets!!!\n");
	exit(1);*/

	char name[512];
	sprintf(name, "%s.qb", filename);
	file = fopen(name, "wb");
	fwrite(&data[0], 1, data.size(), file);
	fclose(file);

	nodump = true;

	return 0;
}
unsigned int GetUID(char *name) {
	unsigned int n;
	for(n = 0; n < names.size(); n++) {
		if(strcmp(names[n].name.c_str(), name) == 0) break;
	}
	if(n < names.size()) {
		return names[n].id;
	} else {
		printf("Name '%s' is unknown - line %i\n", name, line);
		exit(6);
	}
}
void Match(char c) {
	if(qb[pos++] != c) {
		printf("'%c' Expected - line %i\n", c, line);
		exit(5);
	}
	SkipWhite();
}
void GetString() {
	if(qb[pos] != '"') {
		printf("*** error @%i: STRING expected!\n", line);
		exit(1);
	}
	pos++;
	int i = 0;
	while(pos < size && qb[pos] && qb[pos] != '"') {
		lastString[i++] = qb[pos++];
	}
	pos++;
	lastString[i] = 0;
	SkipWhite();
}
bool IsNum()
{
	return (qb[pos] >= '0' && qb[pos] <= '9');
}
bool IsHex()
{
	return (
		(qb[pos] >= '0' && qb[pos] <= '9') ||
		(qb[pos] >= 'a' && qb[pos] <= 'f') ||
		(qb[pos] >= 'A' && qb[pos] <= 'F')
		);
}
bool IsAlpha()
{
	return (
		(qb[pos] >= 'a' && qb[pos] <= 'z') ||
		(qb[pos] >= 'A' && qb[pos] <= 'Z') ||
		qb[pos] == '_' || qb[pos] == '\\'
		);
}
bool IsWhite()
{
	return qb[pos] == ' ' || qb[pos] == '\t';
}
/*bool IsStrChar()
{
	return IsAlpha() || IsWhite() || IsNum() || qb[pos] == '/' || qb[pos] == '?' ||
		qb[pos] == '!' || qb[pos] == '^' || qb[pos] == '(' || qb[pos] == ')' ||
		qb[pos] == '[' || qb[pos] == ']' || qb[pos] == ':' || qb[pos] == ';' ||
		qb[pos] == '-' || qb[pos] == '+' || qb[pos] == '*' || qb[pos] == ',' ||
		qb[pos] == '.' || qb[pos] == '\\' || qb[pos] == '=' || qb[pos] == '\'' ||
		qb[pos] == '&' || qb[pos] == '%' || qb[pos] == '<' || qb[pos] == '>' ||
		qb[pos] == '@' || qb[pos] == 0xae || qb[pos] == 0x7f || qb[pos] == '$' ||
		qb[pos] == '§' || qb[pos] == 0xa7;
}*/

void GetWord()
{
	int i = 0;
	//memset(lastWord, 0, sizeof(lastWord));
	if(!IsAlpha()) {
		printf("*** error @%i: WORD expected!\n", line);
		exit(1);
	}
	while(IsAlpha() || IsNum()) {
		lastWord[i++] = qb[pos++];
	}
	lastWord[i] = 0;
	SkipWhite();
}

int GetNumber()
{
	int ret = 0;
	SkipWhite();
	//ret = strtol((char*)&qb[pos], NULL, 0);
	ret = wgetlong((char*)&qb[pos], NULL, 0);
	while(qb[pos] == '-') pos++;
	if(qb[pos] == '0' && qb[pos+1] == 'x') pos+=2;
	while(qb[pos] == '-') pos++;
	while(ISHEX) pos++;
	SkipWhite();
	return ret;
}
int GetHex() {
	int ret = wgetlong((char*)(qb+pos), NULL, 16);
	while(ISHEX) pos++;
	SkipWhite();
	return ret;
}
void SkipWhite()	// FIXME \n?
{
	while(qb[pos] == ' ' || qb[pos] == '\t' || qb[pos] == '\n' || qb[pos] == '\r') {
		if(qb[pos] == '\n') line++;
		pos++;
	}
}

unsigned long wgetlong(char *t, char**, int base)
{
	bool hex = false;
	bool neg = false;
	int i = 0;
	int j = 0;
	unsigned long ret = 0;

	char num[64];
	memset(num, 0, 64);

	if(base == 16) hex = true;

	while(*t == '-') { neg = !neg; t++; }
	if(*t == '0' && t[1] == 'x') { hex = true; t += 2; }

	while(*t && (
		(t[i] >= 'a' && t[i] <= 'f' && base != 10) ||
		(t[i] >= 'A' && t[i] <= 'F' && base != 10) ||
		(t[i] >= '0' && t[i] <= '9') )) {
		if(	(t[i] >= 'a' && t[i] <= 'f' && base != 10) ||
			(t[i] >= 'A' && t[i] <= 'F' && base != 10) )
		{
			//t[i] += 'A' - 'a';	// TO UPPER CASE
			hex = true;
		}
		num[i] = t[i];
		i++;
	}
	
	for(j = 0; j < (int)strlen(num); j++) {
		if(num[j] >= 'a' && num[j] <= 'f') num[j] += 'A' - 'a';
	}

	j = 0;

	unsigned long inc = hex ? 0x10 : 10;

	if(num[j] >= 'A' && num[j] <= 'F') ret += num[j] - 'A' + 10;
	else ret += num[j] - '0';
	j++;

	for(; num[j]; j++) {
		ret *= inc;
		if(num[j] >= 'A' && num[j] <= 'F') ret += num[j] - 'A' + 10;
		else ret += num[j] - '0';
	}
	if(neg) ret *= -1;

	return ret;
}
