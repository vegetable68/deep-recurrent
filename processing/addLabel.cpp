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

bool tag[10000];
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
  ifstream annot("annotation");
  ifstream ori("original");
  memset(tag, false, sizeof(tag));
  string line;
  while (getline(annot, line)){
	  vector<string> s = split(line, '	');
	  if (s[3] == "GATE_direct-subjective"){
		 vector<string> tmps = split(s[1], ','); 
		 assert(tmps.size() == 2);
		 int start = atoi(tmps[0].c_str()),
			 end = atoi(tmps[1].c_str());
		 for (int i = start; i <= end; i ++) tag[i] = true;
	  }
  }
  int tot = 0;
  while (getline(ori, line)){
	  int L = line.length();
	  string cur = "";
	  for (int i = 0; i < L; i ++){
		  if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z') ||
				  (line[i] >= '0' && line[i] <= '9')){
			  cur += line[i];
		  }else{
			  cout<<cur<<" "<<tag[tot]<<endl;
			  cur = "";
			  if (line[i] != ' ') cout<<line[i]<<endl;
		  }
		  tot ++;
	  }
  }
}

