TorchlightCam
=============

Camera control for Torchlight v1.15 - TorchlightCam released!

TorchlightCam is a stand alone application to enable camera control in latest Torchlight v1.15 game (much like the previous TorchCam tool did for previous Torchlight game versions).

DEMO! >> VIEW TORCHLIGHTCAM IN ACTION ON YOUTUBE! << DEMO!

UPDATED VERSION 1.02 RELEASED!!
New features in 1.02:
Support for all Torchlight retail versions
Better camera control. You can now set the view angle (default keys are Page Up / Page Down, see the TorchlightCam.ini for more details)
Automatic zoom, rotation and view angle reseting when entering new areas/maps (enabled by default)

New features in 1.01:
Support for Torchlight 1.12b
C++ 2008 dll runtime libraries not required anymore (This should fix win7 and xp problems)

Thanks
Special thanks for the original TorchCam developer for showing the community and mod makers what can be done with the Torchlight game.
Also very special thanks for the amazing testers and nice people at Torchlight forums for helping me to test and make this app perfect!
Please note that TorchlightCam developers are not connected with TorchCam developer in any way and the applications are completly different.

Support for Torchlight v1.12b/v1.13/v1.15 retail and v1.15 steam versions
Left/Right arrow keys to rotate view
Up/Down arrow keys to zoom in/out
Page Up/Page Down keys to raise/lower camera (change the view angle)
Caps Lock key to toggle automatic mouse rotation on/off (Enabled by default at startup)
Control key to activate mouse rotation (can also be used to disable automatic mouse rotation temporally)
Middle mouse button to activate direct mouse rotation
Home key to reset rotation
End key to reset zoom
Z key to disable all features temporally
Automatic zoom, rotation and view angle reset when entering new areas/maps
Double-click on the TorchlightCam's window to reload settings


All features and all key mappings can be customized by modifying the included TorchlightCam.ini file. Remember to reload the TorchlightCam after modifications or reload the settings by double-clicking on the TorchlightCam window.
Here is short explanations of some of the settings and their values:

Minimized
Specifies if the TorchlightCam should start as minimized
Set to "1" to enable and "0" to disable (default value).

StartupMode
Specifies which mode to enable at start
Set to "1" to enable automatic mouse rotation at game startup (default value) and "0" to disable.

SafeZoneWhenToggled
Specifies the area (at center of the screen) in % of screen area that will not detect rotation when using MouseToggle (Caps Lock key) rotation mode.
Default value is 65, meaning that 65% of screen area at center of screen will not detect as rotation.

SafeZoneWhenRotating
Specifies the area (at center of the screen) in % of screen area that will not detect rotation when using MouseRotate (Control key) rotation mode.
Default value is 10, meaning that 10% of screen area at center of screen will not detect as rotation.

InvertMouseRotation
Specifies if the mouse rotation should be inverted
Set to "1" to enable and "0" to disable (default value).

AutoInvertMouse
Specifies if the mouse rotation should be inverted automatically when mouse cursor is below the player character.
This options is handy when you want the rotation tracking to follow the mouse cursor more naturally when walking to the camera direction.
Note: You can use InvertMouseRotation at the same time with this setting.
Set to "1" to enable and "0" to disable (default value).

Running the TorchlightCam:
You can start the TorchlightCam before or after the Torchlight game. TorchlightCam will automatically detect when the Torchlight game is running and activate features. There is no need to close TorchlightCam when restarting Torchlight game. If you have minimized TorchlightCam to the system tray you can right-click the TorchlightCam's icon to select "exit" to close TorchlightCam or double click to make the main window visible again. TorchlightCam includes a small menu that you can activate by right clicking on the TorchlightCam window.

Troubleshooting
Note about Windows Vista and Windows 7 support: If the app doesn't seem to work, try to run the application with administrators rights by right-clicking on the TorchlightCam.exe and selecting "Run as administrator".

If you have any questions about the TorchlightCam please post in this forum. If TorchlightCam doesn't work with your Torchlight game please send me an pm and I will take a look at the problem.
