#!/bin/sh

generate_compdb() {
  ninja -C build -t compdb -x c_COMPILER > compile_commands.json
  echo -e "\033[1m- Generated compilation database\033[0m"
}

clang_tidy() {
  run-clang-tidy -use-color -quiet
}

clang_format() {
  find src/ \( -name '*.h' -o -name '*.c' \) -exec "clang-format" -i '{}' \;
  echo -e "\033[1m- Formatted Code\033[0m"
}

add_licenses() {
  find src/ \( -name '*.h' -o -name '*.c' \) -exec "./scripts/license.py" '{}' \;
  echo -e "\033[1m- Added license headers\033[0m"
}


if [ $# -eq 0 ]
  then
    echo "Available commands: "
    echo "- setup-env    Sets up the development environment (compilation database)"
    echo "- tidy         Runs clang-tidy"
    echo "- reformat     Reformats the code"
fi


if [ "$1" = "setup-env" ]
then
  generate_compdb
elif [ "$1" = "tidy" ]
then
  clang_tidy
elif [ "$1" = "reformat" ]
then
  add_licenses
  clang_format
fi
