#include <iostream>
#include <cstdio>
#include <cstdlib>
using namespace std;

int main(){
	int pos_cnt = 0, tot = 0;
	int x;
	while (cin>>x){
		if (x > 0) pos_cnt ++;
		tot ++;
	}
	cout<<pos_cnt<<" "<<tot<<endl;
}
