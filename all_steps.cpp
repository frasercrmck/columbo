#include "all_steps.h"

ColumboStep::~ColumboStep() {}

void EliminateImpossibleCombosStep::anchor() {}
void EliminateConflictingCombosStep::anchor() {}
void EliminateNakedPairsStep::anchor() {}
void EliminateNakedTriplesStep::anchor() {}
void EliminateNakedQuadsStep::anchor() {}
void EliminateNakedQuintsStep::anchor() {}
void EliminateHiddenSinglesStep::anchor() {}
void EliminateHiddenPairsStep::anchor() {}
void EliminateHiddenTriplesStep::anchor() {}
void EliminateHiddenQuadsStep::anchor() {}
void EliminateCageUnitOverlapStep::anchor() {}
void EliminateHardCageUnitOverlapStep::anchor() {}
void EliminatePointingPairsOrTriplesStep::anchor() {}
void EliminateOneCellInniesAndOutiesStep::anchor() {}
void EliminateHardInniesAndOutiesStep::anchor() {}
void PropagateFixedCells::anchor() {}
void XWingsStep::anchor() {}
