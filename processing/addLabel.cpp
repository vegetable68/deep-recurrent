#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <string.h>
#include <queue>
#include <assert.h>

int tag[50000];
using namespace std;
ifstream pos("dse.txt");
ifstream sId("sentenceid.txt");
ofstream out("holder.txt");
string file;

string fuzzy(string word){
	if (word == "Labor") return "Labour";

	if (word == "honor") return "honour";
	if (word == "HIV\\/AIDS") return "HIV/AIDS";
	if (word == "400\\/year") return "400/year";
	if (word == "and\\/or") return "and/or";




	if (word == "organization") return "organisation";
	if (word == "traveling") return "travelling";
	if (word == "harbor") return "harbour";
	if (word == "Defense") return "Defence";

	if (word == "traveled") return "travelled";

	if (word == "labor") return "labour";

	if (word == "center") return "centre";
	if (word == "behavior") return "behaviour";
	if (word == "recognize") return "recognise";
	if (word == "`") return "\'";
	if (word == "``") return "\"";
	if (word == "''") return "\"";
	if (word == "\"") return "``";
	return word;
}

vector<string> split(const string &s, char delim) {
	stringstream ss(s);
	string item;
	vector<string> elems;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

vector<string> getPline(){
	string p;
	vector<string> em;
	if (!getline(pos, p)) return em;
	if (p == ""){
		string sline;
		getline(sId, sline);
		file = "";
		if (sline == "") return em;
		vector<string> ss = split(sline, ' '); 
		file = ss[2];
		out<<endl;
		if (!getline(pos, p)) return em;
	}
	return split(p, '	');
}

string getWord(string word){
	string res = word;
	if (word == "-LRB-")
		res = "(";
	else if (word == "-RRB-")
		res = ")";
	else if (word == "-LSB-")
		res = "[";
	else if (word == "-RSB-")
		res = "]";
	else if (word == "-LCB-")
		res = "{";
	else if (word == "-RCB-")
		res = "}";
	return res;
}

string FileName;
int main(){
	string lab = "OBI";
	string sent;
	getline(sId, sent);
	vector<string> ss = split(sent, ' ');
	file = ss[2];

	vector<string> as = getPline(); 
	string word = getWord(as[0]);
	while (file != ""){
		cout<<file<<endl;
		FileName = "annotation/" + file + "/gateman.mpqa.lre.2.0";
		ifstream annot(FileName.c_str());
		FileName = "original/" + file;
		ifstream ori(FileName.c_str());
		memset(tag, 0, sizeof(tag));
		string line;
		while (getline(annot, line)){
			if (line[0] == '#') continue;
			vector<string> s = split(line, '	');
			if (s[3] == "GATE_agent" && s.size() >= 5 && s[4].find("nested-source") != -1){ 
				vector<string> tmps = split(s[1], ',');
				assert(tmps.size() == 2);
				int start = atoi(tmps[0].c_str()),
					end = atoi(tmps[1].c_str());
				tag[start] = 1;
				for (int i = start + 1; i < end; i ++) tag[i] = 2;
			}
		}
		int tot = 0;
		while (getline(ori, line)){
			//out<<line<<endl;
			if (line == "") {tot ++; continue;}
//			if (as.empty()) continue;
			int L = line.length();
			string cur = "";
			string outtag = "";
			int position = line.find(word);
			if (position == -1){
				word = fuzzy(word);
				position = line.find(word);

			}
			int nxt = position;
			while (position != -1){
				out<<as[0]<<"	"<<as[1]<<"	"<<lab[tag[tot + position]]<<endl;
				as = getPline(); 
				nxt += word.length();
				if (as.empty()) break;

				word = getWord(as[0]);
				position = line.find(word, nxt); 
				if (position == -1 && word == "On" && nxt == 346) break;
				if (position == -1 && word == "SA" && nxt == 3268) break;
				if (position == -1 && word == "Local" && nxt == 220) break;
				if (position == -1 && word == "Havana" && nxt == 695) break;
				if (position == -1 && word == "Notwithstanding" && nxt == 158) break;
				if (position == -1 && nxt < line.length() - 1) {
					word = fuzzy(word);
					position = line.find(word, nxt); 
					if (position == -1){
						out<<line.length()<<","<<nxt<<","<<word<<endl;assert(0);
					}
				} 
				if (position == -1 && word == "."){
					out<<as[0]<<"	"<<as[1]<<"	"<<lab[tag[tot + position]]<<endl;
				as = getPline(); 
				word = getWord(as[0]);
				break;

				}
				nxt = position; //out<<word<<","<<position<<endl;
			}
			tot += line.length();
			tot ++;
		}
//		out<<endl;
	}
}


