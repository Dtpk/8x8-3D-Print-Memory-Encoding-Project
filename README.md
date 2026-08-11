Update new gba and gb rom version is avaliable! I will port the upgraded decode menu to the other versions as well.

Some notes the gba rom was gonna be all hand made but a local llm busted this project out I checked it and did my own tweaks but for the most part I have gave it the same huffman key it will work with
all the other versions of the software the llm made a better decode menu that is interactive if you want it and not when you don't with a live preview I am gonna try to go in and add this to the qbasic and or python builds when I can. Also the gba rom source code is avaliable sound needs to be fixed yet besides that fully usable without the sound right now, the grid background looks busy on a emulator on but real hardware and a ds and its fine not a problem.

GBA version will get a 3D printer option it will let you set the temps home then print a already made square over otg to the 3D printer in my case the ender 3 this is all just for laughs but wanted to try and print over link cable connected to a pico to the machine no promises this build will get done and released but will do my best. Through bit banging pin 3 and some buffer tricks on the pico I got the first 3D print its so close so expect this to be available soon with some kinda 3D print model for a stand. Right now only marlin firmware and ender 3 build plates are set but any printer with marlin this should work with it when it goes live. If any one wishes to add features reach out when it goes live it still has a lot of work to do and honestly having other builds to print other openscad projects could be cool

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
