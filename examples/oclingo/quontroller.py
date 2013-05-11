#!/usr/bin/env python
#########################################################################
#   Copyright 2010-2013 Torsten Grote, Philipp Obermeier
#
#       This program is free software: you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation, either version 3 of the License, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##########################################################################


import sys
import os
import socket
import re
import time
import argparse
import controller
#import pdb; pdb.set_trace()

usage = "usage: %(prog)s -o encoding.lp [-c setup.ini]"
parser = argparse.ArgumentParser(usage=usage)
parser.add_argument("-o", dest = "enc", nargs = "+",
                    help = """Query program. Default:
                        %(default)s""")
parser.add_argument("-c", dest = "setup", nargs= "?",
                    help = """ Query setup. Default:
                        %(default)s""")
parser.add_argument("-d", "--debug", dest="debug",
                    action="store_true",
                    help="Show debugging output")
parser.set_defaults(
    enc = '',
    setup = '',
    debug = False,
    host = 'localhost',
    port = 25277,
)
args = parser.parse_args()
incStepCounter = 0 # incremental StepCounter


# def getClParser():
#     """Parser for command line input"""
#     usage = "usage: %(prog)s -o encoding.lp [-c setup.ini]"
#     parser = argparse.ArgumentParser(usage=usage)
#     parser.add_argument("-o", dest = "enc", nargs = "+",
#                         help = """Query program. Default:
#                             %(default)s""")
#     parser.add_argument("-c", dest = "setup", nargs= "?",
#                         help = """ Query setup. Default:
#                             %(default)s""")
#     parser.add_argument("-d", "--debug", dest="debug",
#                         action="store_true",
#                         help="Show debugging output")
#     parser.set_defaults(
#         enc = '',
#         setup = '',
#         debug = False
#     )
#     return parser
    

# class GlobalData:
#     """Stores all global data"""
#     def __init__(self):
#         self.args = getClParser().parse_args()

#globalData = GlobalData()


SETUP_PARSER = {
    'setup' : re.compile("^#setup\.(\s*%.*)?$"),
    
#     'domain' : re.compile("^#domain\s+(?P<relName>.+)\((?P<relArgs>)\)\s*:\s*(?P<argDomains>[^.]*).(\s*%.*)?$"),

    'domain' :
    re.compile("^#domain\s+(?P<name>\w+)\((?P<args>(([A-Z]\w*)(,[A-Z]\w*)*)?)\)\s*:\s*(?P<assignments>.*)\s*\.(\s*%.*)?$"),  
    
    'choose' : re.compile("^#choose\s+(?P<name>\w+)/(?P<arity>\d+)\s*\.(\s*%.*)?$"),
    
    'define' : re.compile("^#define\s+(?P<name>\w+)/(?P<arity>\d+)\s*\.(\s*%.*)?$"),
    
    'query' : re.compile("^#query\s+(?P<name>\w+)/(?P<arity>\d+)\s*\.(\s*%.*)?$"),
    
    'show' : re.compile("^#show\s+(?P<name>\w+)/(?P<arity>\d+)\s*\.(\s*%.*)?$"),
    
    'endsetup' : re.compile("#endsetup\.(\s*%.*)?$")
}

IN_PARSER = {
    'query' : re.compile("^#query\s*\.(\s*%.*)?$"),
    'assert' : re.compile("^#assert\s*:\s*(?P<name>\w+)\((?P<args>(\w+(,\w+)*)?)\)\s*\.(\s*%.*)?$"),
    'assertv' :  re.compile("^#assert\s*\.(\s*%.*)?$"),
    'fact' :  re.compile("^(?P<name>\w+)\((?P<args>(\w+(,\w+)*)?)\)\s*\.(\s*%.*)?$"),
    'endquery' : re.compile("^#endquery\s*\.(\s*%.*)?$")
}


def main():
    #controller.globalData.args = args

    if args.debug:
        controller.globalData.args.debug = True
    encSetup = ''
    if args.setup:
        encSetup = getRewrittenSetup(getFileContent(args.setup))
        
    controller.launchOclingoWrapper(args.enc, encSetup)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    controller.connectToSocket(sock, args.host, int(args.port))

    # loop to fetch and display answer sets received from ogling
    while not controller.exit:
        try:
            try:
                # receive answer sets
                answer_sets = controller.getAnswerSets(sock)
            except RuntimeWarning as e:
                print e.message
                continue
            except SyntaxError as e:
                print e.message
                
            # send answer sets to stdout
            controller.processAnswerSets(answer_sets)
            
            input = getRewrittenInput()
            controller.sendInput(sock, input)

        except socket.error:
            print "Socket was closed."
            break
    controller.closeSocket(sock)
    return 0

    
def getRewrittenInput():
    """Main method to read input from STDIN, rewrites output to
    compatible input for oclingo"""
    print "[Quontroller-Mode] Please enter your query."
    global incStepCounter
    incStepCounter += 1
    result = ""
    while True:
        line=sys.stdin.readline()
        if IN_PARSER['query'].match(line):
            result += ("#step " + str(incStepCounter) + " : 0.\n" +
                       "#forget " + str(incStepCounter-1) + ".\n"
            )
        elif IN_PARSER['assert'].match(line):
            match = IN_PARSER['assert'].match(line)
            result += ("#assert : " + match.group('name') +
                       "(" + match.group('args') + ")" +
                       ".\n"
            )
        elif IN_PARSER['assertv'].match(line):
            result += "#volatile : 1.\n"
        elif IN_PARSER['fact'].match(line):
            match = IN_PARSER['fact'].match(line)
            result += ("_assert_" + match.group('name') +
                       "(" + match.group('args') + "," +
                       str(incStepCounter) + ")" +
                       ".\n"
            )
        elif controller.IN_PARSER['retract'].match(line):
            result += line
        elif IN_PARSER['endquery'].match(line):
            result += "#endstep.\n"
            break
        elif controller.IN_PARSER['stop'].match(line) != None:
            result += line
            break
        else:
            print "  Warning: Ignoring unknown input."           
