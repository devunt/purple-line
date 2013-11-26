#!/usr/bin/env python2
# coding: utf-8

# Parses decompiled Java source and re-generates a Thrift interface file.

import re
import sys
import os
from os import path

thrift_type_names = [
    None,
    "void",
    "bool",
    "byte",
    "double",
    None,
    "i16",
    None,
    "i32",
    None,
    "i64",
    "string",
    "struct",
    "map",
    "set",
    "list",
    "enum",
]

show_oname = False

class TwoWayMap:
    def __init__(self, *args):
        self._map = {}
        self._map_back = {}

        for a, b in args:
            self.add(a, b)

    def add(self, a, b):
        self._map[a] = b
        self._map_back[b] = a

    def map(self, a):
        return self._map[a]

    def map_back(self, b):
        return self._map_back[b]

    def map_back_fallback(self, b, default):
        return self._map_back[b] if b in self._map_back else default

    def items(self):
        return self._map.items()

    def keys(self):
        return self._map.keys()

    def back_keys(self):
        return self._back_map.keys()

class ThriftField:
    def __init__(self, name, index):
        self.name = name
        self.index = index
        self.type = None

    def serialize(self):
        return "{0}: {1} {2}".format(self.index, self.type.tname(), self.name)

class ThriftPrimitive:
    def __init__(self, type_id, binary=False):
        self.primitive_name = thrift_type_names[type_id]
        self.binary = binary
        self.typedef_name = None

    def tname(self):
        return self.typedef_name if self.typedef_name else self.primitive_name

class ThriftMap:
    def __init__(self, ktype, etype):
        self.ktype = ktype
        self.etype = etype

    def tname(self):
        return "map<{0}, {1}>".format(
            self.ktype.tname(),
            self.etype.tname())

class ThriftSet:
    def __init__(self, etype):
        self.etype = etype

    def tname(self):
        return "set<{0}>".format(self.etype.tname())

class ThriftList:
    def __init__(self, etype):
        self.etype = etype

    def tname(self):
        return "list<{0}>".format(self.etype.tname())

class ThriftStruct:
    def __init__(self, oname, name, virtual, exception):
        self.oname = oname
        self.name = name
        self.virtual = virtual
        self.exception = exception
        self.fields = {}
        self.deps = set()

    def tname(self):
        return self.name

    def serialize(self):
        r = "{0} {1} {{".format(
            "exception" if self.exception else "struct",
            self.tname())

        if show_oname:
            r += " // " + self.oname

        r += "\n"

        for f in sorted(self.fields.values(), key=lambda f: f.index):
            r += "    " + f.serialize() + ";\n"

        r += "}"

        return r

class ThriftEnum:
    def __init__(self, oname, name):
        self.oname = oname
        self.name = name
        self.values = []
        self.name_candidates = {}

    def tname(self):
        return self.name

    def best_name_candidate(self):
        return sorted(self.name_candidates.items(), key=lambda i: i[1], reverse=True)[0][0]

    def serialize(self):
        r = "enum {0} {{".format(
            self.tname())

        if show_oname:
            r += " // " + self.oname

        r += "\n"

        for name, value in self.values:
            r += "    {0} = {1};\n".format(name, value)

        r += "}"

        return r

class ThriftMethod:
    def __init__(self, name, args, return_, exceptions):
        self.name = name
        self.args = args
        self.return_ = return_
        self.exceptions = exceptions

    def serialize(self):
        r = "    " + self.return_.tname() + " " + self.name + "(\n"

        for f in self.args:
            r += "        " + f.serialize() + ",\n"

        r = r.rstrip(",\n")

        r += ")"

        if self.exceptions:
            r += " throws ("
            for f in self.exceptions:
                r += f.serialize()

            r = r.rstrip(",")

            r += ")"

        r += ";"

        return r

