
#include "cpp14.h"

#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#include <grpc++/grpc++.h>

#include "trade.grpc.pb.h"



using marketfeed::Trade; 
using marketfeed::ApplicationTickerSource;


#include <ncurses.h>

enum style {
  NORMAL	= 1,
  DOWN		= 2,
  UP		= 3,
  INFORMATION	= 4
};

static struct Curses {
  Curses(){
    initscr();
    start_color();
    
    init_pair(NORMAL,      COLOR_WHITE,  COLOR_BLACK);
    init_pair(DOWN,        COLOR_WHITE,  COLOR_RED);
    init_pair(UP,          COLOR_BLACK,  COLOR_GREEN);
    init_pair(INFORMATION, COLOR_YELLOW, COLOR_BLACK);
  }
  ~Curses(){
    endwin();
  }
} Manager;


void update_display(Trade t){

  using namespace std;

  // indicate too-long string with "*" at end.
  auto mvcaddstr = [](int r,int c,style color, string txt){
    if(c < COLS){
      int truncation = COLS - 2 - c; 
      if (truncation >= 0 && truncation < int(txt.size())){
	txt = txt.substr(0,truncation) + "*";
      }
      attron(COLOR_PAIR(color));
      mvaddstr(r,c,txt.c_str());
      attroff(COLOR_PAIR(color));
    }
  };

  
  static map<string,Trade> m;

  m[t.ticker()] = t;

  int line = 0;
  int column=0;

  erase();

  for (auto kv : m){

    auto ticker = kv.first;
    auto t = kv.second;

    if (0 == line){
      mvcaddstr(0,column,NORMAL,"ticker              last         open     change        %");
      line = 1;
    }

    move(line,column);

    double change = t.price() - t.open();
    double pct = 100.0 * change / t.open();

    ostringstream oss;
    oss << fixed << setprecision(4)
	<< " "
        << setw(10)  << left   << ticker 
	<< setw(13)  << right  << t.price()
	<< setw(13)  << right  << t.open() 
	<< setw(11)  << right  << change
        << setprecision(2)
	<< setw(9)   << right  << pct
	<< "   ";

    auto color = (change < 0) ? DOWN : (change > 0) ? UP : NORMAL;
    auto txt = oss.str();
    mvcaddstr(line,column,color,txt);

    if (++line >= LINES-1){
      line = 0;
      column += txt.size();
    }
    
  }
  
  refresh();
}



