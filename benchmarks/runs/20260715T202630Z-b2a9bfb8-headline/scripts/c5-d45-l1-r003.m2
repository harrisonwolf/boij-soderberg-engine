-- Task-matched benchmark oracle using Macaulay2's BoijSoederberg package.
-- The Python studio substitutes only the three integer placeholders below.

boijPackage = loadPackage "BoijSoederberg";
boijPackageVersion = toString((options boijPackage)#Version);

CODIM = 5;
MAXDEG = 45;
LOWBOUND = 1;

genDegSeqsBuiltIn = (c,d,lowbound) -> (
    lb := max(1, lowbound);
    select(subsets(toList(1..d), c), seq -> last(seq) >= lb) / (seq -> {0} | seq)
    );

isBEHViolator = (B,c) -> (
    any(toList(0..c), i -> B_i < binomial(c,i))
    );

isLLBCViolator = (B,c) -> (
    sum B < (2^c * 3 / 2)
    );

isBad = (D,c) -> (
    B := pureBetti D;
    isBEHViolator(B,c) or isLLBCViolator(B,c)
    );

gcdRinseBuiltIn = (seqs) -> (
    select(seqs, seq -> gcd(seq#(#seq - 1), seq#(#seq - 2)) == 1)
    );

generationStart := cpuTime();
possibleSeqs := genDegSeqsBuiltIn(CODIM, MAXDEG, LOWBOUND);
generationSeconds := cpuTime() - generationStart;

testingStart := cpuTime();
badOnes := select(possibleSeqs, seq -> isBad(seq, CODIM));
testingSeconds := cpuTime() - testingStart;

rinseStart := cpuTime();
rinsedBadOnes := gcdRinseBuiltIn(badOnes);
rinseSeconds := cpuTime() - rinseStart;

<<"BOIJ_META package_version=" << boijPackageVersion
  << " candidate_count=" << #possibleSeqs
  << " bad_count=" << #badOnes
  << " gcd_rinsed_count=" << #rinsedBadOnes
  << " generation_cpu_seconds=" << generationSeconds
  << " testing_cpu_seconds=" << testingSeconds
  << " gcd_rinse_cpu_seconds=" << rinseSeconds << endl;
<<"BOIJ_BAD_BEGIN" << endl;
for seq in badOnes do << toString(seq) << endl;
<<"BOIJ_BAD_END" << endl;
<<"BOIJ_GCD_RINSED_BEGIN" << endl;
for seq in rinsedBadOnes do << toString(seq) << endl;
<<"BOIJ_GCD_RINSED_END" << endl;

exit 0
