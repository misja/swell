#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Three invented BluOS players on one address, for checking grouping on a device.

    python3 -I tests/demo/household.py <host>

Serves Living Room, Kitchen and Studio on ports 11010, 11020 and 11030 of the
given address until Ctrl-C. Add them in Swell as "address:port". Everything is
invented (names, tracks, MAC addresses 02:...). It follows the same behaviour
list as tests/client/fakehousehold.h (spec 005, plan.md): per-player etags, a
secondary serves its primary's /Status, long polls on /Status and /SyncStatus,
/AddSlave and /RemoveSlave in single and multiple form, and tell_slaves.

With --firmware (spec 006), Studio offers an invented firmware update, and a
test page on port 80 of the address starts it. Port 80 needs root, so run it
where that is harmless, e.g. in the SDK container:

    sudo python3 -I tests/demo/household.py <sdk-container> --firmware

The update follows azzurro's observations (spec 006, Facts): the check says
inProgress while the player still answers, then the player is silent (requests
are held until the phase ends, then dropped), then /SyncStatus answers
UpgradeStatusStage1 and 2, then it is back with the new version. Invented, as
nothing is observed there: 503 on other paths and inProgress on the check
during the stages. After binding port 80 the script drops root to the sudo
user. The page is our own and looks nothing like a player's. Any /upgrade
request other than upgrade=check is answered 400 and logged as REFUSED, since
Swell must never send one.
With --showcase, the players carry invented content for screenshots: albums
and stations with art drawn by this script, presets, inputs, a library and a
play queue. Living Room plays an album with Kitchen in its group; Studio has
a radio station paused.

