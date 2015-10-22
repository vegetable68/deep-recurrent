#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <iterator>
#include <cassert>
#include <thread>
#include "Eigen/Dense"
#include "utils.cpp"

#define uint unsigned int
#define PII pair<int, int>
#define MP make_pair

#define MAX_ITER 4000
#define ALPHA_DR 0.99
#define L2 0.0
#define DROPOUT
#define ETA 0.001
#define NORMALIZE false// keeping this false throughout my own experiments
#define layers 2 // number of EXTRA (not all) hidden layers
#define DR 0.999

#define MR 0.7
uint fold = -1;

using namespace Eigen;
using namespace std;

double LAMBDA = 1e-4;  // L2 regularizer on weights
double LAMBDAH = (layers > 2) ? 1e-5 : 1e-4; //L2 regularizer on activations
double DROP;
string NAME[3] = {"Target", "Agent", "DSE"};
double OCLASS_WEIGHT[3] = {0.3, 0.8, 0.5};
double ENTITY_WEIGHT[3] = {1, 1, 1};

#ifdef DROPOUT
Matrix<double, -1, 1> dropout(Matrix<double, -1, 1> x, double p=DROP);
#endif

void readSentences(vector<vector<string > > &X,
		vector<vector<string> > &TT,
		vector<vector<string> > &AT,
		vector<vector<string> > &DT,
		string fname) {
	ifstream in(fname.c_str());
	string line;
	vector<string> x;
	vector<string> tt, at, dt; // individual sentences and labels
	while(std::getline(in, line)) {
		if (isWhitespace(line)) {
			if (x.size() != 0) {
				X.push_back(x);
				TT.push_back(tt);
				AT.push_back(at);
				DT.push_back(dt);
				x.clear();
				tt.clear();
				at.clear();
				dt.clear();
			}
		} else {
			string token, part, label;
			uint i = line.find_first_of('\t');
			token = line.substr(0, i);
			uint j = line.find_first_of('\t', i+1);
			part = line.substr(i+1,j-i-1);
			//cout << part << endl;
			i = line.find_first_of('\t', j+1);
			label = line.substr(j + 1, i - j - 1);
			tt.push_back(label);

			j = line.find_last_of('\t');
			label = line.substr(i + 1, j - i - 1);
			at.push_back(label);

			label = line.substr(j+1, line.size()-j-1);
			x.push_back(token);
			dt.push_back(label);
		}
	}
	if (x.size() != 0) {
		X.push_back(x);
		TT.push_back(tt);
		AT.push_back(at);
		DT.push_back(dt);
		x.clear();
		tt.clear();
		at.clear();
		dt.clear();
	}
}

void getRelation(vector<map<int, vector<int> > > &R, vector<int> &C,
		string fname) {
	ifstream in(fname.c_str());
	string line;
	int cnt = 0;
	map<int, vector<int> > r;
	while(std::getline(in, line)) {
		if (isWhitespace(line)) {
//			if (r.size() != 0) {
				R.push_back(r);
				r.clear();
				C.push_back(cnt);
					cnt = 0;
//			}
		} else {
			string dseID, argID;
			uint i = line.find_first_of('\t');
			dseID = line.substr(0, i);
			argID= line.substr(i+1, line.size()-i-2);
			cnt ++;
			if (r.find(toDigit(dseID)) != r.end()){
				r[toDigit(dseID)].push_back(toDigit(argID));
			}else{
				vector<int> v;
				v.push_back(toDigit(argID));
				r[toDigit(dseID)] = v;
			}
		}
	}
//	if (r.size() != 0) {
//		R.push_back(r);
//		r.clear();
//	}
}




