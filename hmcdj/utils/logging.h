#pragma once

#include <Grid/Grid.h>

/* HMCDJ Logging

   Uses the base logging functioanality built into Grid,
   but subclasses and creates specific instances for HMCD-specific logs.
   Such logs will be tagged `hmcdj` rather than `Grid`. */

class DJLogger : public Grid::Logger {
 public:
  DJLogger(int on, std::string nm, Grid::Colours& col_class,
           std::string col_key = "NORMAL")
      : Logger("hmcdj", on, nm, col_class, col_key){};
};

extern DJLogger DJLogTiming;
extern DJLogger DJLogMessage;
extern DJLogger DJLogError;
extern DJLogger DJLogDebug;
extern Grid::Colours GridLogColours;
