#!/usr/bin/env python3

# this script will create a nice .js file
# out of a json file
#
# example usage: create_js.py config/signals_config.json def_signals.js defSignals
#

import argparse
import datetime
import pathlib

parser = argparse.ArgumentParser(
    prog="signals_tool",
    description="Tool for creating header file with signal definitions.",
)

parser.add_argument(
    "input",
    type=argparse.FileType("r"),
    help="Json input file",
)
parser.add_argument(
    "output",
    type=argparse.FileType("w"),
    help="Javascript output file",
)
parser.add_argument(
    "name",
    help="Variable name",
)

args = parser.parse_args()

input_str = args.input.read()

print(f"""/**
 *
 * This file is auto generated by create_js.py
 * {str(datetime.date.today())}
 *
 * Do not modify.
 *
 * @file
 */

export let {args.name} = {input_str};
""",
    file=args.output,
)
