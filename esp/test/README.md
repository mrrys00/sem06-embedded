# Development notes
## ESP environment in VSCode
Tutorial pages: https://github.com/espressif/vscode-esp-idf-extension/tree/master/docs/tutorial
- If there are problems with finding USB port, meddle with `BOOT` and `RESET` buttons
- If port is found, but unable to access (usually while flashing):
    - [for Linux] `sudo usermod -a -G dialout $USER` and log out to give yourself permission to manage the port
    - While trying to connect, do following sequence (after that, try retrying to access the port):
        1. Hold `BOOT`
        1. Hold `RESET`
        1. Release `RESET`
        1. Release `BOOT`

## ESP audio stuff
In VSCode go **View** -> **Command palette** and run `ESP-ADF: Install ESP-ADF`
- If git was not found, find the git's path (for Linux use `which git` in terminal) and put it into `idf.gitPath` setting in Espressif IDF extension settings.
