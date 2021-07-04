#ifndef COLUMBO_UNITTEST_FRAMEWORK_H
#define COLUMBO_UNITTEST_FRAMEWORK_H

#include "defs.h"
#include <gtest/gtest.h>

// Just a default grid
class DefaultGridTest : public ::testing::Test {
protected:
  void SetUp() override {
    grid = std::make_unique<Grid>();
    for (unsigned y = 0; y < 9; y++) {
      for (unsigned x = 0; x < 9; x++) {
        auto cage = std::make_unique<Cage>(0u);
        cage->addCell(grid.get(), grid->cells[y][x].coord);
        grid->cages.push_back(std::move(cage));
      }
    }
    for (auto &cage : grid->cages)
      for (auto &cell : cage->cells)
        cell->cage = cage.get();

    grid->assignCageColours();
  }

  // void TearDown() override {}
  std::unique_ptr<Grid> grid;
};

#endif // COLUMBO_UNITTEST_FRAMEWORK_H