Not packaged; for device checks and screenshots only.
"""

import os
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

# --showcase: invented albums, stations and inputs for screenshots. The art is
# drawn here (pattern, two colours), so nothing comes from a real library.
ALBUMS = [
    ("Night Ferry", "Lantern Static", "rings", (24, 40, 82), (236, 168, 64),
     ["Departure Bell", "Deck Lights", "Crossing", "Fog Signal", "Car Deck", "Arrival"]),
    ("Low Tide Sessions", "Harbour Lights", "waves", (18, 92, 110), (230, 222, 196),
     ["Undertow", "Slack Water", "Ebb", "Mooring", "Sand Bar"]),
    ("Long Summer Field", "Copper Meadow", "sun", (214, 120, 52), (250, 214, 120),
     ["Haymaking", "Dust Road", "Late Light", "Crickets"]),
    ("Pale Fruit", "Glass Orchard", "dots", (232, 226, 214), (196, 72, 84),
     ["Windfall", "Blossom", "Cider House", "Frost"]),
    ("Strings for Winter", "Northbound Quartet", "stripes", (40, 44, 52), (180, 196, 214),
     ["Allegro", "Snowline", "Adagio", "Thaw"]),
    ("Orbit Songs", "Paper Satellites", "orbit", (12, 14, 30), (120, 200, 230),
     ["Launch Window", "Apogee", "Static Moon", "Re-entry"]),
    ("Skies", "Cirrus Ensemble", "gradient", (90, 140, 210), (240, 200, 210),
     ["Cirrus", "Nimbus", "Altostratus"]),
    ("Brick and Ivy", "The Quiet Terraces", "grid", (120, 52, 40), (90, 140, 80),
     ["Back Garden", "Chimney Pots", "Ivy", "Sunday"]),
]
STATIONS = [("Invented FM", "gradient", (200, 60, 70), (250, 170, 90)),
            ("Lantern Jazz", "rings", (30, 30, 30), (230, 190, 90)),
            ("Kelp Classical", "waves", (20, 80, 60), (200, 230, 210)),
            ("Dune Radio", "sun", (190, 140, 80), (250, 230, 180))]
INPUTS = [("Optical", "spdif", "Source1"), ("Turntable", "phono", "Source2"),
          ("Bluetooth", "bluetooth", "Source3"), ("HDMI ARC", "arc", "Source4")]
SHOWCASE_PLAYING = 0      # Night Ferry plays in Living Room (Kitchen joins it)
SHOWCASE_SONG = 2         # "Crossing"


def png(size, pixel):
    """An RGBA PNG of size x size, pixel(x, y) -> (r, g, b, a) in 0..255."""
    import struct
    import zlib
    rows = bytearray()
    for y in range(size):
        rows.append(0)
        for x in range(size):
            rows.extend(pixel(x, y))

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xffffffff))
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(bytes(rows), 6)) + chunk(b"IEND", b""))


def art(pattern, a, b, size=540):
    """Album or station art; 2x2 samples per pixel keep the edges smooth on
    Now Playing, where the image fills the width of the screen."""
    import math

    def shade(u, v):
        d = math.hypot(u - 0.5, v - 0.5)
        if pattern == "rings":
            return 1.0 if int(d * 14) % 2 and d < 0.42 else 0.15 * v
        if pattern == "waves":
            return 1.0 if (v * 9 + 0.6 * math.sin(u * 12)) % 2 < 0.35 else 0.0
        if pattern == "sun":
            return 1.0 if math.hypot(u - 0.5, v - 0.62) < 0.24 else 0.35 * (1 - v)
        if pattern == "dots":
            return 1.0 if math.hypot(u * 6 % 1 - 0.5, v * 6 % 1 - 0.5) < 0.22 else 0.0
        if pattern == "stripes":
            return 1.0 if (u + v) * 10 % 2 < 0.25 else 0.0
        if pattern == "orbit":
            r = math.hypot(u - 0.5, (v - 0.5) * 2.4)
            return 1.0 if abs(r - 0.36) < 0.012 or d < 0.09 else 0.0
        if pattern == "grid":
            return 0.0 if (u * 8 % 1 < 0.9 and v * 15 % 1 < 0.85) else 1.0
        return v * 0.8 + 0.2 * u

    def pixel(x, y):
        t = sum(shade((x + dx) / size, (y + dy) / size) for dx in (0.25, 0.75) for dy in (0.25, 0.75)) / 4
        return tuple(int(a[k] + (b[k] - a[k]) * t) for k in range(3)) + (255,)
    return png(size, pixel)


# The Bluetooth rune as line segments, in icon coordinates (-0.5..0.5).
RUNE = [(-0.02, -0.36, -0.02, 0.36), (-0.02, -0.36, 0.17, -0.18), (0.17, -0.18, -0.2, 0.17),
        (-0.02, 0.36, 0.17, 0.18), (0.17, 0.18, -0.2, -0.17)]


def segment(px, py, ax, ay, bx, by):
    """Distance from a point to a line segment."""
    import math
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def icon(kind, size=96):
    """A white line icon on transparent, as players give for their inputs."""
    import math

    def pixel(x, y):
        u, v = x / size - 0.5, y / size - 0.5
        d = math.hypot(u, v)
        on = {"spdif": abs(d - 0.3) < 0.04 or d < 0.07,
              "phono": abs(d - 0.36) < 0.035 or abs(d - 0.18) < 0.03 or d < 0.04,
              "bluetooth": any(segment(u, v, *line) < 0.035 for line in RUNE),
              "arc": abs(v) < 0.22 and abs(u) < 0.38 and (abs(abs(v) - 0.22) < 0.04 or abs(abs(u) - 0.38) < 0.04)
              }.get(kind, d < 0.3)
        return (255, 255, 255, 255 if on else 0)
    return png(size, pixel)


class House:
    def __init__(self, host, refused=(), self_master=False, showcase=False):
        self.host = host
        self.showcase = showcase
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
        if showcase:
            self.leader[1] = 0        # Kitchen plays along with Living Room
            self.state = ["play", "play", "pause"]
            self.volume = [32, 24, 18]
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
        if self.showcase:
            return self.showcase_status(i)
        t1, t2, t3 = TRACKS[i]
        return ("<state>%s</state><title1>%s</title1><title2>%s</title2><title3>%s</title3>"
                "<secs>42</secs><totlen>240</totlen><volume>%d</volume><syncStat>s%d-%d</syncStat>"
                % (self.state[i], t1, t2, t3, self.volume[i], i, self.sync_gen[i]))

    def showcase_status(self, i):
        common = ("<state>%s</state><volume>%d</volume><syncStat>s%d-%d</syncStat><prid>1</prid>"
                  % (self.state[i], self.volume[i], i, self.sync_gen[i]))
        if i == STUDIO:
            name = STATIONS[0][0]
            return (common + "<title1>%s</title1><title2>Late Show</title2><title3>%s</title3>"
                    "<image>/Artwork?station=0</image><streamUrl>Radio:invented</streamUrl>"
                    "<secs>0</secs><totlen>0</totlen>" % (name, name))
        album, artist = ALBUMS[SHOWCASE_PLAYING][:2]
        track = ALBUMS[SHOWCASE_PLAYING][5][SHOWCASE_SONG]
        return (common + "<title1>%s</title1><title2>%s</title2><title3>%s</title3>"
                "<image>/Artwork?album=%d</image><pid>%d</pid><song>%d</song>"
                "<secs>134</secs><totlen>%d</totlen>"
                % (track, artist, album, SHOWCASE_PLAYING, 7, SHOWCASE_SONG,
                   duration(SHOWCASE_PLAYING, SHOWCASE_SONG)))

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


def duration(album, track):
    return 150 + (album * 37 + track * 53) % 170


def esc(text):
    return text.replace("&", "&amp;").replace('"', "&quot;").replace("<", "&lt;")


def showcase_queue():
    """The play queue: Night Ferry, then Low Tide Sessions."""
    songs = [(a, t) for a in (SHOWCASE_PLAYING, 1) for t in range(len(ALBUMS[a][5]))]
    body = ""
    for pos, (a, t) in enumerate(songs):
        album, artist = ALBUMS[a][:2]
        body += ('<song id="%d" songid="Demo:%d-%d"><title>%s</title><art>%s</art><alb>%s</alb>'
                 "<fn>Demo:%d-%d</fn><time>%d</time><image>/Artwork?album=%d</image></song>"
                 % (pos, a, t, esc(ALBUMS[a][5][t]), esc(artist), esc(album), a, t, duration(a, t), a))
    return '<playlist name="Queue" modified="0" length="%d" id="7">%s</playlist>' % (len(songs), body)


def showcase_browse(key):
    """Library > albums > tracks, Radio > stations, and the inputs."""
    if key == "":
        items = ('<item browseKey="Demo:Library" text="Library" type="link"/>'
                 '<item browseKey="Demo:Radio" text="Radio" type="link"/>')
        for name, kind, source in INPUTS:
            items += ('<item playURL="/Play?url=Capture%%3Ademo%%3Fid%%3D%s" text="%s" type="audio" '
                      'inputType="%s" image="/images/%s.png"/>' % (source, name, kind, kind))
        return '<browse type="menu">%s</browse>' % items
    if key == "Demo:Library":
        items = "".join('<item browseKey="Demo:Album/%d" playURL="/Add?playnow=1&amp;album=%d" '
                        'text="%s" text2="%s" image="/Artwork?album=%d" type="album"/>'
                        % (n, n, esc(a[0]), esc(a[1]), n) for n, a in enumerate(ALBUMS))
        return '<browse serviceName="Library" type="albums">%s</browse>' % items
    if key.startswith("Demo:Album/") and key[11:].isdigit() and int(key[11:]) < len(ALBUMS):
        n = int(key[11:])
        items = "".join('<item playURL="/Add?playnow=1&amp;album=%d&amp;track=%d" text="%d. %s" '
                        'duration="%d" type="track"/>' % (n, t, t + 1, esc(title), duration(n, t))
                        for t, title in enumerate(ALBUMS[n][5]))
        return ('<browse serviceName="Library" type="tracks" playURL="/Add?playnow=1&amp;album=%d">%s</browse>'
                % (n, items))
    if key == "Demo:Radio":
        items = "".join('<item playURL="/Play?url=Radio%%3A%d" text="%s" image="/Artwork?station=%d" '
                        'type="audio"/>' % (n, esc(s[0]), n) for n, s in enumerate(STATIONS))
        return '<browse serviceName="Radio" type="menu">%s</browse>' % items
    return None


ART = {}


def showcase_image(path, q):
    """Drawn once per album, station or input, then kept."""
    key = (path, tuple(sorted((k, v[0]) for k, v in q.items())))
    if key not in ART:
        if path == "/Artwork" and q.get("album", [""])[0].isdigit() and int(q["album"][0]) < len(ALBUMS):
            a = ALBUMS[int(q["album"][0])]
            ART[key] = art(a[2], a[3], a[4])
        elif path == "/Artwork" and q.get("station", [""])[0].isdigit() and int(q["station"][0]) < len(STATIONS):
            s = STATIONS[int(q["station"][0])]
            ART[key] = art(s[1], s[2], s[3])
        elif path.startswith("/images/") and path[8:-4] in [k for _, k, _ in INPUTS]:
            ART[key] = icon(path[8:-4])
        else:
            return None
    return ART[key]


def handler_for(house, i):
    class Player(BaseHTTPRequestHandler):
        def log_message(self, fmt, *args):
            sys.stderr.write("%s %r\n" % (NAMES[i], fmt % args))

        def image(self, data):
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def showcase(self, path, q):
            """Answers for --showcase; False when the path is not one of them."""
            if path == "/Artwork" or path.startswith("/images/"):
                data = showcase_image(path, q)
                if data is None:
                    return False
                self.image(data)
                return True
            if path == "/Presets":
                body = "".join('<preset id="%d" name="%s" url="Radio:%d" image="/Artwork?station=%d"/>'
                               % (n + 1, esc(s[0]), n, n) for n, s in enumerate(STATIONS))
                body += ('<preset id="%d" name="Turntable" url="Capture:demo?id=Source2" '
                         'image="/images/phono.png"/>' % (len(STATIONS) + 1))
                self.reply('<presets prid="1">%s</presets>' % body)
                return True
            if path == "/RadioBrowse" and q.get("service") == ["Capture"]:
                items = "".join('<item playerName="%s" text="%s" inputType="%s" id="%s" '
                                'URL="Capture%%3Ademo%%3Fid%%3D%s" image="/images/%s.png" type="audio"/>'
                                % (NAMES[i], name, kind, source, source, kind)
                                for name, kind, source in INPUTS)
                self.reply('<radiotime service="Capture">%s</radiotime>' % items)
                return True
            if path == "/Playlist":
                self.reply(showcase_queue())
                return True
            if path == "/Browse":
                body = showcase_browse(q.get("key", [""])[0])
                if body is None:
                    return False
                self.reply(body)
                return True
            return False

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

        def refuse(self):
            sys.stderr.write("%s REFUSED %s %r\n" % (NAMES[i], self.command, self.path))
            self.reply("<error>refused by the test player</error>", 400)

        def do_POST(self):
            self.refuse()

        def updating(self):
            return house.phase if i == STUDIO else None

        def while_updating(self, path):
            """Answers as Studio does while updating; False when it is not."""
            phase = self.updating()
            if phase in (None, "download"):
                return False
            if phase == "silent":
                # Restarting: no answer at all, until the restart is over.
                with house.lock:
                    house.lock.wait_for(lambda: house.phase != "silent")
                self.close_connection = True
            elif path == "/SyncStatus":
                self.reply('<UpgradeStatusStage%s name="%s" model="DEMO"/>' % (phase[-1], NAMES[i]))
            else:
                self.reply("<html>Upgrading</html>", 503)
            return True

        def held(self, query, doc):
            """A long poll: wait while the etag is unchanged, up to the timeout,
            or until Studio's update moves past the download."""
            etag = query.get("etag", [None])[0]
            raw = query.get("timeout", ["0"])[0]
            timeout = min(int(raw), 300) if raw.isdigit() else 0
            with house.lock:
                if etag:
                    house.lock.wait_for(lambda: ('etag="%s"' % etag) not in doc()
                                        or self.updating() not in (None, "download"), timeout)
                return doc()

        def do_GET(self):
            url = urlsplit(self.path)
            q = parse_qs(url.query)
            path = url.path
            if path == "/upgrade" and (q.get("upgrade") != ["check"] or len(q) != 1):
                return self.refuse()
            if path == "/upgrade" and self.updating() != "silent":
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
            if house.showcase and self.showcase(path, q):
                return
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
            sys.stderr.write("page %r\n" % (fmt % args))

        def do_GET(self):
            url = urlsplit(self.path)
            if url.path == "/upgrade/start":
                with house.lock:
                    start = house.offered and house.phase is None
                    if start:
                        house.phase = "download"   # one update only, however often tapped
                if not start:
                    sys.stderr.write("page REFUSED a second start\n")
                else:
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
    showcase = "--showcase" in args
    rest = [a for a in args if not a.startswith("--")]
    if len(rest) != 1:
        sys.exit("usage: household.py ADDRESS [--refuse=N] [--self-master] [--firmware] [--showcase]\n"
                 "  ADDRESS        the address the phone reaches this machine on\n"
                 "  --refuse=N     /AddSlave silently leaves player N (0-2) out\n"
                 "  --self-master  a primary also names itself in <master> (API 2.2 example)\n"
                 "  --firmware     Studio offers an update; test page on port 80 (needs root)\n"
                 "  --showcase     invented albums, stations, inputs and a queue, for screenshots")
    house = House(rest[0], refused, self_master, showcase)
    house.offered = firmware
    servers = []
    if firmware:
        page = ThreadingHTTPServer((rest[0], 80), page_handler(house))
        page.daemon_threads = True
        threading.Thread(target=page.serve_forever, daemon=True).start()
        servers.append(page)
        print("Update page on http://%s/upgrade" % rest[0])
    # Port 80 is bound; nothing else needs root.
    if os.getuid() == 0 and "SUDO_UID" in os.environ:
        os.setgid(int(os.environ["SUDO_GID"]))
        os.setuid(int(os.environ["SUDO_UID"]))
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
