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

filebuf r, a, d;
int ans[100][100], len, arg_num, dse_num;
vector<int> arg[100], dse[100];

int toDigit(string s){
	if (s == "0") return 0;
	int x = 0;

	for (int i = 0; i < s.length(); i ++)
		x = x * 10 + (s[i] - '0');
	return x;
}


int main(){
	a.open("target.txt", std::ios::in);
	d.open("dse.txt", std::ios::in);
	r.open("relation0.txt", std::ios::in);
	istream ia(&a), id(&d), ir(&r);
	string sa, sd, sr;
	len = 0; arg_num = -1; dse_num = -1;
	while (getline(ia, sa)){
		getline(id, sd);
		sa = sa.substr(0, sa.size() -1 );
//		cout<<sa<<","<<sd<<endl;
		if (sa.size() <= 1){
			memset(ans, 0, sizeof(ans));
			while (sr.size() > 1){
				string dseID, argID;
				uint i = sr.find_first_of('\t');
				dseID = sr.substr(0, i);
				argID= sr.substr(i+1, sr.size()-i-2);
				int x = toDigit(dseID), y = toDigit(argID);
				for (int i = 0; i < dse[x].size(); i ++)
					for (int j = 0; j < arg[y].size(); j ++)
						ans[dse[x][i]][arg[y][j]] = 1;
			}
			for (int i = 0; i < len; i ++){
				cout<<ans[i][0];
				for (int j = 1; j < len; j ++){
					cout<<"\t"<<ans[i][j];
				}
				cout<<"\n";
			}
			cout<<"\n";
			len = 0; arg_num = -1; dse_num = -1;

		}else{
//			sa = sa.substr(0, sa.size() - 1);
			sd = sd.substr(0, sd.size() - 1);
			string token, part, tl, al, dl;
			uint i = sa.find_last_of('\t');
			al = sa.substr(i+1, sa.size()-i-1);
			i = sd.find_last_of('\t');
			dl = sd.substr(i+1, sd.size()-i-1);
//			cerr<<al<<","<<dl<<","<<endl;
			if (al == "B"){
				arg_num ++;
				arg[arg_num].clear(); arg[arg_num].push_back(len);
			}
			if (dl == "B"){
				dse_num ++;
				dse[dse_num].clear(); dse[dse_num].push_back(len);
			}
			if (al == "I"){arg[arg_num].push_back(len);}
			if (dl == "I"){dse[dse_num].push_back(len);}
			len ++;
		}
	}
}

