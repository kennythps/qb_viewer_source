#include "main.h"
#include <iostream>
#include <fstream>
#include <sstream>

char startdata = 0;

char strtype = 0;
int tpos = 0;
char *cp = NULL;

vector<QB_INSTR> inst;
vector<QB_NAME> names;
vector<unsigned int> labels;

bool qb_debuginfo = false;

int Decompile(char *filename)
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

	bool done = false;

	char tmp[256];
	int *iptmp;
	float *fptmp, *fp2, *fp3;

	int ifcount = 0;
	int calltabs = 0;
	bool do_add = true;
	QB_INSTR i;
	unsigned int l;
	char last_Inst = 0;
	while(pos < size && !done) {
		i.off = pos;
		last_Inst = i.inst;
		i.inst = qb[pos++];
		i.len = 1;

		i.tabs = 0;
		i.nl = i.pnl = false;
		i.pnltabs = 0;

		i.pre = "";
		i.text = "";
		i.post = "";

		memset(i.opcode, 0, sizeof(i.opcode));

		for(l = 0; l < labels.size(); l++) {
			if(labels[l] == i.off) {
				sprintf(tmp, " :POS(%i) ", l);
				i.text = tmp;
				l = i.inst;
				i.inst = 0;
				inst.push_back(i);
				i.text = "";
				i.inst = l;
				break;
			}
		}

		do_add = true;

		switch(i.inst) {
			case 00:
				i.text = ":end";
				i.nl = true;
				done = true;
				break;
#define _DO_CALL_TEXT_
			case 01:
				i.pnl = true;
				i.text = ":i ";
#ifdef _DO_CALL_TEXT_
				if(calltabs) {
					calltabs = 0;
					i.pnltabs = -1;
					i.tabs = -1;
				}
#endif
				if(last_Inst == 01 || last_Inst == 02) {
					i.tabs--;
					i.pnltabs--;
				}
				if(qb[pos] == 0x04 || qb[pos] == 0x21 || qb[pos] == 0x24 || qb[pos] == 0x26 || qb[pos] == 0x28 || qb[pos] == 0x48) {
					i.pnltabs--;
				}
				break;

			case 02:
				i.len = 5;
				memcpy(i.opcode, qb+pos, 4);
				pos += 4;
				iptmp = (int*)i.opcode; 
				sprintf(tmp, ":u(02:%08x)", *iptmp);
				i.text = tmp;
				i.pnl = true;
#ifdef _DO_CALL_TEXT_
				if(calltabs) {
					calltabs = 0;
					i.pnltabs = -1;
					i.tabs = -1;
				}
#endif
				if(last_Inst == 01 || last_Inst == 02) {
					i.tabs--;
					i.pnltabs--;
				}
				if(qb[pos] == 0x04 || qb[pos] == 0x21 || qb[pos] == 0x24 || qb[pos] == 0x26 || qb[pos] == 0x28 || qb[pos] == 0x48) {
					i.pnltabs--;
				}
				break;
				break;

			case 03:
				//i.pnl = true;
				i.text = ":s{";
				//i.nl = true;
				i.tabs = 1;
				//i.pnltabs = 1;
				break;
			case 04:
				//i.pnltabs = -1;
				//i.pnl = true;
				i.text = ":s}";
				//i.nl = true;
				i.tabs = -1;
				/*if(calltabs) {
					calltabs--;
					i.pnltabs--;
					i.tabs--;
				}*/
				break;
			case 05:
				//i.pnl = true;
				i.text = ":a{";
				//i.nl = true;
				i.tabs = 1;
				//i.pnltabs = 1;
				break;
			case 06:
				//i.pnltabs = -1;
				//i.pnl = true;
				i.text = ":a}";
				//i.nl = true;
				i.tabs = -1;
				/*if(calltabs) {
					calltabs--;
					i.pnltabs--;
					i.tabs--;
				}*/
				break;
			case 07:
				i.text = " = ";
				break;
			case 0x08:	// Why? Check info appendix A
				i.text = "->";
				break;
			case 0x09:
				i.text = ";";
				break;

			case 0x0A:
				i.text = " - ";
				break;
			case 0x0B:
				i.text = " + ";
				break;
			case 0x0C:
				i.text = " / ";		// MIGHT BE THE OTHER WAY ROUND
				break;
			case 0x0D:
				i.text = " * ";
				break;
			case 0x0E:
				i.text = " (";
				//i.tabs = 1;
				break;
			case 0x0F:
				i.text = ") ";
				//i.tabs = -1;
				/*if(calltabs) {
					calltabs--;
					i.pnltabs--;
					i.tabs--;
				}*/
				break;

				// 12 '<' 14 '>', so 13'<='? and 15 '>='???
			case 0x12:
				i.text = " < ";
				break;
			case 0x14:
				i.text = " > ";
				break;

			case 0x16:
				i.len = 5;
				memcpy(i.opcode, qb+pos, 4);
				i.pre = "$";
				i.name = *((unsigned int*)i.opcode);
				i.post = "$";
				pos += 4;
#ifdef _DO_CALL_TEXT_
				if(qb[pos] == 0x16 && calltabs == 0) {
					i.pre = "call $";
					i.post = "$ arguments ";
					calltabs = 1;
					i.tabs = 1;
					i.nl = true;
				}
#endif
				break;

			case 0x17:
				i.len = 5;
				memcpy(i.opcode, qb+pos, 4);
				iptmp = (int*)&qb[pos];
				pos += 4;
				sprintf(tmp, "%%i(%u,%08x)", *iptmp, *iptmp);
				i.text = tmp;
				break;

			case 0x1A:
				i.len = 5;
				memcpy(i.opcode, qb+pos, 4);
				pos += 4;
				fptmp = (float*)i.opcode;
				sprintf(tmp, "%%f(%f)", *fptmp);
				i.text = tmp;
				break;
			case 0x1B:
			case 0x1C:
				/*memcpy(i.opcode, qb+pos, 4);
				pos += 4;
				iptmp = (int*)i.opcode;
				itmp = *iptmp;
				i.len = 5 + itmp;
				memcpy(i.opcode+4, qb+pos, itmp);
				pos += itmp;
				if(i.inst == 0x1B) {
					sprintf(tmp, "%%s(%x,\"%s\")", itmp, i.opcode+4);
				} else {
					sprintf(tmp, "%%sc(%x,\"%s\")", itmp, i.opcode+4);
				}
				i.text = tmp;*/
				{
					//std::ostringstream str();
					memcpy(i.opcode, qb+pos, 4);
					pos += 4;
					i.len = 5 + *((int*)i.opcode);
					i.text = "";
					if(i.inst == 0x1B) i.text.append("%s("); //str << "%s(";
					else i.text.append("%sc("); //str << "%sc(";
					//str  << (i.len-5) << ",\"" << ((char*)&qb[pos]) << "\")";
					i.text.append(conv::toString(i.len-6));
					i.text.append(",\"");
					i.text.append((char*)&qb[pos]);
					i.text.append("\")");
					//i.text = str.str();
					pos += i.len-5;
				}
				break;
			case 0x1E:
				i.len = 13;
				memcpy(i.opcode, qb+pos, 12);
				pos += 12;
				fptmp = (float*)i.opcode;
				fp2 = (float*)(i.opcode+4);
				fp3 = (float*)(i.opcode+8);
				sprintf(tmp, "%%vec3(%f,%f,%f)", *fptmp, *fp2, *fp3);
				i.text = tmp;
				break;
			case 0x1F:
				i.len = 9;
				memcpy(i.opcode, qb+pos, 8);
				pos += 8;
				fptmp = (float*)i.opcode;
				fp2 = (float*)(i.opcode+4);
				sprintf(tmp, "%%vec2(%f,%f)", *fptmp, *fp2);
				i.text = tmp;
				break;

			case 0x20:
				i.nl = true;
				i.text = "while";
				i.tabs = 1;
				break;
			case 0x21:
				//i.pnl = true;
				//i.pnltabs = -1;
				i.text = "loop_to ";
				i.tabs = -1;
				break;

			case 0x22:
				i.nl = true;
				i.text = "continue";
				break;

			case 0x23:
				i.text = "function ";
				i.tabs = 1;
				break;

			case 0x24:
				//i.pnl = true;
				//i.nl = true;
				i.text = "endfunction";
				i.tabs = -1;
				i.pnltabs = -1;
				break;

			case 0x2C:
				i.text = " isNull ";
				break;

			case 0x2D:
				i.text = "%GLOBAL%";
				break;

			case 0x42:
				i.text = ".";
				break;

			case 0x47:
				i.text = "if ";
				if(qb_debuginfo) {
					ifcount++;
					sprintf(tmp, "if[%i] ", ifcount);
					i.text = tmp;//"if ";
				}
				i.len = 2;
				i.tabs = 1;
				memcpy(i.opcode, qb+pos, 2);
				pos += 2;
				/*if(qb[pos] == 0x39) {
					i.len = 3;
					i.opcode[2]= 0x39;
					pos++;
					i.post = "NOT ";
				}*/
				break;
			case 0x39:
				i.text = "NOT ";
				break;
			case 0x25:
				i.text = "doIf ";
				i.tabs = 1;
				break;
			case 0x26:
				i.text = "doElse";
				break;
			
			case 0x28:
				i.text = "endif";
				if(qb_debuginfo) {
					sprintf(tmp, "endif[%i] ", ifcount);
					ifcount--;
					i.text = tmp;//"endif";
				}
				//i.pnl = i.nl = true;
				//i.pnltabs = -1;
				i.tabs = -1;
				break;
			case 0x29:
				i.text = "return";
				if(qb_debuginfo) {
					sprintf(tmp, "return[%i]", ifcount);
					i.text = tmp;//"return";
				}
				i.nl = true;
				break;
			case 0x48:
				//i.pnl = true;
				//i.pnltabs = -1;
				i.text = "else ";
				if(qb_debuginfo) {
					sprintf(tmp, "else[%i] ", ifcount);
					i.text = tmp;//"else ";
				}
				i.len = 3;
				memcpy(i.opcode, qb+pos, 2);
				pos += 2;
				break;
			case 0x32:
				i.text = " OR ";
				break;
			case 0x33:
				i.text = " AND ";
				break;

			case 0x3C:
				i.text = "switch ";
				i.tabs = 1;
				break;
			case 0x3D:
				i.text = "end_switch";
				i.tabs = -2;
				if(no_switch_offsets) i.tabs++;
				i.nl = true;
				break;
			case 0x3E:
				if(!no_switch_offsets) {
					i.len = 4;
					memcpy(i.opcode, qb+pos, 3);
					pos += 3;
					i.tabs = 1;
				}
				i.text = "case ";
				break;
			case 0x3F:
				if(!no_switch_offsets) {
					i.len = 4;
					memcpy(i.opcode, qb+pos, 3);
					pos += 3;
					i.tabs = 1;
				}
				i.text = "default ";
				break;
			case 0x49:
				if(!no_switch_offsets) {
					i.len = 3;
					memcpy(i.opcode, qb+pos, 2);
					pos += 2;
					i.text = "endcase";
					i.tabs = -1;
					i.nl = true;
				} else {
					printf("GOTO stuff not implemented... please report that cuz it shouldn't happen\n");
					printf("Maybe this file doesn't work with -thug1?\n");
					exit(1);
				}
				break;

/*
unsigned int last_label;
unsigned int switch_label;
unsigned int case_label;
vector<unsigned int> switch_labels;
void switch_push() {
void switch_pop() {

.data = 01 3c;		   +-------| this is the offset|to the next case   +->TO THIS BYTE
camera_angle .data = 01 3e 49 27 00 17 00 00 00 00 01; |
camera_text :s= "CAMERA ANGLE: 1" .data = 01 49 7b 00 3e 49 27 00 17 01 00 00 00 01;
camera_text :s= "CAMERA ANGLE: 2" .data = 01 49 52 00 3e 49 27 00 17 02 00 00 00 01;
camera_text :s= "CAMERA ANGLE: 3" .data = 01 49 29 00 3e 49 24 00 17 03 00 00 00 01;
camera_text :s= "CAMERA ANGLE: 4" .data = 01 3d 01;


:i switch $camera_angle$
	:i case(a) goto(b) long(0)
		:i $camera_text$ = %s("CAMERA ANGLE: 1")
		:i goto(e)
	:i case(b) goto(c) long(1)
		:i $camera_text$ = %s("CAMERA ANGLE: 2")
		:i goto(e)
	:i case(c) goto(d) long(2)
		:i $camera_text$ = %s("CAMERA ANGLE: 3")
		:i goto(e)
	:i case(d) goto(e) long(3)
		:i $camera_text$ = %s("CAMERA ANGLE: 4")
:i endswitch(e)
*/

			case 0x2E:
				i.pnl = true;
				i.pnltabs = -1;
				i.text = "break";
				i.nl = true;
				i.len = 5;
				memcpy(i.opcode, qb+pos, 4);
				pos += 4;
				/*l = *((unsigned int*)i.opcode);
				sprintf(tmp, "breakto(%i)", labels.size());
				labels.push_back(l+pos);
				i.text = tmp;*/
				{
					unsigned int p = *((unsigned int*)i.opcode);
					p += pos;
					for(l = 0; l < labels.size(); l++) {
						if(labels[l] == p) break;
					}
					if(l >= labels.size()) {
						labels.push_back(p);
					}
					sprintf(tmp, ":BREAKTO(%i)", l);
					i.text = tmp;
				}
				break;
			case 0x37:
			case 0x40:
			case 0x41:
			case 0x2F:
				/*i.len += 4;
				itmp = *((int*)(&qb[pos]));
				i.len += 6*itmp;
				//memcpy(i.opcode, qb+pos, i.len-1);
				sprintf(tmp, "select(%i) ", itmp);
				i.text = tmp;
				pos += i.len-1;
				i.nl = true;
				i.tabs = 1;*/
				{
					i.len += 4;
					int nCases = *((int*)(&qb[pos]));
					sprintf(tmp, "select(%02x,%i, ", i.inst, nCases);
					i.text = tmp;
					int ind;
					pos += 4;
					for(ind = 0; ind < nCases; ind++) {
						sprintf(tmp, "%02x ", qb[pos++]);
						i.text.append(tmp);
						sprintf(tmp, "%02x ", qb[pos++]);
						i.text.append(tmp);
					}
					i.text.resize(i.text.length()-1); // remove the space
					i.text.append(") ");
					unsigned int cnt;
					for(ind = 0; ind < nCases; ind++) {
						i.text.append(":OFFSET(");
						cnt = *((unsigned int*)(&qb[pos]));
						pos += 4;
						for(l = 0; l < labels.size(); l++) {
							if(labels[l] == (cnt+pos)) break;
						}
						if(l < labels.size()) {
							l = labels.size();
						} else {
							labels.push_back(cnt+pos);
						}
						i.text.append(conv::toString(l));
						i.text.append(")");
					}
					i.nl = true;
					if(!no_switch_offsets)
						i.tabs = 1;
				}
				break;
/*
	  +number of cases
	  |
:i select(10) [01 00 : A times, A offsets to a non :i starting instruction]
	call $runtwoscripts$ arguments
		$script_text$ = $bail1$
		$script_score$ = $bail1$
		isNull
	:i break	[4 byte offset, why 4 byte offset??? IFs have 2 byte offsets...]
	call $runtwoscripts$ arguments
		$script_text$ = $bail2$
		$script_scores$ = $bail2$
		isNull
	:i break
(...)
	call $runtwoscripts$ arguments
		$script_text$ = $bail3$
		$script_scores$ = $bail3$
		isNull
	:i
:i
*/

			case 0x2B:
				{
					QB_NAME n;
					do_add = false;
					n.id = *((unsigned int*)(&qb[pos]));
					pos += 4;
					n.name = (char*)(qb+pos);
					//fprintf(stderr, "\n%08x::NAME: %08x - (%i)%s\n", pos-4, n.id, n.name.length(), n.name);
					pos += n.name.length()+1;
					names.push_back(n);
				}
				break;

			default:
				done = true;
				sprintf(tmp, "Unknown instruction at %08x:", i.off);
				i.text = tmp;
				char tmp[8];
				sprintf(tmp, "%x", i.inst);
				//i.text.append(tmp);
				i.post = tmp;
				break;
		}
		if(do_add) inst.push_back(i);
	}

	unsigned int _i;
	unsigned int tabs = 0;
	unsigned int t;
	std::cout << "#/ QB Script version 2" << std::endl;
	std::cout << "%include \"" << filename << "_table.qbi\"   #/ Table file" << std::endl;
	for(_i = 0; _i < inst.size(); _i++) {
		if(inst[_i].pnl) {
			std::cout << endl;
			for(t = 0; t < (tabs + inst[_i].pnltabs); t++) cout << "\t";
		}
		std::cout << inst[_i].pre;
		if(inst[_i].inst == 0x16) {
			unsigned int _n = 0;
			for(_n = 0; _n < names.size(); _n++) {
				if(names[_n].id == inst[_i].name) break;
			}
			if(_n < names.size()) {
				std::cout << names[_n].name;
			} else {
				int nn = inst[_i].name;
#ifndef WIN32
				__asm("bswap %0\n" : "=q"(nn) : "0"(nn) : "0");
#else
				__asm {
					mov eax, nn
					bswap eax
					mov nn, eax
				}
#endif
				sprintf(tmp, "%08x", nn);
				std::cout << "[" << tmp << "]";
			}
		} else {
			std::cout << inst[_i].text;
		}
		std::cout << inst[_i].post;
		tabs += inst[_i].tabs;
		if(inst[_i].nl) {
			std::cout << std::endl;
			for(t = 0; t < tabs; t++) cout << "\t";		
		}
	}
	std::cout << std::endl << std::endl << "#/ END" << std::endl;

	//filename << "_table.qbi
	sprintf(tmp, "%s_table.qbi", filename);
	file = fopen(tmp, "w");
	//std::ofstream tabl(tmp);
	unsigned int _n;
	for(_n = 0; _n < names.size(); _n++) {
		/*sprintf(tmp, "%08x", names[_n].id);
		tabl << "#addx 0x" << tmp << " \"" << names[*/
		fprintf(file, "#addx 0x%08x \"%s\"\n", names[_n].id, names[_n].name.c_str());
	}
	fclose(file);
	//tabl.close();
	return 0;
}
