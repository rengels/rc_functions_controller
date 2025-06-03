#!/usr/bin/env python3

# this script will output different files used in serialization of
# rc function controller proc modules.
#
# example usage: serialization_tool.py --procs procs_config.json --cpp serialization.cpp
#

import argparse
import datetime
import json
import pathlib
import re


def to_camel_case(name):
    """Converts the given string to camel case"""

    replaced = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", name)
    tokens = re.split(r"[^a-zA-Z0-9]+", replaced)
    tokens = filter(len, tokens);
    tokens = list(x.lower() for x in tokens)
    return "".join((x.title() for x in tokens))


def output_signals_array(defs_signals, out_file):
    def signal_map_line(signal):
        """Converts an entry in def_signals to a line for id map"""
        if "id" in signal:
            return f"  '{signal["id"]}',  // {signal["name"]}";
        return ""

    print(
        f"""
namespace rcSignals {{

/** Signals map
 *
 * We don't want to invalidate configuration just because
 * a signal is added, removed or moved.
 * That's why they have an ID for serialization.
 */
extern const char signalsMap[];

const char signalsMap[] = {{
{"\n".join([signal_map_line(signal) for signal in defs_signals])}
    }};

}}  // namespace
""",
        file=out_file,
    )


def output_cpp_proc(proc, out_file):
    """Outputs the serialization an deserialization code for a single proc
    classes into the out file.
    """

    if not "name" in proc:  # ignore "description" only
        print(
            f"""
// -- {proc["description"]}
""",
            file=out_file,
        )
        return

    proc_name = to_camel_case(proc["name"])

    # prepare write and read functions
    serialization = []
    deserialization = []
    for typ in proc["types"]:
            serialization.append(
                f"    out << proc.{typ['name']};"
            )
            deserialization.append(
                f"    in >> proc.{typ['name']};"
            )

    for value in proc["values"]:
        if "cast" in value:
            serialization.append(f"    out << static_cast<{value['cast']}>(proc.{value['name']});")
            deserialization.append(f"    proc.{value['name']} = static_cast<{value['type']}>(in.read<{value['cast']}>());")
        else:
            serialization.append(f"    out << proc.{value['name']};")
            deserialization.append(f"    in >> proc.{value['name']};")

    if "ifdef" in proc:
        print(f"#ifdef {proc['ifdef']}", file=out_file)

    print(
        f"""
namespace {proc["namespace"]} {{

/** Serializes {proc_name} to a byte stream. */
SimpleOutStream& operator<<(SimpleOutStream& out,
    const {proc_name}& proc) {{

    out << '{proc["id"][0]}' << '{proc["id"][1]}';
    auto startPos = out.tellg();
    out.write<uint8_t>(0u);  // we need to fill it out later
{'\n'.join(serialization)}

    // fill out the actual length
    auto endPos = out.tellg();
    out.seekg(startPos);
    out.write<uint8_t>(endPos - startPos - 1);
    out.seekg(endPos);
    return out;
}}

/** Deserializes {proc_name} from a byte stream. */
SimpleInStream& operator>>(SimpleInStream& in,
    {proc_name}& proc) {{

{'\n'.join(deserialization)}
    return in;
}}

}}""",
        file=out_file,
    )
    if "ifdef" in proc:
        print(f"#endif // {proc['ifdef']}", file=out_file)

def output_cpp_serialize_factory(defs_proc, out_file):
    """Outputs the factory method for the procs into the out file.
    """

    cases = []

    for proc in defs_proc:
        if "name" in proc:
            proc_name = to_camel_case(proc["name"])
            case = (f"    }} else if (dynamic_cast<const {proc['namespace']}::{proc_name}*>(&proc)) {{\n"
                    f"        id = ProcId('{proc["id"][0]}', '{proc["id"][1]}');"
                    f"        out << dynamic_cast<const {proc['namespace']}::{proc_name}&>(proc);")
            if "ifdef" in proc:
                case = (f"#ifdef {proc['ifdef']}\n"
                        f"{case}\n"
                        f"#endif")
            cases.append(case)
        else:
            cases.append(f"    // -- {proc['description']}")

    print(f"""
void ProcStorage::serializeProc(SimpleOutStream& out, const rcProc::Proc& proc) const {{

    ProcId id('u', 'u');
    // note: the dynamic stuff only works if the procs
    //   are in the correct order in the definition file.
    //   in case of inheritance
    if (false) {{  // just need an 'if' case
{"\n".join(cases)}
    }}

    if (out.fail()) {{
        printf("Error while writing proc ID %c%c.\\n", id.c1, id.c2);
    }}
}}
""",
        file=out_file,
    )

