# Sumbre NodeMCU Web Server

This project serves a local website from your NodeMCU using LittleFS.

## Uploading Filesystem Data

To make your HTML, CSS, and other files available to the web server, place them in the `/data` folder.  
You must upload these files to the device's filesystem using PlatformIO:

```sh
pio run --target uploadfs
```

This command builds a filesystem image from the contents of `/data` and uploads it to the ESP8266's flash memory using LittleFS.  
After uploading, your web server code can access and serve these files to clients.

## Typical Workflow

1. Place your web files (e.g., `index.html`, `style.css`) in `/data`.
2. Upload your firmware:
   ```sh
   pio run --target upload
   ```
3. Upload your filesystem:
   ```sh
   pio run --target uploadfs
   ```
4. Connect to the NodeMCU hotspot and visit its IP address in your browser.

## How It Works

- The ESP8266 runs a web server and serves files stored in LittleFS.
- When a client requests a file (e.g., `/index.html`), the server reads it from LittleFS and sends it to the browser.
- The `/data` folder is used to stage files for upload to the device's filesystem.

