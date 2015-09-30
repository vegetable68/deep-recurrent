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

filebuf attitude, dse, target;

int main(){
	attitude.open("MPQA2.0_processed/attitude_alignannos.txt", std::ios::in);
	dse.open("MPQA2.0_processed/dse_alignannos.txt", std::ios::in);
	target.open("MPQA2.0_processed/target_alignannos.txt", std::ios::in);

	istream ia(&attitude), id(&dse), it(&target);
	string sa, sd, st;
	while (getline(id, sd){
		getline(ia, sa);
		getline(it, st);
		if (st != "") st += '\t';
		int last = 0, targetid = 0;
		while (st.find("id", last) != -1){
			int nxt = st.find("id", last);
			string tar = "";
			for (int j = nxt + 3; j != st.size() && st[j] != '\t'; j ++)
				tar += st[j];
			int
		}
		last = 0;
		int dseid = 0;
		if (sd != "") sd += '\t';
		while (sd.find_first_of('\t', last) != -1){
			int nxt = sd.find_first_of('\t', last);
			string subsd = sd.substr(last, nxt - 1);
			int loc = subsd.find("attrlink");
			loc += 9;
			string attr = "";
			for (int j = loc; j != subsd.length() && subsd[j] != ';'; j ++) attr += subsd[j];
			last = nxt + 1;
		}
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

