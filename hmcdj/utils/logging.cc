#include <hmcdj/utils/logging.h>

Grid::Colours DJLogColours(0);
DJLogger DJLogTiming(1, "Timing", DJLogColours, "NORMAL");
DJLogger DJLogMessage(1, "Message", DJLogColours, "NORMAL");
DJLogger DJLogError(1, "Error", DJLogColours, "RED");
DJLogger DJLogDebug(1, "Debug", DJLogColours, "NORMAL");

/* Borrowing heavily from https://wordaligned.org/articles/cpp-streambufs */

// Construct a streambuf which tees output to both input
// streambufs.
teebuf::teebuf(std::streambuf* first, std::streambuf* second)
    : first(first), second(second) {}

void teebuf::resetFirst(std::streambuf* newFirst) { first = newFirst; }
void teebuf::resetSecond(std::streambuf* newSecond) { second = newSecond; }

// This tee buffer has no buffer. So every character "overflows"
// and can be put directly into the teed buffers.
int teebuf::overflow(int c) {
  if (c == EOF) {
    return !EOF;
  } else {
    int const r1 = first->sputc(c);
    int const r2 = second->sputc(c);
    return r1 == EOF || r2 == EOF ? EOF : c;
  }
}

// Sync both teed buffers.
int teebuf::sync() {
  int const r1 = first->pubsync();
  int const r2 = second->pubsync();
  return r1 == 0 && r2 == 0 ? 0 : -1;
}

teestdout::teestdout() {
  /* Construct a tee to stdout and an in-memory buffer */
  internalBuffer = std::make_unique<std::stringstream>();
  outputBuffer = internalBuffer->rdbuf();
  originalStdout = std::cout.rdbuf();
  tee = std::make_shared<teebuf>(outputBuffer, originalStdout);
  std::cout.rdbuf(tee.get());
}

teestdout::teestdout(std::string outputPath) : teestdout() {
  /* Construct a tee to stdout and an on-disk buffer */
  setOutput(outputPath);
}

void teestdout::setOutput(std::string outputPath) {
  /* setOutput should only be called once. */
  assert(internalBuffer != nullptr);

  /* Reset the output of a tee to a file. */
  outputStream = new std::ofstream(outputPath);
  if (internalBuffer != nullptr) {
    /* Output and free the internal buffer before continuing */
    *outputStream << internalBuffer->str();
    internalBuffer = nullptr;
  }
  /* Note that we don't need to free the outputBuffer,
     as this was released along with internalBuffer */
  outputBuffer = outputStream->rdbuf();
  tee->resetFirst(outputBuffer);
}

teestdout::~teestdout() {
  /* Restore stdout to its original state */
  std::cout.rdbuf(originalStdout);
}

void setUpLogging(std::filesystem::path baseDir, teestdout& tee) {
  Grid::GridLogLayout();
  std::string filename =
      std::format("hmcdj_{0:%F}_{0:%H}{0:%M}{0:%S}.log",
                  std::chrono::time_point_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now()));
  std::filesystem::path outputFile = baseDir / "logs" / filename;
  tee.setOutput(outputFile.string());
}
