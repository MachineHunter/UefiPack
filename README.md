# UefiPack

> **Warning**
> This project is still work in progress.
> UefiPackPkg works just fine, but when DXE Driver is packed by UefiPackPacker,
> execution will stop to that DXE Driver and boot will fail.

Encrypts UEFI Modules (mostly DXE Driver) by AES-128 using keys sealed in TPM2.0 device.


<br/>


## Components Description
* UefiPackPkg (edk2)
	* UefiPackDxe: Retrive TPM key and provides UefiPackProtocol for unpack
	* SealKeyDxe: Seal key to TPM (PCR value can be read by using UefiPackDxe)
	* TestDxe: Simple dxe Driver for testing
* UefiPackPacker (VS2019)
	* UefiPackPacker.exe: Packs Dxe Driver which can be unpacked by UefiPackDxe


<br/>


## Building

### UefiPackPkg
1. Setup edk2
2. Copy `UefiPackPkg` as `edk2/UefiPackPkg`
3. In `edk2/Conf/target.txt`, edit `ACTIVE_PLATFORM` to `UefiPackPkg/UefiPackPkg.dsc`

### UefiPackPacker
1. open UefiPackPacker.sln with Visual Studio 2019
2. `Ctrl-b` to build

> **Note**
> Pre-built binaries are going to be uploaded when I resolved the error
> stated in the warning.


<br/>


## Description

