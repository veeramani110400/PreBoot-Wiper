# PreBoot Wiper

**Disclaimer:** This tool is intended strictly for educational purposes and for use in authorized red teaming engagements. It is a powerful and destructive tool designed to simulate a real-world wiper attack. The author is not responsible for any damage caused by its misuse. Use this tool with extreme caution and only on systems you own or have explicit permission to test.

---

## Overview

This is a proof-of-concept wiper malware that leverages a legitimate and often overlooked Windows feature to schedule files for deletion on the next reboot. Instead of actively deleting files in real-time—an activity that is heavily monitored by EDR and AV solutions—this tool adds file paths to the `PendingFileRenameOperations` registry key.

The Windows Session Manager Subsystem (`smss.exe`), a critical system process, reads this registry key during the boot sequence and deletes the specified files before most security agents are fully operational. This "off-line" deletion makes the activity stealthy and highly effective.

The tool is designed to be self-cleaning, scheduling its own executable for deletion to leave no trace behind for forensic analysis.

### The Technique: `PendingFileRenameOperations`

* **Registry Key:** `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager`
* **Value:** `PendingFileRenameOperations` (REG_MULTI_SZ)

**How it Works:** This registry value holds a list of file operations to be performed at the next boot. To mark a file for deletion, an entry is created with the source file's path prefixed with `\??\` followed by a null terminator, and then an empty destination path followed by another null terminator.

**Why it's Stealthy:**

* **Legitimate Feature Abuse:** The tool isn't using a suspicious API like `DeleteFile`. It's modifying a registry key, an action that can be lost in the noise of normal system activity.
* **Delayed Execution:** The actual deletion is performed by `smss.exe` during startup, a trusted system process. EDR/AV agents running in user-mode may not be active or have full visibility at this early stage of the boot process.
* **No Trace:** By adding its own executable to the list, the malware erases itself, complicating forensic investigation.

---

## Features

* **Stealth Deletion:** Uses a native Windows feature to evade real-time detection.
* **Targeted Destruction:** Scans specific, high-value directories for sensitive user and system data.
* **Smart Filtering:** Focuses only on files with specific extensions to maximize impact on data and backups.
* **Configurable Inclusions/Exclusions:** Comes with pre-defined lists of paths to target and to avoid, preventing system instability.
* **Fallback Mechanisms:** Gracefully skips files or directories it cannot access due to permissions, ensuring the program runs to completion.
* **Self-Destruct:** Schedules its own executable for deletion on reboot.

---

## Targeting Strategy

The tool is configured to cause maximum data loss while minimizing the risk of rendering the OS completely unbootable.

### ✅ Included Target Paths

The following directories are recursively scanned for files with target extensions:

**User Profiles & Data**

* `%USERPROFILE%\Desktop`
* `%USERPROFILE%\Documents`
* `%USERPROFILE%\Downloads`
* `%USERPROFILE%\Pictures`
* `%USERPROFILE%\Music`
* `%USERPROFILE%\Videos`
* `%USERPROFILE%\Contacts`
* `%USERPROFILE%\Favorites`
* `%USERPROFILE%\Links`
* `%USERPROFILE%\OneDrive` (and other cloud sync folders)

**Application Data**

* `%APPDATA%` (e.g., `C:\Users\<user>\AppData\Roaming`)
* `%LOCALAPPDATA%` (e.g., `C:\Users\<user>\AppData\Local`)
* `C:\Users\Public`
* `C:\ProgramData`

**Development & Backup Locations**

* `C:\Repos`
* `C:\Projects`

Any locations containing common backup or virtual machine files are targeted via their extensions.

### ❌ Excluded Paths

To ensure system stability and avoid noisy, low-value areas, the following paths are completely ignored:

**Browser Profiles**

* `%LOCALAPPDATA%\Google\Chrome\User Data`
* `%LOCALAPPDATA%\Microsoft\Edge\User Data`
* `%APPDATA%\Mozilla\Firefox\Profiles`

**Temporary & Cache Folders**

* `%TEMP%` (Per-user temporary folder)
* `C:\Windows\Temp`

**Windows System & Installer Caches**

* `C:\Windows\SoftwareDistribution\Download` (Windows Update cache)
* `C:\Windows\Installer` (MSI installer cache)

> **Note:** Critical directories like `C:\Windows\System32` are not targeted to prevent immediate boot failure.

### 🎯 Targeted File Extensions

The wiper specifically looks for files with the following extensions to maximize its impact:

**User Documents:** `.txt`, `.pdf`, `.doc`, `.docx`, `.xls`, `.xlsx`, `.ppt`, `.pptx`

**Media & Archives:** `.jpg`, `.jpeg`, `.png`, `.gif`, `.zip`, `.rar`, `.7z`

**Databases & Backups:** `.bak`, `.sql`, `.mdb`, `.accdb`, `.mdf`, `.ldf`

**Virtual Machines & Disk Images:** `.vhd`, `.vhdx`, `.vmdk`, `.vdi`

---

## Execution

> **IMPORTANT:** This program requires Administrator privileges to modify the HKLM registry hive.

1. Open a Command Prompt or PowerShell terminal **as an Administrator**.
2. Navigate to the directory containing the compiled executable.
3. Run the executable:

```powershell
.\Wiper.exe
```

4. The program will ask for the maximum number of files to target. Enter a number and press Enter.
5. The tool will begin scanning and scheduling files for deletion. Once complete, it will schedule itself for deletion.
6. Upon the next system reboot, all scheduled files will be permanently deleted by Windows.
