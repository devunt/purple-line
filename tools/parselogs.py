#!/usr/bin/env python2.7
# coding:utf-8

# Parses and displays raw thrift structure and shows deserialized results for available types. The
# log data format is the one used by https://github.com/mvirkkunen/tcp-yoink

from subprocess import call

# Regenerate thrift files
#call(["rm", "-rr", "thrift_line_py"])
#call(["mkdir", "thrift_line_py"])
#call(["thrift", "--gen", "py", "-out", "thrift_line_py", "../line.thrift"])

from thrift.Thrift import TType, TMessageType
from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.transport import THttpClient
from thrift.protocol import TBinaryProtocol, TCompactProtocol
import thrift_line_py.line.ttypes as linetypes
import thrift_line_py.line.Line as lineclient
import zlib
import sys

linetypes.fastbinary = None

def serialize_value(prot, ftype, indent=""):
    if ftype == TType.VOID:
        return "(void)"
    elif ftype == TType.BOOL:
        return ("true" if prot.readBool() else "false")
    elif ftype == TType.BYTE:
        return "byte " + str(prot.readByte())
    elif ftype == TType.DOUBLE:
        return "double " + str(prot.readDouble())
    elif ftype == TType.I16:
        return "i16 " + str(prot.readI16())
    elif ftype == TType.I32:
        return "i32 " + str(prot.readI32())
    elif ftype == TType.I64:
        return "i64 " + str(prot.readI64())
    elif ftype == TType.STRING:
        return "\"" + str(prot.readString()) + "\""

    r = ""
    next_indent = indent + "  ";

    if ftype == TType.STRUCT:
        prot.readStructBegin()
        r += "struct {\n"

        while True:
            (name, ftype, fid) = prot.readFieldBegin()

            if ftype == TType.STOP:
                break

            if name is None:
                name = "field"

            r += (next_indent + str(fid) + ": " + name
                + " = " + serialize_value(prot, ftype, next_indent) + "\n")

            prot.readFieldEnd()

        prot.readStructEnd()
        r += indent + "}"
    elif ftype == TType.MAP:
        (ktype, vtype, size) = prot.readMapBegin()
        r += "{\n"

        for x in range(size):
            key = serialize_value(prot, ktype)
            value = serialize_value(prot, vtype, next_indent)

            r += next_indent + key + " = " + value + "\n"

        prot.readMapEnd()
        r += indent + "]"
    elif ftype == TType.SET:
        (etype, size) = prot.readSetBegin()
        r += "set [\n"

        for x in range(size):
            r += next_indent + serialize_value(prot, etype, next_indent) + "\n"

        prot.readSetEnd()
        r += indent + "]"
    elif ftype == TType.LIST:
        (etype, size) = prot.readListBegin()
        r += "[\n"

        for x in range(size):
            r += next_indent + serialize_value(prot, etype, next_indent) + "\n"

        prot.readListEnd()
        r += indent + "]"
    else:
        return "!UNKNOWN"

    return r

class LogStream():
    def __init__(self, index, host):
        self.index = index
        self.host = host
        self.packets = []
        self.cur_dir = None

    def append(self, dir, data):
        if self.cur_dir != dir:
            self.packets.append(b"")
            self.cur_dir = dir

        self.packets[len(self.packets) - 1] += data

def parse_log(path):
    log = open(path, "rb")

    streams = {}

    while True:
        l = log.readline()
        if l == "":
            break

        p = l.rstrip().split()

        if p[2] == "CONNECT":
            streams[int(p[1])] = LogStream(int(p[1]), p[3])

        if p[2] in ("IN", "OUT"):
            streams[int(p[1])].append(p[2], log.read(int(p[3])))
            log.read(1) # discard newline

    return streams

stream = parse_log(sys.argv[1])[int(sys.argv[2])]

Protocol = locals()["TCompactProtocol" if len(sys.argv) < 4 else sys.argv[3]]

for p in stream.packets:
    parts = p.split(b"\r\n\r\n", 2)

    print parts[0].splitlines()[0]

    data = parts[1]
    if "Content-Encoding: gzip" in parts[0]:
        data = zlib.decompress(data, 16 + zlib.MAX_WBITS)
        print "non-thrift data: " + data
        continue

    if len(data) == 0:
        print "empty data"
        continue

    prot = Protocol(TTransport.TMemoryBuffer(data))

    name, mtype, _ = prot.readMessageBegin()

    print "Message: " + name + " Type: " + ["?", "CALL", "REPLY", "EXCEPTION", "ONEWAY"][mtype]

    type_name = name + ("_args" if mtype == TMessageType.CALL else "_result")

    if hasattr(lineclient, type_name):
        print "Known type:"
        obj = getattr(lineclient, type_name)()
        obj.read(prot)
        print obj
        print

    prot = Protocol(TTransport.TMemoryBuffer(data))
    prot.readMessageBegin()

    print "Dumped:"
    try:
        print serialize_value(prot, TType.STRUCT)
    except:
        print "Error!"
    print
