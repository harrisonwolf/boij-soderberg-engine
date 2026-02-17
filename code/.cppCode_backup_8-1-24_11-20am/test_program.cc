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
	cout << "Enter 3 to call test_conjs_v2.\n";
	cout << "Enter 4 to gen deg seqs and call test_conjs_v2 on each one.\n";
	cout << "Enter 5 to test gen_deg_seqs_v2 (only generate nondegenerates).\n";
	cout << "Enter 6 to test is_degen.\n";
	cout << "Enter 7 to test pi(D).\n";
	cout << "Enter -1 to quit.\n";
	cin >> inp;
	if(!cin) die();
	return inp;
}


int main(int argc, char** args){
	cout << "Running driver program to test functionality of all functions\n";
	int inp = -2;
//	cerr << "About to enter while loop for first time. inp = " << inp << endl;
	while(inp < -1 or inp > 7){
//		cerr << "First line of while loop. inp = " << inp << endl;
		cout << endl;
		inp = read_choice();
		if(inp == -1) exit(0);

		if(inp == 0){ //testing gen_deg_seqs
//			system("clear");
			cout << "Please enter parameters [codimension c] [max degree d] [min degree l]\n";
			int c=-1, d=-1, l=-1;
			cin >> c >> d >> l;
			if(!cin) die();
			//generate them
			vector<vector<int>> seqs = gen_deg_seqs(c,d,l); 
			//print them
//			for(auto seq: seqs) cout << seq_to_string(seq) << endl;
			cout << "Generated " << seqs.size() << " degree sequences." << endl;
		}else if(inp == 1){ //testing pure_betti on a degree seqeuence
			cout << "Please enter the degree sequence you wish to test, integer by integer, separated by spaces, followed by -1 (eg 0 3 5 8 -1):\n";
			vector<int> seq = read_degree_seq();
			cout << "pure_betti(" << seq_to_string(seq) << ") = " << pure_betti_to_string(pure_betti(seq)) << endl;
		}else if(inp == 2){ //testing BEH and LLBC on a degseq
			vector<int> seq = read_degree_seq();
			cerr << "Read seq " << seq_to_string(seq) << endl;
			cerr << "FIXME" << endl;
		}else if(inp == 3){ //call test_conjs_v2
			cout << "Please enter the degree sequence you wish to test, integer by integer, separated by spaces, followed by -1 (eg 0 3 5 8 -1):\n";
			vector<int> seq = read_degree_seq();
			bool hi = test_conjs_v2(seq);
		}else if(inp == 4){ //generate degseqs w/ c and d, test conjs on them
			cout << "Please enter codimension c, max degree d, min degree l:\n";
			int c = -1, d = -1, l = -1;
			cin >> c >> d >> l;
			auto seqs = gen_deg_seqs(c,d,l);
			for(auto seq: seqs){
				//cout << "Calling test_conjs_v2 on " << seq_to_string(seq) << ": " << boolalpha << test_conjs_v2(seq) << endl;
				if(test_conjs_v2(seq) == false) cerr << seq_to_string(seq) << " failed ^^^\n";
			}
		}else if(inp == 5){ //testing gen_deg_seqs_v2
//			system("clear");
			cout << "Please enter parameters [codimension c] [max degree d] [min degree l]\n";
			int c=-1, d=-1, l=-1;
			cin >> c >> d >> l;
			if(!cin) die();
			//generate them
			vector<vector<int>> seqs = gen_deg_seqs_v2(c,d,l); 
			//print them
//			for(auto seq: seqs) cout << seq_to_string(seq) << endl;
			cout << "Generated " << seqs.size() << " degree sequences." << endl;
		}else if(inp == 6){ //testing is_degen
			vector<int> seq = read_degree_seq();
			cerr << "Read seq " << seq_to_string(seq) << endl;
			cout << "is_degen(" << seq_to_string(seq) << ") = " << boolalpha << is_degen(seq) << endl;
		}else if(inp == 7){ //test pi(D)
			vector<int> seq = read_degree_seq(); //this prompts the user automatically
			auto pis = pi(seq);
			//now print them
			//print each pi_1 on its own line as a prod of fracs
			for(int i=0; i<pis.size(); i++){
				vector<pair<int,int>> curr_pi = pis.at(i);
				cout << "pi_" << i << ": ";
				for(int j=0; j<curr_pi.size(); j++){
					pair<int,int> curr_frac = curr_pi.at(j);
					cout << curr_frac.first << "/" << curr_frac.second;
					if(j < curr_pi.size()-1) cout << " * ";
				}
				cout << endl;
			}
			cout << "L(D) = " << calc_L(seq) << endl;
		}
		inp = -2; //reset inp
//		cerr << "Last line of while loop. inp = " << inp << endl;
	}

	clock_t start_program_time;
	start_program_time = clock();

	auto t = clock() - start_program_time;
	double secs = ((float)t)/CLOCKS_PER_SEC;
	
}
