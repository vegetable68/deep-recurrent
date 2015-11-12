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

class RNN {
	public:
	RNN(){}
	RNN(uint nx, uint nhf, uint nhb, uint ny, LookupTable &LT);
	int INDEX;
	void update();
	Matrix<double, 3, 2> testSequential(const vector<vector<string> > &sents,
			const vector<vector<vector<string> > > &labels);
	LookupTable *LT;
	void save(string fname);
	void load(string fname);
	void forward(const vector<string> &, int index=-1);
	void backward(const vector<vector<string> > &);
	void output(ostringstream &s, int MAXEPOCH);
	void present(const vector<vector<string> > &sents, const vector<vector<vector<string> > > &labels);
	void diagonize();


	private:

	MatrixXd (*f)(const MatrixXd& x);
	MatrixXd (*fp)(const MatrixXd& x);

	MatrixXd x,y,hf,hb, hhf[layers],hhb[layers];
	vector<string> s;

	// recurrent network params
	MatrixXd Wo, Wfo, Wbo, WWfo[layers - 1], WWbo[layers - 1], WWfoy, WWboy;
	VectorXd bo;
	MatrixXd Wf, Vf, Wb, Vb;
	VectorXd bhf, bhb;

	MatrixXd WWff[layers], WWfb[layers], WWbb[layers], WWbf[layers];
	MatrixXd VVf[layers], VVb[layers];
	VectorXd bbhf[layers], bbhb[layers];

	MatrixXd gWo, gWfo, gWbo, gWWfo[layers - 1], gWWbo[layers - 1], gWWfoy, gWWboy;
	VectorXd gbo;
	MatrixXd gWf, gVf, gWb, gVb;
	VectorXd gbhf, gbhb;

	MatrixXd gWWff[layers], gWWfb[layers], gWWbb[layers], gWWbf[layers];
	MatrixXd gVVf[layers], gVVb[layers];
	VectorXd gbbhf[layers], gbbhb[layers];

	MatrixXd vWo, vWfo, vWbo, vWWfo[layers - 1], vWWbo[layers - 1], vWWfoy, vWWboy;
	VectorXd vbo;
	MatrixXd vWf, vVf, vWb, vVb;
	VectorXd vbhf, vbhb;

	MatrixXd vWWff[layers], vWWfb[layers], vWWbb[layers], vWWbf[layers];
	MatrixXd vVVf[layers], vVVb[layers];
	VectorXd vbbhf[layers], vbbhb[layers];

	uint nx, nhf, nhb, ny;
	uint epoch;

	double lr, ylr;
};

class AnsPair{
	public:
		AnsPair(MatrixXd _dseSpanf, MatrixXd _argSpanf, MatrixXd _dseSpanb, MatrixXd _argSpanb, double _ans);
		MatrixXd dseSpanf, argSpanf, dseSpanb, argSpanb;
		double ans;
};

AnsPair::AnsPair(MatrixXd _dseSpanf, MatrixXd _argSpanf, MatrixXd _dseSpanb, MatrixXd _argSpanb, double _ans){
	dseSpanf = _dseSpanf; argSpanf = _argSpanf;
	dseSpanb = _dseSpanb; argSpanb = _argSpanb;
	ans = _ans;
}
/*
int Classifier::determineID(int INDEX, int start, int end, vector<vector<string> > labels){
	vector<int> beginSet, endSet;
	//cerr<<"determine"<<endl;
	bool inProcess = false;
	for (int j = 0; j < labels[INDEX].size(); j ++){
		if (labels[INDEX][j] == "O"){
			if (inProcess) endSet.push_back(j);
			inProcess = false;
		}
		if (labels[INDEX][j] == "B"){
			if (inProcess) endSet.push_back(j);
			inProcess = true;
			beginSet.push_back(j);
		}
		assert(labels[INDEX][j] != "I" || inProcess);
		//	if (labels[INDEX][j] == "I") inProcess = true;
	}
	if (inProcess) endSet.push_back(labels[INDEX].size());
	assert(endSet.size() == beginSet.size());
	//cerr<<"middle"<<endl;

	int overlap = 0, id = -1;
	for (int k = 0; k < beginSet.size(); k ++){
		int curLap = min(endSet[k], end) - max(beginSet[k], start);
		if (curLap > overlap && curLap >= 0) {overlap = curLap; id = k;}
	}
	//cerr<<"end of process"<<endl;
	return id;
}
*/

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
	y = softmax(bo*RowVectorXd::Ones(T) + WWfoy *hhf[layers-1] +
			WWboy *hhb[layers-1]);
	//cout<<bo[0].norm()<<" "<<WWfoy[0].norm()<<WWboy[0].norm()<<endl;
}

