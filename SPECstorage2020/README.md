# Building the SPECstorage(TM) Solution 2020/Netmist Framework.

## Build From The Source Repo

#### Requirements

* Unix/Linux gnu make
* Windows Visual Studio 15 or newer

### Unix/Linux

Type "make" which will build all products. All build artifacts will go
into the build subdirectory. It will then do an install to the "bin"
subdirectory which will copy the netmist binary appropriate for the OS
and Arch of the system as well as scripts required to run the benchmark.

If you wish to build your own from scratch, these will be 
built in place, and copies will be stored in the "bin" directory.

### Windows


Installation of the Windows 8.1SDK for use with Visual Studio 2015 Express.

Visual Studio 2015 can be dowloaded here:
https://www.visualstudio.com/vs/older-downloads/

You will have to sign up for a free "Dev Essentials" account.

It is suggested that 64 bit builds be used for the benchmark.  To
assist with this in the case that Visual Studio 2015 is used,
the 8.1 SDK must be installed.  The SDK can be downloaded and
installed from:
http://www.microsoft.com/en-us/download/details.aspx?id=8279


### Build Files

The MSBuild solution files are located in the top level
$REPO/trunk/msbuild sub-directory. There is one solution file for each
product.

#### Build from GUI

Follow the process as described in the SPECstorage Solution 2020 User's guide.

## Creating a release

First make sure all of the required binaries are in the build
directory, then type "make release".

The release step works on Linux and OSX, but may not work on other
versions of Unix due to command line dependencies.


To create the release directories run 'make release' which will create
subdirectories under SPECstorage2020_BUILD for the 'dist', 'pro' and 'lite'
products. This only works in Unix.

If one wishes to use the pre-built binaries, please examine
the "binaries" directory and find the appropriate executable 
for your architecture and copy the files into the $TOP/bin level
of your kit.
