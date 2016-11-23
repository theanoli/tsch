#! /usr/bin/env python
#
# Copyright (c) 2004-2013 University of Utah and the Flux Group.
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

import sys
import getopt
import os, os.path
import xmlrpclib

TBROOT = "/usr/testbed"
TBPATH = os.path.join(TBROOT, "lib")

if TBPATH not in sys.path:
    sys.path.append(TBPATH)
    pass

from emulabclient import *

from M2Crypto.m2xmlrpclib import SSL_Transport
from M2Crypto import SSL

##
# The package version number
#
PACKAGE_VERSION = 0.1

# Default server and port
XMLRPC_SERVER   = "boss.emulab.net"
XMLRPC_PORT     = 3069

# User supplied server name.
xmlrpc_server   = XMLRPC_SERVER
xmlrpc_port     = XMLRPC_PORT

# The default RPC module to invoke.
module          = "experiment"

# The default path
path            = TBROOT

# Where to find the default certificate in the users home dir.
default_cert    = "/.ssl/emulab.pem"
certificate     = None;

# Debugging output.
debug           = 0

# Raw XML mode
rawmode         = 0

##
# Print the usage statement to stdout.
#
def usage():
    print "Make a request to the Emulab XML-RPC (SSL-based) server."
    print ("Usage: " + sys.argv[0] 
                     + " [-hV] [-s server] [-m module] "
                     + "<method> [param=value ...]")
    print
    print "Options:"
    print "  -h, --help\t\t  Display this help message"
    print "  -V, --version\t\t  Show the version number"
    print "  -s, --server\t\t  Set the server hostname"
    print "  -p, --port\t\t  Set the server port"
    print "  -c, --cert\t\t  Set the certificate to use"
    print "  -m, --module\t\t  Set the RPC module (defaults to experiment)"
    print
    print "Required arguments:"
    print "  method\t\t  The method to execute on the server"
    print "  params\t\t\t  The method arguments in param=value format"
    print
    print "Example:"
    print ("  "
           + sys.argv[0]
           + " -s boss.emulab.net echo str=\"Hello World!\"")
    return

#
# Process a single command line
#
def do_method(server, method_and_args):
    # Get a pointer to the function we want to invoke.
    methodname = method_and_args[0]
    if methodname.count(".") == 0:
        methodname = module + "." + methodname
        pass
    
    meth = getattr(server, methodname)

    # Pop off the method, and then convert the rest of the arguments.
    # Be sure to add the version.
    method_and_args.pop(0)

    #
    # Convert all params (name=value) into a Dictionary. 
    # 
    params = {}
    for param in method_and_args:
        plist = string.split(param, "=", 1)
        if len(plist) != 2:
            print ("error: Parameter, '"
                   + param
                   + "', is not of the form: param=value!")
            return -1
        value = plist[1]

        #
        # If the first character of the argument looks like a dictionary,
        # try to evaluate it.
        #
        if value.startswith("{"):
            value = eval(value);
            pass
    
        params[plist[0]] = value
        pass
    meth_args = [ PACKAGE_VERSION, params ]

    #
    # Make the call. 
    #
    try:
        response = apply(meth, meth_args)
        pass
    except xmlrpclib.Fault, e:
        print e.faultString
        return -1

    #
    # Parse the Response, which is a Dictionary. See EmulabResponse in the
    # emulabclient.py module. The XML standard converts classes to a plain
    # Dictionary, hence the code below. 
    # 
    if len(response["output"]):
        print response["output"]
        pass

    rval = response["code"]

    #
    # If the code indicates failure, look for a "value". Use that as the
    # return value instead of the code. 
    # 
    if rval != RESPONSE_SUCCESS:
        if response["value"]:
            rval = response["value"]
            pass
        pass

    if debug and response["value"]:
        print str(response["value"])
        pass
        
    return rval