void RNN::present(const vector<vector<string> > &sents,
		const vector<vector<vector<string> > > &labels){

	for (uint i=0; i<sents.size(); i++) { // per sentence
		forward(sents[i]);
		for (int j = 0; j < sents[i].size(); j++) cout<<setw(15)<<sents[i][j];
		cout<<endl;

		vector<string> labelsPredicted;
		for (uint j=0; j<sents[i].size(); j++) {
			uint maxi = argmax(y.col(j));
			if (maxi == 0)
				labelsPredicted.push_back("O");
			else if (maxi == 1)
				labelsPredicted.push_back("B");
			else
				labelsPredicted.push_back("I");
		}
		cout<<NAME[INDEX]<<":"<<endl;
		for (int j = 0; j < sents[i].size(); j++) cout<<setw(15)<<labelsPredicted[j];
		cout<<endl;
		for (int j = 0; j < sents[i].size(); j++) cout<<setw(15)<<labels[i][INDEX][j];
		cout<<endl;
		cout<<endl;
	}
}


void RNN::backward(const vector<vector<string> > &labels) {
	uint T = x.cols();

	MatrixXd dhb = MatrixXd::Zero(nhb, T);
	MatrixXd dhf = MatrixXd::Zero(nhf, T);

	MatrixXd dhhf[layers], dhhb[layers];
	for (uint l=0; l<layers; l++) {
		dhhf[l] = MatrixXd::Zero(nhf, T);
		dhhb[l] = MatrixXd::Zero(nhb, T);
	}

	MatrixXd yi;
	yi = MatrixXd(3, T);
	for (uint i=0; i<T; i++) {
		if (labels[INDEX][i] == "O")
			yi.col(i) << 1,0,0;
		else if (labels[INDEX][i] == "B")
			yi.col(i) << 0,1,0;
		else
			yi.col(i) << 0,0,1;
	}
	//  cout<<"tag0"<<endl;

	MatrixXd gpyd;
	gpyd = smaxentp(y,yi);
	for (uint i=0; i<T; i++)
		if (labels[INDEX][i] == "O")
			gpyd.col(i) *= OCLASS_WEIGHT[INDEX];
	//  cout<<"gpyd:"<<gpyd[0]<<endl;

	//  cout<<"tag1"<<endl;

	gWWfoy.noalias() += gpyd * hhf[layers - 1].transpose();
	gWWboy.noalias() += gpyd * hhb[layers - 1].transpose();
	gbo.noalias() += gpyd*VectorXd::Ones(T);

	//  cout<<"tag2"<<endl;
	dhhf[layers - 1].noalias() += WWfoy.transpose() * gpyd * ENTITY_WEIGHT[INDEX];
	dhhb[layers - 1].noalias() += WWboy.transpose() * gpyd * ENTITY_WEIGHT[INDEX];

	//  cout<<"tag3"<<endl;
	// activation regularize
	dhf.noalias() += LAMBDAH*hf;
	dhb.noalias() += LAMBDAH*hb;
	for (uint l=0; l<layers; l++) {
		dhhf[l].noalias() += LAMBDAH*hhf[l];
		dhhb[l].noalias() += LAMBDAH*hhb[l];
	}

	for (uint l=layers-1; l != (uint)(-1); l--) {
		MatrixXd *dxf, *dxb, *xf, *xb;
		dxf = (l == 0) ? &dhf : &(dhhf[l-1]);
		dxb = (l == 0) ? &dhb : &(dhhb[l-1]);
		xf = (l == 0) ? &hf : &(hhf[l-1]);
		xb = (l == 0) ? &hb : &(hhb[l-1]);

		MatrixXd fphdh = MatrixXd::Zero(nhf,T);
		for (uint i=T-1; i != (uint)(-1); i--) {
			fphdh.col(i) = fp(hhf[l].col(i)).cwiseProduct(dhhf[l].col(i));
			if (i > 0) {
				gVVf[l].noalias() += fphdh.col(i) * hhf[l].col(i-1).transpose();
				dhhf[l].col(i-1).noalias() += VVf[l].transpose() * fphdh.col(i);
			}
		}
		gWWff[l].noalias() += fphdh * xf->transpose();
		gWWfb[l].noalias() += fphdh * xb->transpose();
		gbbhf[l].noalias() += fphdh * VectorXd::Ones(T);
		dxf->noalias() += WWff[l].transpose() * fphdh;
		dxb->noalias() += WWfb[l].transpose() * fphdh;

		fphdh = MatrixXd::Zero(nhb,T);
		for (uint i=0; i < T; i++) {
			fphdh.col(i) = fp(hhb[l].col(i)).cwiseProduct(dhhb[l].col(i));
			if (i < T-1) {
				dhhb[l].col(i+1).noalias() += VVb[l].transpose() * fphdh.col(i);
				gVVb[l].noalias() += fphdh.col(i) * hhb[l].col(i+1).transpose();
			}
		}
		gWWbb[l].noalias() += fphdh * xb->transpose();
		gWWbf[l].noalias() += fphdh * xf->transpose();
		gbbhb[l].noalias() += fphdh * VectorXd::Ones(T);
		dxf->noalias() += WWbf[l].transpose() * fphdh;
		dxb->noalias() += WWbb[l].transpose() * fphdh;
	}

	for (uint i=T-1; i != 0; i--) {
		VectorXd fphdh = fp(hf.col(i)).cwiseProduct(dhf.col(i));
		gWf.noalias() += fphdh * x.col(i).transpose();
		gVf.noalias() += fphdh * hf.col(i-1).transpose();
		gbhf.noalias() += fphdh;
		dhf.col(i-1).noalias() += Vf.transpose() * fphdh;
	}
	//  cout<<"tag4"<<endl;
	VectorXd fphdh = fp(hf.col(0)).cwiseProduct(dhf.col(0));
	gWf.noalias() += fphdh * x.col(0).transpose();
	gbhf.noalias() += fphdh;

	for (uint i=0; i < T-1; i++) {
		VectorXd fphdh = fp(hb.col(i)).cwiseProduct(dhb.col(i));
		gWb.noalias() += fphdh * x.col(i).transpose();
		gVb.noalias() += fphdh * hb.col(i+1).transpose();
		gbhb.noalias() += fphdh;
		dhb.col(i+1).noalias() += Vb.transpose() * fphdh;
	}
	fphdh = fp(hb.col(T-1)).cwiseProduct(dhb.col(T-1));
	gWb.noalias() += fphdh * x.col(T-1).transpose();
	gbhb.noalias() += fphdh;
}

