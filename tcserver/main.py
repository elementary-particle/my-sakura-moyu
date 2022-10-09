# -*- coding: utf-8 -*-

import json, re
import sqlite3 as sql
from wsgiref.simple_server import make_server

HTTP_MESSAGES = {
    200: "200 OK",
    201: "201 Created",
    202: "202 Accepted",
    203: "203 Non-Authoritative Information",
    204: "204 No Content",
    205: "205 Reset Content",
    206: "206 Partial Content",
    400: "400 Bad Request",
    401: "401 Unauthorized",
    403: "403 Forbidden",
    404: "404 Not Found",
    405: "405 Method Not Allowed",
}


class LocatorParser:
    r"""Callable to turn path expressions into regexes with named groups.

    For instance ``"/hello/{name}"`` becomes ``r"^\/hello\/(?P<name>[^\^.]+)$"``

    For ``/hello/{name:pattern}``
    you get whatever is in ``self.patterns['pattern']`` instead of ``"[^\^.]+"``

    Optional portions of path expression can be expressed ``[like this]``

    ``/hello/{name}[/]`` (can have trailing slash or not)

    Example::

        /blog/archive/{year:digits}/{month:digits}[/[{article}[/]]]

    This would catch any of these::

        /blog/archive/2005/09
        /blog/archive/2005/09/
        /blog/archive/2005/09/1
        /blog/archive/2005/09/1/

    (I am not suggesting that this example is a best practice.
    I would probably have a separate mapping for listing the month
    and retrieving an individual entry. It depends, though.)
    """

    var_start, var_end = "{}"
    opt_start, opt_end = "[]"
    _patterns = {
        "word": r"\w+",
        "alpha": r"[a-zA-Z]+",
        "digits": r"\d+",
        "number": r"\d*.?\d+",
        "chunk": r"[^/^.]+",
        "segment": r"[^/]+",
        "any": r".+",
    }
    default_pattern = "chunk"

    def __init__(self, patterns=None):
        """Initialize with character class mappings."""
        self.patterns = dict(self._patterns)
        if patterns is not None:
            self.patterns.update(patterns)

    def lookup(self, name):
        """Return the replacement for the name found."""
        if ":" in name:
            name, pattern = name.split(":")
            pattern = self.patterns[pattern]
        else:
            pattern = self.patterns[self.default_pattern]
        if name == "":
            name = "__%s" % self._pos
            self._pos += 1
        return f"(?P<{name}>{pattern})"

    @staticmethod
    def lastly(regex):
        """Process the result of __call__ right before it returns.

        Adds the ^ and the $ to the beginning and the end, respectively.
        """
        return "^%s$" % regex

    @staticmethod
    def prefix(regex):
        """Process the result of ``__call__`` right before it returns.

        Adds the ^ to the beginning but no $ to the end.
        Called as a special alternative to ‘lastly’.
        """
        return "^%s" % regex

    def outermost_optionals_split(self, text):
        """Split out optional portions by outermost matching delimits."""
        parts = []
        buffer = ""
        starts = ends = 0
        for c in text:
            if c == self.opt_start:
                if starts == 0:
                    parts.append(buffer)
                    buffer = ""
                else:
                    buffer += c
                starts += 1
            elif c == self.opt_end:
                ends += 1
                if starts == ends:
                    parts.append(buffer)
                    buffer = ""
                    starts = ends = 0
                else:
                    buffer += c
            else:
                buffer += c
        if not starts == ends == 0:
            raise ValueError("Mismatch of optional portion delimiters.")
        parts.append(buffer)
        return parts

    def parse(self, text):
        """Turn a path expression into regex."""
        if self.opt_start in text:
            parts = self.outermost_optionals_split(text)
            parts = map(self.parse, parts)
            parts[1::2] = ["(%s)?" % p for p in parts[1::2]]
        else:
            parts = [part.split(self.var_end) for part in text.split(self.var_start)]
            parts = [y for x in parts for y in x]
            parts[::2] = map(re.escape, parts[::2])
            parts[1::2] = map(self.lookup, parts[1::2])
        return "".join(parts)

    def __call__(self, url_pattern):
        """Turn a path expression into regex via parse and lastly."""
        self._pos = 0
        if url_pattern.endswith("|"):
            return self.prefix(self.parse(url_pattern[:-1]))
        else:
            return self.lastly(self.parse(url_pattern))


