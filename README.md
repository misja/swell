<p><img src="icons/harbour-swell.svg" alt="" width="96"></p>

# Swell

A Sailfish OS controller for BluOS players.

- See what's playing, with cover art, and control playback and volume
- Play presets and switch inputs
- View and edit the play queue
- Browse your library, radio and streaming services, and search
- Group players, with group volume and each player's own volume
- Control playback from the cover and the lock screen
- Update a player's firmware

Players are found automatically, or added by their address.

<p>
<img src="screenshots/now-playing.png" alt="Now Playing" width="200">
<img src="screenshots/home.png" alt="Home: presets and inputs" width="200">
</p>

## Installing

Download the RPM for your phone from
[Releases](https://github.com/misja/swell/releases): aarch64 for most current
devices, armv7hl for older 32-bit ones.

## Building

Use the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/):
open `harbour-swell.pro` in Qt Creator, or run `sfdk build` in the
repository.

## Privacy

Swell connects only to your players and to the servers their artwork comes
from. There is no account, and no data is collected or shared.

## Translations

Swell is available in English and Dutch. Translations are in
`translations/`.

## License

MIT. See [LICENSE](LICENSE).

Not affiliated with or endorsed by Lenbrook Industries. BluOS is a
trademark of Lenbrook Industries Limited.
