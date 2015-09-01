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
	t.open("target.txt", std::ios::in);
	a.open("agent.txt", std::ios::in);
	d.open("dse.txt", std::ios::in);
	istream it(&t), ia(&a), id(&d);
	string st, sa, sd;
	while (getline(it, st)){
		getline(ia, sa);
		getline(id, sd);
		if (st == "") cout<<"\n";
		else{
			st = st.substr(0, st.size() - 1);
			sa = sa.substr(0, sa.size() - 1);
			sd = sd.substr(0, sd.size() - 1);
			string token, part, tl, al, dl;
			uint i = st.find_first_of('\t');
			token = st.substr(0, i);
			uint j = st.find_first_of('\t', i+1);
			part = st.substr(i+1,j-i-1);
			//cout << part << endl;
			i = st.find_last_of('\t');
			tl = st.substr(i+1, st.size()-i-1);
			i = sa.find_last_of('\t');
			al = sa.substr(i+1, sa.size()-i-1);
			i = sd.find_last_of('\t');
			dl = sd.substr(i+1, sd.size()-i-1);
			cout<<token<<"\t"<<part<<"\t"<<tl<<"\t"<<al<<"\t"<<dl<<endl;
		}
	}
}

