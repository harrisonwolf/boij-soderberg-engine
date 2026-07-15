#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "algorithm_helpers.h"
#include "benchmark_build_info.h"
#include "seq_funcs.h"

using namespace std;

namespace {

using Clock = chrono::steady_clock;

struct Options {
	int codimension = -1;
	int max_degree = -1;
	int lowbound = 1;
	bool build_info_only = false;
	string probe_sequence;
};

string json_escape(const string& value) {
	ostringstream out;
	for(unsigned char character : value){
		switch(character){
			case '\\': out << "\\\\"; break;
			case '"': out << "\\\""; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default:
				if(character < 0x20){
					out << "\\u" << hex << setw(4) << setfill('0')
						<< static_cast<int>(character) << dec << setfill(' ');
				}else{
					out << character;
				}
		}
	}
	return out.str();
}

void write_build_json(ostream& out) {
	out << "{"
		<< "\"commit\":\"" << json_escape(benchmark_build_info::commit()) << "\","
		<< "\"branch\":\"" << json_escape(benchmark_build_info::branch()) << "\","
		<< "\"dirty\":" << (benchmark_build_info::dirty() ? "true" : "false") << ","
		<< "\"profile\":\"" << json_escape(benchmark_build_info::profile()) << "\","
		<< "\"compiler_command\":\"" << json_escape(benchmark_build_info::compiler_command()) << "\","
		<< "\"compiler_version\":\"" << json_escape(benchmark_build_info::compiler_version()) << "\","
		<< "\"compiler_flags\":\"" << json_escape(benchmark_build_info::compiler_flags()) << "\""
		<< "}";
}

template <typename Number>
void write_number_array(ostream& out, const vector<Number>& values) {
	out << "[";
	for(size_t index = 0; index < values.size(); ++index){
		if(index != 0) out << ",";
		out << values[index];
	}
	out << "]";
}

void write_sequence_list(ostream& out, const vector<vector<int>>& sequences) {
	out << "[";
	for(size_t index = 0; index < sequences.size(); ++index){
		if(index != 0) out << ",";
		write_number_array(out, sequences[index]);
	}
	out << "]";
}

bool starts_with(const string& value, const string& prefix) {
	return value.compare(0, prefix.size(), prefix) == 0;
}

int parse_integer_option(const string& argument, const string& name) {
	const string prefix = "--" + name + "=";
	if(!starts_with(argument, prefix)){
		throw invalid_argument("expected " + prefix + "<integer>");
	}
	size_t consumed = 0;
	const string raw = argument.substr(prefix.size());
	const int parsed = stoi(raw, &consumed);
	if(consumed != raw.size()) throw invalid_argument("invalid integer for --" + name);
	return parsed;
}

Options parse_options(int argc, char** argv) {
	Options options;
	for(int index = 1; index < argc; ++index){
		const string argument = argv[index];
		if(argument == "--build-info"){
			options.build_info_only = true;
		}else if(starts_with(argument, "--codim=")){
			options.codimension = parse_integer_option(argument, "codim");
		}else if(starts_with(argument, "--max-degree=")){
			options.max_degree = parse_integer_option(argument, "max-degree");
		}else if(starts_with(argument, "--lowbound=")){
			options.lowbound = parse_integer_option(argument, "lowbound");
		}else if(starts_with(argument, "--probe-sequence=")){
			options.probe_sequence = argument.substr(string("--probe-sequence=").size());
		}else{
			throw invalid_argument("unknown argument: " + argument);
		}
	}
	return options;
}

vector<int> parse_probe_sequence(const string& raw) {
	vector<int> sequence;
	stringstream input(raw);
	string token;
	while(getline(input, token, ',')){
		if(token.empty()) throw invalid_argument("probe sequence contains an empty entry");
		size_t consumed = 0;
		const int value = stoi(token, &consumed);
		if(consumed != token.size()) throw invalid_argument("probe sequence contains a non-integer");
		sequence.push_back(value);
	}
	if(sequence.size() < 2 || sequence.front() != 0){
		throw invalid_argument("probe sequence must start at 0 and contain at least two entries");
	}
	for(size_t index = 1; index < sequence.size(); ++index){
		if(sequence[index] <= sequence[index - 1]){
			throw invalid_argument("probe sequence must be strictly increasing");
		}
	}
	return sequence;
}

double elapsed_seconds(Clock::time_point start, Clock::time_point finish) {
	return chrono::duration<double>(finish - start).count();
}

int emit_probe(const string& raw) {
	const vector<int> sequence = parse_probe_sequence(raw);
	const vector<long long> betti = pure_betti(sequence);
	cout << "{\"schema_version\":1,\"status\":\"ok\",\"build\":";
	write_build_json(cout);
	cout << ",\"probe_sequence\":";
	write_number_array(cout, sequence);
	cout << ",\"pure_betti\":";
	write_number_array(cout, betti);
	cout << "}\n";
	return 0;
}

int emit_benchmark(const Options& options) {
	if(options.codimension < 2 || options.codimension > 20){
		throw invalid_argument("--codim must be in [2,20]");
	}
	if(options.max_degree < options.codimension){
		throw invalid_argument("--max-degree must be at least the codimension");
	}
	if(options.lowbound < 1 || options.lowbound > options.max_degree){
		throw invalid_argument("--lowbound must be in [1,max-degree]");
	}

	const long long expected_candidates = count_degree_sequences(
		options.codimension, options.max_degree, options.lowbound);
	const auto generation_start = Clock::now();
	vector<vector<int>> candidates = gen_deg_seqs(
		options.codimension, options.max_degree, options.lowbound);
	const auto generation_finish = Clock::now();
	if(expected_candidates < 0
		|| static_cast<unsigned long long>(expected_candidates) != candidates.size()){
		throw runtime_error("candidate count disagrees with the production generator");
	}

	vector<vector<int>> bad_sequences;
	const auto testing_start = Clock::now();
	for(const vector<int>& sequence : candidates){
		const vector<long long> betti = pure_betti(sequence);
		if(!test_BEH(betti) || !test_LLBC(betti)) bad_sequences.push_back(sequence);
	}
	const auto testing_finish = Clock::now();
	const auto rinse_start = Clock::now();
	vector<vector<int>> rinsed_sequences = gcd_rinse(bad_sequences);
	const auto rinse_finish = Clock::now();

	sort(bad_sequences.begin(), bad_sequences.end());
	sort(rinsed_sequences.begin(), rinsed_sequences.end());
	cout << setprecision(17)
		<< "{\"schema_version\":1,\"status\":\"ok\",\"build\":";
	write_build_json(cout);
	cout << ",\"task\":{"
		<< "\"definition\":\"materialize gen_deg_seqs; pure_betti; explicit BEH or LLBC violation; gcd_rinse\","
		<< "\"codimension\":" << options.codimension << ","
		<< "\"max_degree\":" << options.max_degree << ","
		<< "\"lowbound\":" << options.lowbound << "},"
		<< "\"counts\":{"
		<< "\"candidates\":" << candidates.size() << ","
		<< "\"bad\":" << bad_sequences.size() << ","
		<< "\"gcd_rinsed\":" << rinsed_sequences.size() << "},"
		<< "\"timing_seconds\":{"
		<< "\"generation\":" << elapsed_seconds(generation_start, generation_finish) << ","
		<< "\"testing\":" << elapsed_seconds(testing_start, testing_finish) << ","
		<< "\"gcd_rinse\":" << elapsed_seconds(rinse_start, rinse_finish) << ","
		<< "\"total\":" << elapsed_seconds(generation_start, rinse_finish) << "},"
		<< "\"bad_sequences\":";
	write_sequence_list(cout, bad_sequences);
	cout << ",\"gcd_rinsed_sequences\":";
	write_sequence_list(cout, rinsed_sequences);
	cout << "}\n";
	return 0;
}

void emit_error(const string& status, const string& message) {
	cout << "{\"schema_version\":1,\"status\":\"" << status
		<< "\",\"error\":\"" << json_escape(message) << "\",\"build\":";
	write_build_json(cout);
	cout << "}\n";
}

} // namespace

int main(int argc, char** argv) {
	try{
		const Options options = parse_options(argc, argv);
		if(options.build_info_only){
			cout << "{\"schema_version\":1,\"status\":\"build_info\",\"build\":";
			write_build_json(cout);
			cout << "}\n";
			return 0;
		}
		if(!options.probe_sequence.empty()) return emit_probe(options.probe_sequence);
		return emit_benchmark(options);
	}catch(const overflow_error& error){
		emit_error("arithmetic_overflow", error.what());
		return 4;
	}catch(const exception& error){
		emit_error("error", error.what());
		return 2;
	}
}