RNN::RNN(uint nx, uint nhf, uint nhb, uint ny, LookupTable &LT) {
	lr = ETA;
	ylr = ETA;

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
	for (uint l=0; l<layers - 1; l++) {
		WWfo[l] = MatrixXd(ny,nhf).unaryExpr(ptr_fun(urand));
		WWbo[l] = MatrixXd(ny,nhb).unaryExpr(ptr_fun(urand));
	}
	bo = VectorXd(ny).unaryExpr(ptr_fun(urand));
	WWfoy = MatrixXd(ny,nhf).unaryExpr(ptr_fun(urand));
	WWboy = MatrixXd(ny,nhb).unaryExpr(ptr_fun(urand));
	Wo = MatrixXd(ny,nx).unaryExpr(ptr_fun(urand));

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
	for (uint l=0; l<layers - 1; l++) {
		gWWfo[l] = MatrixXd::Zero(ny,nhf);
		gWWbo[l] = MatrixXd::Zero(ny,nhb);
	}
	gWWfoy = MatrixXd::Zero(ny,nhf);
	gWWboy = MatrixXd::Zero(ny,nhb);
	gbo = VectorXd::Zero(ny);
	gWo = MatrixXd::Zero(ny,nx);

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
	for (uint l=0; l<layers - 1; l++) {
		vWWfo[l] = MatrixXd::Zero(ny,nhf);
		vWWbo[l] = MatrixXd::Zero(ny,nhb);
	}
	vWWfoy = MatrixXd::Zero(ny,nhf);
	vWWboy = MatrixXd::Zero(ny,nhb);
	vbo = VectorXd::Zero(ny);
	vWo = MatrixXd::Zero(ny,nx);
}

