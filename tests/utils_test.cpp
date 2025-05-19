#include <gtest/gtest.h>
#include <utils.h>

// Check that the ensemble reader loads the correct lattice dimensions string
TEST(UtilTest, EnsembleReaderTest) {

    // Load the test "track.yaml" file
    std::string filename = std::string(TOP_SRCDIR) + "/track.yaml";
    EnsembleReader reader(filename);

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
    EXPECT_EQ(reader.Trajectories, 1);

}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
