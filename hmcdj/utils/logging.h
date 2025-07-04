#pragma once

#include <Grid/Grid.h>

class DJLogger : public Grid::Logger {
 public:
  DJLogger(int on, std::string nm, Grid::Colours& col_class,
           std::string col_key = "NORMAL")
      : Logger("hmcdj", on, nm, col_class, col_key){};
};

extern DJLogger DJLogTiming;
extern Grid::Colours GridLogColours;
