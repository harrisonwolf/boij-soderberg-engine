#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "algorithm_helpers.h"
#include "binom.h"
#include "search_algorithms.h"
#include "seq_funcs.h"
#include "test_funcs.h"
#include "test_harness.h"

using namespace std;

namespace {

// Mathematical fixtures in this file are baked from native Macaulay2 output.
// Refresh them with research/macaulay2/generate_cpp_test_fixtures.m2.

struct DegreeResultData {
	vector<long long> pure_betti_values;
	bool beh;
	bool llbc;
	bool conjs;
	bool conjs_v2;
	string violations;
	long long l_value;
};

inline bool operator==(const DegreeResultData& lhs, const DegreeResultData& rhs) {
	auto normalize_violations = [](const string& value) {
		if(value.empty() || value == "(passed both)") return string("(passed both)");
		return value;
	};
	return lhs.pure_betti_values == rhs.pure_betti_values
	    && lhs.beh == rhs.beh
	    && lhs.llbc == rhs.llbc
	    && lhs.conjs == rhs.conjs
	    && lhs.conjs_v2 == rhs.conjs_v2
	    && normalize_violations(lhs.violations) == normalize_violations(rhs.violations)
	    && lhs.l_value == rhs.l_value;
}

struct DegreeExpectation {
	string name;
	vector<int> degrees;
	DegreeResultData expected;
	vector<RationalValue> pi_values;
	vector<vector<pair<int, int>>> pi_factors;
};

struct GenerationResultData {
	vector<vector<int>> subsets;
	vector<vector<int>> subsets_v2;
	vector<vector<int>> degree_sequences;
	vector<vector<int>> degree_sequences_v2;
};

inline bool operator==(const GenerationResultData& lhs, const GenerationResultData& rhs) {
	return lhs.subsets == rhs.subsets
	    && lhs.subsets_v2 == rhs.subsets_v2
	    && lhs.degree_sequences == rhs.degree_sequences
	    && lhs.degree_sequences_v2 == rhs.degree_sequences_v2;
}

struct SearchParamCase {
	string name;
	int c;
	int d;
	int lowbound;
	vector<vector<int>> expected;
};

template <typename T>
string to_repr(const vector<T>& values);

string to_repr(const string& value) {
	if(value.empty()) return "\"\"";
	return "\"" + value + "\"";
}

string to_repr(const bool value) {
	return value ? "true" : "false";
}

string to_repr(const int value) {
	return to_string(value);
}

string to_repr(const long long value) {
	return to_string(value);
}

string to_repr(const pair<int, int>& value) {
	return "(" + to_string(value.first) + "," + to_string(value.second) + ")";
}

string to_repr(const RationalValue& value) {
	return "(" + to_string(value.num) + "/" + to_string(value.den) + ")";
}

string to_repr(const LargeBettiResult& value) {
	return "{degrees=" + to_repr(value.degrees) + ", betti=" + to_repr(value.betti) + "}";
}

string to_repr(const DegreeResultData& value) {
	ostringstream out;
	out << "{PB=" << to_repr(value.pure_betti_values)
	    << ", BEH=" << to_repr(value.beh)
	    << ", LLBC=" << to_repr(value.llbc)
	    << ", TC=" << to_repr(value.conjs)
	    << ", TCv2=" << to_repr(value.conjs_v2)
	    << ", Viol=" << to_repr(value.violations)
	    << ", L=" << value.l_value
	    << "}";
	return out.str();
}

string to_repr(const GenerationResultData& value) {
	ostringstream out;
	out << "{S=" << to_repr(value.subsets)
	    << ", S2=" << to_repr(value.subsets_v2)
	    << ", D=" << to_repr(value.degree_sequences)
	    << ", D2=" << to_repr(value.degree_sequences_v2)
	    << "}";
	return out.str();
}

template <typename T>
string to_repr(const vector<T>& values) {
	ostringstream out;
	out << "[";
	for(size_t i = 0; i < values.size(); ++i){
		if(i > 0) out << ", ";
		out << to_repr(values.at(i));
	}
	out << "]";
	return out.str();
}

template <typename T>
TestCaseResult expect_equal(const string& name, const T& actual, const T& expected) {
	return {name, actual == expected, to_repr(expected), to_repr(actual)};
}

struct StreamSilencer {
	ostringstream sink;
	streambuf* old_cout;
	streambuf* old_cerr;

	StreamSilencer() : old_cout(cout.rdbuf(sink.rdbuf())), old_cerr(cerr.rdbuf(sink.rdbuf())) {}

