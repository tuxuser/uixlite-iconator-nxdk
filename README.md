
# UIX-Lite Iconator (NXDK)

Runs on the console and stores Icons / TitleImages for your installed titles at the appropriate location for UIX-Lite to recognize.

As usual, USE AT YOUR OWN RISK!

## What it does

- Scans through drives (E, F, G) on HDD0 and HDD1 for folders which contain a default.xbe
- Extracts Title Id and Title name
- Creates `TitleNames.ini` in `C:\UIX Configs\`

```ini
[default]
<directory name>=<title name>
```

- Creates `Icons.ini` in `C:\UIX Configs\`

```ini
[default]
<directory name>=<title id %08x>
```

For each title:

- Creates `E:\UDATA\<title id %08x>\TitleMeta.xbx`

```ini
TitleName=<title name>
```

- Saves Title Image xbx to `E:\UDATA\<title id %08x>\TitleImage.xbx`


## Credits

- @OfficialTeamUIX / @BigJx - UIX-Lite magic
- @Milenko - UIX-Lite; but mostly, cool announcement pings
- @MobCat - Original Iconator (PC sided python tool)
- @XboxDev - NXDK and awesome ecosystem / docs
