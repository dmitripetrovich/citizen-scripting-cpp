#!/usr/bin/env python3
import hashlib
import json
import os
import struct
import sys
import time
import urllib.request
from collections import defaultdict

CACHE_DIR = os.path.join(os.path.dirname(__file__), ".cache")
CACHE_TTL = 3600

CFX_URL = "https://runtime.fivem.net/doc/natives_cfx.json"
GAME_URLS = {
        "fivem": "https://raw.githubusercontent.com/alloc8or/gta5-nativedb-data/master/natives.json",
        "redm": "https://raw.githubusercontent.com/alloc8or/rdr3-nativedb-data/master/natives.json",
}

PARAM_TYPE_MAP = {
        "int": "int",
        "float": "float",
        "BOOL": "bool",
        "char*": "const char*",
        "Hash": "uint32_t",
        "Entity": "int",
        "Player": "int",
        "Vehicle": "int",
        "Ped": "int",
        "Object": "int",
        "Pickup": "int",
        "Blip": "int",
        "Cam": "int",
        "FireId": "int",
        "Interior": "int",
        "ScrHandle": "int",
        "Any": "int",
        "long": "int64_t",
        "int*": "int*",
        "float*": "float*",
        "BOOL*": "int*",
        "Hash*": "uint32_t*",
        "Entity*": "int*",
        "Player*": "int*",
        "Vehicle*": "int*",
        "Ped*": "int*",
        "Object*": "int*",
        "Pickup*": "int*",
        "Blip*": "int*",
        "Cam*": "int*",
        "FireId*": "int*",
        "Interior*": "int*",
        "ScrHandle*": "int*",
        "Any*": "int*",
        "long*": "int64_t*",
        "Vector3*": "Vector3*",
}

RETURN_TYPE_MAP = {
        "void": ("void", None),
        "int": ("int", "int"),
        "float": ("float", "float"),
        "BOOL": ("bool", "bool"),
        "char*": ("std::string", "std::string"),
        "Hash": ("uint32_t", "uint32_t"),
        "Entity": ("int", "int"),
        "Player": ("int", "int"),
        "Vehicle": ("int", "int"),
        "Ped": ("int", "int"),
        "Object": ("int", "int"),
        "Pickup": ("int", "int"),
        "Blip": ("int", "int"),
        "Cam": ("int", "int"),
        "FireId": ("int", "int"),
        "Interior": ("int", "int"),
        "ScrHandle": ("int", "int"),
        "Any": ("int", "int"),
        "long": ("int64_t", "int64_t"),
        "Vector3": ("Vector3", "Vector3"),
}


def screaming_to_pascal(name: str) -> str:
        return "".join(word.capitalize() for word in name.split("_"))


def to_safe_param(name: str) -> str:
        keywords = {"class", "struct", "enum", "union", "template", "operator", "new", "delete", "this", "return", "switch", "case", "default", "break", "continue", "goto", "if", "else", "for", "while", "do", "try", "catch", "throw", "register", "auto", "const", "static", "extern", "volatile", "inline", "virtual", "explicit", "friend", "namespace", "using", "typedef", "typename", "public", "private", "protected", "override", "final", "nullptr", "true", "false", "bool", "int", "float", "double", "char", "long", "short", "signed", "unsigned", "void", "object", "near", "far"}
        return name + "_" if name in keywords else name


def generate_wrapper(native: dict) -> str | None:
        name = native.get("name")
        if not name:
                return None
        hash_str = native.get("hash", "0x0")
        params = native.get("params", [])
        result_type = native.get("results", "void")
        if result_type == "object":
                return None
        if result_type not in RETURN_TYPE_MAP:
                return None
        cpp_ret, invoke_ret = RETURN_TYPE_MAP[result_type]
        func_name = screaming_to_pascal(name)
        cpp_params = []
        call_args = []
        skip = False
        for p in params:
                ptype = p.get("type", "int")
                pname = to_safe_param(p.get("name", "p"))
                if ptype not in PARAM_TYPE_MAP:
                        skip = True
                        break
                cpp_type = PARAM_TYPE_MAP[ptype]
                cpp_params.append(f"{cpp_type} {pname}")
                call_args.append(pname)
        if skip:
                return None
        param_str = ", ".join(cpp_params)
        args_str = ", ".join(call_args)
        comma_args = f", {args_str}" if args_str else ""
        if invoke_ret is None:
                body = f"invoke({hash_str}{comma_args});"
        else:
                body = f"return invoke<{invoke_ret}>({hash_str}{comma_args});"

        return f"        inline {cpp_ret} {func_name}({param_str})\n        {{\n                {body}\n        }}"


def generate_namespace_block(namespace: str, natives: list[dict]) -> tuple[str, str, int] | None:
        wrappers = []
        for n in sorted(natives, key=lambda x: x.get("name", "")):
                w = generate_wrapper(n)
                if w:
                        wrappers.append(w)
        if not wrappers:
                return None
        body = "\n\n".join(wrappers)
        ns_lower = namespace.lower()
        block = f"namespace {ns_lower}\n{{\n\n{body}\n\n}} // namespace {ns_lower}"
        return ns_lower, block, len(wrappers)


