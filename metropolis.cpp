#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <cctype>
#include <cstdlib>
#include <ctime>

using std::runtime_error;
using std::string;
using std::vector;
using std::map;
using std::stringstream;
using std::pair;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::isspace;
using std::isalpha;
using std::rand;
using std::srand;
using std::time;
using std::set;

typedef enum { Left, Right, Straight, Backward } direction;
typedef enum { North, South, East, West } cdirection;
const string symbols(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		     "~!@#$%^&*()_+`1234567890-=[]\\{}|/.,?><';\":");
// takes a direction and applies a transformation
cdirection apply( cdirection d, direction t ) {
  switch(t) {
  case Left:
    switch(d) {
    case North: return West;
    case South: return East;
    case East: return North;
    case West: return South;
    }
  case Right:
    switch(d) {
    case North: return East;
    case South: return West;
    case East: return South;
    case West: return North;
    }
  case Straight:
    return d;
  case Backward:
    switch(d) {
    case North: return South;
    case South: return North;
    case East: return West;
    case West: return East;
    }
  }
}

// rule represents one clause of a transition function
// f( state, read_symbol ) : ( new_state, write_symbol, direction ) % weight
class rule {
public:
  // start state, end state
  string s0,s1; 
  // change in direction
  direction dir;
  // read symbol, write symbol
  char rs,ws;
  // weight
  unsigned long weight;

  rule( string s0, string s1, char rs, char ws, char dir ) 
    : s0(s0), s1(s1), rs(rs), ws(ws), weight(1)
  {
    switch(dir) {
    case 'l': case 'L': this->dir = Left; break;
    case 'r': case 'R': this->dir = Right; break;
    case 's': case 'S': this->dir = Straight; break;
    case 'b': case 'B': this->dir = Backward; break;
    default:
      throw runtime_error("Invalid direction.");
    }
  }

  rule() 
  {
    weight=1;
    s0 = "0";
    s1 = "0";
    rs = ' ';
    ws = '*';
    dir= Left;
  }

  // create a short, readable string representing this rule
  string str() {
    stringstream ss;
    ss << "'" << rs << "'->'" << ws << "'";
    switch(dir) {
    case Left: ss <<     " L "; break;
    case Right: ss <<    " R "; break;
    case Straight: ss << " S "; break;
    case Backward: ss << " B "; break;
    default: ss <<       " ERROR "; break;
    }
    
    ss << s0 << "->" << s1;
    return ss.str();
  }

  // given an initial state and symbol, randomly generate a rule 
  // (direction, state, and write symbol)
  // states are assumed to be named after integer 0 through nstates-1
  void randomize( unsigned s0, char rs, unsigned nstates, string const &symbols ) {
    stringstream ss;
    ss << s0;
    this->s0 = ss.str ();
    this->rs = rs;
    stringstream p;
    p << ( rand()%nstates );    
    p >> this->s1;
    ws = symbols[rand()%symbols.size()];
    switch(rand()%4) {
    case 0: dir = Left; break;
    case 1: dir = Right; break;
    case 2: dir = Straight; break;
    case 3: dir = Backward; break;
    }
    weight=1;
  }
};

class ttable {
public:
  vector<rule> rules;
  // support lookup operator ( state, read symbol -> applicable rule index )
  map< pair<string,char>, vector<size_t> > index;
  
  // check if there is a rule
  bool has( string s, char c ) {
    return index.count( pair<string,char>(s,c) ) > 0;
  }

  // create an index by state and read symbol
  void build_index() {
    index.clear();
    for( size_t i=0; i<rules.size(); ++i) {
      index[ pair<string,char>(rules[i].s0,rules[i].rs)].push_back(i);
    }
  }

  // lookup a rule. NB: call has first or this will
  // throw an exception. If this throws an exception,
  // you might have changed a ruleset without calling
  // build index.
  rule & operator() ( string s, char c ) {
    pair<string,char> l(s,c);
    if( index.count(l) > 0 ) {
      vector<size_t> rs = index[l];
      unsigned int total_weight(0);
      for( size_t r : rs ) {
	total_weight += rules[r].weight;
      } 
      size_t selected_rule(0);
      do {
	selected_rule = rand()%rs.size();
      } while( rand()%total_weight > rules[rs[selected_rule]].weight );
      return rules[rs[selected_rule]];
    } 
    throw runtime_error("ttable lookup error. No such rule.");
  }
};

