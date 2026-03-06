//header file for testing conjs. that doesn't require computing entire pure_betti (since it isn't completely necessary for big numbers)
#ifndef FAST_TESTS_H
#define FAST_TESTS_H

#include <vector>

//WARNING: THESE TAKE DEGREE SEQUENCES, NOT BETTI TABLES

bool test_BEH_fast(std::vector<int> D);

bool test_LLBC_fast(std::vector<int> D);

#endif
