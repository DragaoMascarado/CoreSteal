# CoreSteal

**A Chromium browser data dump tool.**

> **Security Research / Proof of Concept**
>
> This repository contains code that demonstrates how sensitive data stored by Chromium-based browsers can be accessed from inside a browser process, including data protected by Chromium's App-Bound Encryption mechanisms.
>
> **Use only in environments, systems, and browser profiles that you own or are explicitly authorized to test.**

## ⚠️ Important notice

CoreSteal handles highly sensitive browser data such as saved passwords, authentication tokens, payment-card information, CVC values, and IBAN records.

Because of that, this project should be treated as **security-research code**, not as a general-purpose credential backup utility. Running it against another person's profile or system without explicit authorization may be illegal and harmful.

This README intentionally focuses on the project's architecture, security implications, and defensive analysis rather than providing operational instructions for extracting real credentials.

---

## Overview

CoreSteal is a Windows C/C++ Chromium browser data dump tool and security-research proof of concept designed to study Chromium credential storage and App-Bound Encryption behavior.

At a high level, the project consists of two components:

- **CoreSteal launcher** — locates a supported Chromium browser, starts a browser process, loads the extractor component into that process, and receives extracted records through local IPC.
- **Extractor** — runs inside the browser process, identifies the browser environment, obtains the browser's encrypted master key, accesses Chromium profile databases, decrypts supported records, and sends structured results back to the launcher.

---

## What the project demonstrates

The source code demonstrates several Windows and Chromium internals that are relevant to browser-security research:

- Browser discovery through the Windows Registry and common installation paths.
- Creation of a Chromium browser process in a suspended state.
- Loading a DLL into another process.
- Local inter-process communication through a Windows named pipe.
- Browser/profile discovery under the user's Chromium data directory.
- Parsing Chromium's `Local State` JSON file.
- Handling the App-Bound encrypted browser key.
- Interaction with browser-specific COM elevator interfaces.
- AES-GCM decryption through Windows CNG / BCrypt.
- Reading Chromium SQLite databases while the browser profile may be active.
- Loading SQLite database, WAL, and SHM data into memory through a custom VFS.
- Enumerating multiple Chromium profiles.

---

## Sensitive data handled

The current implementation contains routines for processing the following categories of browser data:

| Category | Chromium data source | Example fields handled |
|---|---|---|
| Saved passwords | `Login Data` / `Login Data For Account` | URL, username, password |
| Payment cards | `Web Data` | cardholder name, expiration date, card number |
| Stored CVC | `Web Data` | card GUID and CVC |
| IBANs | `Web Data` | nickname and IBAN |
| Service tokens | `Web Data` | service, encrypted token, optional binding key |

The code currently targets Chromium's `v20` AES-GCM encrypted records.

---

## Architecture

```text
┌──────────────────────────┐
│      CoreSteal.exe       │
│        Launcher          │
└────────────┬─────────────┘
             │
             │ starts browser process
             │ and loads Extractor.dll
             ▼
┌──────────────────────────┐
│ Chromium browser process │
│                          │
│     Extractor.dll        │
└────────────┬─────────────┘
             │
             ├── Detect browser/profile layout
             ├── Read Local State
             ├── Obtain/decrypt App-Bound key
             ├── Read Chromium SQLite databases
             ├── Decrypt supported v20 records
             │
             ▼
┌──────────────────────────┐
│     Local named pipe     │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│     Launcher output      │
│  categorized text data   │
└──────────────────────────┘
```

### 1. Launcher

`CoreSteal/CoreSteal/injector.cpp` is responsible for the outer process orchestration.

Its responsibilities include:

- Locating Chrome, Edge, or Brave.
- Starting the selected browser without opening its normal UI.
- Preparing local IPC.
- Loading `Extractor.dll` into the browser process.
- Receiving categorized records from the extractor.
- Writing the received data into local output files.

### 2. Extractor

`CoreSteal/Extractor/Extractor.c` contains the main extraction workflow.

Once loaded inside the browser process, it:

1. Detects the browser implementation.
2. Connects to the launcher's local IPC channel.
3. Reads the App-Bound encrypted key from Chromium's `Local State` file.
4. Requests decryption of that key through the browser-specific elevator interface.
5. Enumerates Chromium profile directories.
6. Opens the relevant SQLite databases.
7. Decrypts supported encrypted fields.
8. Sends structured records to the launcher.

### 3. Decryption layer

`Decrypt.c` implements the cryptographic portion of the project.

It uses Windows CNG (`BCrypt`) for AES-GCM decryption and parses Chromium's encrypted `v20` record format.

### 4. Browser elevator layer

`Elevator.c` contains browser-specific COM interface definitions and logic used to work with the App-Bound encrypted key.

The source includes implementations/identifiers for multiple Chromium-derived browser environments. The public launcher, however, currently exposes Chrome, Edge, and Brave as its selectable targets.

### 5. SQLite in-memory VFS

