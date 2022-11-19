#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>


/**
 
 TestDxe is for testing UefiPack.
 This produces DummyProtocol too see if protocol can be
 accessed after this driver is packed.

**/
typedef EFI_STATUS (EFIAPI *DUMMY_FUNC1)();
typedef EFI_STATUS (EFIAPI *DUMMY_FUNC2)();

// e9811f61-14bd-4637-8d17-aec540d8f508
#define EFI_DUMMY_PROTOCOL_GUID \
	{ 0x4bd56579, 0x9d59, 0x4e0f, { 0xb3, 0xab, 0x7a, 0x3a, 0xce, 0xba, 0xc1, 0x87 } }

extern EFI_GUID gEfiDummyProtocolGuid;
	
typedef struct _EFI_DUMMY_PROTOCOL {
	DUMMY_FUNC1 DummyFunc1;
	DUMMY_FUNC2 DummyFunc2;
} EFI_DUMMY_PROTOCOL;
