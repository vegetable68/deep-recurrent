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

#define DROPOUT
#define ETA 0.001
#define NORMALIZE true// keeping this false throughout my own experiments
#define OCLASS_WEIGHT 0.12
#define layers 2 // number of EXTRA (not all) hidden layers

#define MR 0.95
uint fold = -1;

using namespace Eigen;
using namespace std;

double LAMBDA = 1e-4;  // L2 regularizer on weights
double LAMBDAH = (layers > 2) ? 1e-5 : 1e-4; //L2 regularizer on activations
double DROP;

#ifdef DROPOUT
Matrix<double, -1, 1> dropout(Matrix<double, -1, 1> x, double p=DROP);
#endif

class RNN {
	public:
		RNN(uint nx, uint nhf, uint nhb, uint ny, LookupTable &LT);
		Matrix<double, 6, 2> train(vector<vector<string> > &sents,
				vector<vector<string> > &labels,
				vector<vector<string> > &validX,
				vector<vector<string> > &validL,
				vector<vector<string> > &testX,
				vector<vector<string> > &testL);
		void update();
		Matrix<double, 3, 2> testSequential(vector<vector<string> > &sents,
				vector<vector<string> > &labels);
		LookupTable *LT;
		void save(string fname);
		void load(string fname);

	private:
		void forward(const vector<string> &, int index=-1);
		void backward(const vector<string> &);

		MatrixXd (*f)(const MatrixXd& x);
		MatrixXd (*fp)(const MatrixXd& x);

		MatrixXd x,y,hf,hb, hhf[layers],hhb[layers];
		vector<string> s;

		// recurrent network params
		MatrixXd Wo, Wfo, Wbo,
				 WWfoy, WWboy;
		VectorXd bo;
		MatrixXd Wf, Vf, Wb, Vb;
		VectorXd bhf, bhb;

		MatrixXd WWff[layers], WWfb[layers], WWbb[layers], WWbf[layers];
		MatrixXd VVf[layers], VVb[layers];
		VectorXd bbhf[layers], bbhb[layers];

		MatrixXd gWo, gWfo, gWbo,
				 gWWfoy, gWWboy;
		VectorXd gbo;
		MatrixXd gWf, gVf, gWb, gVb;
		VectorXd gbhf, gbhb;

		MatrixXd gWWff[layers], gWWfb[layers], gWWbb[layers], gWWbf[layers];
		MatrixXd gVVf[layers], gVVb[layers];
		VectorXd gbbhf[layers], gbbhb[layers];

		MatrixXd vWo, vWfo, vWbo,
				 vWWfoy, vWWboy;
		VectorXd vbo;
		MatrixXd vWf, vVf, vWb, vVb;
		VectorXd vbhf, vbhb;

		MatrixXd vWWff[layers], vWWfb[layers], vWWbb[layers], vWWbf[layers];
		MatrixXd vVVf[layers], vVVb[layers];
		VectorXd vbbhf[layers], vbbhb[layers];

		uint nx, nhf, nhb, ny;
		uint epoch;

		double lr;
};