#
# Process program arguments.
# 
try:
    # Parse the options,
    opts, req_args =  getopt.getopt(sys.argv[1:],
                      "dhVc:s:m:p:r",
                      [ "help", "version", "rawmode", "server=", "module=",
                        "cert=", "port=", "path="])
    # ... act on them appropriately, and
    for opt, val in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit()
            pass
        elif opt in ("-V", "--version"):
            print PACKAGE_VERSION
            sys.exit()
            pass
        elif opt in ("-s", "--server"):
	    xmlrpc_server = val
            #
            # Allow port spec here too.
            #
            if val.find(":") > 0:
                xmlrpc_server,xmlrpc_port = string.split(val, ":", 1)
                pass
            pass
        elif opt in ("-p", "--port"):
	    xmlrpc_port = val
            pass
        elif opt in ("-c", "--cert"):
	    certificate = val
            pass
        elif opt in ("-m", "--module"):
	    module = val
            pass
        elif opt in ("-d", "--debug"):
	    debug = 1
            pass
        elif opt in ("--path",):
	    path = val
            pass
        elif opt in ("-r", "--rawmode"):
	    rawmode = 1
            pass
        pass
    pass
except getopt.error, e:
    print e.args[0]
    usage()
    sys.exit(2)
    pass

class MySSL_Transport(SSL_Transport):
    def parse_response(self, file):
        # compatibility interface
        return self._parse_response(file, None)

    def _parse_response(self, file, sock):
        stuff = ""
        
        while 1:
            if sock:
                response = sock.recv(1024)
            else:
                response = file.read(1024)
            if not response:
                break
            stuff += response
        return stuff
        pass
    pass

class MyServerProxy(xmlrpclib.ServerProxy):
    def raw_request(self, xmlgoo):

        #
        # I could probably play tricks with the getattr method, but
        # not sure what those tricks would be! If I try to access the
        # members by name, the getattr definition in the ServerProxy class
        # tries to turn that into a method lookup at the other end. 
        #
        transport = self.__dict__["_ServerProxy__transport"]
        host      = self.__dict__["_ServerProxy__host"]
        handler   = self.__dict__["_ServerProxy__handler"]
        
        response = transport.request(host, handler, xmlgoo)

        if len(response) == 1:
            response = response[0]

        return response

    pass

#
# Vanilla SSL CTX initialization.
#
if certificate == None:
    certificate = os.environ["HOME"] + default_cert
    pass
if not os.access(certificate, os.R_OK):
    print "Certificate cannot be accessed: " + certificate
    sys.exit(-1);
    pass
    
ctx = SSL.Context('sslv23')
ctx.load_cert(certificate, certificate)
ctx.set_verify(SSL.verify_none, 16)
ctx.set_allow_unknown_ca(0)

# This is parsed by the Proxy object.
URI = "https://" + xmlrpc_server + ":" + str(xmlrpc_port) + path
if debug:
    print >>sys.stderr, URI
    pass

if rawmode:
    # Get a handle on the server,
    server = MyServerProxy(URI, MySSL_Transport(ctx));

    stuff = ""
    while (True):
        foo = sys.stdin.read(1024 * 16)
        if foo == "":
            break
        stuff += foo
        pass

    #
    # Make the call. 
    #
    try:
        response = server.raw_request(stuff)
        pass
    except xmlrpclib.Fault, e:
        print e.faultString
        sys.exit(-1);
        pass

    print str(response);
    sys.exit(0);
elif len(req_args):
    # Get a handle on the server,
    server = xmlrpclib.ServerProxy(URI, SSL_Transport(ctx));
    # Method and args are on the command line.
    sys.exit(do_method(server, req_args))
else:
    # Get a handle on the server,
    server = xmlrpclib.ServerProxy(URI, SSL_Transport(ctx));
    # Prompt the user for input.
    try:
        while True:
            line = raw_input("$ ")
            tokens = line.split(" ")
            if len(tokens) >= 1 and len(tokens[0]) > 0:
                print str(do_method(server, tokens))
                pass
            pass
        pass
    except EOFError:
        pass
    print
    pass