#    result = controller.getInput()
    return result

def getRewrittenSetup(setup):
    """Rewrites setup given as string and returns result"""

    def getArgVars(n):
        """ Creates argument vars "X0,X1,X2,..."""
        argvars = ""
        for i in range(n):
            if i == 0:
                argvars += "X1"
            else:
                argvars += ",X" + str(i + 1)
        return argvars

    result = ''
    for line in setup.splitlines():
        if SETUP_PARSER['domain'].match(line):
            match = SETUP_PARSER['domain'].match(line)
            result += ("% " + line + "\n" +
                       "#base.\n" +
                       "_domain_" + match.group('name') +
                       "(" + match.group("args") + ")" +
                       " :- " + match.group("assignments") +
                       ".\n"
            )
        if SETUP_PARSER['choose'].match(line):
            match = SETUP_PARSER['choose'].match(line)
            argvars = getArgVars(int(match.group('arity')))
            result += ("% " + line + "\n" +
                       "#base.\n" +
                       "{ " + match.group('name') +
                       "(" + argvars + ")" + " : " +
                       "_domain_" + match.group('name') +
                       "(" + argvars + ")" +
                       " }.\n"
            )
        if SETUP_PARSER['show'].match(line):
            match = SETUP_PARSER['show'].match(line)
            argvars = getArgVars(int(match.group('arity')))
            result += ( "% " + line + "\n" +
                        "#base.\n" +
                        "#show " + match.group('name') +
                        "(" + argvars + ")" + " : "
                        "_domain_" + match.group('name') +
                          "(" + argvars + ")" +
                        ".\n"
            )
        if SETUP_PARSER['query'].match(line):
            match = SETUP_PARSER['query'].match(line)
            argvars = getArgVars(int(match.group('arity')))
            result += ( "% " + line + "\n"
                        "#cumulative t.\n"
                        "#external " +
                        "_assert_" + match.group('name') +
                         "(" + argvars + ",t)" + " : " +
                         "_domain_" + match.group('name') +
                        "(" + argvars + ")" +
                        ".\n" +
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + " :- " +
                        "_domain_" + match.group('name') +
                        "(" + argvars + ")" + ", "+
                        "_assert_" + match.group('name') +
                        "(" + argvars + ",t)" +
                        ".\n" +
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + " :- " +
                        "_domain_" + match.group('name') +
                        "(" + argvars + ")" + ", "+
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t-1)" +
                        ".\n" +
                        "#volatile t.\n" +
                        ":- " +
                        "_domain_" + match.group('name') +
                         "(" + argvars + ")" + ", " +
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + ", " +
                        "not " + match.group('name') +
                        "(" + argvars + ")" +
                        ".\n"
            )
        if SETUP_PARSER['define'].match(line):
            match = SETUP_PARSER['define'].match(line)
            argvars = getArgVars(int(match.group('arity')))
            result += ( "% " + line + "\n"
                        "#cumulative t.\n"
                         "#external " +
                        "_assert_" + match.group('name') +
                         "(" + argvars + ",t)" + " : " +
                         "_domain_" + match.group('name') +
                        "(" + argvars + ")" +
                        ".\n"+ 
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + " :- " +
                        "_domain_" + match.group('name') +
                        "(" + argvars + ")" + ", "+
                        "_assert_" + match.group('name') +
                        "(" + argvars + ",t)" +
                        ".\n" +
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + " :- " +
                        "_domain_" + match.group('name') +
                        "(" + argvars + ")" + ", "+
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t-1)" +
                        ".\n" +
                        "#volatile t.\n" +
                        ":- " +
                        "_domain_" + match.group('name') +
                         "(" + argvars + ")" + ", " +
                        "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" + ", " +
                        "not " + match.group('name') +
                        "(" + argvars + ")" +
                        ".\n"
                         ":- " +
                        "_domain_" + match.group('name') +
                         "(" + argvars + ")" + ", " +
                        match.group('name') +
                        "(" + argvars + ")" + ", " +
                        "not " + "_derive_" + match.group('name') +
                        "(" + argvars + ",t)" +
                        ".\n"
            )
        result +="\n"
       # print line
    return result

def getFileContent(path):
    """Rewrites setup file into corresponding ASP rules"""
    if os.path.exists(path):
        with open(path, 'r') as content_file:
            content = content_file.read()
    else:
        raise RuntimeError("Could not find setup file '%s'." % file)
    return content

    
if __name__ == '__main__':
    if sys.version < '2.7':
        print 'You need at least python 2.7'
        sys.exit(1)

    if args.debug:
        sys.exit(main())
    else:
        try:
            sys.exit(main())
        except Exception, err:
            sys.stderr.write('ERROR: %s\n' % str(err))
            if 'pdb' in sys.modules:
                e, m, tb = sys.exc_info()
                pdb.post_mortem(tb)
            sys.exit(1)