void RNN::update() {

	double lambda = LAMBDA;
	double mr = MR;
	double norm = 0;

	// regularize
	gbo.noalias() += lambda*bo;
	gWWfoy.noalias() += (lambda)*WWfoy;
	gWWboy.noalias() += (lambda)*WWboy;

	norm += 0.1* (gWo.squaredNorm());
	norm += 0.1 *(gWWfoy.squaredNorm() + gWWboy.squaredNorm() + gbo.squaredNorm());
	for (uint l=0; l<layers - 1; l++)
		norm+= 0.1*(gWWfo[l].squaredNorm() + gWWbo[l].squaredNorm());

	gWf.noalias() += lambda*Wf;
	gVf.noalias() += lambda*Vf;
	gWb.noalias() += lambda*Wb;
	gVb.noalias() += lambda*Vb;
	gbhf.noalias() += lambda*bhf;
	gbhb.noalias() += lambda*bhb;

	norm += gWf.squaredNorm() + gVf.squaredNorm()
		+ gWb.squaredNorm() + gWf.squaredNorm()
		+ gbhf.squaredNorm() + gbhb.squaredNorm();

	for (uint l=0; l<layers; l++) {
		gWWff[l].noalias() += lambda*WWff[l];
		gWWfb[l].noalias() += lambda*WWfb[l];
		gWWbf[l].noalias() += lambda*WWbf[l];
		gWWbb[l].noalias() += lambda*WWbb[l];
		gVVf[l].noalias() += lambda*VVf[l];
		gVVb[l].noalias() += lambda*VVb[l];
		gbbhf[l].noalias() += lambda*bbhf[l];
		gbbhb[l].noalias() += lambda*bbhb[l];

		norm += gWWff[l].squaredNorm() + gWWfb[l].squaredNorm()
			+ gWWbf[l].squaredNorm() + gWWbb[l].squaredNorm()
			+ gVVf[l].squaredNorm() + gVVb[l].squaredNorm()
			+ gbbhf[l].squaredNorm() + gbbhb[l].squaredNorm();

	}

	// update velocities
	vbo = 0.1*lr*gbo + mr*vbo;
	vWWfoy = 0.1*lr*gWWfoy + mr*vWWfoy;
	vWWboy = 0.1*lr*gWWboy + mr*vWWboy;

	//cout<<"update:"<<gWf<<" norm:"<<norm<<endl;
	if (NORMALIZE)
		norm = (norm > 25) ? sqrt(norm/25) : 1;
	else
		norm = 1;


	vWf = lr*gWf/norm + mr*vWf;
	vVf = lr*gVf/norm + mr*vVf;
	vWb = lr*gWb/norm + mr*vWb;
	vVb = lr*gVb/norm + mr*vVb;
	vbhf = lr*gbhf/norm + mr*vbhf;
	vbhb = lr*gbhb/norm + mr*vbhb;

	for (uint l=0; l<layers; l++) {
		vWWff[l] = lr*gWWff[l]/norm + mr*vWWff[l];
		vWWfb[l] = lr*gWWfb[l]/norm + mr*vWWfb[l];
		vVVf[l] = lr*gVVf[l]/norm + mr*vVVf[l];
		vWWbb[l] = lr*gWWbb[l]/norm + mr*vWWbb[l];
		vWWbf[l] = lr*gWWbf[l]/norm + mr*vWWbf[l];
		vVVb[l] = lr*gVVb[l]/norm + mr*vVVb[l];
		vbbhf[l] = lr*gbbhf[l]/norm + mr*vbbhf[l];
		vbbhb[l] = lr*gbbhb[l]/norm + mr*vbbhb[l];
	}

	// update params
	bo.noalias() -= vbo;
	WWfoy.noalias() -= vWWfoy;
	WWboy.noalias() -= vWWboy;

	Wf.noalias() -= vWf;
	Vf.noalias() -= vVf;
	Wb.noalias() -= vWb;
	Vb.noalias() -= vVb;
	bhf.noalias() -= vbhf;
	bhb.noalias() -= vbhb;

	for (uint l=0; l<layers; l++) {
		WWff[l].noalias() -= vWWff[l];
		WWfb[l].noalias() -= vWWfb[l];
		VVf[l].noalias() -= vVVf[l];
		WWbb[l].noalias() -= vWWbb[l];
		WWbf[l].noalias() -= vWWbf[l];
		VVb[l].noalias() -= vVVb[l];
		bbhf[l].noalias() -= vbbhf[l];
		bbhb[l].noalias() -= vbbhb[l];
	}

	// reset gradients
	gbo.setZero();
	gWWfoy.setZero();
	gWWboy.setZero();

	gWf.setZero();
	gVf.setZero();
	gWb.setZero();
	gVb.setZero();
	gbhf.setZero();
	gbhb.setZero();

	for (uint l=0; l<layers; l++) {
		gWWff[l].setZero();
		gWWfb[l].setZero();
		gVVf[l].setZero();
		gWWbb[l].setZero();
		gWWbf[l].setZero();
		gVVb[l].setZero();
		gbbhf[l].setZero();
		gbbhb[l].setZero();
	}

	lr *= DR;
	ylr *= DR;
	//cout << Wuo << endl;
	/*cout<<"diagonize:"<<endl;
	  cout << Wf.norm() << " " << Wb.norm() << " "
	  << Vf.norm() << " " << Vb.norm() << " "
	  << Wfo.norm() << " " << Wbo.norm() << endl;
	  for (uint l=0; l<layers; l++) {
	  cout << WWff[l].norm() << " " << WWfb[l].norm() << " "
	  << WWbb[l].norm() << " " << WWbf[l].norm() << " "
	  << VVf[l].norm() << " " << VVb[l].norm() << " "
	  << WWfo[l].norm() << " " << WWbo[l].norm() << endl;
	  }
	  cout<<NAME[INDEX]<<":"<<vWWfoy.norm()<<" "<<vWWboy.norm()<<bo.norm()<<endl;*/
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
	for (uint l=0; l<layers; l++)
		in >> WWfo[l] >> WWbo[l];
	in>>WWfoy>>WWboy>>bo;
	in >> Wo;
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
	for (uint l=0; l<layers - 1; l++) {
		out << WWfo[l] << endl;
		out << WWbo[l] << endl;
	}
	out<<WWfoy<<endl;
	out<<WWboy<<endl;
	out<<bo<<endl;
	out << Wo << endl;
}

