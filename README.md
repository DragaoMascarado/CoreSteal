# CoreSteal

Windows research project for studying how Chromium-based browsers handle encrypted credentials and App-Bound Encryption.

CoreSteal runs an extractor inside the browser process, reads supported Chromium profile databases and sends the results back to a small launcher over a named pipe.

> **For security research and testing only.**
>
> Use this only on systems and browser profiles you own or have explicit permission to test.

## Overview

The project is split into two main parts:

- **CoreSteal.exe** — finds the selected browser, launches it, loads the extractor and receives its output.
- **Extractor.dll** — runs inside the Chromium process and handles profile discovery, database access and decryption.

The current launcher supports:

- Google Chrome
- Microsoft Edge
- Brave

The extractor contains some additional browser-specific code, but those three are the targets currently exposed by the launcher.

## How it works

At a high level:

```text
CoreSteal.exe
     |
     | starts Chromium
     | loads Extractor.dll
     v
Chromium process
     |
     |-- finds browser profiles
     |-- reads Local State
     |-- obtains the App-Bound key
     |-- opens Chromium databases
     |-- decrypts supported records
     |
     v
Named Pipe
     |
     v
CoreSteal.exe
```

The launcher handles process setup and IPC while most of the Chromium-specific work happens inside `Extractor.dll`.

## What it currently handles

The extractor has routines for the following Chromium data:

| Data | Source |
|---|---|
| Saved passwords | `Login Data` / `Login Data For Account` |
| Payment cards | `Web Data` |
| Stored CVCs | `Web Data` |
| IBANs | `Web Data` |
| Service tokens | `Web Data` |

Encrypted fields currently target Chromium's `v20` AES-GCM format.

Schemas and encryption details can change between Chromium versions, so compatibility with every release isn't expected.

## Project layout

```text
CoreSteal/
├── CoreSteal.sln
│
├── CoreSteal/
│   ├── Common.h
│   ├── injector.cpp
│   └── CoreSteal.vcxproj
│
└── Extractor/
    ├── Extractor.c
    ├── Extractor.h
    ├── Decrypt.c
    ├── Decrypt.h
    ├── Elevator.c
    ├── Elevator.h
    ├── VFS.c
    ├── VFS.h
    │
    ├── Memory/
    │   ├── Memory.c
    │   └── Memory.h
    │
    └── Libs/
        ├── cJSON.c
        ├── cJSON.h
        ├── sqlite3.h
        └── libsqlite3.lib
```

## Launcher

Most of the launcher code is in:

```text
CoreSteal/CoreSteal/injector.cpp
```

It is responsible for:

- finding the requested browser;
- starting the browser process;
- setting up the named pipe;
- loading `Extractor.dll`;
- receiving records from the extractor;
- writing the received data to local files.

## Extractor

The main extractor flow lives in:

```text
CoreSteal/Extractor/Extractor.c
```

Once loaded, the DLL determines which browser it is running under and locates the corresponding Chromium user-data directory.

From there it reads `Local State`, obtains the encrypted browser key, enumerates profiles and processes the supported SQLite databases.

Results are sent back to the launcher through the pipe instead of being handled directly by the DLL.

## App-Bound Encryption

Recent Chromium versions use App-Bound Encryption to make simply copying browser files and decrypting them from an unrelated process considerably harder.

`Elevator.c` contains the browser-specific COM definitions and code used by the project when dealing with the App-Bound key.

The interesting part from a security-research point of view is the process boundary: code already executing in the browser's context can have access to functionality that an unrelated process does not.

## Decryption

`Decrypt.c` contains the AES-GCM handling.

Windows CNG / BCrypt is used for the cryptographic operations.

The implementation currently expects Chromium `v20` encrypted records, so older formats or future changes may need separate handling.

## SQLite handling

Chromium normally keeps several of its profile databases open while the browser is running.

Instead of modifying the original files, `VFS.c` implements a small SQLite VFS which loads the database and, when available, its WAL and SHM state into memory.

Files currently accessed include:

```text
Local State
Login Data
Login Data For Account
Web Data
```

along with their associated SQLite state files when needed.

## Memory helpers

`Memory/Memory.c` and `Memory/Memory.h` contain the small allocation, copy, comparison and cleanup helpers used throughout the extractor.

Nothing especially complicated lives there; they mostly exist to keep the rest of the C code cleaner.

## Dependencies

The project uses:

- C / C++
- Win32 API
- Windows Registry API
- COM
- Windows CNG / BCrypt
- SQLite3
- cJSON
- Visual Studio / MSVC

The supplied Visual Studio projects currently use the `v142` platform toolset.

## Things defenders can look for

The project also has a few useful behaviors for EDR/testing experiments.

For example:

- a Chromium process being created in an unusual way;
- memory being written into that process;
- remote thread activity associated with loading a DLL;
- an unexpected DLL appearing inside a browser process;
- access to `Local State`, `Login Data` or `Web Data`;
- unusual interaction with browser elevator COM interfaces;
- a local named pipe connecting the browser process to another executable.

Those signals are generally more useful than relying on a single filename or static indicator.

## Testing

Use a VM.

A dedicated Chromium profile containing fake credentials is strongly recommended. Don't test this against your normal browser profile just because it's convenient.

A basic lab setup is enough:

- isolated Windows VM;
- separate browser profile;
- dummy logins and payment information;
- VM snapshot before testing.

That also makes debugging database or browser-version differences much easier.

## Limitations

A few things to keep in mind:

- Windows only.
- The public launcher currently exposes Chrome, Edge and Brave.
- The current decryption code targets Chromium `v20`.
- Chromium database schemas are not stable APIs.
- App-Bound Encryption internals can change.
- COM interface details may change between browser releases.
- This is research code, not a supported backup or migration utility.

## Third-party code

### cJSON

The repository includes cJSON for parsing JSON.

cJSON is licensed under the MIT License. If you redistribute it, keep the upstream copyright and license notice.

### SQLite

SQLite is used for reading Chromium databases and is released into the public domain by its authors.

Neither component is part of CoreSteal itself.

## License

There is currently no project license included in the repository.

Add one before publishing if you want to explicitly define how the original CoreSteal code may be used, modified or redistributed.

Third-party components keep their own licensing terms regardless of the license chosen for this project.

## Responsible use

CoreSteal deals with passwords, authentication tokens and financial information. That obviously makes it easy to cross the line between security research and unauthorized credential access.

Keep testing to systems, accounts and profiles that belong to you or that you have explicit authorization to examine.

Good uses for the project include:

- browser-security research;
- App-Bound Encryption research;
- malware analysis;
- detection engineering;
- digital-forensics labs;
- authorized penetration-testing environments.

Don't collect or publish somebody else's credentials.

## Disclaimer

This project is provided for educational and authorized security-research purposes.

You are responsible for making sure you have permission to test the system, browser profile and data involved.