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

With --firmware (spec 006), Studio offers an invented firmware update, and a
test page on port 80 of the address starts it. Port 80 needs root, so run it
where that is harmless, e.g. in the SDK container:

    sudo python3 -I tests/demo/household.py 192.168.1.18 --firmware

The update follows azzurro's observations (spec 006, Facts): the check says
inProgress while the player still answers, then the player is silent, then
/SyncStatus answers UpgradeStatusStage1 and 2, then it is back with the new
version. The page is our own and looks nothing like a player's. Any /upgrade
request other than upgrade=check is answered 400 and logged as REFUSED, since
Swell must never send one.
Not packaged; for device checks only.
"""

import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlsplit

NAMES = ["Living Room", "Kitchen", "Studio"]
PORTS = [11010, 11020, 11030]
STUDIO = 2
OLD, NEW = "9.9.1", "9.9.2"
# The update's phases on Studio, in seconds.
PHASES = [("download", 10), ("silent", 10), ("stage1", 8), ("stage2", 8)]
TRACKS = [("Undertow", "Harbour Lights", "Low Tide Sessions"),
          ("Morning Radio", "Invented FM", ""),
          ("Cirrus", "Weather Report Band", "Skies")]


class House:
    def __init__(self, host, refused=(), self_master=False):
        self.host = host
        self.refused = set(refused)
        self.self_master = self_master
        self.lock = threading.Condition()
        self.leader = [-1, -1, -1]
        self.volume = [30, 30, 30]
        self.mute = [False, False, False]
        self.state = ["play", "pause", "stop"]
        self.sync_gen = [1, 1, 1]
        self.status_gen = [1, 1, 1]
        self.version = [OLD, OLD, OLD]
        self.offered = False      # Studio offers NEW (set by --firmware)
        self.phase = None         # a name from PHASES while updating
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
        elif self.members(i) and self.self_master:
            inner += '<master port="%d">%s</master>' % (PORTS[i], self.host)
        for m in self.members(i):
            inner += '<slave port="%d" id="%s"/>' % (PORTS[m], self.host)
        lead = self.leader[i] if self.leader[i] >= 0 else (i if self.members(i) else -1)
        group = "%s + %d" % (NAMES[lead], len(self.members(lead))) if lead >= 0 else ""
        return ('name="%s" model="DEMO" modelName="Demo player" brand="Demo" volume="%d" mute="%d" '
                'group="%s" id="%s:%d" mac="02:00:00:00:00:%02X" version="%s">%s'
                % (NAMES[i], self.volume[i], self.mute[i], group, self.host, PORTS[i], i + 1,
                   self.version[i], inner))

    def upgrade_check(self, i):
        available = i == STUDIO and self.offered and self.phase is None
        return ('<upgrade inProgress="%s"%s available="%s">check</upgrade>'
                % ("true" if i == STUDIO and self.phase else "false",
                   ' version="%s"' % NEW if available else "", "true" if available else "false"))

    def run_update(self):
        """The update on Studio, in a thread of its own."""
        for name, secs in PHASES:
            with self.lock:
                self.phase = name
                self.lock.notify_all()
            sys.stderr.write("Studio firmware: %s\n" % name)
            time.sleep(secs)
        with self.lock:
            self.phase = None
            self.offered = False
            self.version[STUDIO] = NEW
            self.publish()
        sys.stderr.write("Studio firmware: done, %s\n" % NEW)

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

        def targets(self, q):
            hosts = q.get("slaves", q.get("slave", [""]))[0].split(",")
            ports = q.get("ports", q.get("port", ["11000"]))[0].split(",")
            return [house.index(h, int(p)) for h, p in zip(hosts, ports) if p.isdigit()]

        def updating(self):
            return house.phase if i == STUDIO else None

        def while_updating(self, path):
            """Answers as Studio does while updating; False when it is not."""
            phase = self.updating()
            if phase in (None, "download"):
                return False
            if phase == "silent":
                self.close_connection = True   # restarting: no answer at all
            elif path == "/SyncStatus":
                self.reply('<UpgradeStatusStage%s name="%s" model="DEMO"/>' % (phase[-1], NAMES[i]))
            else:
                self.reply("<html>Upgrading</html>", 503)
            return True

        def held(self, query, doc):
            """A long poll: wait while the etag is unchanged, up to the timeout,
            or until Studio's update moves past the download."""
            etag = query.get("etag", [None])[0]
            timeout = min(int(query.get("timeout", ["0"])[0] or 0), 300)
            with house.lock:
                if etag:
                    house.lock.wait_for(lambda: ('etag="%s"' % etag) not in doc()
                                        or self.updating() not in (None, "download"), timeout)
                return doc()

        def do_GET(self):
            url = urlsplit(self.path)
            q = parse_qs(url.query)
            path = url.path
            if path == "/upgrade" and self.updating() != "silent":
                if q.get("upgrade") != ["check"] or len(q) != 1:
                    sys.stderr.write("%s REFUSED /upgrade?%s\n" % (NAMES[i], url.query))
                    return self.reply("<error>refused by the test player</error>", 400)
                with house.lock:
                    return self.reply(house.upgrade_check(i))
            if self.while_updating(path):
                return
            if path in ("/Status", "/SyncStatus"):
                doc = self.held(q, lambda: house.status_doc(i) if path == "/Status" else house.sync_doc(i))
                if not self.while_updating(path):
                    self.reply(doc)
                return
            with house.lock:
                if path == "/AddSlave":
                    added = ""
                    for j in self.targets(q):
                        # Refused players, leaders and our own primary stay out.
                        if j >= 0 and j != i and j not in house.refused and not house.members(j) \
                                and house.leader[i] != j:
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


