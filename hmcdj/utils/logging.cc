#include <hmcdj/utils/logging.h>

Grid::Colours DJLogColours(0);
DJLogger DJLogTiming(1, "Timing", DJLogColours, "NORMAL");
DJLogger DJLogMessage(1, "Message", DJLogColours, "NORMAL");
DJLogger DJLogError(1, "Error", DJLogColours, "RED");
DJLogger DJLogDebug(1, "Debug", DJLogColours, "NORMAL");
