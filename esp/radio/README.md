# Radio
## Config
To set a station, use a MicroSC card with `radio.cfg` file on it. Only the first line will be read. It chould have a single URL of an `MP3` or `AAC` radio station. Type of the stream will be read from URL's extension. If you want to manually choose the format, append `#mp3` or `#aac` to the URL.

## Station examples
Folowing stations are tested to be compatible with this software.
```
http://radio.canstream.co.uk:8083/live.aac
http://stream4.nadaje.com:11986/prs#mp3
```
Many stations are unable to work with it, mostly due to the fact that is doesn't support SSL certificates, which renders `https://...` streams impossible to use.
