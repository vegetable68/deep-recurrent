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

filebuf t, a, d;

int main(){
	t.open("sentenceid.txt", std::ios::in);
	istream it(&t);
	string st;
	while (getline(it, st)){
		st = st.substr(0, st.size() - 1);
		cout<<st<<endl;
	}
}

