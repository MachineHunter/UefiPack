#include <UefiPackImpl.h>


/**
 
	TODO: keys stored in this DXE Driver's global variable
	needs further investigation if this implementation is safe or not.

**/
UINT16 KeyLength = 16;   // AES-128 key so 128bit=16bytes
BYTE   Key[16]   = {0};  // REMEMBER when changing KeyLength, change ORIG_MAX_NV_BUFFER in typedef too!
BYTE   IV[16]    = {0};




/**
 
 Unpack function of UefiPackProtocol.
 This decrypts DataSize-sized data starting from DataAddr by AES-128
 using Key defined above (which is the key retrieved from TPM).

 @param[in]  DataAddr  .text section base address of packed Dxe Driver
 @param[in]  DataSize  .text section size of packed Dxe Driver

**/
EFI_STATUS
EFIAPI
Unpack (
		IN VOID   *DataAddr,
		IN UINT32 DataSize
		)
{
	struct AES_ctx ctx;

	AES_init_ctx_iv(&ctx, Key, IV);
	AES_CBC_decrypt_buffer(&ctx, (UINT8*)DataAddr, DataSize);
	return EFI_SUCCESS;
}



/**
 
 Function to write data into UEFI variable (MyDxeStatus) for debugging purpose.
 Since my environment doesn't show outputs to screen using ConOut, I'm refering
 this UEFI variable to check the output from Dxe driver.

 @param[in]  No       Id value indicating what function went wrong
 @param[in]  Offset   Offset from the start of MyDxeStatus
 @param[in]  Val      Value to write at the Offset
 @param[in]  ValSize  Size of Val

**/
UINT32   myvarSize        = 0x50;
CHAR8    myvarValue[0x50] = {0};
CHAR16   myvarName[30]    = L"MyDxeStatus";
EFI_GUID myvarGUID        = {0xeefbd379, 0x9f5c, 0x4a92, { 0xa1, 0x57, 0xae, 0x40, 0x79, 0xeb, 0x14, 0x48 }}; // eefbd379-9f5c-4a92-a157-ae4079eb1448

VOID
DebugVarLog (
		IN UINT8  No,
		IN UINT8  Offset,
		IN VOID*  Val,
		IN UINT32 ValSize
		)
{
	myvarValue[0] = No;
	CopyMem(myvarValue+Offset, Val, ValSize);
	
	gRT->SetVariable(
			myvarName,
			&myvarGUID,
			EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
			myvarSize,
			myvarValue);
}



/**
 
 Function to get the Key from TPM.

 @param[out]  Key        Returns key read from TPM
 @param[in]   KeyLength  Size of Key

 Note:
   Key is defined as argument even though it's global variable
   to make it compatible when I changed the way to store Key.

**/
EFI_STATUS
GetTpmKey (
		OUT BYTE             *Key,
		IN  UINT16           KeyLength
		) 
{
	EFI_STATUS Status;
	TPM_HANDLE sessionHandle;
	UINT16     DigestSize = 32;
	BYTE       Digest[32] = {0};

	TPMI_RH_NV_INDEX KeyNvIndex = NV_INDEX_FIRST+1; 

	//
	// This sets requestUse bit of TPM_ACCESS_x register (TPM_ACCESS_x[1])
	//
	Status = TpmRequestUse();
	if(EFI_ERROR(Status)) {
		DebugVarLog(1, 1, &Status, sizeof(EFI_STATUS));
		return EFI_DEVICE_ERROR;
	}

	//
	// Start Policy session
	//
	Status = TpmStartAuthSession(&sessionHandle);
	if(EFI_ERROR(Status)) {
		DebugVarLog(2, 1, &Status, sizeof(EFI_STATUS));
		return EFI_DEVICE_ERROR;
	}

	//
	// This is just for reading PCR value of UefiPackDxe's execution phase
	//
	Status = TpmPcrRead(
			TPM_ALG_SHA256,
			0,
			Digest,
			&DigestSize
			);
	if(EFI_ERROR(Status)) {
		DebugVarLog(6, 1, &Status, sizeof(EFI_STATUS));
		return EFI_DEVICE_ERROR;
	}
	DebugVarLog(0, 0x12, Digest, DigestSize);

	//
	// Select PCR to use for authorization
	//
	Status = TpmPolicyPCR(
			&sessionHandle,
			TPM_ALG_SHA256,
			0
			);
	if(Status!=EFI_SUCCESS) {
		DebugVarLog(3, 1, &Status, sizeof(EFI_STATUS));
		return EFI_DEVICE_ERROR;
	}

	//
	// Read key from TPM NV space (pcr authorization runs here)
	//
	Status = TpmNVRead(
			KeyNvIndex,
			KeyLength,
			&sessionHandle,
			Key
			);
	if(Status!=EFI_SUCCESS) {
		DebugVarLog(4, 1, &Status, sizeof(EFI_STATUS));
		return EFI_DEVICE_ERROR;
	}

	//
	// End session (somehow error occurs so disabling it for now)
	//
	/*
	 *Status = TpmFlushContext(&sessionHandle);
	 *if(EFI_ERROR(Status)) {
	 *  DebugVarLog(5, 1, &Status, sizeof(EFI_STATUS));
	 *  return EFI_DEVICE_ERROR;
	 *}
	 */

	DebugVarLog(9, 1, Key, KeyLength);
	return EFI_SUCCESS;
}



/**
 
 Driver's entry point.
 This first reads key from TPM and stores it in global variable.
 Then, installs UefiPackProtocol for other Dxe drivers to use when unpacking them self.

 @param[in]  ImageHandle  The firmware allocated handle for the EFI image
 @param[in]  SystemTable  A pointer to the EFI System Table.

**/
EFI_STATUS
EFIAPI 
DriverEntry(
		IN EFI_HANDLE ImageHandle,
		IN EFI_SYSTEM_TABLE *SystemTable
		)
{
	EFI_STATUS Status;

	//
	// 1: Read and set Key as a global variable
	//    (just return success to continue execution for debug)
	//
	Status = GetTpmKey(Key, KeyLength);
	if(EFI_ERROR(Status)) {
		return EFI_SUCCESS;
	}

	// 
	// 2: Install UefiPackProtocol
	//
	EFI_HANDLE mUefiPackHandle = NULL;
	EFI_UEFI_PACK_PROTOCOL mUefiPack = {
		Unpack
	};

	gBS->InstallMultipleProtocolInterfaces(
			&mUefiPackHandle,
			&gEfiUefiPackProtocolGuid,
			&mUefiPack,
			NULL
			);

	// 
	// 3: Just a testing of UefiPackProtocol
	//
	BYTE buf[0x10] = {0};
	UINT32 i;
	for(i=0; i<0x10; i++) {
		buf[i] = i;
	}

	struct AES_ctx ctx;
	AES_init_ctx_iv(&ctx, Key, IV);
	AES_CBC_encrypt_buffer(&ctx, (UINT8*)buf, 0x10);

	EFI_UEFI_PACK_PROTOCOL *UefiPackProtocol;
	gBS->LocateProtocol(
			&gEfiUefiPackProtocolGuid,
			NULL,
			(VOID**)&UefiPackProtocol
			);
	UefiPackProtocol->Unpack(buf, 0x10);

	DebugVarLog(0, 0x33, buf, 0x10);
	
	return EFI_SUCCESS;
}
