#include <gtest/gtest.h>
#include <hmcdj/utils/mathutils.h>

TEST(MathUtilsTest, MeanTest) {
  EXPECT_EQ(mean(std::vector<int>{1, 2, 3, 4, 5}), 3.0);
  EXPECT_EQ(mean(std::vector<double>{-1.0, 0.0, 1.0}), 0.0);
}

TEST(MathUtilsTest, StdErrTest) {
  EXPECT_NEAR(stdErr(std::vector<int>{1, 2, 3, 4, 5}), 1.0 / sqrt(2), 1.0e-6);
  EXPECT_NEAR(stdErr(std::vector<double>{-1.0, 0.0, 1.0}), 1.0 / sqrt(3),
              1.0e-6);
}

TEST(MathUtilsTest, ErfInvTest) {
  EXPECT_EQ(erfInv(0.0), 0.0);
  EXPECT_EQ(erfInv(1.0), 1.0 / 0.0);
  EXPECT_NEAR(erfInv(0.5), 0.4769, 1.0e-4);
}

TEST(MathUtilsTest, ErfcInvTest) {
  EXPECT_EQ(erfcinv(0.0), 1.0 / 0.0);
  EXPECT_EQ(erfcinv(1.0), 0.0);
  EXPECT_NEAR(erfcinv(0.5), 0.4769, 1.0e-4);
}

TEST(NumberWithErrorTest, NumberWithErrorInterfaceTest) {
  NumberWithError x(1.0, 0.1);
  EXPECT_EQ(x.value, 1.0);
  EXPECT_EQ(x.error, 0.1);
}

TEST(NumberWithErrorTest, NumberWithErrorIsCloseTest) {
  NumberWithError x(1.0, 0.1);
  EXPECT_TRUE(x.isClose(NumberWithError(1.05, 0.01)));
  EXPECT_TRUE(x.isClose(NumberWithError(1.15, 0.12)));
  EXPECT_TRUE(x.isClose(NumberWithError(1.25, 0.5)));
  EXPECT_FALSE(x.isClose(NumberWithError(1.25, 0.1)));
  EXPECT_FALSE(x.isClose(NumberWithError(0.75, 0.1)));
}

TEST(NumberWithErrorTest, DoubleIsCloseTest) {
  NumberWithError x(1.0, 0.1);
  EXPECT_TRUE(x.isClose(1.05, 0.01));
  EXPECT_TRUE(x.isClose(1.15, 0.12));
  EXPECT_TRUE(x.isClose(1.25, 0.5));
  EXPECT_FALSE(x.isClose(1.25, 0.1));
  EXPECT_FALSE(x.isClose(0.75, 0.1));
}

TEST(MathUtilsTest, GetTargetMDStepsTest) {
  for (double pacc = 0.2; pacc < 1.0; pacc *= 1.5) {
    for (double target_pacc = 0.5; target_pacc < 1.0; target_pacc += 0.1) {
      for (int MDsteps = 4; MDsteps < 50; MDsteps *= 2) {
        for (double trajL = 0.5; trajL < 3.0; trajL *= 2) {
          const int newMDsteps =
              get_target_MDsteps(trajL, MDsteps, pacc, target_pacc);
          if (target_pacc > pacc) {
            if (MDsteps > 8) {
              EXPECT_GT(newMDsteps, MDsteps);
            } else {
              // Once we get below 8 steps,
              // then trying to suppress MDsteps slightly fails
              // because we're constrained to integers
              EXPECT_GE(newMDsteps, MDsteps);
            }
          } else if (target_pacc < pacc) {
            if (MDsteps > 8) {
              EXPECT_LT(newMDsteps, MDsteps);
            } else {
              EXPECT_LE(newMDsteps, MDsteps);
            }
          } else {
            EXPECT_EQ(newMDsteps, MDsteps);
          }
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