void RNN::output(ostringstream &strS, int MAXEPOCH){
	strS << "models/drnt_" << layers << "_" << nhf << "_"
		<< nhf << "_" << DROP << "_"
		<< MAXEPOCH << "_" << lr << "_" << LAMBDA << "_"
		<< MR << "_" << fold;
}

void RNN::diagonize(){
	cout<<Wf<<endl;
	cout << Wf.norm() << " " << Wb.norm() << " "
		<< Vf.norm() << " " << Vb.norm() << " "
		<< Wfo.norm() << " " << Wbo.norm() << endl;
	for (uint l=0; l<layers; l++) {
		cout << WWff[l].norm() << " " << WWfb[l].norm() << " "
			<< WWbb[l].norm() << " " << WWbf[l].norm() << " "
			<< VVf[l].norm() << " " << VVb[l].norm() << " "
			<< WWfo[l].norm() << " " << WWbo[l].norm() << endl;
	}
	cout<<WWfoy.norm()<<" "<<WWboy.norm()<<" "<<bo.norm()<<endl;
}

vector<Matrix<double, 3, 2>>
train(RNN brnn[3],
		const vector<vector<string> > &sents,
		const vector<vector<vector<string> > > &labels,
		const vector<vector<map<int, vector<int> > > > &relation,
		const vector<vector<int> > &cnt,

		const vector<vector<string> > &validX,
		const vector<vector<vector<string> > > &validL,
		const vector<vector<map<int, vector<int> > > > &validR,
		const vector<vector<int> > &validC,

		const vector<vector<string> > &testX,
		const vector< vector<vector<string> > > &testL,
		const vector<vector<map<int, vector<int> > > > &testR,
		const vector<vector<int> > &testC) {
	uint MAXEPOCH = 200;
	uint MINIBATCH = 80;
	string fname[3];
	ostringstream strS;
	for (int j = 0; j < 3; j ++){
		brnn[j].output(strS, MAXEPOCH);
		fname[j] = strS.str();
	}

	vector<uint> perm;
	for (int j = 0; j < 3; j ++) brnn[j].INDEX = j;
	for (uint i=0; i<sents.size(); i++)
		perm.push_back(i);

	vector<Matrix<double, 3, 2>> bestVal, bestTest;
	Matrix<double, 3, 2> tmp;
	tmp<<0,0,0,0,0,0;
	for (uint k = 0; k < 3; k ++){
		bestVal.push_back(tmp);
		bestTest.push_back(tmp);
	}

	//cout<<Wf.norm()<<endl;
	//	cout<<"Start training"<<endl;
	for (uint epoch=0; epoch<MAXEPOCH; epoch++) {
		shuffle(perm);
		for (int i=0; i<sents.size(); i++) {
			//cerr<<i<<endl;
			for (int j = 0; j < 3; j ++) brnn[j].forward(sents[perm[i]], perm[i]);
			//		cout<<"fwd"<<endl;
			for (int j = 0; j < 3; j ++) brnn[j].backward(labels[perm[i]]);
			//		cout<<"bwd"<<endl;
			if ((i+1) % MINIBATCH == 0 || i == sents.size()-1)
				for (int j = 0; j < 3; j ++) brnn[j].update();
		}

		if (epoch % 5 == 0) {
			agent_dse.clear(25, 0.1); target_dse.clear(25, 0.1);

			agent_dse.train(brnn[1], brnn[2], sents, labels, relation);
			target_dse.train(brnn[0], brnn[2], sents, labels, relation);

			Matrix<double, 3, 2> resVal, resTest, resVal2, resTest2;
			cout << "Epoch " << epoch << endl;

			// diagnostic
			//			brnn[0].diagonize();
			/*
			   cout<<Wf<<endl;
			   cout << Wf.norm() << " " << Wb.norm() << " "
			   << Vf.norm() << " " << Vb.norm() << " "
			   << Wfo.norm() << " " << Wbo.norm() << endl;
			   for (uint l=0; l<layers; l++) {
			   cout << WWff[l].norm() << " " << WWfb[l].norm() << " "
			   << WWbb[l].norm() << " " << WWbf[l].norm() << " "
			   << VVf[l].norm() << " " << VVb[l].norm() << " "
			   << WWfo[l].norm() << " " << WWbo[l].norm() << endl;
			   }
			   for (uint k = 0; k < 3; k ++) cout<<WWfoy[k].norm()<<" "<<WWboy[k].norm()<<bo[k].norm()<<endl;
			   */
			for (int j = 0; j < 3; j ++){
				Matrix<double, 3, 2> res;
				res = brnn[j].testSequential(sents, labels);
				cout << "Train:\n";
				cout<<NAME[j]<<":\n"<<res<<endl;

				resVal = brnn[j].testSequential(validX, validL);
				cout << "Val:\n";
				cout<<NAME[j]<<":\n"<<resVal<<endl;

				resTest = brnn[j].testSequential(testX, testL);
				//cerr<<"tested"<<endl;
				/*			cout << "Test:\n";
							for (int k = 0; k < 3; k ++)
							cout<<NAME[k]<<":\n"<<resTest[k]<<endl;*/

				if (bestVal[j](2,0) < resVal(2,0)) {
					bestVal[j] = resVal;
					bestTest[j] = resTest;
					brnn[j].save(fname[j]);
				}
				//cerr<<NAME[j]<<endl;
			}
			cout<<"is from:"<<endl;
			cout<<"Train:"<<agent_dse.test(brnn[1], brnn[2], sents, labels, relation, cnt)<<endl;
			cout<<"Val:"<<agent_dse.test(brnn[1], brnn[2], validX, validL, validR, validC)<<endl;
			cout<<"Test:"<<agent_dse.test(brnn[1],brnn[2], testX, testL, testR, testC)<<endl;

			cout<<"is about:"<<endl;
			cout<<"Train:"<<target_dse.test(brnn[1], brnn[2], sents, labels, relation, cnt)<<endl;
			cout<<"Val:"<<target_dse.test(brnn[1], brnn[2], validX, validL, validR, validC)<<endl;
			cout<<"Test:"<<target_dse.test(brnn[1], brnn[2], testX, testL, testR, testC)<<endl;

			/*			cout<<"Diagonize:"<<endl;
						cout<<"is from:"<<endl; agent_dse.diag();
						cout<<"is about:"<<endl; target_dse.diag(); */
		}
		//		cout<<epoch<<endl;
	}
	//	cout<<"epoch ends"<<endl;
	for (int i = 0; i < 3; i ++) bestVal.push_back(bestTest[i]);
	/*	cout<<"Train:"<<endl;
		for (int j = 0; j < 3; j ++) brnn[j].present(sents, labels);
		cout<<"Valid:"<<endl;
		for (int j = 0; j < 3; j ++) brnn[j].present(validX, validL);
		cout<<"Test:"<<endl;
		for (int j = 0; j < 3; j ++) brnn[j].present(testX, testL); */

		//	cout<<"ready to return"<<endl;
		return bestVal;
}