	~StreamSilencer() {
		cout.rdbuf(old_cout);
		cerr.rdbuf(old_cerr);
	}
};

bool run_test_conjs_v2_silenced(const vector<int>& degrees) {
	StreamSilencer silence;
	return test_conjs_v2(degrees);
}

string run_which_violations_silenced(const vector<int>& degrees) {
	StreamSilencer silence;
	return which_violations(degrees);
}

vector<DegreeExpectation> degree_cases() {
	return {
		// Regression: c=1 has L=1 but total Betti sum 2, below the LLBC target 3.
		{"D01", {0,1}, {{1,1}, true, false, false, false, "LLBC (2)", 1}, {{1,1}, {1,1}}, {{{1,1}}, {}}},
		{"D02", {0,2}, {{1,1}, true, false, false, false, "LLBC (2)", 1}, {{1,1}, {1,1}}, {{{1,1}}, {}}},
		{"D03", {0,1,2}, {{1,2,1}, true, false, false, false, "LLBC (4)", 1}, {{1,1}, {2,1}, {1,1}}, {{{1,1}}, {{2,1}}, {{1,1}}}},
		{"D04", {0,1,3}, {{2,3,1}, true, true, true, true, "", 2}, {{1,1}, {3,2}, {1,2}}, {{{1,1}}, {{3,2}}, {{1,2}}}},
		{"D05", {0,2,3}, {{1,3,2}, true, true, true, true, "", 1}, {{1,1}, {3,1}, {2,1}}, {{{1,1}}, {{3,1}}, {{2,1}}}},
		{"D06", {0,2,4}, {{1,2,1}, true, false, false, false, "LLBC (4)", 1}, {{1,1}, {2,1}, {1,1}}, {{{1,1}}, {{2,1}}, {{1,1}}}},
		{"D07", {0,3,4}, {{1,4,3}, true, true, true, true, "", 1}, {{1,1}, {4,1}, {3,1}}, {{{1,1}}, {{4,1}}, {{3,1}}}},
		{"D08", {0,1,2,3}, {{1,3,3,1}, true, false, false, false, "LLBC (8)", 1}, {{1,1}, {3,1}, {3,1}, {1,1}}, {{{1,1}}, {{1,1},{3,1}}, {{1,1},{3,1}}, {{1,1},{1,1}}}},
		{"D09", {0,1,3,4}, {{1,2,2,1}, false, false, false, false, "B_1 (2) B_2 (2) LLBC (6)", 1}, {{1,1}, {2,1}, {2,1}, {1,1}}, {{{1,1}}, {{1,1},{2,1}}, {{1,1},{2,1}}, {{1,1},{1,1}}}},
		{"D10", {0,2,3,4}, {{1,6,8,3}, true, true, true, true, "", 1}, {{1,1}, {6,1}, {8,1}, {3,1}}, {{{1,1}}, {{3,1},{2,1}}, {{2,1},{4,1}}, {{1,1},{3,1}}}},
		{"D11", {0,2,4,6}, {{1,3,3,1}, true, false, false, false, "LLBC (8)", 1}, {{1,1}, {3,1}, {3,1}, {1,1}}, {{{1,1}}, {{1,1},{3,1}}, {{1,1},{3,1}}, {{1,1},{1,1}}}},
		{"D12", {0,3,5,8}, {{1,4,4,1}, true, false, false, false, "LLBC (10)", 1}, {{1,1}, {4,1}, {4,1}, {1,1}}, {{{1,1}}, {{1,1},{4,1}}, {{1,1},{4,1}}, {{1,1},{1,1}}}},
		{"D13", {0,2,7,8}, {{15,28,48,35}, true, true, true, true, "", 15}, {{1,1}, {28,15}, {16,5}, {7,3}}, {{{1,1}}, {{7,5},{4,3}}, {{2,5},{8,1}}, {{1,3},{7,1}}}},
		{"D14", {0,4,7,9}, {{5,21,30,14}, true, true, true, true, "", 5}, {{1,1}, {21,5}, {6,1}, {14,5}}, {{{1,1}}, {{7,1},{3,5}}, {{2,1},{3,1}}, {{2,5},{7,1}}}},
		{"D15", {0,1,2,4,5}, {{3,10,10,5,2}, true, true, true, true, "", 3}, {{1,1}, {10,3}, {10,3}, {5,3}, {2,3}}, {{{1,1}}, {{1,1},{2,3},{5,1}}, {{1,1},{2,1},{5,3}}, {{1,3},{1,1},{5,1}}, {{1,1},{1,3},{2,1}}}},
		{"D16", {0,1,3,4,6}, {{5,12,20,15,2}, true, true, true, true, "", 5}, {{1,1}, {12,5}, {4,1}, {3,1}, {2,5}}, {{{1,1}}, {{1,1},{2,1},{6,5}}, {{1,1},{2,1},{2,1}}, {{1,1},{1,1},{3,1}}, {{1,5},{1,1},{2,1}}}},
		{"D17", {0,2,4,5,7}, {{3,14,35,28,4}, true, true, true, true, "", 3}, {{1,1}, {14,3}, {35,3}, {28,3}, {4,3}}, {{{1,1}}, {{2,1},{1,3},{7,1}}, {{1,1},{5,1},{7,3}}, {{1,3},{4,1},{7,1}}, {{1,1},{4,3},{1,1}}}},
		{"D18", {0,2,5,6,9}, {{14,45,126,105,10}, true, true, true, true, "", 14}, {{1,1}, {45,14}, {9,1}, {15,2}, {5,7}}, {{{1,1}}, {{5,1},{1,2},{9,7}}, {{1,1},{1,1},{9,1}}, {{1,2},{5,1},{3,1}}, {{1,7},{5,1},{1,1}}}},
		{"D19", {0,3,4,7,8}, {{5,56,70,40,21}, true, true, true, true, "", 5}, {{1,1}, {56,5}, {14,1}, {8,1}, {21,5}}, {{{1,1}}, {{1,1},{7,1},{8,5}}, {{1,1},{7,1},{2,1}}, {{1,1},{1,1},{8,1}}, {{3,5},{1,1},{7,1}}}},
		{"D20", {0,4,6,9,10}, {{1,9,15,16,9}, true, true, true, true, "", 1}, {{1,1}, {9,1}, {15,1}, {16,1}, {9,1}}, {{{1,1}}, {{1,1},{9,1},{1,1}}, {{1,1},{3,1},{5,1}}, {{4,1},{2,1},{2,1}}, {{1,1},{1,1},{9,1}}}}
	};
}

TestSuiteResult run_binom_suite() {
	const vector<tuple<string, int, int, int>> cases = {
		{"B01",0,0,1}, {"B02",1,0,1}, {"B03",1,1,1}, {"B04",2,0,1}, {"B05",2,1,2},
		{"B06",2,2,1}, {"B07",3,1,3}, {"B08",3,2,3}, {"B09",4,2,6}, {"B10",5,2,10},
		{"B11",5,3,10}, {"B12",6,3,20}, {"B13",7,0,1}, {"B14",7,7,1}, {"B15",7,1,7},
		{"B16",7,6,7}, {"B17",8,2,28}, {"B18",8,4,70}, {"B19",5,6,0}, {"B20",3,5,0}
	};

	TestSuiteResult suite{"Binomial coefficients", {}};
	for(const auto& [name, n, k, expected] : cases){
		suite.cases.push_back(expect_equal(name, binom(n, k), expected));
	}
	return suite;
}

TestSuiteResult run_count_suite() {
	const vector<tuple<string, int, int, int, long long>> cases = {
		{"C01",1,0,1,0}, {"C02",1,1,1,1}, {"C03",1,3,1,3}, {"C04",2,2,1,1}, {"C05",2,4,1,6},
		{"C06",2,4,3,5}, {"C07",3,3,1,1}, {"C08",3,4,1,4}, {"C09",3,5,1,10}, {"C10",3,5,4,9},
		{"C11",4,4,1,1}, {"C12",4,5,1,5}, {"C13",4,6,1,15}, {"C14",4,6,5,14}, {"C15",5,5,1,1},
		{"C16",5,6,1,6}, {"C17",5,7,1,21}, {"C18",5,7,6,20}, {"C19",3,2,1,0}, {"C20",3,5,0,10}
	};

	TestSuiteResult suite{"Degree-sequence counts (M2 fixtures)", {}};
	for(const auto& [name, c, d, lowbound, expected] : cases){
		suite.cases.push_back(expect_equal(name, count_degree_sequences(c, d, lowbound), expected));
	}
	return suite;
}

TestSuiteResult run_degree_suite() {
	TestSuiteResult suite{"Degree algorithms (M2 fixtures)", {}};
	for(const DegreeExpectation& test_case : degree_cases()){
		DegreeResultData actual = {
			pure_betti(test_case.degrees),
			false,
			false,
			false,
			false,
			run_which_violations_silenced(test_case.degrees),
			calc_L(test_case.degrees)
		};
		actual.beh = test_BEH(actual.pure_betti_values);
		actual.llbc = test_LLBC(actual.pure_betti_values);
		actual.conjs = test_conjs(test_case.degrees);
		actual.conjs_v2 = run_test_conjs_v2_silenced(test_case.degrees);
		suite.cases.push_back(expect_equal(test_case.name, actual, test_case.expected));
	}
	return suite;
}

TestSuiteResult run_is_degen_suite() {
	const vector<tuple<string, vector<int>, bool>> cases = {
		{"DG01",{0,1},false}, {"DG02",{0,2},false}, {"DG03",{0,1,2},false}, {"DG04",{0,1,3},false}, {"DG05",{0,2,3},false},
		{"DG06",{0,2,4},false}, {"DG07",{0,3,4},false}, {"DG08",{0,1,2,3},true}, {"DG09",{0,1,3,4},true}, {"DG10",{0,2,3,4},true},
		{"DG11",{0,2,4,6},true}, {"DG12",{0,3,5,8},true}, {"DG13",{0,2,7,8},true}, {"DG14",{0,4,7,9},true}, {"DG15",{0,1,2,4,5},true},
		{"DG16",{0,1,3,4,6},true}, {"DG17",{0,2,4,5,7},true}, {"DG18",{0,2,5,6,9},true}, {"DG19",{0,3,4,7,8},true}, {"DG20",{0,4,6,9,10},true}
	};

	TestSuiteResult suite{"Legacy is_degen fixtures", {}};
	for(const auto& [name, degrees, expected] : cases){
		suite.cases.push_back(expect_equal(name, is_degen(degrees), expected));
	}
	return suite;
}

TestSuiteResult run_calc_sum_suite() {
	TestSuiteResult suite{"Pure-Betti total (M2 fixtures)", {}};
	for(const DegreeExpectation& test_case : degree_cases()){
		long long expected_sum = 0;
		for(long long value: test_case.expected.pure_betti_values){
			expected_sum += value;
		}
		suite.cases.push_back(expect_equal(
			test_case.name, calc_sum(test_case.degrees), expected_sum));
	}
	bool overflow_detected = false;
	try{
		calc_sum({0,847,2078,3809,3918});
	}catch(const overflow_error&){
		overflow_detected = true;
	}
	suite.cases.push_back(expect_equal(
		"SUM21-total-overflow", overflow_detected, true));
	return suite;
}

vector<vector<int>> sorted_sequences(vector<vector<int>> seqs) {
	sort(seqs.begin(), seqs.end());
	return seqs;
}

TestSuiteResult run_pi_values_suite() {
	TestSuiteResult suite{"Pi rational values (M2 fixtures)", {}};
	for(const DegreeExpectation& test_case : degree_cases()){
		suite.cases.push_back(expect_equal(test_case.name, compute_pi_values(test_case.degrees), test_case.pi_values));
	}
	return suite;
}

TestSuiteResult run_pi_factors_suite() {
	TestSuiteResult suite{"Legacy pi factor display (M2-derived fixtures)", {}};
	for(const DegreeExpectation& test_case : degree_cases()){
		suite.cases.push_back(expect_equal(test_case.name, pi(test_case.degrees), test_case.pi_factors));
	}
	return suite;
}

TestSuiteResult run_generation_suite() {
	const vector<tuple<string, int, int, int, GenerationResultData>> cases = {
		{"G01",1,1,2,{ {}, {}, {}, {} }},
		{"G02",1,1,1,{ {{1}}, {{1}}, {{0,1}}, {{0,1}} }},
		{"G03",1,3,1,{ {{1},{2},{3}}, {{1},{2},{3}}, {{0,1},{0,2},{0,3}}, {{0,1},{0,2},{0,3}} }},
		{"G04",1,4,3,{ {{3},{4}}, {{3},{4}}, {{0,3},{0,4}}, {{0,3},{0,4}} }},
		{"G05",2,2,1,{ {{1,2}}, {{1,2}}, {{0,1,2}}, {{0,1,2}} }},
		{"G06",2,3,1,{ {{1,2},{1,3},{2,3}}, {{1,2},{1,3},{2,3}}, {{0,1,2},{0,1,3},{0,2,3}}, {{0,1,2},{0,1,3},{0,2,3}} }},
		{"G07",2,4,1,{ {{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}}, {{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}}, {{0,1,2},{0,1,3},{0,1,4},{0,2,3},{0,2,4},{0,3,4}}, {{0,1,2},{0,1,3},{0,1,4},{0,2,3},{0,2,4},{0,3,4}} }},
		{"G08",2,4,3,{ {{1,3},{1,4},{2,3},{2,4},{3,4}}, {{1,3},{1,4},{2,3},{2,4},{3,4}}, {{0,1,3},{0,1,4},{0,2,3},{0,2,4},{0,3,4}}, {{0,1,3},{0,1,4},{0,2,3},{0,2,4},{0,3,4}} }},
		{"G09",2,5,4,{ {{1,4},{1,5},{2,4},{2,5},{3,4},{3,5},{4,5}}, {{1,4},{1,5},{2,4},{2,5},{3,4},{3,5},{4,5}}, {{0,1,4},{0,1,5},{0,2,4},{0,2,5},{0,3,4},{0,3,5},{0,4,5}}, {{0,1,4},{0,1,5},{0,2,4},{0,2,5},{0,3,4},{0,3,5},{0,4,5}} }},
		{"G10",2,5,5,{ {{1,5},{2,5},{3,5},{4,5}}, {{1,5},{2,5},{3,5},{4,5}}, {{0,1,5},{0,2,5},{0,3,5},{0,4,5}}, {{0,1,5},{0,2,5},{0,3,5},{0,4,5}} }},
		{"G11",3,3,1,{ {{1,2,3}}, {}, {{0,1,2,3}}, {} }},
		{"G12",3,4,1,{ {{1,2,3},{1,2,4},{1,3,4},{2,3,4}}, {}, {{0,1,2,3},{0,1,2,4},{0,1,3,4},{0,2,3,4}}, {} }},
		{"G13",3,4,4,{ {{1,2,4},{1,3,4},{2,3,4}}, {}, {{0,1,2,4},{0,1,3,4},{0,2,3,4}}, {} }},
		{"G14",3,5,1,{ {{1,2,3},{1,2,4},{1,2,5},{1,3,4},{1,3,5},{1,4,5},{2,3,4},{2,3,5},{2,4,5},{3,4,5}}, {}, {{0,1,2,3},{0,1,2,4},{0,1,2,5},{0,1,3,4},{0,1,3,5},{0,1,4,5},{0,2,3,4},{0,2,3,5},{0,2,4,5},{0,3,4,5}}, {} }},
		{"G15",3,5,4,{ {{1,2,4},{1,2,5},{1,3,4},{1,3,5},{1,4,5},{2,3,4},{2,3,5},{2,4,5},{3,4,5}}, {}, {{0,1,2,4},{0,1,2,5},{0,1,3,4},{0,1,3,5},{0,1,4,5},{0,2,3,4},{0,2,3,5},{0,2,4,5},{0,3,4,5}}, {} }},
		{"G16",3,6,1,{ {{1,2,3},{1,2,4},{1,2,5},{1,2,6},{1,3,4},{1,3,5},{1,3,6},{1,4,5},{1,4,6},{1,5,6},{2,3,4},{2,3,5},{2,3,6},{2,4,5},{2,4,6},{2,5,6},{3,4,5},{3,4,6},{3,5,6},{4,5,6}}, {}, {{0,1,2,3},{0,1,2,4},{0,1,2,5},{0,1,2,6},{0,1,3,4},{0,1,3,5},{0,1,3,6},{0,1,4,5},{0,1,4,6},{0,1,5,6},{0,2,3,4},{0,2,3,5},{0,2,3,6},{0,2,4,5},{0,2,4,6},{0,2,5,6},{0,3,4,5},{0,3,4,6},{0,3,5,6},{0,4,5,6}}, {} }},
		{"G17",3,6,4,{ {{1,2,4},{1,2,5},{1,2,6},{1,3,4},{1,3,5},{1,3,6},{1,4,5},{1,4,6},{1,5,6},{2,3,4},{2,3,5},{2,3,6},{2,4,5},{2,4,6},{2,5,6},{3,4,5},{3,4,6},{3,5,6},{4,5,6}}, {}, {{0,1,2,4},{0,1,2,5},{0,1,2,6},{0,1,3,4},{0,1,3,5},{0,1,3,6},{0,1,4,5},{0,1,4,6},{0,1,5,6},{0,2,3,4},{0,2,3,5},{0,2,3,6},{0,2,4,5},{0,2,4,6},{0,2,5,6},{0,3,4,5},{0,3,4,6},{0,3,5,6},{0,4,5,6}}, {} }},
		{"G18",3,6,6,{ {{1,2,6},{1,3,6},{1,4,6},{1,5,6},{2,3,6},{2,4,6},{2,5,6},{3,4,6},{3,5,6},{4,5,6}}, {}, {{0,1,2,6},{0,1,3,6},{0,1,4,6},{0,1,5,6},{0,2,3,6},{0,2,4,6},{0,2,5,6},{0,3,4,6},{0,3,5,6},{0,4,5,6}}, {} }},
		{"G19",2,6,5,{ {{1,5},{1,6},{2,5},{2,6},{3,5},{3,6},{4,5},{4,6},{5,6}}, {{1,5},{1,6},{2,5},{2,6},{3,5},{3,6},{4,5},{4,6},{5,6}}, {{0,1,5},{0,1,6},{0,2,5},{0,2,6},{0,3,5},{0,3,6},{0,4,5},{0,4,6},{0,5,6}}, {{0,1,5},{0,1,6},{0,2,5},{0,2,6},{0,3,5},{0,3,6},{0,4,5},{0,4,6},{0,5,6}} }},
		{"G20",1,5,6,{ {}, {}, {}, {} }}
	};

	TestSuiteResult suite{"Supported/legacy generation fixtures", {}};
	for(const auto& [name, c, d, lowbound, expected] : cases){
		GenerationResultData actual = {
			gen_subsets_fast(1, c, d, max(lowbound, 1)),
			gen_subsets_fast_v2(1, c, d, max(lowbound, 1), c),
			gen_deg_seqs(c, d, lowbound),
			gen_deg_seqs_v2(c, d, lowbound)
		};
		suite.cases.push_back(expect_equal(name, actual, expected));
	}
	return suite;
}

TestSuiteResult run_compare_suite() {
	const vector<tuple<string, vector<int>, vector<int>, bool>> cases = {
		{"Q01",{0,1,2},{0,1,2},true}, {"Q02",{0,1,2},{0,2,1},false}, {"Q03",{0,1},{0,1,0},false}, {"Q04",{}, {}, true},
		{"Q05",{1},{1},true}, {"Q06",{1},{2},false}, {"Q07",{0,2,4},{0,2,4},true}, {"Q08",{0,2,4},{0,4,8},false},
		{"Q09",{0,3,5,8},{0,3,5,8},true}, {"Q10",{0,3,5,8},{0,3,6,8},false}, {"Q11",{0,1,3,4},{0,1,3,4},true}, {"Q12",{0,1,3,4},{0,1,4,3},false},
		{"Q13",{0,2},{0,2},true}, {"Q14",{0,2},{2,0},false}, {"Q15",{0,1,2,3},{0,1,2,3},true}, {"Q16",{0,1,2,3},{0,1,2},false},
		{"Q17",{0},{0},true}, {"Q18",{0},{1},false}, {"Q19",{5,10},{5,10},true}, {"Q20",{5,10},{10,5},false}
	};

	TestSuiteResult suite{"Sequence comparison", {}};
	for(const auto& [name, left, right, expected] : cases){
		suite.cases.push_back(expect_equal(name, compare_seqs(left, right), expected));
	}
	return suite;
}

TestSuiteResult run_mult_suite() {
	const vector<tuple<string, vector<int>, int, vector<int>>> cases = {
		{"M01",{0,1,2},1,{0,1,2}}, {"M02",{0,1,2},2,{0,2,4}}, {"M03",{0,1,2},3,{0,3,6}}, {"M04",{0,2,4},2,{0,4,8}},
		{"M05",{0,3,5,8},2,{0,6,10,16}}, {"M06",{1,2,3},0,{0,0,0}}, {"M07",{1,2,3},-1,{-1,-2,-3}}, {"M08",{},5,{}},
		{"M09",{0},10,{0}}, {"M10",{5},3,{15}}, {"M11",{0,1,3,4},4,{0,4,12,16}}, {"M12",{2,3},5,{10,15}},
		{"M13",{0,2},7,{0,14}}, {"M14",{4,6,8},1,{4,6,8}}, {"M15",{4,6,8},2,{8,12,16}}, {"M16",{0,1,2,3},6,{0,6,12,18}},
		{"M17",{10,20},3,{30,60}}, {"M18",{1},1,{1}}, {"M19",{1},2,{2}}, {"M20",{1},3,{3}}
	};

	TestSuiteResult suite{"Sequence scaling", {}};
	for(const auto& [name, degrees, factor, expected] : cases){
		suite.cases.push_back(expect_equal(name, mult_seq(degrees, factor), expected));
	}
	return suite;
}

TestSuiteResult run_gcd_rinse_suite() {
	const vector<tuple<string, vector<vector<int>>, vector<vector<int>>>> cases = {
		{"R01",{{0,1,2},{0,1,3},{0,2,4}},{{0,1,2},{0,1,3}}}, {"R02",{{0,2,3},{0,2,4},{0,3,5}},{{0,2,3},{0,3,5}}},
		{"R03",{{0,1,2,3},{0,1,3,4},{0,2,4,6}},{{0,1,2,3},{0,1,3,4}}}, {"R04",{{0,3,5,8},{0,2,7,8},{0,4,6,10}},{{0,3,5,8},{0,2,7,8}}},
		{"R05",{{0,2,5,6,9},{0,4,6,9,10}},{{0,4,6,9,10}}}, {"R06",{{0,4,6},{0,5,7}},{{0,5,7}}},
		{"R07",{{0,6,9},{0,6,10}},{}}, {"R08",{{0,2,4,5,7},{0,3,6,9,12}},{{0,2,4,5,7}}},
		{"R09",{{0,1,2,4,5},{0,2,4,6,8}},{{0,1,2,4,5}}}, {"R10",{{0,1,4},{0,2,6}},{{0,1,4}}},
		{"R11",{{0,7,9},{0,8,10}},{{0,7,9}}}, {"R12",{{0,2,3,7},{0,4,8,12}},{{0,2,3,7}}},
		{"R13",{{0,5,6,7},{0,4,6,8}},{{0,5,6,7}}}, {"R14",{{0,9,10},{0,9,12}},{{0,9,10}}},
		{"R15",{{0,1,5,6},{0,2,10,12}},{{0,1,5,6}}}, {"R16",{{0,2,3},{0,4,6}},{{0,2,3}}},
		{"R17",{{0,10,15},{0,10,14}},{}}, {"R18",{{0,3,4,5},{0,6,8,10}},{{0,3,4,5}}},
		{"R19",{{0,2,9},{0,2,8}},{{0,2,9}}}, {"R20",{{0,11,13},{0,12,18}},{{0,11,13}}}
	};

	TestSuiteResult suite{"GCD rinse", {}};
	for(const auto& [name, seqs, expected] : cases){
		suite.cases.push_back(expect_equal(name, gcd_rinse(seqs), expected));
	}
	return suite;
}

TestSuiteResult run_rinse_suite() {
	const vector<tuple<string, vector<vector<int>>, vector<vector<int>>>> cases = {
		{"S01",{{0,1,2},{0,2,4}},{{0,1,2}}}, {"S02",{{0,1,2},{0,1,3},{0,2,4}},{{0,1,2},{0,1,3}}},
		{"S03",{{0,1,2,3},{0,2,4,6},{0,1,3,4}},{{0,1,2,3},{0,1,3,4}}}, {"S04",{{0,3,5,8},{0,6,10,16},{0,1,2,3}},{{0,3,5,8},{0,1,2,3}}},
		{"S05",{{0,1,5,6},{0,2,10,12}},{{0,1,5,6}}}, {"S06",{{0,2,3},{0,4,6}},{{0,2,3}}},
		{"S07",{{0,1,3,4},{0,2,6,8}},{{0,1,3,4}}}, {"S08",{{0,2,3,4},{0,4,6,8}},{{0,2,3,4}}},
		{"S09",{{0,1,2},{0,1,2},{0,2,4}},{{0,1,2}}}, {"S10",{{0,2,5,6,9},{0,4,10,12,18}},{{0,2,5,6,9}}},
		{"S11",{{0,1,4},{0,2,8}},{{0,1,4}}}, {"S12",{{0,1,2,4,5},{0,2,4,8,10}},{{0,1,2,4,5}}},
		{"S13",{{0,4,6},{0,8,12},{0,5,7}},{{0,4,6},{0,5,7}}}, {"S14",{{0,1,2,3,4},{0,2,4,6,8}},{{0,1,2,3,4}}},
		{"S15",{{0,1,3},{0,2,6}},{{0,1,3}}}, {"S16",{{0,5,7},{0,10,14}},{{0,5,7}}},
		{"S17",{{0,3,4,5},{0,6,8,10}},{{0,3,4,5}}}, {"S18",{{0,2,4},{0,4,8},{0,3,6}},{{0,2,4},{0,3,6}}},
		{"S19",{{0,7,9},{0,14,18},{0,8,10}},{{0,7,9},{0,8,10}}}, {"S20",{{0,2,3,7},{0,4,6,14}},{{0,2,3,7}}}
	};

	TestSuiteResult suite{"Full rinse", {}};
	for(const auto& [name, seqs, expected] : cases){
		suite.cases.push_back(expect_equal(name, rinse_seqs(seqs), expected));
	}
	return suite;
}

TestSuiteResult run_bad_search_suite() {
	const vector<SearchParamCase> cases = {
		{"F01",2,2,1,{{0,1,2}}}, {"F02",2,3,1,{{0,1,2}}}, {"F03",2,4,1,{{0,1,2},{0,2,4}}}, {"F04",2,5,1,{{0,1,2},{0,2,4}}},
		{"F05",2,6,1,{{0,1,2},{0,2,4},{0,3,6}}}, {"F06",2,7,1,{{0,1,2},{0,2,4},{0,3,6}}},
		{"F07",3,3,1,{{0,1,2,3}}}, {"F08",3,4,1,{{0,1,2,3},{0,1,3,4}}}, {"F09",3,5,1,{{0,1,2,3},{0,1,3,4}}},
		{"F10",3,6,1,{{0,1,2,3},{0,1,3,4},{0,1,5,6},{0,2,4,6}}}, {"F11",3,7,1,{{0,1,2,3},{0,1,3,4},{0,1,5,6},{0,2,4,6}}},
		{"F12",3,8,1,{{0,1,2,3},{0,1,3,4},{0,1,5,6},{0,2,4,6},{0,2,6,8},{0,3,5,8}}},
		{"F13",4,4,1,{{0,1,2,3,4}}}, {"F14",4,5,1,{{0,1,2,3,4}}}, {"F15",4,6,1,{{0,1,2,3,4},{0,1,2,5,6},{0,1,4,5,6}}},
		{"F16",4,7,1,{{0,1,2,3,4},{0,1,2,5,6},{0,1,4,5,6}}}, {"F17",4,8,1,{{0,1,2,3,4},{0,1,2,5,6},{0,1,4,5,6},{0,2,4,6,8}}},
		// Regression: {0,1,2,8,9} has B={7,18,12,3,2}; B_3=3 violates BEH's target 4.
		{"F18",4,9,1,{{0,1,2,3,4},{0,1,2,5,6},{0,1,2,8,9},{0,1,4,5,6},{0,1,7,8,9},{0,2,4,6,8}}},
		{"F19",5,5,1,{{0,1,2,3,4,5}}}, {"F20",5,6,1,{{0,1,2,3,4,5},{0,1,2,3,5,6},{0,1,2,4,5,6},{0,1,3,4,5,6}}}
	};

	TestSuiteResult suite{"Bad-sequence search (M2 fixtures)", {}};
	for(const SearchParamCase& test_case : cases){
		suite.cases.push_back(expect_equal(test_case.name, sorted_sequences(find_bad_degree_sequences(test_case.c, test_case.d, test_case.lowbound)), sorted_sequences(test_case.expected)));
	}
	return suite;
}

TestSuiteResult run_self_dual_suite() {
	const vector<vector<int>> expected = {
		{0,1,2,3,4,5}, {0,1,2,4,5,6}, {0,1,4,6,9,10}, {0,2,3,4,5,7}, {0,2,3,5,6,8},
		{0,2,3,7,8,10}, {0,2,4,5,7,9}, {0,2,4,6,8,10}, {0,2,4,8,10,12}, {0,2,5,9,12,14},
		{0,2,8,12,18,20}, {0,3,4,5,6,9}, {0,3,4,6,7,10}, {0,3,4,10,11,14}, {0,3,5,6,8,11},
		{0,3,6,9,12,15}, {0,3,6,12,15,18}, {0,3,12,18,27,30}, {0,4,5,6,7,11}, {0,4,5,7,8,12}
	};
	vector<vector<int>> actual = find_self_dual_l_one_sequences(100, expected.size());

	TestSuiteResult suite{"Self-dual L=1 search (M2 fixtures)", {}};
	for(size_t i = 0; i < expected.size(); ++i){
		vector<int> actual_entry;
		if(i < actual.size()) actual_entry = actual.at(i);
		suite.cases.push_back(expect_equal("L" + to_string(i + 1), actual_entry, expected.at(i)));
	}
	return suite;
}

TestSuiteResult run_scale_invariance_suite() {
	const vector<int> base = {0,1,2,3,4,5,6,7};
	const vector<int> formerly_false_overflow = {0,1000,2000,3000,4000,5000,6000,7000};
	const vector<int> larger_scale = {
		0,1000000,2000000,3000000,4000000,5000000,6000000,7000000
	};
	const vector<long long> expected = {1,7,21,35,35,21,7,1};
	const vector<long long> base_betti = pure_betti(base);

	TestSuiteResult suite{"Pure-Betti scale invariance", {}};
	suite.cases.push_back(expect_equal("SI01-base", base_betti, expected));
	suite.cases.push_back(expect_equal(
		"SI02-former-false-overflow", pure_betti(formerly_false_overflow), expected));
	suite.cases.push_back(expect_equal(
		"SI03-larger-scale", pure_betti(larger_scale), expected));
	suite.cases.push_back(expect_equal(
		"SI04-betti-equality", pure_betti(formerly_false_overflow), base_betti));
	suite.cases.push_back(expect_equal(
		"SI05-pi-equality", compute_pi_values(formerly_false_overflow), compute_pi_values(base)));
	bool beh_boundary_rejected = false;
	bool llbc_boundary_rejected = false;
	const vector<long long> unsupported_codimension(22, 1);
	try{
		test_BEH(unsupported_codimension);
	}catch(const invalid_argument&){
		beh_boundary_rejected = true;
	}
	try{
		test_LLBC(unsupported_codimension);
	}catch(const invalid_argument&){
		llbc_boundary_rejected = true;
	}
	suite.cases.push_back(expect_equal("SI06-BEH-boundary", beh_boundary_rejected, true));
	suite.cases.push_back(expect_equal("SI07-LLBC-boundary", llbc_boundary_rejected, true));
	return suite;
}

TestSuiteResult run_large_betti_suite() {
	const vector<LargeBettiResult> expected = {
		{{0,1,12,98},{45881,50568,4753,66}}, {{0,1,13,95},{46248,50635,4465,78}}, {{0,1,13,98},{49470,54145,4753,78}},
		{{0,1,14,93},{47242,51429,4278,91}}, {{0,1,14,94},{48360,52640,4371,91}}, {{0,1,14,95},{49491,53865,4465,91}},
		{{0,1,14,96},{50635,55104,4560,91}}, {{0,1,14,97},{51792,56357,4656,91}}, {{0,1,14,100},{55341,60200,4950,91}},
		{{0,1,16,98},{59655,64288,4753,120}}, {{0,1,17,87},{48160,51765,3741,136}}, {{0,1,17,90},{51976,55845,4005,136}},
		{{0,1,17,91},{53280,57239,4095,136}}, {{0,1,17,94},{57288,61523,4371,136}}, {{0,1,17,95},{58656,62985,4465,136}},
		{{0,1,17,98},{62856,67473,4753,136}}, {{0,1,17,99},{64288,69003,4851,136}}, {{0,1,18,89},{53108,56871,3916,153}},
		{{0,1,18,92},{57239,61272,4186,153}}, {{0,1,18,95},{61523,65835,4465,153}}
	};
	vector<LargeBettiResult> actual = find_large_betti_sequences(3, 100, 50000, expected.size());

	TestSuiteResult suite{"Large-Betti search (M2 fixtures)", {}};
	for(size_t i = 0; i < expected.size(); ++i){
		LargeBettiResult actual_entry;
		if(i < actual.size()) actual_entry = actual.at(i);
		suite.cases.push_back(expect_equal("LB" + to_string(i + 1), actual_entry, expected.at(i)));
	}
	return suite;
}

}

int main() {
	vector<TestSuiteResult> suites = {
		run_binom_suite(),
		run_count_suite(),
		run_scale_invariance_suite(),
		run_degree_suite(),
		run_is_degen_suite(),
		run_calc_sum_suite(),
		run_pi_values_suite(),
		run_pi_factors_suite(),
		run_generation_suite(),
		run_compare_suite(),
		run_mult_suite(),
		run_gcd_rinse_suite(),
		run_rinse_suite(),
		run_bad_search_suite(),
		run_self_dual_suite(),
		run_large_betti_suite()
	};

	print_test_report(suites);
	return all_suites_passed(suites) ? 0 : 1;
}
