#include "all_steps.h"

ColumboStep::~ColumboStep() {}

void EliminateImpossibleCombosStep::anchor() {}
void EliminateNakedPairsStep::anchor() {}
void EliminateNakedTriplesStep::anchor() {}
void EliminateHiddenSinglesStep::anchor() {}
void EliminateHiddenPairsStep::anchor() {}
void EliminateHiddenTriplesStep::anchor() {}
void EliminateCageUnitOverlapStep::anchor() {}
void EliminatePointingPairsOrTriplesStep::anchor() {}
void EliminateOneCellInniesAndOutiesStep::anchor() {}
void PropagateFixedCells::anchor() {}