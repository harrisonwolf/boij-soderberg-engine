all: bad_one_generator parse_huge_output remove_duplicates test_program

bad_one_generator: bad_one_generator.o seq_funcs.o
	g++ -o bad_one_generator bad_one_generator.o seq_funcs.o binom.o

parse_huge_output: parse_huge_output.o seq_funcs.o
	g++ -o parse_huge_output parse_huge_output.o seq_funcs.o binom.o

remove_duplicates: remove_duplicates.o seq_funcs.o
	g++ -o remove_duplicates remove_duplicates.o seq_funcs.o binom.o

test_program: test_program.o seq_funcs.o
	g++ -o test_program test_program.o seq_funcs.o binom.o

bad_one_generator.o: bad_one_generator.cc seq_funcs.o
	g++ -c bad_one_generator.cc 

parse_huge_output.o: parse_huge_output.cc seq_funcs.o
	g++ -c parse_huge_output.cc

remove_duplicates.o: remove_duplicates.cc seq_funcs.o
	g++ -c remove_duplicates.cc

test_program.o: test_program.cc seq_funcs.o
	g++ -c test_program.cc

binom.o: binom.cc binom.h
	g++ -c binom.cc binom.h

seq_funcs.o: seq_funcs.cc seq_funcs.h binom.o
	g++ -c seq_funcs.cc seq_funcs.h 

test_funcs.o: test_funcs.cc test_funcs.h binom.o
	g++ -c test_funcs.cc test_funcs.o 

.PHONY: all clean

clean:
	rm *.o bad_one_generator parse_huge_output remove_duplicates test_program