def output_cpp_deserialize_factory(defs_proc, out_file):
    """Outputs the deserialize method into the out file
    """

    cases = []

    for proc in defs_proc:
        if "name" in proc:
            proc_name = to_camel_case(proc["name"])
            case = (f"    }} else if (id == ProcId{{'{proc['id'][0]}', '{proc['id'][1]}'}}) {{\n"
                    f"        auto proc2 = new {proc['namespace']}::{proc_name};\n"
                    f"        in >> *proc2;\n"
                    f"        proc = proc2;")
            if "ifdef" in proc:
                case = (f"#ifdef {proc['ifdef']}\n"
                        f"{case}\n"
                        f"#endif")
            cases.append(case)
        else:
            cases.append(f"    // -- {proc['description']}")

    print(f"""
rcProc::Proc* ProcStorage::deserializeProc(SimpleInStream& in) {{

    const ProcId id{{in.read<char>(), in.read<char>()}};
    const uint8_t len = in.read<uint8_t>();
    const auto startPos = in.tellg();

    rcProc::Proc *proc = nullptr;
    if (false) {{  // just need an 'if' case
{"\n".join(cases)}
    }} else {{
        printf("Unknown proc ID %c%c.\\n", id.c1, id.c2);
        in.seekg(startPos + len);
        proc = nullptr;
    }}

    // in case we did end up reading a different amount of
    // bytes than expected
    const auto endPos = in.tellg();
    if (len != endPos - startPos) {{
        printf("Actual length and received len for proc ID %c%c does not match.\\n", id.c1, id.c2);
        printf("%d vs. %d.\\n", static_cast<int>(endPos - startPos), static_cast<int>(len));
        in.seekg(startPos + len);
        delete proc;
        proc = nullptr;
    }}
    if (in.fail()) {{
        printf("Error while reading proc ID %c%c.\\n", id.c1, id.c2);
        delete proc;
        proc = nullptr;
    }}
    return proc;
}}
""",
        file=out_file,
    )


def output_cpp(defs_proc, out_file):
    """Outputs the serialization an deserialization code for the proc
    classes into the out file.
    """

    # get all proc filenames for our includes
    header_files = []
    for proc in defs_proc:
        if "name" in proc:
            include = f'#include "{proc["filename"]}.h"'
            if "ifdef" in proc:
                include = (f"#ifdef {proc['ifdef']}\n"
                    f"{include}\n"
                    f"#endif")
            header_files.append(include)

    print(
        f"""/** Serialization code for the proc classes.
 *
 * This file is auto generated by serialization_tool.py
 * {str(datetime.date.today())}
 *
 * Do not modify.
 *
 * @file
 */

#include "proc_storage.h"
#include "proc.h"
#include "simple_byte_stream.h"
#include <cstdio>

{'\n'.join(header_files)}

struct ProcId {{
  char c1;
  char c2;
}};

bool operator==(const ProcId& lhs, const ProcId& rhs) {{
    return (lhs.c1 == rhs.c1) && (lhs.c2 == rhs.c2);
}}

using namespace rcSignals;
using namespace rcProc;
using namespace rcInput;
""",
        file=out_file,
    )

    for proc in defs_proc:
        output_cpp_proc(proc, out_file)
    output_cpp_serialize_factory(defs_proc, out_file)
    output_cpp_deserialize_factory(defs_proc, out_file)


# --- main code

parser = argparse.ArgumentParser(
    prog="serialization_tool",
    description="Tool for creating files used in the serialization of the rc function controller proc modules.",
    epilog="Have fun",
)

parser.add_argument(
    "--procs",
    "-p",
    type=argparse.FileType("r"),
    help="Proc definition file",
    default=str(pathlib.Path(__file__).parent.parent / "config" / "procs_config.json"),
)
parser.add_argument(
    "--signals",
    "-s",
    type=argparse.FileType("r"),
    help="Signals definition file",
    default=str(pathlib.Path(__file__).parent.parent / "config" / "signals_config.json"),
)
parser.add_argument(
    "--cpp",
    "-c",
    type=argparse.FileType("w"),
    help="Create .cpp file for proc serialization.",
)

args = parser.parse_args()

defs_signals = json.load(args.signals)
# ensure no duplicate IDs
signal_ids = set()
for signal in defs_signals:
    if "id" in signal:
        if signal["id"] in signal_ids:
            exit(f"Duplicate id {signal['id']} in signal {signal['name']}")
        signal_ids.add(signal["id"])

defs_proc = json.load(args.procs)
# ensure no duplicate IDs
ids = set()
for proc in defs_proc:
    if "id" in proc:
        if proc["id"] in ids:
            exit(f"Duplicate id {proc['id']} in proc {proc['name']}")
        ids.add(proc["id"])


# add a namespace derived from the name
for proc in defs_proc:
    if "filename" in proc:
        if proc["filename"].startswith("audio_"):
            proc["namespace"] = "rcAudio"
        elif proc["filename"].startswith("engine_"):
            proc["namespace"] = "rcEngine"
        elif proc["filename"].startswith("input_"):
            proc["namespace"] = "rcInput"
        elif proc["filename"].startswith("output_"):
            proc["namespace"] = "rcOutput"
        else:
            proc["namespace"] = "rcProc"

if args.cpp:
    output_cpp(defs_proc, args.cpp)
    output_signals_array(defs_signals, args.cpp)
