#include <gtest/gtest.h>
#include <utils.h>

// Check that the ensemble reader loads the correct lattice dimensions string
TEST(EnsembleReaderTest, CorrectGridDimString) {

    std::string filename = std::string(TOP_SRCDIR) + "/track.yaml";
    EnsembleReader reader(filename);

    // Check that the reader loads the correct values
    const char* dimStr = reader.GetDimStringPointer();

    EXPECT_STREQ(dimStr, "4.4.4.4");

    EXPECT_STREQ(reader.StartingType, "HotStart")
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
