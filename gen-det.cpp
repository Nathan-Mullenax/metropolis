#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <cstdlib>
#include <ctime>

using std::string;
using std::runtime_error;
using std::stringstream;

namespace {
  const string dirs("LRSB");
  const string symbols(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		       "~!@#$%^&*()_+`1234567890-=[]\\{}|/.,?><';\":");
}

string make_rule( unsigned s0, unsigned s1, char rs, char ws, char dir ) {
  stringstream rule;
  rule << "{" << s0 << "," << s1 
       << "}['" << rs << "','" 
       << ws << "'," << dir << "]";
  return rule.str();
}

void generate_all_rules( unsigned nstates, unsigned nsymbols ) {
  for( unsigned s0=0; s0<nstates; ++s0 ) {
    for( unsigned symbol=0; symbol<nsymbols; ++symbol ) {
      std::cout << 
	make_rule( s0, rand()%nstates, 
		   symbols[symbol], symbols[rand()%nsymbols], 
		   dirs[rand()%4] ) 
		<< "\n";
    }
  }
}

int main( int argc, char **argv ) {
  std::srand(std::time(NULL));
  stringstream args;
  
  unsigned nstates, nsymbols;
  args << argv[1] << " " << argv[2];
  args >> nstates;
  args >> nsymbols;
  generate_all_rules( nstates, nsymbols );
}
