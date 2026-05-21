#!/usr/bin/env python3
import os
import sys

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(PROJECT_DIR, "src", "SDK.h")

FILES = [
    ("include/CppScriptRuntime.h", "kEmbeddedCppScriptRuntimeH"),
    ("src/DB.h", "kEmbeddedDBH"),
]

DELIM = "__EMBEDDED_SDK__"


def main():
    lines = ["#pragma once", "", "#include <cstddef>", ""]
    for relpath, varname in FILES:
        fullpath = os.path.join(PROJECT_DIR, relpath)
        if not os.path.isfile(fullpath):
            print(f"Error: {fullpath} not found", file=sys.stderr)
            sys.exit(1)
        content = open(fullpath).read()
        if DELIM in content:
            print(f"Error: delimiter {DELIM} found in {relpath}", file=sys.stderr)
            sys.exit(1)
        lines.append(f'static const char {varname}[] = R"{DELIM}({content}){DELIM}";')
        lines.append(f"static const size_t {varname}_len = sizeof({varname}) - 1;")
        lines.append("")
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, "w") as f:
        f.write("\n".join(lines))
    print(f"Generated {OUTPUT}")


if __name__ == "__main__":
    main()
