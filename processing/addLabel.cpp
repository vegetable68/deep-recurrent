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

vector<string> split(const string &s, char delim) {
  stringstream ss(s);
  string item;
  vector<string> elems;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
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
  while (getline(ori, line)){
	  int L = line.length();
	  string cur = "";
	  string outtag = "";
	  for (int i = 0; i < L; i ++){
		  if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z') 
				  || (line[i] >= '0' && line[i] <= '9')){ 
			  if (cur == "") outtag = lab[tag[tot]];
			  cur += line[i];
		  }else{
			  if (cur != "") cout<<cur<<" "<<outtag<<endl;
			  cur = "";
		      if (line[i] != ' ' && line[i] != '	') cout<<line[i]<<" "<<lab[tag[tot]]<<endl;
		  }
		  tot ++;
	  }
	  tot ++;
  }
}