// represents the 2D grid on which the automaton is run
class space {
private:
  struct datum { char c; short color; };
  map< pair<int,int>, datum > data;
public:
  // lookup coordinate
  char& operator() ( int x, int y ) {
    pair<int,int> l(x,y);
    if( data.count(l) == 0 ) {
      data[l].c = ' ';
      data[l].color = 0;
    }
    return data[l].c;
  }
  
  // the () operator doesn't return all of the info
  // stored in the map.
  void get( int x, int y, char &c, short &color ) {
    pair<int,int> l(x,y);
    if( data.count(l) == 0 ) {
      data[l].c = ' ';
      data[l].color = 0;
    }
    c = data[l].c;
    color = data[l].color;
  }

  // similarly, using space(x,y) = 'v' doesn't allow us
  // to assign a color
  void put( int x, int y, char c, short color ) {
    datum d;
    d.c = c; d.color = color;
    pair<int,int> l(x,y);
    data[l] = d;
  }
};

// where the next symbol is to be
// read and then written, also has
// direction and state.
class turtle {
public: 
  int x,y;
  cdirection dir;
  string state;
  short color;
  turtle() : x(0), y(0), dir(South), state("0"), color(1) {
  }

  // create a brief description of the current
  // state of this turtle
  string str() {
    stringstream t;
    t << x << "," << y << ": " << state;
    return t.str();
  }
};

// move the turtle according to ruleset. modify space
// accordingly
void step( turtle &t, ttable &ruleset, space &s ) {
  // get symbol at current location
  char rs = s(t.x,t.y);
  // current state of turtle
  string s0 = t.state;
  if( ruleset.has( s0,rs) ) {
    // lookup appropriate rule. Note: non-determinism 
    // is supported by the lookup function, operator() for
    // the ttable class. 
    rule r = ruleset( s0, rs );
    // turn
    t.dir = apply( t.dir, r.dir );
    // do state transition
    t.state = r.s1;
    // write symbol
    s.put(t.x,t.y, r.ws,t.color);
  }
  // move
  switch(t.dir) {
  case North: --t.y; break;
  case South: ++t.y; break;
  case East: ++t.x; break;
  case West: --t.x; break;
  }
}

void reset( turtle &t, space &s ) {
  t = turtle();
  s = space();
}

void show_help() {
  clear();
  move(0,0);
  printw( "HELP - Metropolis Runner\n" );
  printw( "-----------------------------------------------\n" );
  printw( "Keys:\n" );
  printw( "   F1 = Help, but you already figured that out.\n" );
  printw( "   F2 = Show rules\n" );
  printw( "   F3 = Reset\n" );
  printw( "   F4 = Generate random turtle\n");
  printw( "      Parameters:\n");
  printw( "      N - increase number of states\n");
  printw( "      n - decrease number of states\n");
  printw( "      S - increate number of symbols\n");
  printw( "      s - decrease number of symbols\n");
  printw( "   F5 = Run\n" );
  printw( "    C = Change camera\n" );
  printw( "    T = Switch turtle\n" );
  printw( "    Q = Quit\n" );
}

void show_ttable(turtle &tur, ttable const &t, int width, int height) {
  clear();

  printw( tur.str().c_str() );
  printw("\n\n");

  attron(COLOR_PAIR( tur.color ) );
  int cwidth = 20;
  int row(0),col(0);
  int maxcells = (width/cwidth)*height;
  int i(0);
  for( rule r : t.rules ) {
    if( i < maxcells-1 ) {
      string sr(r.str());
      move(row,col);
      printw( sr.c_str() );
      ++row;
      if(row>=height) {
	row = 0;
	col += cwidth;
      }
    } else {
      ++row;
      move(row,col);
      printw("...");
      return;
    }
    ++i;
    
  }
  attroff(COLOR_PAIR( tur.color ) );
}

