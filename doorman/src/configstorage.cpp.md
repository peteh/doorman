## Table of Contents

* [Introduction](#Introduction)
* [Function List](#Function-List)
  * [formatLittleFS](#formatLittleFS)
  * [saveSettings](#saveSettings)
  * [loadSettings](#loadSettings)

<br><br>

## Introduction

This document provides detailed code documentation for the ConfigStorage module, which handles the storage and retrieval of configuration settings in LittleFS for internal team use.

<br><br>

## Function List

<br>

### <a name="formatLittleFS"></a>formatLittleFS

**Description:** Formats LittleFS to the default file system format.

**Warning:** Formatting LittleFS will erase all existing data in the file system.

**Arguments:**

* None

**Returns:**

* `bool`: `true` if format is successful, else `false`

```cpp
// Example usage
if (formatLittleFS()) {
  Serial.println("LittleFS formatted successfully");
} else {
  Serial.println("Failed to format LittleFS");
}
```

<br><br>

### <a name="saveSettings"></a>saveSettings

**Description:** Saves configuration settings to LittleFS.

**Arguments:**

* `Config` config: Reference to the Config object containing the configuration settings to be saved.

**Returns:**

* None

```cpp
// Example usage
Config config;

// Set config values
config.codeApartmentDoorBell = 1234;
config.codeEntryDoorBell = 5678;

saveSettings(config);
```

<br><br>

### <a name="loadSettings"></a>loadSettings

**Description:** Loads configuration settings from LittleFS.

**Arguments:**

* `Config` config: Reference to the Config object to be populated with the loaded settings.

**Returns:**

* None

```cpp
// Example usage
Config config;

loadSettings(config);

// Get config values
int codeApartmentDoorBell = config.codeApartmentDoorBell;
int codeEntryDoorBell = config.codeEntryDoorBell;
```