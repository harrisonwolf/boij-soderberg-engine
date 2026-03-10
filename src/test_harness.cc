#include "test_harness.h"

#include <iostream>

using namespace std;

namespace {

constexpr const char* kReset = "\033[0m";
constexpr const char* kGreen = "\033[32m";
constexpr const char* kYellow = "\033[33m";
constexpr const char* kRed = "\033[31m";

const char* suite_color(const TestSuiteResult& suite) {
	if(suite.cases.empty()) return kYellow;
	if(suite.passed_count() == suite.cases.size()) return kGreen;
	if(suite.passed_count() == 0) return kRed;
	return kYellow;
}

const char* suite_label(const TestSuiteResult& suite) {
	if(suite.cases.empty()) return "EMPTY";
	if(suite.passed_count() == suite.cases.size()) return "PASS";
	if(suite.passed_count() == 0) return "FAIL";
	return "PARTIAL";
}

}

size_t TestSuiteResult::passed_count() const {
	size_t total = 0;
	for(const TestCaseResult& test_case : cases){
		if(test_case.passed) total++;
	}
	return total;
}

size_t TestSuiteResult::failed_count() const {
	return cases.size() - passed_count();
}

void print_test_report(const vector<TestSuiteResult>& suites) {
	cout << "Algorithm test suites\n";
	for(const TestSuiteResult& suite : suites){
		cout << suite_color(suite) << "[" << suite_label(suite) << "] " << kReset
		     << suite.name << " (" << suite.passed_count() << "/" << suite.cases.size() << ")\n";

		if(suite.failed_count() == 0) continue;

		for(const TestCaseResult& test_case : suite.cases){
			if(test_case.passed) continue;
			cout << "  " << kRed << "[FAIL]" << kReset << " " << test_case.name << "\n";
			cout << "    expected: " << test_case.expected << "\n";
			cout << "    actual:   " << test_case.actual << "\n";
		}
	}
}

bool all_suites_passed(const vector<TestSuiteResult>& suites) {
	for(const TestSuiteResult& suite : suites){
		if(suite.failed_count() > 0) return false;
	}
	return true;
}
