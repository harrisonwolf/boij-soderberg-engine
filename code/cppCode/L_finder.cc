#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include "seq_funcs.h"
#include "test_funcs.h"
using namespace std;

int main(){
	//first manually generate the self-dual degree seqs up to c=100
	//they have the form {0,a,b,c,b+c-a,b+c}
	cout << "Finding self_dual degseqs in c=5 with L=1: \n";
	vector<int> curr_D = {};
	int curr_L = -1;
	for(int a=1; a<=98; a++){
		for(int b=a+1; b<=99; b++){
			for(int c=b+1; c<=100; c++){
				curr_D.clear(); //clear D
				curr_L = -1; //reset L
				curr_D.push_back(0);
				curr_D.push_back(a);
				curr_D.push_back(b);
				curr_D.push_back(c);
				curr_D.push_back(b+c-a);
				curr_D.push_back(b+c);
				curr_L = calc_L(curr_D);
				if(curr_L == 1){
					cout << seq_to_string(curr_D) << endl;
				}
			}
		}
	}
}
