/*
 * This program is to test all functions, including gen_deg_seqs, pure_betti, test_BEH, and test_LLBC
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include <fstream>
#include <numeric> //for gcd
#include <sstream>
#include <ctime>
#include "seq_funcs.h"
#include "test_funcs.h"
using namespace std;

vector<vector<int>> gcd_rinse_long(vector<vector<int>> seqs){
	int num_seqs = seqs.size();
	vector<vector<int>> retvec;
	for(int i=0; i<num_seqs; i++){
		auto seq = seqs.at(i); //curr deg seq
		if(seq.size() < 3){ cerr << "called gcd_rinse with a sequence of size less than 3\n"; exit(1); }

	}

	return retvec;
}

//die() if bad input
int read_choice(){
	cin.clear();
	int inp = -1;
	cout << "Enter 0 to test gen_deq_seqs.\n";
	cout << "Enter 1 to find pure_betti of a degree sequence.\n";
	cout << "Enter 2 to test BEH and LLBC on a degree sequence.\n";
	cout << "Enter -1 to quit.\n";
	cin >> inp;
	if(!cin) die();
	return inp;
}


int main(int argc, char** args){
	cout << "Running driver program to test functionality of all functions\n";
	int inp = -2;
//	cerr << "About to enter while loop for first time. inp = " << inp << endl;
	while(inp < -1 or inp > 2){
//		cerr << "First line of while loop. inp = " << inp << endl;
		cout << endl;
		inp = read_choice();
		if(inp == -1) exit(0);

		if(inp == 0){ //testing gen_deg_seqs
			system("clear");
			cout << "Please enter parameters [codimension c] [max degree d] [min degree l]\n";
			int c=-1, d=-1, l=-1;
			cin >> c >> d >> l;
			if(!cin) die();
			//generate them
			vector<vector<int>> seqs = gen_deg_seqs(c,d,l); 
			//print them
			for(auto seq: seqs) cout << seq_to_string(seq) << endl;
		}else if(inp == 1){ //testing pure_betti on a degree seqeuence
			cout << "Please enter the degree sequence you wish to test, integer by integer, separated by spaces, followed by -1 (eg 0 3 5 8 -1):\n";
			vector<int> seq = read_degree_seq();
			cout << "pure_betti(" << seq_to_string(seq) << ") = " << pure_betti_to_string(pure_betti(seq)) << endl;
		}else if(inp == 2){ //testing BEH and LLBC on a degseq
			vector<int> seq = read_degree_seq();
			cerr << "Read seq " << seq_to_string(seq) << endl;
		}
		inp = -2; //reset inp
//		cerr << "Last line of while loop. inp = " << inp << endl;
	}

	clock_t start_program_time;
	start_program_time = clock();

	auto t = clock() - start_program_time;
	double secs = ((float)t)/CLOCKS_PER_SEC;
	
}
