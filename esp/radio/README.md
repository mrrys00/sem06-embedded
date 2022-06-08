# ESP Lyra-T Radio
## Usage
1. Prepare a MicroSD card with a config (according to **Config** section below)
1. Insert the card into ESP-32's slot
1. Run the software (or press `RESET` if it's already running)
1. After a few seconds press `PLAY`
1. Radio should be available at the headphone jack socket. If not, verify the config and try again
1. To change the station, just repeat these steps from the beginning

## Config
To set a station, use a MicroSD card with `radio.cfg` file on it. Only the first line will be read. It chould have a single URL of an `MP3` or `AAC` radio station. Type of the stream will be read from URL's extension. If you want to manually choose the format, append `#mp3` or `#aac` to the URL.

## Station examples
Folowing stations are tested to be compatible with this software.

- `http://radio.canstream.co.uk:8083/live.aac` ***(PL)***
- `http://stream4.nadaje.com:11986/prs#mp3` ***(EN)***
- `http://live-icy.dr.dk/A/A05H.mp3` ***(DK)***
- `http://mcrscast.mcr.iol.pt/comercial.mp3` ***(PT)***
- `http://icecast.rtl.fr/rtl-1-44-128#mp3` ***(FR)***
- `http://stream.live.vc.bbcmedia.co.uk/bbc_radio_cymru#mp3` ***(CYM [Welsh])***

Many stations are unable to work with it, mostly due to the fact that is doesn't support SSL certificates, which renders `https://...` streams impossible to use.
