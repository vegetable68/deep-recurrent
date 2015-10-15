#include <iostream>
#include "Eigen/Dense"

using namespace Eigen;
using namespace std;

int main(){
	MatrixXd tmp, a;
	int nhf = 5;
	tmp = RowVectorXd::Zero(2 * nhf);
	a = VectorXd::Zero(nhf);
	a = a + tmp.block(0,0,1,nhf).transpose();
	cerr<<a<<endl;
	cerr<<tmp.block(0,nhf,1,nhf).transpose()<<endl;
}
