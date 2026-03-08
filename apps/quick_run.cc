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
	//trying to find where, if anywhere, 0,204,696,697 goes to in c=4; 0,204,x,696,697
	for(int i=205; i<=695; i++){
		vector<int> D = {0,204,i,696,697};
		cerr << "Testing " << seq_to_string(D) << endl;
		bool pass = test_conjs_v2(D);
		if(!pass) cout << seq_to_string(D) << " did not pass.\n";
	}
}