void killspace( istream &s ) {
  while( isspace(s.peek()) ) { s.get(); }
}

void expect( istream &f, string const &s ) {
  killspace(f);
  for( char c : s ) {
    if( f.peek()!=c ) {
      throw runtime_error( "Invalid syntax!" );
    }
    f.get();
  }
}

bool not_among( char c, string const &s ) {
  for( char t : s ) {
    if( c==t )
      return false;
  }
  return true;
}

string read_name( istream &f ) {
  stringstream name;
  while( not_among( f.peek(), "[]{}%, \t\n\r") ) {
    name << static_cast<char>( f.get() );
  }  
  return name.str();
}

unsigned int read_int( istream &f ) {
  unsigned int r=0;
  killspace(f);
  while(isdigit(f.peek()))
    r = r*10 + (f.get()-'0');
  return r;
}

// characters are single quoted
char read_char( istream &f ) {
  killspace(f);
  expect(f,"'");
  char c = f.get();
  expect(f,"'");
  return c;
}

direction read_dir( istream &f )
{
  killspace(f);
  if( not_among(f.peek(),"SBLRsblr") ) {
    throw runtime_error("Invalid direction.");
  }
  switch(f.get()) {
  case 'L': case 'l': return Left;
  case 'R': case 'r': return Right;
  case 'S': case 's': return Straight;
  case 'B': case 'b': return Backward;
  }
}

rule read_rule( istream &f ) {
  rule r;
  expect(f,"{");
  r.s0 = read_name(f);
  expect(f,",");
  r.s1 = read_name(f);
  expect(f,"}");
  expect(f,"[");
  r.rs = read_char(f );
  expect(f,",");
  r.ws = read_char(f );
  expect(f,",");
  r.dir = read_dir(f);
  expect(f,"]");
  killspace(f);
  if( !f.eof() &&f.peek()=='%' ) {
    expect(f,"%");
    r.weight = read_int(f);
  } else {
    r.weight = 1;
  }
  return r;
}

ttable load_rules( string const &filename ) {
  ttable p;
  ifstream f(filename);
  killspace(f);
  while( !f.eof() ) {
    rule r = read_rule(f);
    p.rules.push_back(r);
    killspace(f);
  }
  p.build_index();
  return p;
}

void show_space( space &s, turtle &t, int width, int height, int x0, int y0 ) {
  move(0,0);
  for( int y=y0; y<y0+height; ++y ) {  
    for( int x=x0; x<x0+width; ++x ) {
      char c(0);
      short color(0);
      s.get(x,y,c,color);
      attron(COLOR_PAIR( color ) );
      addch(c);
      attroff(COLOR_PAIR( color ) );
    }
   
  }
}

