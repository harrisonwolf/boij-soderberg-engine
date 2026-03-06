//implementation file for fast_tests.h

#include "fast_tests.h"
#include "seq_funcs.h"
using namespace std;

/*
 * For both of these, I only need to compute B_i(D) only so far as to test the conj. Eg. if L > LLBC target, I don't even need to compute anything since just L*pi_0 will be enough to pass
 * Similarly, if
 *
 * Idea: If my numbers are overflowing or even getting close to doing to, it pretty much auto-passed
 * For test_BEH_fast, c choose i is greatest at c choose (c/2). So set this to the upper bound and if at any step in computing B(D) a number surpasses this, we're done
 * Same for LLBC_fast: We need the sum to be 2^c * 1.5. If each B_i is at least target/(c+1), we're done. And in particular, if the first B, which is the lowest, is at least
 * target/(c+1), it passes
 */

bool test_BEH_fast(std::vector<int> D{
		
}

bool test_LLBC_fast(std::vector<int> D){

}
