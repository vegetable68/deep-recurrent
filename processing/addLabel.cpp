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

int tag[10000];
using namespace std;
ifstream pos("ans.txt");

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
		cout<<endl;
		if (!getline(pos, p)) return em;
	}
	return split(p, '	');
}

string getWord(string word){
	string res;
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
	if (word == "``") res = "\"";
	if (word == "''") res = "\"";
	return res;
}

int main(){
	string lab = "OBI";

  ifstream annot("annotation");
  ifstream ori("original");
  memset(tag, 0, sizeof(tag));
  string line;
  while (getline(annot, line)){
	  vector<string> s = split(line, '	');
	  if (s[3] == "GATE_agent" && s.size() >= 5 && s[4].find("nested-source") != string::npos){
		 vector<string> tmps = split(s[1], ','); 
		 assert(tmps.size() == 2);
		 int start = atoi(tmps[0].c_str()),
			 end = atoi(tmps[1].c_str());
		 tag[start] = 1;
		 for (int i = start + 1; i < end; i ++) tag[i] = 2;
	  }
  }
  int tot = 0;
vector<string> as = getPline(); 
  while (getline(ori, line)){
	  if (line == "") {tot ++; continue;}
	  if (as.empty()) continue;
	  int L = line.length();
	  string cur = "";
	  string outtag = "";
	  string word = getWord(as[0]);
	  int position = line.find(word);
	  int nxt = position;
	  while (position != string::npos){
		  cout<<as[0]<<" "<<as[1]<<" "<<lab[tag[tot + position]]<<endl;
		  as = getPline(); 
	  if (as.empty()) break;
		
		  word = getWord(as[0]);
		  position = line.find(word, nxt); nxt = position;
	  }
	  tot += line.length();
	  tot ++;
  }
}

