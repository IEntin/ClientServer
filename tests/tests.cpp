#include <gtest/gtest.h>
#include <iostream>

TEST(FakeTest, FakeTest1) { 
  std::clog << "\tNot yet implemented!" << std::endl;
  ASSERT_EQ(6, 6);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