void RNN::forward(const vector<string> & s, int index) {
	VectorXd dropper;
	uint T = s.size();
	this->s = s;
	x = MatrixXd(nx, T);
	for (uint i=0; i<T; i++)
		x.col(i) = (*LT)[s[i]];

	hf = MatrixXd::Zero(nhf, T);
	hb = MatrixXd::Zero(nhb, T);

	for (uint l=0; l<layers; l++) {
		hhf[l] = MatrixXd::Zero(nhf, T);
		hhb[l] = MatrixXd::Zero(nhb, T);
	}

	MatrixXd Wfx = Wf*x + bhf*RowVectorXd::Ones(T);
	dropper = dropout(VectorXd::Ones(nhf));
	for (uint i=0; i<T; i++) {
		hf.col(i) = (i==0) ? f(Wfx.col(i)) : f(Wfx.col(i) + Vf*hf.col(i-1));
#ifdef DROPOUT
		if (index == -1)
			hf.col(i) *= (1-DROP);
		else
			hf.col(i) = hf.col(i).cwiseProduct(dropper);
#endif
	}

	MatrixXd Wbx = Wb*x + bhb*RowVectorXd::Ones(T);
	dropper = dropout(VectorXd::Ones(nhb));
	for (uint i=T-1; i!=(uint)(-1); i--) {
		hb.col(i) = (i==T-1) ? f(Wbx.col(i)) : f(Wbx.col(i) + Vb*hb.col(i+1));
#ifdef DROPOUT
		if (index == -1)
			hb.col(i) *= (1-DROP);
		else
			hb.col(i) = hb.col(i).cwiseProduct(dropper);
#endif
	}

	for (uint l=0; l<layers; l++) {
		MatrixXd *xf, *xb; // input to this layer (not to all network)
		xf = (l == 0) ? &hf : &(hhf[l-1]);
		xb = (l == 0) ? &hb : &(hhb[l-1]);

		MatrixXd WWffxf = WWff[l]* *xf + bbhf[l]*RowVectorXd::Ones(T);
		MatrixXd WWfbxb = WWfb[l]* *xb;
		dropper = dropout(VectorXd::Ones(nhf));
		for (uint i=0; i<T; i++) {
			hhf[l].col(i) = (i==0) ? f(WWffxf.col(i) + WWfbxb.col(i))
				: f(WWffxf.col(i) + WWfbxb.col(i) +
						VVf[l]*hhf[l].col(i-1));
#ifdef DROPOUT
			if (index == -1)
				hhf[l].col(i) *= (1-DROP);
			else
				hhf[l].col(i) = hhf[l].col(i).cwiseProduct(dropper);
#endif
		}

		MatrixXd WWbfxf = WWbf[l]* *xf + bbhb[l]*RowVectorXd::Ones(T);
		MatrixXd WWbbxb = WWbb[l]* *xb;
		dropper = dropout(VectorXd::Ones(nhb));
		for (uint i=T-1; i!=(uint)(-1); i--) {
			hhb[l].col(i) = (i==T-1) ? f(WWbbxb.col(i) + WWbfxf.col(i))
				: f(WWbbxb.col(i) + WWbfxf.col(i) +
						VVb[l]*hhb[l].col(i+1));
#ifdef DROPOUT
			if (index == -1)
				hhb[l].col(i) *= (1-DROP);
			else
				hhb[l].col(i) = hhb[l].col(i).cwiseProduct(dropper);
#endif
		}
	}

	// output layer uses the last hidden layer
	// you can experiment with the other version by changing this
	// (backward pass needs to change as well of course)
	y = softmax(bo*RowVectorXd::Ones(T) + WWfoy*hhf[layers-1] +
			WWboy*hhb[layers-1]);
}

void RNN::backward(const vector<string> &labels) {
	uint T = x.cols();

	MatrixXd dhb = MatrixXd::Zero(nhb, T);
	MatrixXd dhf = MatrixXd::Zero(nhf, T);

	MatrixXd dhhf[layers], dhhb[layers];
	for (uint l=0; l<layers; l++) {
		dhhf[l] = MatrixXd::Zero(nhf, T);
		dhhb[l] = MatrixXd::Zero(nhb, T);
	}

	MatrixXd yi(3,T);
	for (uint i=0; i<T; i++) {
		if (labels[i] == "O")
			yi.col(i) << 1,0,0;
		else if (labels[i] == "B")
			yi.col(i) << 0,1,0;
		else
			yi.col(i) << 0,0,1;
	}

	MatrixXd gpyd = smaxentp(y,yi);
	for (uint i=0; i<T; i++)
		if (labels[i] == "O")
			gpyd.col(i) *= OCLASS_WEIGHT;

	gWWfoy.noalias() += gpyd * hhf[layers - 1].transpose();
	gWWboy.noalias() += gpyd * hhb[layers - 1].transpose();
	gbo.noalias() += gpyd*VectorXd::Ones(T);

}


