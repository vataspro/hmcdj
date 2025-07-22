#include <gtest/gtest.h>
#include <hmcdj/utils/utils.h>

#include <algorithm>  // needed for count

// The file used to test the RNG seed
#define TESTSEEDFILE \
  std::string(TOP_SRCDIR) + "/example_tracks/NoParamsTrack.yaml"

// Number of spaces in a Grid seed string
#define NUM_GRID_SEED_STR_INTS 4

// Checks that a string contains only whitespace or digits
bool StrIsDigits(const std::string str) {
  std::istringstream stream(str);
  std::string word;
  while (stream >> word) {
    for (char c : word) {
      if (!(std::isdigit(c))) {
        return false;
      }
    }
  }

  return true;
}

// Check that the ensemble reader loads the correct lattice dimensions string
TEST(UtilTest, EnsembleReaderTest) {
  // Load the test "track.yaml" file
  std::string filename = TESTSEEDFILE;
  EnsembleReader reader("NoParams", filename, {});

  // Check that the reader loads the correct values
  const char* dimStr = reader.GetDimStringPointer();
  EXPECT_STREQ(dimStr, "4.4.4.4");

  // Check that the correct checkpointing parameters are loaded
  EXPECT_EQ(reader.saveInterval, 5);
  EXPECT_STREQ(reader.format.c_str(), "IEEE64BIG");
  EXPECT_STREQ(reader.config_prefix.c_str(), "cfg_ckpoint");
  EXPECT_STREQ(reader.rng_prefix.c_str(), "rng_ckpoint");

  // Check that the correct HMC parameters are loaded
  EXPECT_STREQ(reader.StartingType.c_str(), "HotStart");
  EXPECT_EQ(reader.trajL, 1.0);
  EXPECT_EQ(reader.MDsteps, 10);
  EXPECT_EQ(reader.Thermalisations, 0);
  EXPECT_EQ(reader.Trajectories, 10);
}

// Check that the RNG manager is consistent
/*
     The RNG used by Grid is not architecture
   independent. In order to check consistency
   on the same machine/compiler two managers
   with the same seed file are instantiated,
   checking that they generate the same Grid
   seed string.
     The Grid seed string is also checked to
   conform with the following format:
     - Contains 4 spaces
     - The first and last character are not spaces
     - All non-whitespace characters are digits

     An example string satisfying the format is:
     "32 54 6 123 3"

 */
TEST(RNGTest, CheckGridString) {
  // Initialise two RNG Managers with the same seed
  RNGManager rng1(TESTSEEDFILE);
  RNGManager rng2(TESTSEEDFILE);

  std::string str1, str2;

  // Verify that the two Grid seed strings are equal
  str1 = rng1.GenerateGridRNGSeedString();
  str2 = rng2.GenerateGridRNGSeedString();
  // For std::string EXPECT_EQ should be used rather than EXPECT_STREQ
  EXPECT_EQ(str1, str2);

  // Count the number of spaces in the string and verify that they are 4
  int space_count = std::count(str1.begin(), str1.end(), ' ');
  EXPECT_EQ(space_count, NUM_GRID_SEED_STR_INTS);

  // Check that the first and last characters are not spaces
  bool flag = true;
  if (str1.front() == ' ' || str1.back() == ' ') {
    flag = false;
  }
  EXPECT_TRUE(flag);

  // Check that all the non-whitespace characters are digits
  EXPECT_TRUE(StrIsDigits(str1));
}

TEST(CommandLineArgsTest, CheckStartingTypesAccepted) {
  EXPECT_EQ(isValidStartingType("HotStart"), true);
  EXPECT_EQ(isValidStartingType("TepidStart"), true);
  EXPECT_EQ(isValidStartingType("ColdStart"), true);
}

TEST(CommandLineArgsTest, CheckValidGridStartingTypesRejected) {
  EXPECT_EQ(isValidStartingType("CheckpointStart"), false);
  EXPECT_EQ(isValidStartingType("CheckpointStartWithReseed"), false);
}

TEST(CommandLineArgsTest, CheckInvalidStartingTypesRejected) {
  EXPECT_EQ(isValidStartingType("ThisIsNonsenseAndShouldBeRejected"), false);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
