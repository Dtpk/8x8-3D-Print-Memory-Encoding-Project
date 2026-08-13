Update new gba and gb rom version is available! I will port the upgraded decode menu to the other versions as well.

Some notes the gba rom was gonna be all hand made but a local llm busted this project out I checked it and did my own tweaks but for the most part I have gave it the same huffman key it will work with
all the other versions of the software the llm made a better decode menu that is interactive if you want it and not when you don't with a live preview I am gonna try to go in and add this to the qbasic and or python builds when I can. Also the gba rom source code is available sound needs to be fixed yet besides that fully usable without the sound right now, the grid background looks busy on a emulator on but real hardware and a ds and its fine not a problem. "The handshaking debugging and making the link cable bit-bang all home grown by me with the 3D printer version"

GBA version has a 3D print mode now!!! Right now only marlin firmware and ender 3 build plates are set but any printer with marlin this should work this uses a pico to connect over otg Y cable so no board modifying needed or printer disassembly. If any one wishes to add features "cough keychain mode or top layer drawing" reach out but I will be making a new repository for a custom gcode converter and pre-made docker build environment so really anything can be printed not just these 32mm 8x8 grids and it will automatically convert gcode and compile the gba rom with it..

<img width="1920" height="1080" alt="same" src="https://github.com/user-attachments/assets/bd1ace25-25f7-4b10-9edd-c9cda23de19b" />



Really any openscad project that's basic might be convertible but yeah this 3D print gba rom generates its own bit grid from the encode menu of the rom. It uses a handshake and is 1 to 1 real time so when bit section is hit it is normal to slow down and it is normal if you hit print while not connected to the machine to freeze as of right now.

GBA 3D printer steps!!!
Pin 3 on gba cable goes to gpio 5 on pico and pin bellow it pin 4 of link cable goes to gpio 4 on the pico ground of cable pin 6 goes to ground on the pico. Only 2 are needed for debug tests but to print it needs a handshake before any bitbanging over pin 3 can occur.  Steps for full set up flash the pico drag and drop the .uf2 on to it and then unplug it is in otg mode now so don't leave it plugged in to the pc. Next flash the rom I used a ds and a piece of homebrew to flash a repo but any flash card will work. Connect the 2 as described and it should work. This was made with cheap 3rd party cables in mind "not that I just happened to get the wrong cable cough" also only one end of your cable will have all the pins you need so use that half.

So the .bas file is to be ran on DOS systems using qbasic. I am currently using qbasic 4.5.
The .py can be launched in python under Windows or Linux and doesn't need any requirements.

Update single write generator build in openscad is up you can either load this in online versions of openscad or a local instants to run the openscad code give it a few words tell it if you want a notch or keychain hole set the size ect then print and you got yourself a coaster/keychain with data you only see when it got a light behind it. Please test the bits first in the preview before printing there is little error protection. 

If you wanna just hop right into a generator for the single use print here you go.


Keychain hole and coin mode isn't in this version of the generator I will replace this link when I can the script is updated how ever so just heads up about the keychain mode.
https://tinyurl.com/64byteGenerator

Keychain size is 32mm by 32mm 0.4mm height printed with no infill but might be better with it I have not tried yet.
In the slicer it is 8 layers and I have the top and bottom layers set to 2.

The airsoft re-writable version will be up next you will need to punch the back single layer out this is for easy printing on 3D printers since it wont force the printer to draw 64 holes. 

Video of the project in use on a DOS system but it is identical to the python version "https://youtu.be/C47VS0j3yO4"
