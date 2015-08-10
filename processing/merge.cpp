#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <string.h>
#include <queue>
#include <assert.h>
#include <fstream>
#include <sstream>

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
	ifstream out("out");
	ifstream ans("ans.txt");
	string oline, aline;
	while (getline(ans, aline)){
		if (aline == "") {cout<<endl; continue;}
		getline(out, oline) ;
		cout<<oline<<","<<aline<<endl;
		vector<string> os = split(oline, ' '); 
		vector<string> as = split(aline, '	');
		string cur = os[0];
		string op_tag = os[1];
		string pos_tag = as[1];
		if (as[0] == "-LRB-" || as[0] == "-RRB-" || as[0] == "-LSB-" || as[0] == "-RSB-" 
				|| as[0] == "-LCB-" || as[0] == "-RCB-")
			cur = as[0];
		if (cur != "\"") 
			while (cur.length() < as[0].length()){
				getline(out, oline);
				os = split(oline, ' '); 
				cur += os[0];
			}
		cur = as[0];
		cout<<cur<<"	"<<pos_tag<<"	"<<op_tag<<endl;
	}

}

