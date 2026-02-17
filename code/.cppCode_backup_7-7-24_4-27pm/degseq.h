//header file for degree sequence class
#ifndef DEGSEQ_H
#define DEQSEQ_H

class degseq{
private:
	vector<int> seq; //the d_i's
	int c; //codimension (will always be size-1)
	
public:
	/* Constructors */
	deqseq(); //default constructor
	degseq(vector<int> D);

	/* Accessors */
	int get(size_t index); //returns d_index //prints (somewhat) detailed error message if index out of bounds
	int get_c(){ return c; }
	int get_D(){  }

	/* Mutators */ 

	/* Miscellaneous */
	string to_string(); //returns "{0,d_1,...,d_c}"
};

#endif
