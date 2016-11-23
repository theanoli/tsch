#! /usr/bin/env python
#
# Copyright (c) 2004-2010 University of Utah and the Flux Group.
# 
# {{{EMULAB-LICENSE
# 
# This file is part of the Emulab network testbed software.
# 
# This file is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
# 
# This file is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
# License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this file.  If not, see <http://www.gnu.org/licenses/>.
# 
# }}}
#
import re
import sys
import socket
import os
import getopt
import string

# Maximum size of an NS file that the server will accept. 
MAXNSFILESIZE = (1024 * 512)

#
# This class defines a simple structure to return back to the caller.
# It includes a basic response code (success, failure, badargs, etc),
# as well as a return "value" which can be any valid datatype that can
# be represented in XML (int, string, hash, float, etc). You can also
# send back some output (a string with embedded newlines) to print out
# to the user.
#
# Note that XMLRPC does not actually return a "class" to the caller; It gets
# converted to a hashed array (Python Dictionary), but using a class gives
# us a ready made constructor.
#
# WARNING: If you change this stuff, also change libxmlrpc.pm in this dir.
#
RESPONSE_SUCCESS        = 0
RESPONSE_BADARGS        = 1
RESPONSE_ERROR          = 2
RESPONSE_FORBIDDEN      = 3
RESPONSE_BADVERSION     = 4
RESPONSE_SERVERERROR    = 5
RESPONSE_TOOBIG         = 6
RESPONSE_REFUSED        = 7  # Emulab is down, try again later.
RESPONSE_TIMEDOUT       = 8

class EmulabResponse:
    def __init__(self, code, value=0, output=""):
        self.code     = code            # A RESPONSE code
        self.value    = value           # A return value; any valid XML type.
        self.output   = re.sub(         # Pithy output to print
            r'[^' + re.escape(string.printable) + ']', "", output)
        
        return

#
# Read an nsfile and return a single string. 
#
def readnsfile(nsfilename, debug):
    nsfilestr  = ""
    try:
        fp = os.open(nsfilename, os.O_RDONLY)

        while True:
	    str = os.read(fp, 1024)

            if not str:
                break
            nsfilestr = nsfilestr + str
            pass

        os.close(fp)

    except:
        if debug:
            print "%s:%s" % (sys.exc_type, sys.exc_value)
            pass

        print "batchexp: Cannot read NS file '" + nsfilename + "'"
        return None
        pass

    return nsfilestr