def page_handler(house):
    """The test update page on port 80, which the app opens in its web view."""
    class Page(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            sys.stderr.write("page %s\n" % (fmt % args))

        def do_GET(self):
            url = urlsplit(self.path)
            if url.path == "/upgrade/start":
                with house.lock:
                    start = house.offered and house.phase is None
                if start:
                    threading.Thread(target=house.run_update, daemon=True).start()
                self.send_response(303)
                self.send_header("Location", "/upgrade?noheader=1")
                self.end_headers()
                return
            if url.path != "/upgrade":
                self.send_error(404)
                return
            with house.lock:
                if house.phase:
                    text, action = "Updating Studio: %s." % house.phase, ""
                elif house.offered:
                    text = "Studio (test player) runs %s; %s is offered." % (house.version[STUDIO], NEW)
                    action = '<p><a href="/upgrade/start">Start the test update</a></p>'
                else:
                    text, action = "Studio (test player) runs %s. Nothing offered." % house.version[STUDIO], ""
            body = ('<!doctype html><html><head><meta charset="utf-8">'
                    '<meta name="viewport" content="width=device-width">'
                    '<meta http-equiv="refresh" content="2"><title>Swell test player</title></head>'
                    '<body style="font-family:sans-serif;padding:1em"><h1>Swell test player</h1>'
                    '<p>%s</p>%s<p><small>Invented firmware, spec 006 device check.</small></p>'
                    '</body></html>' % (text, action)).encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

    return Page


def main():
    args = sys.argv[1:]
    refused = [int(a.split("=")[1]) for a in args if a.startswith("--refuse=")]
    self_master = "--self-master" in args
    firmware = "--firmware" in args
    rest = [a for a in args if not a.startswith("--")]
    if len(rest) != 1:
        sys.exit("usage: household.py ADDRESS [--refuse=N] [--self-master] [--firmware]\n"
                 "  ADDRESS        the address the phone reaches this machine on\n"
                 "  --refuse=N     /AddSlave silently leaves player N (0-2) out\n"
                 "  --self-master  a primary also names itself in <master> (API 2.2 example)\n"
                 "  --firmware     Studio offers an update; test page on port 80 (needs root)")
    house = House(rest[0], refused, self_master)
    house.offered = firmware
    servers = []
    if firmware:
        page = ThreadingHTTPServer((rest[0], 80), page_handler(house))
        page.daemon_threads = True
        threading.Thread(target=page.serve_forever, daemon=True).start()
        servers.append(page)
        print("Update page on http://%s/upgrade" % rest[0])
    for i, port in enumerate(PORTS):
        server = ThreadingHTTPServer((rest[0], port), handler_for(house, i))
        server.daemon_threads = True
        threading.Thread(target=server.serve_forever, daemon=True).start()
        servers.append(server)
        print("%s on %s:%d" % (NAMES[i], rest[0], port))
    print("Ctrl-C stops all three.")
    try:
        threading.Event().wait()
    except KeyboardInterrupt:
        pass
    for server in servers:
        server.shutdown()


if __name__ == "__main__":
    main()