int main( int argc, char **argv ) {
  int nstates(2);
  int nsymbols(5);

  stringstream log;
  srand(time(NULL));

  initscr();
  start_color();

  // ttable program;
  vector<ttable> programs;
  
  vector<turtle> turtles;
  size_t current_turtle=0;

  space s;
  for(int i=0; i<COLOR_PAIRS; ++i) {
    init_pair( i, (i+1)%COLORS, COLOR_BLACK );
  }
  if( argc==1 ) {
    ttable program;
    program.rules.push_back( rule( "0","0",' ', '*','l' ) );
    program.rules.push_back( rule( "0","0",'*', ' ','r' ) );
    program.build_index();
    programs.push_back(program);
    turtles.push_back(turtle());
    
  } else {
    for(int i=1; i<argc; ++i) {
      programs.push_back(load_rules( string(argv[i]) ));
      turtle t;
      t.color = i+1;
      turtles.push_back(t);
      //  init_pair(i+1, (i+1)%COLORS, COLOR_BLACK );
    }
  }
 

  // console dimensions
  int cwidth,cheight;
  
  getmaxyx(stdscr,cheight,cwidth);  
  nodelay(stdscr,true);
  keypad(stdscr,true);
  noecho();
  
  enum { rules, result, help } mode;
  enum { t_cam, re_center, user, nocam } camera;
  camera = re_center;
  mode = result;
  bool redraw(true);

  int x0(-cwidth/2), y0(-cheight/2);

  while(true) {

    int c = getch();
   
    switch(c) {
    case 'Q':
    case 'q': goto end; break;
    case 'c':
    case 'C':
      switch(camera) {
      case t_cam: camera=re_center; break;
      case re_center: camera=user; break;
      case user: camera=nocam; break;
      case nocam: camera=t_cam; break;
      }
      break;
    case 't':
      current_turtle = (current_turtle+1)%turtles.size();
      redraw = true;
      break;
    case KEY_F(5): mode = result; redraw = true; break;
    case KEY_F(2): mode = rules; redraw = true; break;
    case 'N': nstates++; break;
    case 'n': nstates = nstates > 1 ? nstates-1 : 1; break;
    case 'S': nsymbols = (nsymbols < symbols.size()) ? nsymbols+1 : 1; break;
    case 's': nsymbols = nsymbols > 1 ? nsymbols-1: 1; break;
    case KEY_F(4): 
      {
	programs = vector<ttable>();
	turtles = vector<turtle>();
	s = space();
	
	// generate a deterministic automaton
	ttable p;
	string alpha = symbols.substr(0,nsymbols);
       
      
	for( unsigned s0=0; s0<nstates; ++s0 ) {
	  for( unsigned symbol=0; symbol<nsymbols; ++symbol ) {
	    rule r;
	   
	     
	    r.randomize( s0, symbols[symbol], nstates, alpha );
	   

	    p.rules.push_back( r );
	  }
	}
	current_turtle = 0;
	p.build_index();
	programs.push_back(p);
	turtle t;
	t.state = "0";
	t.color=1 + (rand()%(COLOR_PAIRS-1));
	turtles.push_back(t);
	
      } 
      break;
    case KEY_F(6):
      {
	ttable p = programs[current_turtle];
	turtle t = turtles[current_turtle];
	t.dir = apply( t.dir, Left );
	turtles.push_back(t);
	programs.push_back(p);
      }
      break;
    case KEY_F(3): 
      mode = result;
      {
	int i=2;
	for( turtle &t : turtles ) {
	  t = turtle();
	  t.color = i++;
	}
      }
      reset( turtles[0],s ); 
      redraw = true; 
      break;
    case KEY_F(1): mode = help; redraw = true; break;
    case KEY_LEFT: --x0; redraw=true; break;
    case KEY_RIGHT: ++x0; redraw=true; break;
    case KEY_UP: --y0; redraw=true; break;
    case KEY_DOWN: ++y0; redraw=true; break;
    }
    
    if(mode==result) {
      for( size_t i=0; i<turtles.size(); ++i) {
	step(turtles[i],programs[i],s);
      }
      switch(camera) {
      case t_cam: 
	{
	  x0 = turtles[current_turtle].x - cwidth/2;
	  y0 = turtles[current_turtle].y - cheight/2;
	  show_space(s,turtles[current_turtle],cwidth,cheight,x0,y0); 
	}
	break;
      case re_center:
	{
	  turtle &t = turtles[current_turtle];
	  if( t.x < x0 || t.x > x0 + cwidth || t.y < y0 || t.y > y0 + cheight) {
	    x0 = t.x - cwidth/2;
	    y0 = t.y - cheight/2;
	  }
	  show_space(s,t,cwidth,cheight,x0,y0);
	}
	break;
      case user:
	{
	  show_space(s,turtles[current_turtle],cwidth,cheight,x0,y0);
	}
	break;
      }
      move(0,0);
      switch( camera ) {
      case t_cam:     printw("Turtle-centered."); break;	
      case user:      printw("User-controlled."); break;
      case re_center: printw("Center on exit. "); break;
      case nocam:     printw("---Camera off---"); break;
      }
    }
    if( redraw ) {
      switch(mode) {
      case rules: show_ttable( turtles[current_turtle], programs[current_turtle],cwidth,cheight ); break;
      case help: show_help(); break;
      }
    }
    
    refresh();
    redraw = false;
  }
 end:
  endwin();
  std::cout << log.str();
}