RNN::RNN(uint nx, uint nhf, uint nhb, uint ny, LookupTable &LT) {
	lr = ETA;

	this->LT = &LT;

	this->nx = nx;
	this->nhf = nhf;
	this->nhb = nhb;
	this->ny = ny;

	f = &relu;
	fp = &relup;

	// init randomly
	Wf = MatrixXd(nhf,nx).unaryExpr(ptr_fun(urand));
	Vf = MatrixXd(nhf,nhf).unaryExpr(ptr_fun(urand));
	bhf = VectorXd(nhf).unaryExpr(ptr_fun(urand));

	Wb = MatrixXd(nhb,nx).unaryExpr(ptr_fun(urand));
	Vb = MatrixXd(nhb,nhb).unaryExpr(ptr_fun(urand));
	bhb = VectorXd(nhb).unaryExpr(ptr_fun(urand));

	for (uint l=0; l<layers; l++) {
		WWff[l] = MatrixXd(nhf,nhf).unaryExpr(ptr_fun(urand));
		WWfb[l] = MatrixXd(nhf,nhb).unaryExpr(ptr_fun(urand));
		VVf[l] = MatrixXd(nhf,nhf).unaryExpr(ptr_fun(urand));
		bbhf[l] = VectorXd(nhf).unaryExpr(ptr_fun(urand));

		WWbb[l] = MatrixXd(nhb,nhb).unaryExpr(ptr_fun(urand));
		WWbf[l] = MatrixXd(nhb,nhf).unaryExpr(ptr_fun(urand));
		VVb[l] = MatrixXd(nhb,nhb).unaryExpr(ptr_fun(urand));
		bbhb[l] = VectorXd(nhb).unaryExpr(ptr_fun(urand));
	}

	Wfo = MatrixXd(ny,nhf).unaryExpr(ptr_fun(urand));
	Wbo = MatrixXd(ny,nhb).unaryExpr(ptr_fun(urand));
	WWfoy = MatrixXd(ny,nhf).unaryExpr(ptr_fun(urand));
	WWboy = MatrixXd(ny,nhf).unaryExpr(ptr_fun(urand));

	Wo = MatrixXd(ny,nx).unaryExpr(ptr_fun(urand));
	bo = VectorXd(ny).unaryExpr(ptr_fun(urand));

	gWf = MatrixXd::Zero(nhf,nx);
	gVf = MatrixXd::Zero(nhf,nhf);
	gbhf = VectorXd::Zero(nhf);

	gWb = MatrixXd::Zero(nhb,nx);
	gVb = MatrixXd::Zero(nhb,nhb);
	gbhb = VectorXd::Zero(nhb);

	for (uint l=0; l<layers; l++) {
		gWWff[l] = MatrixXd::Zero(nhf,nhf);
		gWWfb[l] = MatrixXd::Zero(nhf,nhb);
		gVVf[l] = MatrixXd::Zero(nhf,nhf);
		gbbhf[l] = VectorXd::Zero(nhf);

		gWWbb[l] = MatrixXd::Zero(nhb,nhb);
		gWWbf[l] = MatrixXd::Zero(nhb,nhf);
		gVVb[l] = MatrixXd::Zero(nhb,nhb);
		gbbhb[l] = VectorXd::Zero(nhb);
	}


	gWfo = MatrixXd::Zero(ny,nhf);
	gWbo = MatrixXd::Zero(ny,nhb);

	gWWfoy = MatrixXd::Zero(ny,nhf);
	gWWboy = MatrixXd::Zero(ny,nhb);

	gWo = MatrixXd::Zero(ny,nx);
	gbo = VectorXd::Zero(ny);

	vWf = MatrixXd::Zero(nhf,nx);
	vVf = MatrixXd::Zero(nhf,nhf);
	vbhf = VectorXd::Zero(nhf);

	vWb = MatrixXd::Zero(nhb,nx);
	vVb = MatrixXd::Zero(nhb,nhb);
	vbhb = VectorXd::Zero(nhb);

	for (uint l=0; l<layers; l++) {
		vWWff[l] = MatrixXd::Zero(nhf,nhf);
		vWWfb[l] = MatrixXd::Zero(nhf,nhb);
		vVVf[l] = MatrixXd::Zero(nhf,nhf);
		vbbhf[l] = VectorXd::Zero(nhf);

		vWWbb[l] = MatrixXd::Zero(nhb,nhb);
		vWWbf[l] = MatrixXd::Zero(nhb,nhf);
		vVVb[l] = MatrixXd::Zero(nhb,nhb);
		vbbhb[l] = VectorXd::Zero(nhb);
	}


	vWfo = MatrixXd::Zero(ny,nhf);
	vWbo = MatrixXd::Zero(ny,nhb);
	vWWfoy = MatrixXd::Zero(ny,nhf);
	vWWboy = MatrixXd::Zero(ny,nhb);

	vWo = MatrixXd::Zero(ny,nx);
	vbo = VectorXd::Zero(ny);
}

