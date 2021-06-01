#include "all_steps.h"
#include "cli.h"
#include "defs.h"
#include "strategy.h"
#include "printers/terminal_printer.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <cstring>

static bool QUIET = false;

static void print_help() {
  std::cout << R"(
    COLUMBO: A solver of killers. Of killer sudokus.

    Usage:
      columbo [option] -f <sudoku file>

    Options:
      -h                         Print help and exit
      -f    --file <sudoku file> Use <sudoku file> as input
      -o           <sudoku file> Write <sudoku file> as output
      -p    --print-after-all    Print grid after every step
      -t    --time               Print detailed timing information
      -d    --debug              Print debug text for every step
      -s    --run-step <step>    Run <step> and only <step>
      -q    --quiet              Print nothing at all
            --no-colour          Don't print grids using colour

  )";
}

using StepList = std::vector<std::unique_ptr<ColumboStep>>;

static void initializeAllSteps(const Grid *grid, StepList &steps,
                               StepIDMap &step_map) {
  CageSubsetMap *subset_map = grid->getSubsetMap();

  steps.push_back(std::make_unique<PropagateFixedCells>());
  steps.push_back(std::make_unique<EliminateImpossibleCombosStep>(subset_map));
  steps.push_back(std::make_unique<EliminateNakedPairsStep>());
  steps.push_back(std::make_unique<EliminateNakedTriplesStep>());
  steps.push_back(std::make_unique<EliminateNakedQuadsStep>());
  steps.push_back(std::make_unique<EliminateHiddenSinglesStep>());
  steps.push_back(std::make_unique<EliminateHiddenPairsStep>());
  steps.push_back(std::make_unique<EliminateHiddenTriplesStep>());
  steps.push_back(std::make_unique<EliminateHiddenQuadsStep>());
  steps.push_back(std::make_unique<EliminateCageUnitOverlapStep>(subset_map));
  steps.push_back(std::make_unique<EliminatePointingPairsOrTriplesStep>());
  steps.push_back(std::make_unique<EliminateOneCellInniesAndOutiesStep>());
  steps.push_back(std::make_unique<XWingsStep>());

  for (auto &step : steps) {
    step_map[step->getID()] = step.get();
  }
}

std::vector<std::string> split(const std::string &str, const char delim) {
  std::vector<std::string> tokens;
  size_t prev = 0, pos = 0;
  do {
    pos = str.find(delim, prev);
    if (pos == std::string::npos)
      pos = str.length();
    std::string token = str.substr(prev, pos - prev);
    if (!token.empty())
      tokens.push_back(token);
    prev = pos + 1;
  } while (pos < str.length() && prev < str.length());
  return tokens;
}

int main(int argc, char *argv[]) {
  const char *file_name = nullptr;
  const char *out_file_name = nullptr;
  const char *step_to_run = nullptr;

  DebugOptions dbg_opts;

  for (int i = 1; i < argc; ++i) {
    const char *opt = argv[i];
    if (isOpt(opt, "-d", "--debug")) {
      DEBUG = true;
    } else if (isOpt(opt, "-t", "--time")) {
      TIME = true;
    } else if (isOpt(opt, "", "--print-after")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      auto steps = split(argv[++i], ',');
      dbg_opts.print_after_steps.insert(std::begin(steps), std::end(steps));
    } else if (isOpt(opt, "", "--print-after-all")) {
      dbg_opts.print_after_all = true;
    } else if (isOpt(opt, "", "--print-before")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      auto steps = split(argv[++i], ',');
      dbg_opts.print_before_steps.insert(std::begin(steps), std::end(steps));
    } else if (isOpt(opt, "", "--print-before-all")) {
      dbg_opts.print_before_all = true;
    } else if (isOpt(opt, "", "--no-colour")) {
      USE_COLOUR = false;
    } else if (isOpt(opt, "-f", "--file")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      file_name = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-o", "")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      out_file_name = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-s", "--run-step")) {
      if (i + 1 >= argc) {
        std::cout << "Expected a value to option '" << opt << "'...\n";
        return 1;
      }
      step_to_run = argv[i + 1];
      ++i;
    } else if (isOpt(opt, "-h", "--help")) {
      print_help();
      return 0;
    } else if (isOpt(opt, "-q", "--quiet")) {
      QUIET = true;
    } else {
      std::cout << "Unrecognized argument '" << opt << "'...\n";
      print_help();
      return 1;
    }
  }

  if (!file_name) {
    std::cout << "Did not specify a file name\n";
    print_help();
    return 1;
  }

  if (QUIET) {
    DEBUG = false;
    dbg_opts.print_after_all = false;
  }

  std::ifstream sudoku_file;
  sudoku_file.open(file_name);

  if (!sudoku_file.is_open()) {
    std::cout << "Could not open file '" << file_name << "'...\n";
    return 1;
  }

  auto grid = std::make_unique<Grid>();

  if (grid->initialize(sudoku_file)) {
    std::cout << "Invalid grid...\n";
    return 1;
  }

  sudoku_file.close();

  if (!QUIET) {
    std::cout << "Starting Out...\n";
    printGrid(grid.get(), std::cout, USE_COLOUR);
  }

  StepList steps;
  StepIDMap step_map;
  initializeAllSteps(grid.get(), steps, step_map);

  Strategy strat;
  bool err = false;
  if (!step_to_run) {
    err = strat.initializeDefault(step_map);
  } else {
    err = strat.initializeSingleStep(step_to_run, step_map);
  }

  if (err) {
    std::cout << "Could not initialize strategy\n";
    return 1;
  }

  Stats stats;
  bool error = false;
  try {
    stats = strat.solveGrid(grid.get(), dbg_opts);
  } catch (invalid_grid_exception &) {
    error = true;
  }

  if (!QUIET) {
    printGrid(grid.get(), std::cout, USE_COLOUR);
  }

  if (out_file_name) {
    if (std::strcmp(out_file_name, "-") == 0) {
      std::streambuf *buf = std::cout.rdbuf();
      std::ostream out(buf);
      grid->writeToFile(out);
    } else {
      std::ofstream out_file;
      out_file.open(out_file_name);
      grid->writeToFile(out_file);
    }
  }

  if (error) {
    std::cout << "Found a bad (invalid) grid!\n";
    return 1;
  } else if (stats.is_complete) {
    if (!QUIET) {
      std::cout << "Complete in " << stats.num_useful_steps << "/"
                << stats.num_steps << " steps!\n";
    }
  } else if (!step_to_run) {
    std::cout << "Stuck after " << stats.num_useful_steps << "/"
              << stats.num_steps << " steps!\n";
    return 1;
  }

  return 0;
}
