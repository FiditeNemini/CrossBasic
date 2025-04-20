#!/usr/bin/env bash
set -euo pipefail

# Path to your xojoscript executable
XOSCRIPT="./crossbasic"

# Directory containing .xs scripts
SCRIPT_DIR="Scripts"

# Loop over all .xs files in SCRIPT_DIR
for script in "${SCRIPT_DIR}"/*.xs; do
  # If no files match, the glob will remain literal—guard against that:
  if [[ ! -e "$script" ]]; then
    echo "No .xs files found in ${SCRIPT_DIR}."
    break
  fi

  echo "Executing $script"
  "$XOSCRIPT" --s "$script"
done

# Pause (press any key to continue)
read -n1 -r -p $'\nPress any key to continue...\n'