void RNN::update() {

	double lambda = LAMBDA;
	double mr = MR;
	double norm = 0;

	// regularize
	gbo.noalias() += lambda*bo;
	gWWfoy.noalias() += (lambda)*WWfoy;
	gWWboy.noalias() += (lambda)*WWboy;

//	norm += 0.1* (gWo.squaredNorm() + gbo.squaredNorm());
//	norm+= 0.1*(gWWfoy.squaredNorm() + gWWboy.squaredNorm());

	// update velocities
	vbo = 0.1*lr*gbo + mr*vbo;
	vWWfoy = 0.1*lr*gWWfoy + mr*vWWfoy;
	vWWboy = 0.1*lr*gWWboy + mr*vWWboy;

/*
	if (NORMALIZE)
		norm = (norm > 25) ? sqrt(norm/25) : 1;
	else
		norm = 1;
*/

	// update params
	bo.noalias() -= vbo;
	WWfoy.noalias() -= vWWfoy;
	WWboy.noalias() -= vWWboy;

	// reset gradients
	gbo.setZero();
	gWWfoy.setZero();
	gWWboy.setZero();

	lr *= 0.999;
	//cout << Wuo << endl;
}

void RNN::load(string fname) {
	ifstream in(fname.c_str());

	in >> nx >> nhf >> nhb >> ny;

	in >> Wf >> Vf >> bhf
		>> Wb >> Vb >> bhb;

	for (uint l=0; l<layers; l++) {
		in >> WWff[l] >> WWfb[l] >> VVf[l] >> bbhf[l]
			>> WWbb[l] >> WWbf[l] >> VVb[l] >> bbhb[l];
	}

	in >> Wfo >> Wbo;
//	in >> WWfoy >> WWboy;
//	in >> Wo >> bo;
}

void RNN::save(string fname) {
	ofstream out(fname.c_str());

	out << nx << " " << nhf << " " << nhb << " " << ny << endl;

	out << Wf << endl;
	out << Vf << endl;
	out << bhf << endl;

	out << Wb << endl;
	out << Vb << endl;
	out << bhb << endl;

	for (uint l=0; l<layers; l++) {
		out << WWff[l] << endl;
		out << WWfb[l] << endl;
		out << VVf[l] << endl;
		out << bbhf[l] << endl;

		out << WWbb[l] << endl;
		out << WWbf[l] << endl;
		out << VVb[l]  << endl;
		out << bbhb[l] << endl;
	}

	out << Wfo << endl;
	out << Wbo << endl;
	out << WWfoy << endl;
	out << WWboy << endl;

	out << Wo << endl;
	out << bo << endl;
}


Matrix<double, 6, 2>
RNN::train(vector<vector<string> > &sents,
		vector<vector<string> > &labels,
		vector<vector<string> > &validX,
		vector<vector<string> > &validL,
		vector<vector<string> > &testX,
		vector<vector<string> > &testL) {
	uint MAXEPOCH = 200;
	uint MINIBATCH = 80;

	ostringstream strS;
	strS << "models/drnt_" << layers << "_" << nhf << "_"
		<< nhf << "_" << DROP << "_"
		<< MAXEPOCH << "_" << lr << "_" << LAMBDA << "_"
		<< MR << "_" << fold;
	string fname = strS.str();

	vector<uint> perm;
	for (uint i=0; i<sents.size(); i++)
		perm.push_back(i);

	Matrix<double, 3, 2> bestVal, bestTest;
	bestVal << 0,0,0,0,0,0;

	for (epoch=0; epoch<MAXEPOCH; epoch++) {
		shuffle(perm);
		//Q: Why shuffle here? What's 'update' for?
		for (int i=0; i<sents.size(); i++) {
			forward(sents[perm[i]], perm[i]);
			backward(labels[perm[i]]);
			if ((i+1) % MINIBATCH == 0 || i == sents.size()-1)
				update();
		}
		if (epoch % 5 == 0) {
			Matrix<double, 3, 2> resVal, resTest, resVal2, resTest2;
			cout << "Epoch " << epoch << endl;

			// diagnostic
			/*
				   cout << Wf.norm() << " " << Wb.norm() << " "
				   << Vf.norm() << " " << Vb.norm() << " "
				   << Wfo.norm() << " " << Wbo.norm() << endl;
				   for (uint l=0; l<layers; l++) {
				   cout << WWff[l].norm() << " " << WWfb[l].norm() << " "
				   << WWbb[l].norm() << " " << WWbf[l].norm() << " "
				   << VVf[l].norm() << " " << VVb[l].norm() << " "
				   << WWfo[l].norm() << " " << WWbo[l].norm() << endl;
				   }
				   */
			cout << "P, R, F1:\n" << testSequential(sents, labels) << endl;
			resVal = testSequential(validX, validL);
			resTest = testSequential(testX, testL);
			cout << "P, R, F1:\n" << resVal << endl;
			cout << "P, R, F1" << endl;
			cout << resTest  << endl<< endl;
			if (bestVal(2,0) < resVal(2,0)) {
				bestVal = resVal;
				bestTest = resTest;
				save(fname);
			}
		}
	}
	Matrix<double, 6, 2> results;
	results << bestVal, bestTest;
	return results;
}

