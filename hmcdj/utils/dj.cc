#include <hmcdj/utils/dj.h>

char** getGridArgv(int argc, char* argv[], const char* grid) {
  std::vector<std::string> forbiddenTokens = {
      "--StartingType",    "--StartingTrajectory", "--Trajectories",
      "--Thermalizations", "--ParameterFile",      "--grid",
  };
  char** updatedArgv =
      (char**)malloc(sizeof(char*) * (argc + DJ_NUM_EXTRA_ARGS));
  updatedArgv[0] = argv[0];
  for (int argIndex = 2; argIndex < argc; argIndex++) {
    for (const std::string& forbiddenToken : forbiddenTokens) {
      if (argv[argIndex] == forbiddenToken) {
        std::cerr << "The argument " << forbiddenToken
                  << " was specified, but this option is controlled by the "
                     "hmcdj input file."
                  << std::endl;
        std::cerr << "To avoid potential confusion, please remove this "
                     "argument and retry."
                  << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    updatedArgv[argIndex - 1] = argv[argIndex];
  }

  updatedArgv[argc - 1] = (char*)(new std::string("--grid"))->c_str();
  updatedArgv[argc] = (char*)grid;
  updatedArgv[argc + 1] = nullptr;

  return updatedArgv;
}