`VFS.c` implements a custom SQLite VFS that reads a browser database together with its associated WAL/SHM state into memory.

This allows the project to inspect a consistent SQLite view without directly modifying the original browser database.

### 6. Memory helpers

`Memory/Memory.c` and `Memory/Memory.h` provide small wrappers used by the extractor for allocation, copying, comparison, and cleanup.

---

## Project structure

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

---

## Technologies and dependencies

The project uses:

- C and C++
- Windows API
- Windows Registry API
- Windows COM
- Windows CNG / BCrypt
- SQLite3
- cJSON
- Visual Studio project files
- MSVC Platform Toolset `v142`

The solution contains separate launcher and extractor projects. In the current project configuration, the extractor is built as a DLL for the relevant x64 configuration.

---

## Observable artifacts / defensive indicators

For defenders and researchers, the source contains several behaviors that can be useful when developing detections.

### Process behavior

Potentially notable behavior includes:

- A browser process created suspended and without its normal startup window.
- Remote memory allocation and writing into a browser process.
- Remote thread creation associated with library loading.
- A DLL loaded into a Chromium browser process from the launcher's directory.

### File access

The extractor accesses Chromium profile files including:

- `Local State`
- `Login Data`
- `Login Data For Account`
- `Web Data`
- Associated SQLite WAL/SHM files when present

### IPC

The launcher and DLL communicate using a local Windows named pipe.

### Local output

The current launcher separates extracted records into local text files by data category, including passwords, tokens, payment-card data, and IBAN data.

These artifacts can be useful for endpoint-detection experiments and malware-analysis exercises.

---

## Safe research environment

If you are studying this repository, use a controlled lab environment.

Recommended precautions:

- Use an isolated Windows virtual machine.
- Use a dedicated test browser profile.
- Populate the profile only with synthetic/fake credentials.
- Do not use personal accounts, real payment information, or production authentication tokens.
- Keep snapshots of the VM before testing.
- Avoid exposing the test environment to sensitive networks or systems.
- Review the source code before executing any compiled binary.

---

## Security implications

Modern Chromium versions introduced App-Bound Encryption to make offline theft of browser secrets substantially harder by tying key decryption more closely to the browser/application context.

This project demonstrates an important security boundary: code executing inside a trusted browser process may have access to capabilities that are intentionally unavailable to an unrelated external process.

For defenders, the relevant lesson is therefore broader than credential-file monitoring alone. Detection strategies should also consider suspicious process injection, unusual browser-process ancestry, unexpected DLL loads, abnormal COM/elevator interaction, and access patterns around Chromium profile databases.

---

## Limitations

From the current source tree:

- The launcher explicitly targets Chrome, Edge, and Brave.
- The implementation is Windows-specific.
- The decryption routine expects Chromium `v20` AES-GCM records.
- Database schemas may change between browser versions.
- Browser App-Bound Encryption implementations and COM interfaces may change over time.
- Compatibility with every browser/channel/version is not guaranteed.
- The repository should be considered research code rather than production software.

---

## Responsible use

This repository should only be used for legitimate purposes such as:

- Browser-security research
- Malware analysis
- Detection engineering
- Digital-forensics experiments
- Authorized penetration-testing labs
- Understanding Chromium credential-storage protections

Do not use it to access, collect, or disclose credentials or financial information belonging to other people.

---

## Disclosure and publishing recommendation

If this repository is published publicly, consider making its purpose explicit in the repository description and documentation, for example:

> Windows security-research PoC for studying Chromium App-Bound Encryption, browser-process security boundaries, and defensive detection techniques.

It is also a good idea to avoid publishing real extracted data, test only with synthetic profiles, and clearly document the authorization requirements for anyone experimenting with the code.

---

## Third-party software and attribution

This project uses third-party software/components that are not authored by this project:

- **cJSON** — used for JSON parsing. cJSON is distributed under the **MIT License**. If you redistribute cJSON source or binaries containing it, retain the upstream copyright and MIT license notice supplied by the cJSON project.
- **SQLite3** — used for reading Chromium SQLite databases. SQLite is released into the **public domain** by its authors. See the official SQLite documentation for its public-domain dedication and usage terms.

The presence of these components does not imply ownership of them by the CoreSteal project. Their respective upstream licenses, notices, and terms remain applicable.

If this repository is redistributed, it is recommended to include the original cJSON license notice (for example in a `THIRD_PARTY_NOTICES` or `LICENSES` file) alongside the distributed cJSON files.

---

## License

No license was included in the analyzed source archive.

Before publishing, add an explicit license that matches how you want others to use the repository. Keep in mind that a software license does not override applicable laws, authorization requirements, or responsible-use obligations.

---

## Disclaimer

This project is provided for educational and authorized security-research purposes. The author and contributors are responsible for defining the project's license and intended scope. Users are responsible for complying with all applicable laws and obtaining permission before testing systems, accounts, profiles, or data that they do not own.
