#!/usr/bin/env python

import ctk_cli
from rstcloth import rstcloth

import argparse

def cli2rst(cliFile, rstFile):
    cli = ctk_cli.CLIModule(cliFile)

    rst = rstcloth.RstCloth()
    rst.title(cli.title)
    rst.newline()
    rst.content(cli.description)
    rst.newline()

    rst.field('Authors', cli.contributor)
    rst.field('Version', cli.version)
    rst.field('License', cli.license)
    rst.newline()
    rst.newline()

    for parameterGroup in cli:
        rst.h2(parameterGroup.label + ' Parameters')
        rst.content(parameterGroup.description)
        rst.newline()

        for parameter in parameterGroup:
            rst.definition(parameter.label, parameter.description, bold=True)
            rst.newline()

    rst.write(rstFile)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert a CTK CLI XML description file to a reStructuredText documentation file.')
    parser.add_argument('cliFile')
    parser.add_argument('rstFile')

    args = parser.parse_args()

    cli2rst(args.cliFile, args.rstFile)

