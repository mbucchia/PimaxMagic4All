# Pimax Foveated Rendering interoperability with Varjo (and more)

## Installing

1) Install [Pimax Client](https://pimax.com/pimax-pc) from the Pimax website. This is temporary, you can uninstall it after we grab the necessary files.

2) Go to `%ProgramFiles%\Pimax\Runtime` and copy the following files to a staging folder: `LibMagicD3D1164.dll` and `MagicAttach_x64.exe`.
These are the only files you need, you can then uninstall Pimax Client entirely.

3) Unzip the Foveated Rendering Utility in the folder of your choice.

4) Copy the `LibMagicD3D1164.dll` and `MagicAttach_x64.exe` files obtained in step 2) into the folder created in step 3).
The `LibMagicD3D1164.dll` and `MagicAttach_x64.exe` shall be next to the `DFR-UI.exe` file.

5) Copy the `libpvrclient64.dll` and `VarjoLib.dll` filed from the folder created in step 3) into the `%SystemRoot%\System32` folder (aka: `C:\Windows\System32`).

6) (For Varjo users) Start Varjo Base and make sure eye tracking is calibrated and allowed for applications (search 'Privacy' settings under 'System').

## Running

Double-click `DFR-UI.exe`. **This app must be running at all time in order to initiate Foveated Rendering.**

Start SteamVR and the game of your choice. Enable Foveated Rendering in the user interface.

If all went well, you will now have foveated rendering in your app.

## Troubleshooting

You can inspect the log file at the bottom of the `DFR-UI` tool (which is loaded from `%LocalAppData%\PvrEmu\PvrEmu.log`).

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
