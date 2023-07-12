# Pimax PVR Client interoperability to Varjo

1) Install [Pimax Client](https://pimax.com/pimax-pc) from the Pimax website. This is temporary, you can uninstall it after we grab the necessary files.

2) Go to `%ProgramFiles%\Pimax\Runtime` and copy the following files to a staging folder: `LibMagicD3D1164.dll` and `MagicAttach_x64.exe`.
These are the only files you need, you can then uninstall Pimax Client entirely.

3) Build `pvr-emu.sln`. If you use the Debug profile, you will get goodies such as hotkeys, which can be useful for debugging.

4) From the project output under `bin\x64\Debug`, grab `libpvrclient64.dll` and `VarjoLib.dll`, and copy them to `%SystemRoot%\System32`.

5) Start Varjo Base and make sure eye tracking is calibrated and allowed for applications.

Ok, we can now start a game. 

1) Run your OpenVR, D3D11 game.

2) Using something like [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer), figure out the process ID of the game.

3) Now run the `MagicAttach_x64.exe` tool that we copied earlier, and pass arguments `/pid <process ID>`.

If all went well, you will now have foveated rendering in your app. You can inspect the log file at `%LocalAppData%\PvrEmu\PvrEmu.log`.

```
2023-07-11 23:43:33 -0700: Hello World from 'F:\SteamLibrary\steamapps\common\assettocorsa\acs.exe'!
2023-07-11 23:43:33 -0700: Requested PVR SDK: 1.24
2023-07-11 23:43:33 -0700: Varjo SDK: 3.10.0.6
2023-07-11 23:43:33 -0700: Detected IPD: 0.0635
```

If you see the stuff above, it's a good start, the loading into the game process succeeded, and Varjo initialized OK. The key line you want to look for next is:

```
2023-07-11 23:43:33 -0700: Foveation is active
```

That line means that the process was successfully hooked and foveated rendering is possible. If you don't see that, then the app cannot support foveated rendering.

In Debug builds, you have the following hotkeys you can press at any point (game window must have focus):

- Ctrl-F1: Disable (or rather Pause) foveated rendering.
- Ctrl-F2: Enable foveated rendering.
- Ctrl-F5: Select minimum (lowest quality) foveated level.
- Ctrl-F6: Select balanced foveated level.
- Ctrl-F7: Select maximum (highest quality) foveated level.
- Ctrl-F8: Select debug mode (fovea region is culled).
