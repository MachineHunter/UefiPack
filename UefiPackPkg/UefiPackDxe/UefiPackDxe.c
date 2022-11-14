#include <UefiPackImpl.h>

EFI_STATUS EFIAPI DriverEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
	TPMI_RH_NV_INDEX KeyNvIndex = NV_INDEX_FIRST+1;   // this is the nvIndex to store AES key. in this case, NV_INDEX_FIRST+1
	UINT16 KeyLength            = 16;                 // AES-128 key so 128bit=16bytes
	BYTE   Key[16]              = {0};                // REMEMBER when changing KeyLength, change ORIG_MAX_NV_BUFFER in typedef too!
	UINT32 myvarSize            = 60;
	CHAR8  myvarValue[60]       = {0};
	CHAR16 myvarName[30]        = L"MyDxeStatus";
	EFI_GUID myvarGUID          = { 0xeefbd379, 0x9f5c, 0x4a92, { 0xa1, 0x57, 0xae, 0x40, 0x79, 0xeb, 0x14, 0x48 }}; // eefbd379-9f5c-4a92-a157-ae4079eb1448
	
/*
 *   be aware that this value is not the value of PCR but 
 *   is the "digest of concatenated pcr values" by algorithm 
 *   specified in CmdBuffer.authHash. So in this case, it's sha256(pcr[0]).
 *   Also, pcr[0] doesn't need to SwapBytes! (meaning, RecvBuffer4.pcrValues.digest doesn't need to SwapBytes)
 *   so just sha256 RecvBuffer4.pcrValues.digest and put the value below (sha256 value also doesn't need to be swapped)
 *   sha256 can be calculated by "openssl dgst -sha256 -binary pcrvalue.bin | xxd -p -c 32" while pcrvalue.bin
 *   is the file which has the pcr value
 */

	UINT16 DigestSize = 32;
	BYTE   Digest[32] = {0};


	EFI_STATUS Status;
	TPM_HANDLE sessionHandle;

	Status = TpmRequestUse();
	if(EFI_ERROR(Status)) {
		myvarValue[0] = 1;
		CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
		goto End;
	}

	Status = TpmStartAuthSession(&sessionHandle);
	if(EFI_ERROR(Status)) {
		myvarValue[0] = 2;
		CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
		goto End;
	}

	Status = TpmPcrRead(
			TPM_ALG_SHA256,
			0,
			Digest,
			&DigestSize
			);
	if(EFI_ERROR(Status)) {
		myvarValue[0] = 6;
		CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
		goto End;
	}
	CopyMem(myvarValue+0x12, Digest, DigestSize);


	Status = TpmPolicyPCR(
			&sessionHandle,
			TPM_ALG_SHA256,
			0
			);
	if(Status!=EFI_SUCCESS) {
		myvarValue[0] = 3;
		CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
		goto End;
	}

	Status = TpmNVRead(
			KeyNvIndex,
			KeyLength,
			&sessionHandle,
			Key
			);
	if(Status!=EFI_SUCCESS) {
		myvarValue[0] = 4;
		CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
		goto End;
	}

	/*
	 *Status = TpmFlushContext(&sessionHandle);
	 *if(EFI_ERROR(Status)) {
	 *  myvarValue[0] = 5;
	 *  CopyMem(myvarValue+1, &Status, sizeof(EFI_STATUS));
	 *  goto End;
	 *}
	 */


	myvarValue[0] = 9;
	CopyMem(myvarValue+1, Key, KeyLength);


End:
	gRT->SetVariable(
			myvarName,
			&myvarGUID,
			EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
			myvarSize,
			myvarValue);

	return EFI_SUCCESS;
}