class Router:
    @staticmethod
    def method_not_allowed(environ, start_response):
        start_response(
            "405 Method Not Allowed",
            [
                ("Allow", ", ".join(environ["router.methods"])),
                ("Content-Type", "text/plain"),
            ],
        )
        return [
            "405 Method Not Allowed\n\n"
            "The request method is known by the server but "
            "is not supported by the target resource."
        ]

    @staticmethod
    def not_found(environ, start_response):
        start_response("404 Not Found", [("Content-Type", "text/plain")])
        return [
            "404 Not Found\n\n"
            "The server can not find the requested resource."
        ]

    def __init__(self, prefix=""):
        self.prefix = prefix
        self.parser = LocatorParser()
        self.routes = list()

    def add(self, path, **handle_func):
        regex = self.parser(self.prefix + path)
        compiled_regex = re.compile(regex, re.DOTALL | re.MULTILINE)
        self.routes.append((compiled_regex, handle_func))

    def select(self, path, method):
        for regex, handle_func in self.routes:
            match = regex.search(path)
            if match:
                methods = handle_func.keys()
                if method in handle_func:
                    return (
                        handle_func[method],
                        match.groupdict(),
                        methods,
                        match.group(0)
                    )
                else:
                    return self.method_not_allowed, dict(), methods, str()
        return self.not_found, dict(), list(), str()

    def __call__(self, environ, start_response):
        app, kwargs, methods, matched = self.select(
            environ["PATH_INFO"], environ["REQUEST_METHOD"]
        )
        unnamed, named = environ.get("wsgiorg.routing_args", ([], {}))
        t = list()
        for k, v in kwargs.items():
            if k.startswith("__"):
                t.append((int(k[2:]), v))
            else:
                named[k] = v
        t.sort()
        t = [_[1] for _ in t]
        unnamed += t
        environ["wsgiorg.routing_args"] = unnamed, named
        environ["router.methods"] = methods
        return app(environ, start_response)


def router_func(func):
    def wrap_func(environ, start_response):
        args, kwargs = environ.get("wsgiorg.routing_args", ([], {}))
        return func(environ, start_response, *list(args), **dict(kwargs))

    return wrap_func


def router_meth(func):
    def wrap_meth(self, environ, start_response):
        args, kwargs = environ.get("wsgiorg.routing_args", ([], {}))
        return func(self, environ, start_response, *list(args), **dict(kwargs))

    return wrap_meth


def http_bad_request(environ, start_response):
    start_response("400 Bad Request", [("Content-Type", "text/plain")])
    return [
        "400 Bad Request\n\n"
        "The server cannot or will not process the request due to "
        "something that is perceived to be a client error."
    ]


def json_app(meth):
    def wrap_app(self, environ, start_response, *args, **kwargs):
        try:
            content_length = environ.get("CONTENT_LENGTH", "")
            if not content_length:
                request_json = None
            else:
                request_size = int(content_length)
                request_data = environ["wsgi.input"].read(request_size)
                request_json = json.loads(request_data.decode("utf-8"))
        except ValueError:
            return http_bad_request(environ, start_response)
        kwargs["data"] = request_json
        code, reply = meth(self, environ, *args, **kwargs)
        start_response(HTTP_MESSAGES[code],
                       [("Access-Control-Allow-Origin", "*"),
                        ("Content-Type", "application/json")])
        return [json.dumps(reply)]

    return wrap_app


