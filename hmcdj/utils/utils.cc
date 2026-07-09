#include <hmcdj/utils/utils.h>

/*
    * Guard
    Ensures that the program is called correctly.

    Further checks on the validity of the requested yaml file
    are implemented in the Ensemble Reader.
*/
void djGuard(int argc, char* argv[]) {
  // Usage
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " filename --<Other Grid arguments>"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Check that the file exists
  std::ifstream file(argv[1]);  // opens a filestream of argv[1]
  if (!file) {
    std::cerr << "File " << argv[1] << " does not exist!" << std::endl;
    if (static_cast<std::string>(argv[1]).substr(0, 2) == "--") {
      std::cerr << "The first provided argument should be the track"
                << std::endl;
    }
    std::exit(EXIT_FAILURE);
  }
}

/*
    * RNGManager Constructor

    The RNG Manager initialises an instance of the sitmo
    RNG engine from Grid's source code. This is deterministic
    and should be compiler indipendent.

    Using the Seed method, the RNG engine is seeded from the
    filename when the class is initialised.
 */
RNGManager::RNGManager(std::string filename) : engine() { Seed(filename); }

/* RNGManager Seeding */
void RNGManager::Seed(std::string filename) {
  engine.seed((md5FileToInt(filename)));
}

/* Generate Grid RNG string */
std::string RNGManager::GenerateGridRNGSeedString() {
  std::ostringstream RNGstr;
  std::uniform_int_distribution<int> dist(0, 100);

  RNGstr << dist(engine);

  for (int i = 1; i < 5; i++) {
    RNGstr << " ";
    RNGstr << dist(engine);
  }

  return RNGstr.str();
}

/* Get an integer hash from a file's contents */
uint32_t md5FileToInt(const std::string& filename) {
  constexpr std::size_t bufferSize = 4096;
  unsigned char buffer[bufferSize];
  unsigned char md5Digest[EVP_MAX_MD_SIZE];
  unsigned int md5Len = 0;

  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file");
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");

  if (!EVP_DigestInit_ex(ctx, EVP_md5(), nullptr)) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestInit failed");
  }

  while (file.read(reinterpret_cast<char*>(buffer), bufferSize) ||
         file.gcount() > 0) {
    if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
      EVP_MD_CTX_free(ctx);
      throw std::runtime_error("DigestUpdate failed");
    }
  }

  if (!EVP_DigestFinal_ex(ctx, md5Digest, &md5Len)) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("DigestFinal failed");
  }

  EVP_MD_CTX_free(ctx);

  return *reinterpret_cast<uint32_t*>(md5Digest);
}
