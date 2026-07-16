#include <algorithm>
#include <cctype>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "algorithm_helpers.h"
#include "seq_funcs.h"
#include "test_funcs.h"

namespace {

std::string trim(const std::string &input) {
	size_t start = 0;
	while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
		++start;
	}

	size_t end = input.size();
	while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
		--end;
	}

	return input.substr(start, end - start);
}

bool is_quit_command(const std::string &line) {
	std::string lowered = trim(line);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return lowered == "q" || lowered == "quit" || lowered == "exit";
}

bool parse_degree_sequence(const std::string &line, std::vector<int> &degrees, std::string &error) {
	degrees.clear();

	std::istringstream stream(line);
	int value = 0;
	while (stream >> value) {
		degrees.push_back(value);
	}

	if (!stream.eof()) {
		error = "Input must contain only integers separated by spaces.";
		return false;
	}

	if (degrees.size() < 2) {
		error = "A degree sequence must contain at least two entries and start with 0.";
		return false;
	}

	if (degrees.front() != 0) {
		error = "A degree sequence must start with 0.";
		return false;
	}

	for (size_t i = 1; i < degrees.size(); ++i) {
		if (degrees[i] <= degrees[i - 1]) {
			error = "A degree sequence must be strictly increasing.";
			return false;
		}
	}

	return true;
}

std::string rational_to_string(const RationalValue &value) {
	if (value.den == 1) {
		return std::to_string(value.num);
	}
	return std::to_string(value.num) + "/" + std::to_string(value.den);
}

std::string shift_term(int degree, long long betti_number) {
	std::ostringstream out;
	out << "S";
	if (degree != 0) {
		out << "(-" << degree << ")";
	}
	out << "^" << betti_number;
	return out.str();
}

void print_report(const std::vector<int> &degrees) {
	const std::vector<RationalValue> pi_values = compute_pi_values(degrees);
	const std::vector<long long> betti_numbers = pure_betti(degrees);
	const long long L = calc_L(degrees);
	const bool passes_beh = test_BEH(betti_numbers);
	const bool passes_llbc = test_LLBC(betti_numbers);

	std::cout << "\nDegree sequence: " << seq_to_string(degrees) << "\n";
	std::cout << "Codimension: " << (degrees.size() - 1) << "\n";
	std::cout << "L value: " << L << "\n";
	std::cout << "Pure Betti numbers B_i: " << pure_betti_to_string(betti_numbers) << "\n";
	for (size_t i = 0; i < betti_numbers.size(); ++i) {
		std::cout << "  B_" << i << " = " << betti_numbers[i] << "\n";
	}
	std::cout << "pi_i values:\n";
	for (size_t i = 0; i < pi_values.size(); ++i) {
		std::cout << "  pi_" << i << " = " << rational_to_string(pi_values[i]) << "\n";
	}

	std::cout << "Pure Betti resolution:\n";
	std::cout << "  0 <- M <- ";
	for (size_t i = 0; i < degrees.size(); ++i) {
		if (i > 0) {
			std::cout << " <- ";
		}
		std::cout << shift_term(degrees[i], betti_numbers[i]);
	}
	std::cout << " <- 0\n";

	std::cout << "Conjecture checks:\n";
	std::cout << "  BEH: " << (passes_beh ? "passes" : "violates") << "\n";
	std::cout << "  LLBC: " << (passes_llbc ? "passes" : "violates") << "\n";
}

}  // namespace

int main() {
	std::cout << "Pure Betti calculator\n";
	std::cout << "Enter a degree sequence as space-separated integers beginning with 0.\n";
	std::cout << "Example: 0 3 5 8\n";
	std::cout << "Type q, quit, or exit to leave.\n";

	std::string line;
	while (true) {
		std::cout << "\nSequence> ";
		if (!std::getline(std::cin, line)) {
			std::cout << "\n";
			break;
		}

		if (is_quit_command(line)) {
			break;
		}

		if (trim(line).empty()) {
			continue;
		}

		std::vector<int> degrees;
		std::string error;
		if (!parse_degree_sequence(line, degrees, error)) {
			std::cout << "Invalid degree sequence: " << error << "\n";
			continue;
		}

		print_report(degrees);
	}

	return 0;
}