// returns soft (precision, recall, F1) per expression
// counts proportional overlap & binary overlap
Matrix<double, 3, 2> RNN::testSequential(const vector<vector<string> > &sents,
		const vector<vector<vector<string> > > &labels) {

	Matrix<double, 3, 2> results;
	Matrix<double, 3, 2> tmp;

	uint nExprPredicted = 0;
	double nExprPredictedCorrectly = 0;
	uint nExprTrue = 0;
	double precNumerProp = 0, precNumerBin = 0;
	double recallNumerProp = 0, recallNumerBin = 0;
	int tot = 0;

	for (uint i=0; i<sents.size(); i++) { // per sentence
		vector<string> labelsPredicted;
		//	cerr<<"fwd"<<endl;
		forward(sents[i]);
		//	cerr<<"fwd fin"<<endl;

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

		if (labels[i][INDEX].size() != labelsPredicted.size())
			cout << labels[i][INDEX].size() << " " << labelsPredicted.size() << endl;
		for (uint j=0; j<labels[i][INDEX].size(); j++) { // per token in a sentence
			tot ++;
			t = labels[i][INDEX][j];
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
		if ((l1 != -1) && (l1 != labels[i][INDEX].size()))
			tru.push_back(make_pair(l1,labels[i][INDEX].size()));
		if ((l2 != -1) && (l2 != labels[i][INDEX].size()))
			pred.push_back(make_pair(l2,labels[i][INDEX].size()));

		vector<bool> trum = vector<bool>(tru.size(),false);
		vector<bool> predm = vector<bool>(pred.size(),false);
		//	cerr<<tru.size()<<endl;
		//	cerr<<pred.size()<<endl;
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

	//cerr<<nExprPredicted<<" "<<nExprTrue<<endl;
	double precisionProp = (nExprPredicted==0) ? 1 : precNumerProp/nExprPredicted;
	double recallProp = recallNumerProp/nExprTrue;
	double f1Prop = (2*precisionProp*recallProp)/(precisionProp+recallProp);
	double precisionBin = (nExprPredicted==0) ? 1 : precNumerBin/nExprPredicted;
	double recallBin = recallNumerBin/nExprTrue;
	double f1Bin = (2*precisionBin*recallBin)/(precisionBin+recallBin);
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
			//cerr<<label<<" ";

			j = line.find_last_of('\t');
			label = line.substr(i + 1, j - i - 1);
			at.push_back(label);
			//cerr<<label<<" ";

			label = line.substr(j+1, line.size()-j-1);
			x.push_back(token);
			dt.push_back(label);
			//cerr<<label<<endl;
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


int main(int argc, char **argv) {

	fold = atoi(argv[1]); // between 0-9
	srand(time(0));
	cout << setprecision(6);

	LookupTable LT;
	// i used mikolov's word2vec (300d) for my experiments, not CW
	LT.load("embeddings-original.EMBEDDING_SIZE=25.txt", 268810, 25, false);
	vector<vector<string> > X;
	vector<vector<string> > T[3];
	vector<map<int, vector<int> > >  R[2];
	vector<int> C[2];
	readSentences(X, T[0], T[1], T[2], "all.txt"); // dse.txt or ese.txt
	for (int i = 0; i < 2; i ++){
		string fname = "relation";
		fname = fname + char('0' + i);
		fname = fname + ".txt";
		//		cerr<<i<<endl;
		getRelation(R[i], C[i], fname);
	}
	//	cerr<<"get relation"<<endl;
	//relation0: dse - target relation1: dse - agent

	unordered_map<string, set<uint> > sentenceIds;
	set<string> allDocs; //Store the name of all docs
	ifstream in("sentenceid.txt");
	string line;
	uint numericId = 0;
	cerr<<"tag"<<endl;

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
	vector<vector<map<int, vector<int> > > > trainR, validR, testR;
	vector<vector<vector<string> > > trainL, validL, testL;
	vector<vector<int> > trainC, validC, testC;
	vector<bool> isUsed(X.size(), false);


	ifstream in4("datasplit_bishan/doclist");
	while(getline(in4, line))
		allDocs.insert(line);

	ifstream in2("datasplit_bishan/filelist_train"+to_string(fold));
	//10 fold
	while(getline(in2, line)) {
		for (const auto &id : sentenceIds[line]) {
			trainX.push_back(X[id]);
			vector<map<int, vector<int> > > tmp;
			tmp.clear();
			for (uint k = 0; k < 2; k ++) tmp.push_back(R[k][id]);
			trainR.push_back(tmp);
			//cerr<<C[1][id]<<endl;
			vector<vector<string> > nxt;
			nxt.clear();
			for (uint k = 0; k < 3; k ++) nxt.push_back(T[k][id]);
			trainL.push_back(nxt);
			vector<int> cur; cur.push_back(C[0][id]); cur.push_back(C[1][id]);
			trainC.push_back(cur);
		}
		allDocs.erase(line);
	}
	cerr<<"train"<<endl;
	ifstream in3("datasplit_bishan/filelist_test"+to_string(fold));
	while(getline(in3, line)) {
		for (const auto &id : sentenceIds[line]) {
			testX.push_back(X[id]);
			vector<map<int, vector<int> > > tmp;
			tmp.clear();
			for (uint k = 0; k < 2; k ++) tmp.push_back(R[k][id]);
			testR.push_back(tmp);
			vector<vector<string> > nxt;
			nxt.clear();
			for (uint k = 0; k < 3; k ++) nxt.push_back(T[k][id]);
			testL.push_back(nxt);
			vector<int> cur; cur.push_back(C[0][id]); cur.push_back(C[1][id]);
			testC.push_back(cur);
		}
		allDocs.erase(line);
	}
	//cerr<<"test"<<endl;

	uint validSize = 0;
	for (const auto &doc : allDocs) {
		for (const auto &id : sentenceIds[doc]) {
			validX.push_back(X[id]);
			vector<map<int, vector<int> > > tmp;
			tmp.clear();
			for (uint k = 0; k < 2; k ++) tmp.push_back(R[k][id]);
			validR.push_back(tmp);
			vector<vector<string> > nxt;
			nxt.clear();
			for (uint k = 0; k < 3; k ++) nxt.push_back(T[k][id]);
			validL.push_back(nxt);
			vector<int> cur; cur.push_back(C[0][id]); cur.push_back(C[1][id]);
			validC.push_back(cur);
		}
	}
	//cerr<<"valid"<<endl;

	cout << X.size() << " " << trainX.size() << " " << testX.size() << endl;
	cout << "Valid size: " << validX.size() << endl;
	Matrix<double, 3, 2> best;
	double bestDrop;
	RNN dsenn, argnn;
	dsenn = RNN(25,25,25,3,LT); argnn = RNN(25, 25, 25, 3, LT);
	dsenn.load("dse_model.txt");
	auto results = argnn.train(dsenn, trainX, trainL, trainR, trainC,
			validX, validL, validR, validC,
			testX, testL, testR, testC);
	cout << "Best: " << endl;
	for (int j = 0; j < 3; j ++)
		cout << "OCLASS_WEIGHT "<<NAME[j]<<":" << bestDrop[j] << endl;
	cout<<"Validation:"<<endl;
	for (uint k = 0; k < 3;k ++) cout << NAME[k]<<"\n"<< best[k] << endl;
	cout<<"Test:"<<endl;
	for (uint k = 3; k < 6;k ++) cout << NAME[k - 3]<<"\n"<< best[k] << endl;

	return 0;
}