// returns soft (precision, recall, F1) per expression
// counts proportional overlap & binary overlap
Matrix<double, 3, 2> RNN::testSequential(vector<vector<string> > &sents,
		vector<vector<string> > &labels) {
	uint nExprPredicted = 0;
	double nExprPredictedCorrectly = 0;
	uint nExprTrue = 0;
	double precNumerProp = 0, precNumerBin = 0;
	double recallNumerProp = 0, recallNumerBin = 0;
	int tot = 0;
	for (uint i=0; i<sents.size(); i++) { // per sentence
		vector<string> labelsPredicted;
		forward(sents[i]);

		for (uint j=0; j<sents[i].size(); j++) {
			uint maxi = argmax(y.col(j));
			if (maxi == 0)
				labelsPredicted.push_back("O");
			else if (maxi == 1)
				labelsPredicted.push_back("B");
			else
				labelsPredicted.push_back("I");
		}
		assert(labelsPredicted.size() == y.cols());

		string y, t, py="", pt="";
		uint match = 0;
		uint exprSize = 0;
		vector<pair<uint,uint> > pred, tru;
		int l1=-1, l2=-1;

		if (labels[i].size() != labelsPredicted.size())
			cout << labels[i].size() << " " << labelsPredicted.size() << endl;
		for (uint j=0; j<labels[i].size(); j++) { // per token in a sentence
			tot ++;
			t = labels[i][j];
			y = labelsPredicted[j];

			if (t == "B") {
				//nExprTrue++;
				if (l1 != -1)
					tru.push_back(make_pair(l1,j));
				l1 = j;
			} else if (t == "I") {
				assert(l1 != -1);
			} else if (t == "O") {
				if (l1 != -1)
					tru.push_back(make_pair(l1,j));
				l1 = -1;
			} else
				assert(false);

			if ((y == "B") || ((y == "I") && ((py == "") || (py == "O")))) {
				nExprPredicted++;
				if (l2 != -1)
					pred.push_back(make_pair(l2,j));
				l2 = j;
			} else if (y == "I") {
				assert(l2 != -1);
			} else if (y == "O") {
				if (l2 != -1)
					pred.push_back(make_pair(l2,j));
				l2 = -1;
			} else {
				cout << y << endl;
				assert(false);
			}

			py = y;
			pt = t;
		}
		if ((l1 != -1) && (l1 != labels[i].size()))
			tru.push_back(make_pair(l1,labels[i].size()));
		if ((l2 != -1) && (l2 != labels[i].size()))
			pred.push_back(make_pair(l2,labels[i].size()));

		vector<bool> trum = vector<bool>(tru.size(),false);
		vector<bool> predm = vector<bool>(pred.size(),false);
		for (uint a=0; a<tru.size(); a++) {
			pair<uint,uint> truSpan = tru[a];
			nExprTrue++;
			for (uint b=0; b<pred.size(); b++) {
				pair<uint,uint> predSpan = pred[b];

				uint lmax, rmin;
				if (truSpan.first > predSpan.first)
					lmax = truSpan.first;
				else
					lmax = predSpan.first;
				if (truSpan.second < predSpan.second)
					rmin = truSpan.second;
				else
					rmin = predSpan.second;

				uint overlap = 0;
				if (rmin > lmax)
					overlap = rmin-lmax;
				if (predSpan.second == predSpan.first) cout << predSpan.first << endl;
				assert(predSpan.second != predSpan.first);
				precNumerProp += (double)overlap/(predSpan.second-predSpan.first);
				recallNumerProp += (double)overlap/(truSpan.second-truSpan.first);
				if (!predm[b] && overlap > 0) {
					precNumerBin += (double)(overlap>0);
					predm[b] = true;
				}
				if (!trum[a] && overlap>0) {
					recallNumerBin += 1;
					trum[a]=true;
				}
			}
		}

	}
	double precisionProp = (nExprPredicted==0) ? 1 : precNumerProp/nExprPredicted;
	double recallProp = recallNumerProp/nExprTrue;
	double f1Prop = (2*precisionProp*recallProp)/(precisionProp+recallProp);
	double precisionBin = (nExprPredicted==0) ? 1 : precNumerBin/nExprPredicted;
	double recallBin = recallNumerBin/nExprTrue;
	double f1Bin = (2*precisionBin*recallBin)/(precisionBin+recallBin);
	Matrix<double, 3, 2> results;
	results << precisionProp, precisionBin,
			recallProp, recallBin,
			f1Prop, f1Bin;
	return results;
}

