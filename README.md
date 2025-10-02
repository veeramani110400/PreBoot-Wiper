# PreBoot-Wiper
PBW(PreBoot Wiper) is a proof-of-concept wiper malware that leverages a legitimate and overlooked Windows feature to schedule files for deletion on the next reboot. Instead of actively deleting files in real-time—an activity that is heavily monitored by EDR and AV solutions—this tool adds file paths to the PendingFileRenameOperations registry key.
