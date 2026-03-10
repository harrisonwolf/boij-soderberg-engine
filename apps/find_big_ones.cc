#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include "search_algorithms.h"
#include "seq_funcs.h"
using namespace std;

int main(){
	vector<LargeBettiResult> matches = find_large_betti_sequences(3, 100, 50000);
	for(const LargeBettiResult& match : matches){
		cout << seq_to_string(match.degrees) << ": " << pure_betti_to_string(match.betti) << endl;
	}
}
