#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <string>
#include <vector>

struct TestCaseResult {
	std::string name;
	bool passed;
	std::string expected;
	std::string actual;
};

struct TestSuiteResult {
	std::string name;
	std::vector<TestCaseResult> cases;

	std::size_t passed_count() const;
	std::size_t failed_count() const;
};

void print_test_report(const std::vector<TestSuiteResult>& suites);

bool all_suites_passed(const std::vector<TestSuiteResult>& suites);

#endif
