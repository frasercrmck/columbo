#!/bin/bash

BIN_DIR="build/bin"

SUDOKUS=(sudokus/*)

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

FAILS=0

for FILE in "${SUDOKUS[@]}"; do
  printf "%-40s" "${FILE}..."
  OUTPUT=$(${BIN_DIR}/columbo -q -f "${FILE}" 2>&1)
  if [ $? -eq 0 ]; then
    echo -e "[${GREEN}PASS${NC}]"
  else
    echo -e "[${RED}FAIL${NC}]"
    echo "Command output:"
    echo "==============="
    echo "${OUTPUT}"
    echo "==============="
    echo "Test '${FILE}' failed!"

    ((FAILS++))
  fi
done

if [ ${FAILS} -eq 0 ]; then
  echo "All tests passed!"
  exit 0
else
  echo "Error: ${FAILS} tests failed!"
  exit 1
fi
