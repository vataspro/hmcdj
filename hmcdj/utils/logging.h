#pragma once

#include <Grid/Grid.h>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>

/* HMCDJ Logging

   Uses the base logging functioanality built into Grid,
   but subclasses and creates specific instances for HMCD-specific logs.
   Such logs will be tagged `hmcdj` rather than `Grid`.

   Additionally, add the capability to mirror logs to a file as well as
   stdout.*/

class teebuf : public std::streambuf {
 public:
  teebuf(std::streambuf* first, std::streambuf* second);
  void resetFirst(std::streambuf* newFirst);
  void resetSecond(std::streambuf* newSecond);

 private:
  virtual int overflow(int c);
  virtual int sync();

  std::streambuf* first;
  std::streambuf* second;
};

class teestdout {
 public:
  teestdout();
  teestdout(std::string outputPath);
  void setOutput(std::string outputPath);
  ~teestdout();

 private:
  std::unique_ptr<std::stringstream> internalBuffer;
  std::streambuf* outputBuffer;
  std::streambuf* originalStdout;
  std::ofstream* outputStream;
  std::shared_ptr<teebuf> tee;
};

class DJLogger : public Grid::Logger {
 public:
  DJLogger(int on, std::string nm, Grid::Colours& col_class,
           std::string col_key = "NORMAL")
      : Logger("hmcdj", on, nm, col_class, col_key){};
};

void setUpLogging(std::filesystem::path baseDir, teestdout& tee);

extern DJLogger DJLogTiming;
extern DJLogger DJLogMessage;
extern DJLogger DJLogError;
extern DJLogger DJLogDebug;
extern Grid::Colours GridLogColours;
