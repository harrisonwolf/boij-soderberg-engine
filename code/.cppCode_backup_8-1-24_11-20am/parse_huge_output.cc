/*
 * This program takes an input file which is given by smashing together a bunch of individual output files given by run_tests.sh and writes to a new output file
 * only the unadulterated violators without any gcd rinsing. Duplicates intended. The purpose is to run the actual remove_duplicates program on this big file since gcd_rinse technically is
 * not airtight
 * 
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <cmath>
#include <sstream>
#include <fstream>
#include "seq_funcs.h"
using namespace std;

//usage: parse_huge_output [input file] [output file]
int main(int argc, char** args){
	//sanity check
	if(argc != 3){ cerr << "usage: parse_huge_output [input file] [output file]\n"; exit(1); }
	string infile_name = args[1];
	string outfile_name = args[2];
	fstream outs;
	outs.open(outfile_name);
	if(!outs.is_open()){ cerr << "Make sure " << outfile_name << " is an existing file\n"; exit(1); }
	//now read
	vector<vector<int>> seqs = read_degree_seqs(infile_name);
	//write it to outfile
//	cerr << "Writing parsed seqs to " << outfile_name << "...\n";
	for(auto seq: seqs) outs << seq_to_string(seq) << "\n";

	return 0;
}
