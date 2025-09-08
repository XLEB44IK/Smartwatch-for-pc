# Smartwatch-for-pc

ESP8266-based smart watch firmware with multiple sensors and Telegram integration.

What is included

- Firmware source in `src/`.
- Example private config: `src/config_private_example.h` (copy to `src/config_private.h` and fill your keys).
- Optional local backup template: `src/config_local_backup.h` (already provided as a template).
- Uses PlatformIO (see `platformio.ini`).

Quick start

1. Install PlatformIO (VS Code extension) or PlatformIO CLI.
2. Copy `src/config_private_example.h` to `src/config_private.h` and set your real values:
   - `WeatherApiKey` (OpenWeatherMap)
   - `Cityid` or coordinates (`CityLat`/`CityLon`)
   - `BOT_TOKEN` and `CHAT_ID`(s)
     Note: `src/config_private.h` is ignored by git and will not be committed.
3. (Optional) If you prefer a local backup file, copy `src/config_local_backup.h` to `src/config_local_backup.h` and fill it.
   That file is also ignored by git and kept strictly local.
4. Build and upload to your ESP8266 board from PlatformIO.

## CI / GitHub

This repository includes a simple GitHub Actions workflow that builds the project on push and pull requests to `master`.
The workflow file is `.github/workflows/ci.yml` and runs `pio run -e esp8266` to verify the project builds.

Security notes

- Never commit real API keys, tokens or chat IDs to the repository. Use `src/config_private.h` or `src/config_local_backup.h` locally.
- `.gitignore` has entries to exclude these local config files and PlatformIO build artifacts.

Dependencies / Libraries

- See `platformio.ini` for the list of libraries required by the project.

License
This project is released under the MIT License (see `LICENSE`).
