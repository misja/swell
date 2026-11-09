<p><img src="icons/harbour-swell.svg" alt="" width="96"></p>

# Swell

A Sailfish OS controller for BluOS players.

Swell finds the BluOS players on your network and lets you control them
from your phone:

- See what's playing, with cover art, and control playback and volume
- Play presets and switch inputs
- View and edit the play queue
- Browse your library, radio and streaming services, and search
- Group players, with group volume and each player's own volume
- Control playback from the cover and the lock screen
- See when a player has a firmware update, and install it on the player's
  own update page

Swell works with any player that runs BluOS. You can also add a player by
its address.

## Building

Use the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/):
open `harbour-swell.pro` in Qt Creator, or run `sfdk build` in the
repository.

## Privacy

Swell connects only to your players and to the servers their artwork comes
from. A player's update page is the player's own, and loads what the player
puts on it. There is no account, and no data is collected or shared.

## Translations

Swell is available in English and Dutch. Translations are in
`translations/`; contributions are welcome.

## License

MIT. See [LICENSE](LICENSE).

Not affiliated with or endorsed by Lenbrook Industries. BluOS is a
trademark of Lenbrook Industries Limited.
