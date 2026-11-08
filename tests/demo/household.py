#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Three invented BluOS players on one address, for checking grouping on a device.

    python3 -I tests/demo/household.py 192.168.1.2

Serves Living Room, Kitchen and Studio on ports 11010, 11020 and 11030 of the
given address until Ctrl-C. Add them in Swell as "address:port". Everything is
invented (names, tracks, MAC addresses 02:...). It follows the same behaviour
list as tests/client/fakehousehold.h (spec 005, plan.md): per-player etags, a
secondary serves its primary's /Status, long polls on /Status and /SyncStatus,
/AddSlave and /RemoveSlave in single and multiple form, and tell_slaves.
Not packaged; for device checks only.
"""

import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit

NAMES = ["Living Room", "Kitchen", "Studio"]
PORTS = [11010, 11020, 11030]
TRACKS = [("Undertow", "Harbour Lights", "Low Tide Sessions"),
          ("Morning Radio", "Invented FM", ""),
          ("Cirrus", "Weather Report Band", "Skies")]


class House:
    def __init__(self, host):
        self.host = host
        self.lock = threading.Condition()
        self.leader = [-1, -1, -1]
        self.volume = [30, 30, 30]
        self.mute = [False, False, False]
        self.state = ["play", "pause", "stop"]
        self.sync_gen = [1, 1, 1]
        self.status_gen = [1, 1, 1]
        self._sync = [self.sync_body(i) for i in range(3)]
        self._status = [self.status_body(i) for i in range(3)]

    def index(self, host, port):
        if host != self.host and host not in ("127.0.0.1", "localhost"):
            return -1
        return PORTS.index(port) if port in PORTS else -1

    def members(self, lead):
        return [i for i in range(3) if self.leader[i] == lead]

    def sync_body(self, i):
        inner = ""
        if self.leader[i] >= 0:
            inner += '<master port="%d" reconnecting="false">%s</master>' % (PORTS[self.leader[i]], self.host)
        for m in self.members(i):
            inner += '<slave port="%d" id="%s"/>' % (PORTS[m], self.host)
        lead = self.leader[i] if self.leader[i] >= 0 else (i if self.members(i) else -1)
        group = "%s + %d" % (NAMES[lead], len(self.members(lead))) if lead >= 0 else ""
        return ('name="%s" model="DEMO" modelName="Demo player" brand="Demo" volume="%d" mute="%d" '
                'group="%s" id="%s:%d" mac="02:00:00:00:00:%02X">%s'
                % (NAMES[i], self.volume[i], self.mute[i], group, self.host, PORTS[i], i + 1, inner))

    def status_body(self, i):
        t1, t2, t3 = TRACKS[i]
        return ("<state>%s</state><title1>%s</title1><title2>%s</title2><title3>%s</title3>"
                "<secs>42</secs><totlen>240</totlen><volume>%d</volume><syncStat>s%d-%d</syncStat>"
                % (self.state[i], t1, t2, t3, self.volume[i], i, self.sync_gen[i]))

    def publish(self):
        """Call with the lock held. Moves only the etags of changed documents."""
        for i in range(3):
            body = self.sync_body(i)
            if body != self._sync[i]:
                self._sync[i] = body
                self.sync_gen[i] += 1
        for i in range(3):
            body = self.status_body(i)
            if body != self._status[i]:
                self._status[i] = body
                self.status_gen[i] += 1
        self.lock.notify_all()

    def sync_doc(self, i):
        etag = "s%d-%d" % (i, self.sync_gen[i])
        return '<SyncStatus etag="%s" syncStat="%s" %s</SyncStatus>' % (etag, etag, self._sync[i])

    def status_doc(self, i):
        s = self.leader[i] if self.leader[i] >= 0 else i
        return '<status etag="p%d-%d">%s</status>' % (s, self.status_gen[s], self._status[s])


def handler_for(house, i):
    class Player(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            sys.stderr.write("%s %s\n" % (NAMES[i], fmt % args))

        def reply(self, body, code=200):
            data = body.encode()
            self.send_response(code)
            self.send_header("Content-Type", "text/xml")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def held(self, query, doc):
            """A long poll: wait while the etag is unchanged, up to the timeout."""
            etag = query.get("etag", [None])[0]
            timeout = min(int(query.get("timeout", ["0"])[0] or 0), 300)
            with house.lock:
                if etag:
                    house.lock.wait_for(lambda: ('etag="%s"' % etag) not in doc(), timeout)
                return doc()

        def targets(self, q):
            hosts = q.get("slaves", q.get("slave", [""]))[0].split(",")
            ports = q.get("ports", q.get("port", ["11000"]))[0].split(",")
            return [house.index(h, int(p)) for h, p in zip(hosts, ports) if p.isdigit()]

        def do_GET(self):
            url = urlsplit(self.path)
            q = parse_qs(url.query)
            path = url.path
            if path == "/Status":
                return self.reply(self.held(q, lambda: house.status_doc(i)))
            if path == "/SyncStatus":
                return self.reply(self.held(q, lambda: house.sync_doc(i)))
            with house.lock:
                if path == "/AddSlave":
                    added = ""
                    for j in self.targets(q):
                        if j >= 0 and j != i:
                            house.leader[j] = i
                            added += '<slave port="%d" id="%s"/>' % (PORTS[j], house.host)
                    house.publish()
                    return self.reply("<addSlave>%s</addSlave>" % added)
                if path == "/RemoveSlave":
                    for j in self.targets(q):
                        if j == i:
                            rest = house.members(i)
                            for m in rest:
                                house.leader[m] = rest[0] if len(rest) >= 2 and m != rest[0] else -1
                        elif j >= 0 and house.leader[j] == i:
                            house.leader[j] = -1
                    house.publish()
                    return self.reply(house.sync_doc(i))
                if path == "/Volume":
                    who = [i] + (house.members(i) if q.get("tell_slaves", ["0"])[0] == "1" else [])
                    for t in who:
                        if "level" in q:
                            house.volume[t] = max(0, min(100, int(q["level"][0])))
                        if "mute" in q:
                            house.mute[t] = q["mute"][0] == "1"
                    house.publish()
                    return self.reply('<volume db="-30" mute="%d">%d</volume>' % (house.mute[i], house.volume[i]))
                source = house.leader[i] if house.leader[i] >= 0 else i
                if path in ("/Play", "/Pause"):
                    house.state[source] = "play" if path == "/Play" else "pause"
                    house.publish()
                    return self.reply("<state>%s</state>" % house.state[source])
            if path in ("/Skip", "/Back"):
                return self.reply("<id>1</id>")
            if path == "/Presets":
                return self.reply('<presets prid="1"><preset id="1" name="Invented FM" url="Demo:1"/></presets>')
            if path == "/RadioBrowse":
                return self.reply('<radiotime service="Capture"></radiotime>')
            if path == "/Playlist":
                return self.reply('<playlist name="Queue" id="1" length="0"/>')
            if path == "/Browse":
                return self.reply('<browse type="menu"/>')
            self.reply("<html>Not found</html>", 404)

    return Player


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: household.py ADDRESS   (the address the phone reaches this machine on)")
    house = House(sys.argv[1])
    servers = []
    for i, port in enumerate(PORTS):
        server = ThreadingHTTPServer((sys.argv[1], port), handler_for(house, i))
        server.daemon_threads = True
        threading.Thread(target=server.serve_forever, daemon=True).start()
        servers.append(server)
        print("%s on %s:%d" % (NAMES[i], sys.argv[1], port))
    print("Ctrl-C stops all three.")
    try:
        threading.Event().wait()
    except KeyboardInterrupt:
        pass
    for server in servers:
        server.shutdown()


if __name__ == "__main__":
    main()