def fetch_json(url: str, no_cache: bool = False):
        cache_file = os.path.join(CACHE_DIR, hashlib.sha1(url.encode()).hexdigest() + ".json")
        if not no_cache and os.path.exists(cache_file):
                age = time.time() - os.path.getmtime(cache_file)
                if age < CACHE_TTL:
                        print(f"Using cached {url} ({int(age)}s old)")
                        with open(cache_file) as f:
                                return json.load(f)
        print(f"Fetching {url}...")
        req = urllib.request.Request(url, headers={"User-Agent": "citizen-scripting-cpp-nativedb"})
        with urllib.request.urlopen(req, timeout=30) as resp:
                data = resp.read()
        os.makedirs(CACHE_DIR, exist_ok=True)
        with open(cache_file, "wb") as f:
                f.write(data)
        return json.loads(data)


def parse_cfx_natives(data: list | dict) -> dict[str, list[dict]]:
        by_ns = defaultdict(list)
        if isinstance(data, list):
                for entry in data:
                        ns = entry.get("ns", "CFX")
                        by_ns[ns].append(entry)
        elif isinstance(data, dict):
                for key, entry in data.items():
                        if isinstance(entry, dict) and "name" in entry:
                                ns = entry.get("ns", "CFX")
                                by_ns[ns].append(entry)
                        elif isinstance(entry, dict):
                                for hash_key, native in entry.items():
                                        if isinstance(native, dict) and "name" in native:
                                                ns = native.get("ns", key)
                                                by_ns[ns].append(native)
        return dict(by_ns)


def normalize_native(native: dict, hash_key: str) -> dict:
        if "hash" not in native:
                native["hash"] = hash_key
        if "results" not in native and "return_type" in native:
                native["results"] = native["return_type"]
        return native


def parse_game_natives(data: dict) -> dict[str, list[dict]]:
        by_ns = defaultdict(list)
        for ns, natives in data.items():
                if not isinstance(natives, dict):
                        continue
                for hash_key, native in natives.items():
                        if isinstance(native, dict) and "name" in native:
                                by_ns[ns].append(normalize_native(native, hash_key))
        return dict(by_ns)


def embed_sdk(base_dir, output):
        files = ["include/CppScriptRuntime.h"]
        native_dir = os.path.join(base_dir, "src", "natives")
        if os.path.isdir(native_dir):
                for f in sorted(os.listdir(native_dir)):
                        if f.endswith(".h"):
                                files.append(f"src/natives/{f}")
        blob = struct.pack("<I", len(files))
        for rel_path in files:
                with open(os.path.join(base_dir, rel_path), "rb") as f:
                        data = f.read()
                path_bytes = rel_path.encode("utf-8")
                blob += struct.pack("<I", len(path_bytes)) + path_bytes
                blob += struct.pack("<I", len(data)) + data
        with open(output, "wb") as f:
                f.write(blob)
        print(f"Embedded {len(files)} SDK files ({len(blob)} bytes) into {output}")


def main():
        import argparse
        parser = argparse.ArgumentParser()
        parser.add_argument("--game", choices=GAME_URLS.keys(), default="fivem")
        parser.add_argument("--output", default="src/natives")
        parser.add_argument("--embed", default="src/sdk.blob")
        parser.add_argument("--no-cache", action="store_true")
        args = parser.parse_args()
        output_dir = args.output
        game = args.game
        no_cache = args.no_cache
        os.makedirs(output_dir, exist_ok=True)
        all_ns: dict[str, list[dict]] = {}
        try:
                cfx_data = fetch_json(CFX_URL, no_cache)
                for ns, natives in parse_cfx_natives(cfx_data).items():
                        all_ns.setdefault(ns, []).extend(natives)
        except Exception as e:
                print(f"Failed to fetch CFX natives: {e}")
        try:
                game_data = fetch_json(GAME_URLS[game], no_cache)
                for ns, natives in parse_game_natives(game_data).items():
                        all_ns.setdefault(ns, []).extend(natives)
        except Exception as e:
                print(f"Failed to fetch {game} natives: {e}")
        if not all_ns:
                print("No natives fetched")
                sys.exit(1)
        def param_sig(native):
                return tuple(p.get("type", "") for p in native.get("params", []))
        global_seen: dict[tuple, tuple[str, str]] = {}
        for ns in all_ns:
                for native in all_ns[ns]:
                        name = native.get("name")
                        if not name:
                                continue
                        key = (name, param_sig(native))
                        h = native.get("hash", "")
                        prev = global_seen.get(key)
                        if prev is None or len(h) > len(prev[1]):
                                global_seen[key] = (ns, h)
        for ns in all_ns:
                all_ns[ns] = [n for n in all_ns[ns]
                                if not n.get("name") or global_seen.get((n["name"], param_sig(n)), (None,))[0] == ns]
        files = []
        total_count = 0
        for ns in sorted(all_ns.keys()):
                natives = all_ns[ns]
                result = generate_namespace_block(ns, natives)
                if not result:
                        continue
                ns_lower, block, count = result
                total_count += count
                filename = ns_lower.replace(" ", "_") + ".h"
                files.append((filename, ns_lower, count))
                file_content = f"""// Auto-generated, do not edit.
#pragma once

namespace fx::natives
{{

{block}

}} // namespace fx::natives
"""
                with open(os.path.join(output_dir, filename), "w") as f:
                        f.write(file_content)
                print(f"  {ns}: {count} wrappers -> {filename}")

        includes = "\n".join(f'#include "{fn}"' for fn, _, _ in files)
        all_content = f"""// Auto-generated, do not edit.
#pragma once

{includes}
"""
        with open(os.path.join(output_dir, "all.h"), "w") as f:
                f.write(all_content)
        print(f"\nGenerated {total_count} wrappers across {len(files)} namespaces")
        print(f"Output: {output_dir}/")

        base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        embed_sdk(base_dir, args.embed)


if __name__ == "__main__":
        main()