class TCServer:
    attr_list = ["id", "location", "source", "target", "comment", "state"]
    attr_defs = ["int primary key not null", "text", "text not null", "text", "text", "int"]
    attr_set = frozenset(attr_list)

    @staticmethod
    def to_object(row):
        result = dict()
        for i in range(len(TCServer.attr_list)):
            result[TCServer.attr_list[i]] = row[i]
        return result

    @staticmethod
    def options(environ, start_response):
        start_response("200 OK", [
            ("Access-Control-Allow-Headers", "*"),
            ("Access-Control-Allow-Methods", "*"),
            ("Access-Control-Allow-Origin", "*"),
        ])
        return []

    def __init__(self, db_path):
        self.conn = sql.connect(db_path)
        self.conn.row_factory = sql.Row
        self.router = Router("/api")
        self.router.add("",
                        GET=self.list_project,
                        OPTIONS=self.options)
        self.router.add("/commit",
                        GET=self.commit,
                        OPTIONS=self.options)
        self.router.add("/{project:word}",
                        POST=self.add_project,
                        DELETE=self.del_project,
                        OPTIONS=self.options)
        self.router.add("/{project:word}/range",
                        GET=self.get_range,
                        OPTIONS=self.options)
        self.router.add("/{project:word}/batch",
                        POST=self.add_batch,
                        OPTIONS=self.options)
        self.router.add("/{project:word}/batch/{start:digits}_{end:digits}",
                        GET=self.get_batch,
                        OPTIONS=self.options)
        self.router.add("/{project:word}/{unit:digits}",
                        POST=self.add_unit,
                        DELETE=self.del_unit,
                        GET=self.get_unit,
                        OPTIONS=self.options)

    def close(self):
        self.conn.commit()
        self.conn.close()

    def __call__(self, environ, start_response):
        data = self.router(environ, start_response)
        data = [text.encode("utf-8") for text in data]
        return data

    @router_meth
    @json_app
    def commit(self, environ, data):
        self.conn.commit()
        return 200, {"code": 0}

    @router_meth
    @json_app
    def list_project(self, environ, data):
        try:
            cursor = self.conn.cursor()
            project_list = [project_name for (project_name,) in cursor.execute(
                "select name from sqlite_master "
                "where type=\"table\" order by name;"
            )]
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0, "object": project_list}

    @router_meth
    @json_app
    def add_project(self, environ, project, data):
        try:
            self.conn.execute(
                "create table %s (%s);" % (project, ",".join([
                    " ".join(info) for info in zip(self.attr_list, self.attr_defs)
                ]))
            )
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0}

    @router_meth
    @json_app
    def del_project(self, environ, project, data):
        try:
            self.conn.execute(
                "drop table %s;" % project
            )
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0}

    @router_meth
    @json_app
    def get_range(self, environ, project, data):
        try:
            cursor = self.conn.cursor()
            min_id = cursor.execute("select min(id) from %s;" % project).fetchone()[0]
            max_id = cursor.execute("select max(id) from %s;" % project).fetchone()[0]
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0, "object": {"min": min_id, "max": max_id}}

    @router_meth
    @json_app
    def add_batch(self, environ, project, data):
        try:
            self.conn.executemany(
                "replace into %s values (%s);"
                % (project, ",".join([":" + attr for attr in self.attr_list])),
                data
            )
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0}

    @router_meth
    @json_app
    def get_batch(self, environ, project, start, end, data):
        try:
            cursor = self.conn.cursor()
            rows = [dict(row) for row in
                    cursor.execute("select * from %s where id>=? and id<=?;" % project,
                                   (int(start), int(end)))]
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        return 200, {"code": 0, "object": rows}

    @router_meth
    @json_app
    def add_unit(self, environ, project, unit, data):
        data.setdefault("id", unit)
        columns = list()
        for key in data.keys():
            if key in self.attr_set:
                columns.append(":" + key)
        try:
            self.conn.execute(
                "replace into %s values (%s);"
                % (project, ",".join(columns)),
                data
            )
        except sql.Error as e:
            return 400, {"code": 2, "error": str(e)}
        return 200, {"code": 0}

    @router_meth
    @json_app
    def del_unit(self, environ, project, unit, data):
        try:
            self.conn.execute(
                "delete from %s where id=?;" % project, (int(unit),)
            )
        except sql.Error as e:
            return 400, {"code": 2, "error": str(e)}
        return 200, {"code": 0}

    @router_meth
    @json_app
    def get_unit(self, environ, project, unit, data):
        try:
            cursor = self.conn.cursor()
            result = cursor.execute("select * from %s where id=?;" % project, (int(unit),))
            if result:
                result = dict(result.fetchone())
        except sql.Error as e:
            return 400, {"code": 1, "error": str(e)}
        if not result:
            return 404, {"code": 2}
        else:
            return 200, {"code": 0, "object": result}


def main():
    server = TCServer("tc_data.db")
    with make_server("", 8000, server) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            server.close()


if __name__ == "__main__":
    main()