#ifdef DROPOUT
Matrix<double, -1, 1> dropout(Matrix<double, -1, 1> x, double p) {
	for (uint i=0; i<x.size(); i++) {
		if ((double)rand()/RAND_MAX < p)
			x(i) = 0;
	}
	return x;
}
#endif

void readSentences(vector<vector<string > > &X,
		vector<vector<string> > &T, string fname) {
	ifstream in(fname.c_str());
	string line;
	vector<string> x;
	vector<string> t; // individual sentences and labels
	while(std::getline(in, line)) {
		if (isWhitespace(line)) {
			if (x.size() != 0) {
				X.push_back(x);
				T.push_back(t);
				x.clear();
				t.clear();
			}
		} else {
			line = line.substr(0, line.size() - 1);
			string token, part, label;
			uint i = line.find_first_of('\t');
			token = line.substr(0, i);
			uint j = line.find_first_of('\t', i+1);
			part = line.substr(i+1,j-i-1);
			//cout << part << endl;
			i = line.find_last_of('\t');
			label = line.substr(i+1, line.size()-i-1);
			x.push_back(token);
			t.push_back(label);
		}
	}
	if (x.size() != 0) {
		X.push_back(x);
		T.push_back(t);
		x.clear();
		t.clear();
	}
}

int main(int argc, char **argv) {

	fold = atoi(argv[1]); // between 0-9
	srand(135);
	cout << setprecision(6);

	LookupTable LT;
	// i used mikolov's word2vec (300d) for my experiments, not CW
	LT.load("glove.6B.300d.txt", 400000, 300, true);
	vector<vector<string> > X;
	vector<vector<string> > T;
	readSentences(X, T, "target.txt"); // dse.txt or ese.txt

	unordered_map<string, set<uint> > sentenceIds;
	set<string> allDocs; //Store the name of all docs
	ifstream in("sentenceid.txt");
	string line;
	uint numericId = 0;

	//map the numeric Id to the file locations
	while(getline(in, line)) {
		vector<string> s = split(line, ' ');
		//	cout<<numericId<<endl;
		assert(s.size() == 3);
		string strId = s[2];

		if (sentenceIds.find(strId) != sentenceIds.end()) {
			sentenceIds[strId].insert(numericId);
		} else {
			sentenceIds[strId] = set<uint>();
			sentenceIds[strId].insert(numericId);
		}
		numericId++;
	}

	vector<vector<string> > trainX, validX, testX;
	vector<vector<string> > trainL, validL, testL;
	vector<bool> isUsed(X.size(), false);

	ifstream in4("datasplit/doclist.mpqaOriginalSubset");
	while(getline(in4, line))
		allDocs.insert(line);

	ifstream in2("datasplit/filelist_train"+to_string(fold));
	//10 fold
	while(getline(in2, line)) {
		for (const auto &id : sentenceIds[line]) {
			trainX.push_back(X[id]);
			trainL.push_back(T[id]);
		}
		allDocs.erase(line);
	}
	ifstream in3("datasplit/filelist_test"+to_string(fold));
	while(getline(in3, line)) {
		for (const auto &id : sentenceIds[line]) {
			testX.push_back(X[id]);
			testL.push_back(T[id]);
		}
		allDocs.erase(line);
	}

	uint validSize = 0;
	for (const auto &doc : allDocs) {
		for (const auto &id : sentenceIds[doc]) {
			validX.push_back(X[id]);
			validL.push_back(T[id]);
		}
	}

	cout << X.size() << " " << trainX.size() << " " << testX.size() << endl;
	cout << "Valid size: " << validX.size() << endl;

	Matrix<double, 6, 2> best = Matrix<double, 6, 2>::Zero();
	double bestDrop;
	for (DROP=0; DROP<0.1; DROP+=0.2) { // can use this loop for CV
		RNN brnn(300,25,25,3,LT);
//		brnn.load("dse_para/model.txt");
		auto results = brnn.train(trainX, trainL, validX, validL, testX, testL);
		if (best(2,0) < results(2,0)) { // propF1 on val set
			best = results;
			bestDrop = DROP;
		}
		brnn.save("output/model_target.txt");
	}
	cout << "Best: " << endl;
	cout << "Drop: " << bestDrop << endl;
	cout << best << endl;

	return 0;
}

