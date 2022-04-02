# Development notes
## ESP environment in VSCode
Tutorial pages: https://github.com/espressif/vscode-esp-idf-extension/tree/master/docs/tutorial
- While flashing built program, when `Connecting......`, do following sequence:
    1. Hold `BOOT`
    1. Press `RESET`
    1. Release `BOOT`
- If there are problems with finding USB port while setting it, meddle with `BOOT` and `RESET` buttons
- If port is found, but unable to access (usually while flashing):
    - [for Linux] `sudo usermod -a -G dialout $USER` and log out to give yourself permission to manage the port

## ESP audio stuff
In VSCode go **View** -> **Command palette** and run `ESP-ADF: Install ESP-ADF`
- If git was not found, find the git's path (for Linux use `which git` in terminal) and put it into `idf.gitPath` setting in Espressif IDF extension settings.


# Known issues
### printf stack overflow
Following code will print what it should just once, and after that will raise a stack overflow exception:
```c
int x = 0;
while(1) {
    printf("Number: %d", x);
    vTaskDelay(1000);
}
```
However, `printf("Number: ");` works perfectly

### UART read
Able to send UART messages to PUTTY, but unable to receive any back.
