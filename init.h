#ifndef SOLVER_INIT_H
#define SOLVER_INIT_H

#include "defs.h"
#include "utils.h"

static void init(Grid *grid, CageList &cages) {
#if 1
  cages.push_back(Cage{26, grid, {{A, 0}, {B, 0}, {B, 1}, {C, 0}}});
  cages.push_back(Cage{12, grid, {{A, 1}, {A, 2}}});
  cages.push_back(Cage{7, grid, {{B, 2}, {C, 1}, {C, 2}}});
  cages.push_back(Cage{21, grid, {{A, 3}, {A, 4}, {B, 3}}});
  cages.push_back(Cage{3, grid, {{A, 5}, {A, 6}}});
  cages.push_back(Cage{22, grid, {{A, 7}, {A, 8}, {B, 7}}});
  cages.push_back(Cage{10, grid, {{B, 4}, {B, 5}, {B, 6}}});
  cages.push_back(Cage{19, grid, {{D, 0}, {E, 0}, {E, 1}, {E, 2}}});
  cages.push_back(Cage{13, grid, {{D, 1}, {D, 2}}});
  cages.push_back(Cage{4, grid, {{C, 3}, {D, 3}}});
  cages.push_back(Cage{10, grid, {{C, 4}, {D, 4}}});
  cages.push_back(Cage{23, grid, {{C, 5}, {C, 6}, {D, 5}}});
  cages.push_back(Cage{5, grid, {{B, 8}, {C, 8}}});
  cages.push_back(Cage{9, grid, {{C, 7}, {D, 7}}});
  cages.push_back(Cage{13, grid, {{D, 6}, {E, 5}, {E, 6}}});
  cages.push_back(Cage{15, grid, {{D, 8}, {E, 8}}});
  cages.push_back(Cage{4, grid, {{E, 7}, {F, 7}}});
  cages.push_back(Cage{12, grid, {{F, 0}, {G, 0}}});
  cages.push_back(Cage{9, grid, {{F, 1}, {G, 1}}});
  cages.push_back(Cage{4, grid, {{F, 2}, {G, 2}}});
  cages.push_back(Cage{11, grid, {{E, 3}, {F, 3}, {G, 3}}});
  cages.push_back(Cage{14, grid, {{E, 4}, {F, 4}}});
  cages.push_back(Cage{16, grid, {{F, 5}, {F, 6}}});
  cages.push_back(Cage{11, grid, {{F, 8}, {G, 8}}});
  cages.push_back(Cage{12, grid, {{H, 0}, {J, 0}}});
  cages.push_back(Cage{6, grid, {{H, 1}, {J, 1}}});
  cages.push_back(Cage{15, grid, {{H, 2}, {J, 2}}});
  cages.push_back(Cage{6, grid, {{G, 4}, {G, 5}}});
  cages.push_back(Cage{16, grid, {{H, 3}, {H, 4}}});
  cages.push_back(Cage{10, grid, {{J, 3}, {J, 4}, {J, 5}}});
  cages.push_back(Cage{16, grid, {{H, 5}, {H, 6}, {J, 6}}});
  cages.push_back(Cage{3, grid, {{H, 7}, {H, 8}}});
  cages.push_back(Cage{12, grid, {{J, 7}, {J, 8}}});
  cages.push_back(Cage{16, grid, {{G, 6}, {G, 7}}});
#else
  cages.push_back(Cage{22, grid, {{A, 0}, {A, 1}, {A, 2}}});
  cages.push_back(Cage{10, grid, {{A, 3}, {A, 4}, {B, 2}, {B, 3}}});
  cages.push_back(Cage{4, grid, {{A, 5}, {A, 6}}});
  cages.push_back(Cage{13, grid, {{A, 7}, {A, 8}}});
  cages.push_back(Cage{6, grid, {{B, 0}, {B, 1}}});
  cages.push_back(Cage{19, grid, {{B, 4}, {B, 5}, {B, 6}}});
  cages.push_back(Cage{19, grid, {{B, 7}, {B, 8}, {C, 8}, {D, 8}}});
  cages.push_back(Cage{17, grid, {{C, 0}, {C, 1}, {D, 0}}});
  cages.push_back(Cage{9, grid, {{C, 2}, {D, 2}}});
  cages.push_back(Cage{24, grid, {{C, 3}, {C, 4}, {D, 3}, {D, 4}}});
  cages.push_back(Cage{22, grid, {{C, 5}, {D, 5}, {D, 6}}});
  cages.push_back(Cage{7, grid, {{C, 6}, {C, 7}, {D, 7}}});
  cages.push_back(Cage{21, grid, {{D, 1}, {E, 0}, {E, 1}, {F, 0}}});
  cages.push_back(Cage{14, grid, {{E, 2}, {F, 2}, {G, 2}}});
  cages.push_back(Cage{17, grid, {{E, 3}, {E, 4}, {F, 3}, {G, 3}}});
  cages.push_back(Cage{19, grid, {{E, 5}, {E, 6}, {E, 7}}});
  cages.push_back(Cage{15, grid, {{E, 8}, {F, 8}, {G, 8}}});
  cages.push_back(Cage{11, grid, {{F, 1}, {G, 1}, {H, 1}}});
  cages.push_back(Cage{28, grid, {{F, 4}, {F, 5}, {F, 6}, {G, 4}}});
  cages.push_back(Cage{21, grid, {{F, 7}, {G, 7}, {H, 7}, {H, 8}}});
  cages.push_back(Cage{22, grid, {{G, 0}, {H, 0}, {J, 0}, {J, 1}}});
  cages.push_back(Cage{6, grid, {{G, 5}, {G, 6}}});
  cages.push_back(Cage{12, grid, {{H, 2}, {J, 2}}});
  cages.push_back(Cage{11, grid, {{H, 3}, {J, 3}}});
  cages.push_back(Cage{15, grid, {{H, 4}, {J, 4}, {J, 5}}});
  cages.push_back(Cage{5, grid, {{H, 5}, {H, 6}}});
  cages.push_back(Cage{16, grid, {{J, 6}, {J, 7}, {J, 8}}});
#endif
}

#endif // SOLVER_INIT_H
