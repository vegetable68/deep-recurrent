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

filebuf ans, out;

int main(){
	ans.open((relation_type + "_dse_"+ category).c_str(), std::ios::in);
	out.open(((relation_type + "_results/") + category + ".out").c_str(), std::ios::in);
	string so, sa;
	int trupos = 0, ans_pos = 0, falpos = 0, falneg = 0;
	while (getline(ans, sa)){
		getline(out, so);
		if (sa[0] == "+"){
			if (so[0] == "+") trupos ++; else falneg ++;
		}else{
			if (so[0] == "+") falpos ++; else truneg ++;
		}
	}

}

