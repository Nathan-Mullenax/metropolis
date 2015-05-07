#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <string>

using std::srand;
using std::rand;
using std::cout;
using std::string;
using std::stringstream;

int main(int argc, char **argv ) {
  srand(time(NULL));
  int nstates(2);
  int nsymbols(3);
  float fillrate(3);

  if( argc==4 ) {
    stringstream args;
    args << argv[1] << " " << argv[2];
    args >> nstates; 
    args >> nsymbols;
    args >> fillrate;
  }
  int transitions = static_cast<int>(nstates*nsymbols*fillrate);
  string symbols(" ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		 "abcdefghijklmnopqrstuvwxyz"
		 "~-_%$#@!*&^=+:;/?<>.{}[]()0123456789`'\"" );
  string dirs("LRSB");
  for( int i=0; i<transitions; ++i) {
    int s0 = rand()%nstates;
    int s1 = rand()%nstates;
    char rs = symbols[ (rand()%nsymbols) ];
    char ws = symbols[ (rand()%nsymbols) ];
    if(i==0) { rs = ' '; }

    char dir = dirs[rand()%4];
    std::cout << "{" << s0 << "," << s1 << "}['" << rs << "','" << ws 
	      << "'," << dir << "]\n";
  }
  
}