class ThriftService:
    def __init__(self, fileroot, indexfile, name):
        self.fileroot = fileroot
        self.indexfile = indexfile
        self.name = name
        self.methods = {}
        self.structs = {}
        self.enums = {}

        self._enum_name_hints = {}

        self._code = {}

        # Based on version "?"
        self.names = TwoWayMap(
            ("TStruct", "j"),
            ("TField", "b"),
            ("EnumMetaData", "dtd"),
            ("FieldMetaData", "dte"),
            ("FieldValueMetaData", "dtf"),
            ("ListMetaData", "dtg"),
            ("MapMetaData", "dth"),
            ("SetMetaData", "dti"),
            ("StructMetaData", "dtj"),
        )

        self._parse()

    def serialize(self):
        r = "// This file was generated with rethrift.py\n"
        r += "// enum names were guessed, but the rest of the names should be accurate.\n\n"

        for e in sorted(self.enums.values(), key=lambda e: e.tname()):
            r += e.serialize() + "\n\n"

        structs = sorted((s for s in self.structs.values() if not s.virtual), key=lambda s: s.tname())

        for s in self._struct_dep_sort(structs):
            r += s.serialize() + "\n\n"

        r += "service " + self.name + " {"

        for m in sorted(self.methods.values(), key=lambda m: m.name):
            r += "\n" + m.serialize() + "\n"

        r += "}\n"

        return r

    def obfuscate(self, name):
        for o, u in self.name_map:
            if u == name:
                return o

        return name

    def unobfuscate(self, name):
        for o, u in self.name_map:
            if o == name:
                return u

        return name

    def _get_code(self, fn):
        if fn in self._code:
            return self._code[fn]

        code = open(path.join(self.fileroot, fn + ".jad"), "r").read()

        self._code[fn] = code
        return code

    def _get_enum(self, fn):
        if fn in self.enums:
            return self.enums[fn]

        e = ThriftEnum(fn, self.names.map_back_fallback(fn, None))
        self.enums[fn] = e

        for m in re.finditer("new " + fn + "\\(\"([^\"]+)\", \\d+, (\\d+)\\);", self._get_code(fn)):
            e.values.append((m.group(1), int(m.group(2))))

        return e

    def _parse_meta(self, meta, context):
        # TODO: typedefs

        m = re.match("new " + self.names.map("EnumMetaData") + "\\(([a-z]+)\)", meta)
        if m:
            enum = self._get_enum(m.group(1))

            context["enums"].add(enum)

            return enum, meta[len(m.group(0)):]

        m = re.match("new " + self.names.map("FieldValueMetaData") + "\\(\\(byte\\)(\\d+)(?:, \"([^\"]+)\")?\\)", meta)
        if m:
            return ThriftPrimitive(int(m.group(1)), True), meta[len(m.group(0)):]

        # binary strings?
        m = re.match("new " + self.names.map("FieldValueMetaData") + "\\(\\(byte\\)(\\d+), true\\)", meta)
        if m:
            return ThriftPrimitive(int(m.group(1))), meta[len(m.group(0)):]

        m = re.match("new " + self.names.map("ListMetaData") + "\\(", meta)
        if m:
            etype, meta = self._parse_meta(meta[len(m.group(0)):], context)
            return ThriftList(etype), meta[1:]

        m = re.match("new " + self.names.map("MapMetaData") + "\\(", meta)
        if m:
            ktype, meta = self._parse_meta(meta[len(m.group(0)):], context)
            meta = meta[2:]
            etype, meta = self._parse_meta(meta, context)

            return ThriftMap(ktype, etype), meta[1:]

        m = re.match("new " + self.names.map("SetMetaData") + "\\(", meta)
        if m:
            etype, meta = self._parse_meta(meta[len(m.group(0)):], context)
            return ThriftList(etype), meta[1:]

        m = re.match("new " + self.names.map("StructMetaData") + "\\(([a-z]+)\)", meta)
        if m:
            struct, meta = self._get_struct(m.group(1)), meta[len(m.group(0)):]

            context["deps"].add(struct)

            return struct, meta

        raise Exception("Can't parse metadata: " + meta)

    def _get_struct(self, fn, virtual=False, result_struct=False, exception=False):
        if fn in self.structs:
            return self.structs[fn]

        code = self._get_code(fn)
        m = re.search("new " + self.names.map("TStruct") + "\\(\"(.+?)\"\\);", code)

        s = ThriftStruct(fn, m.group(1), virtual, exception)
        self.structs[fn] = s

        for m in re.finditer("new " + self.names.map("TField") + "\\(\"(.+?)\", \\(byte\\)\\d+, \\(short\\)(\\d+)\\);", code):
            s.fields[m.group(1)] = ThriftField(m.group(1), int(m.group(2)))

        for m in re.finditer("\\.put\\(\\w+\\.(\\w+), new " + self.names.map("FieldMetaData") + "\\(\"(.+?)\", ([^;]+)\\);", code):
            ofname = m.group(1)
            f = s.fields[m.group(2)]

            context = { "deps": set(), "enums": set() }
            f.type, remaining = self._parse_meta(m.group(3), context)

            if remaining != ")":
                raise Exception("Couldn't fully parse: " + m.group(3))

            s.deps.update(context["deps"])

            for e in context["enums"]:
                sname = s.name[0].upper() + s.name[1:].replace("_args", "")

                if s.name.endswith("_result"):
                    key = (sname.replace("_result", ""), "Result")
                else:
                    key = (sname, f.name[0].upper() + f.name[1:])

                nc = e.name_candidates
                if key in nc:
                    nc[key] += 1
                else:
                    nc[key] = 1

            if isinstance(f.type, ThriftPrimitive) and f.type.tname() == "struct":
                m = re.search("\n    public (\\w+) " + ofname + ";", code)
                f.type = self._get_struct(m.group(1), exception=result_struct)

        return s

    def _guess_enum_names(self):
        counts = {}

        for s in self.structs.values():
            counts[s.tname()] = 1

        for e in self.enums.values():
            sname, fname = e.best_name_candidate()

            if fname in counts:
                counts[fname] += 1
            else:
                counts[fname] = 1

        for e in self.enums.values():
            sname, fname = e.best_name_candidate()

            if not e.name:
                if counts[fname] == 1:
                    e.name = fname
                else:
                    e.name = sname + fname

    # http://stackoverflow.com/questions/4106862/how-to-sort-depended-objects-by-dependency
    def _struct_dep_sort(self, structs):
        sorted_structs = []
        visited = set()

        for s in structs:
            self._visit_struct(s, visited, sorted_structs)

        return sorted_structs

    def _visit_struct(self, s, visited, sorted_structs):
        if s in visited:
            return

        visited.add(s)

        for dep in s.deps:
            self._visit_struct(dep, visited, sorted_structs)

        sorted_structs.append(s)

    def _parse(self):
        index = self._get_code(self.indexfile)

        snames = TwoWayMap()

        for fn in (
            path.splitext(fn)[0]
            for fn
            in os.listdir(self.fileroot)
            if fn.endswith(".jad")
        ):
            m = re.search("new " + self.names.map("TStruct") + "\\(\"(.+?)\"\\);",
                self._get_code(fn))

            if m:
                snames.add(m.group(1), fn)

        for aname, afn in (i for i in snames.items() if i[0].endswith("_args")):
            name = aname.replace("_args", "")

            if not "\"" + name + "\"" in index:
                continue

            args = self._get_struct(afn, True)
            result = self._get_struct(snames.map(name + "_result"), True, True)

            rfields = sorted(result.fields.values(), key=lambda f: f.index)

            return_ = ThriftPrimitive(1)

            if len(rfields) and rfields[0].name == "success":
                return_ = rfields[0].type
                exceptions = rfields[1:]
            else:
                exceptions = rfields

            self.methods[name] = ThriftMethod(
                name,
                sorted(args.fields.values(), key=lambda f: f.index),
                return_,
                exceptions)

        self._guess_enum_names()

print ThriftService(*sys.argv[1:]).serialize()
