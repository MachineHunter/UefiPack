#include <UefiPackImpl.h>


UINT16 KeyLength = 16;   // AES-128 key so 128bit=16bytes
BYTE   Key[16]   = {0};  // REMEMBER when changing KeyLength, change ORIG_MAX_NV_BUFFER in typedef too!
BYTE   IV[16]    = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };



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

EFI_HANDLE mUefiPackHandle = NULL;
EFI_UEFI_PACK_PROTOCOL mUefiPack = {
  Unpack
};



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
    UartPrint("TpmRequestUse failed %d\r\n", Status);
    return EFI_DEVICE_ERROR;
  }

  //
  // Start Policy session
  //
  Status = TpmStartAuthSession(&sessionHandle);
  if(EFI_ERROR(Status)) {
    UartPrint("TpmStartAuthSession failed %d\r\n", Status);
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
    UartPrint("TpmPcrRead failed %d\r\n", Status);
    return EFI_DEVICE_ERROR;
  }
  UartPrint("TpmPcrRead: ");
  UINT16 i;
  for(i=0; i<DigestSize; i++) {
    UartPrint("%02X ", Digest[i]);
  }
  UartPrint("\r\n");

  //
  // Select PCR to use for authorization
  //
  Status = TpmPolicyPCR(
      &sessionHandle,
      TPM_ALG_SHA256,
      0
      );
  if(Status!=EFI_SUCCESS) {
    UartPrint("TpmPolicyPCR failed %d\r\n", Status);
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
    UartPrint("TpmNVRead failed %d\r\n", Status);
    return EFI_DEVICE_ERROR;
  }

  //
  // End session (somehow error occurs so disabling it for now)
  //
  /*
   *Status = TpmFlushContext(&sessionHandle);
   *if(EFI_ERROR(Status)) {
   *  UartPrint("TpmFlushContext failed %d\r\n", Status);
   *  return EFI_DEVICE_ERROR;
   *}
   */

  UartPrint("Key: ");
  for(i=0; i<KeyLength; i++) {
    UartPrint("%02X ", Key[i]);
  }
  UartPrint("\r\n");

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
  UartPrint(">>> UefiPackDxe start\r\n");

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
  Status = gBS->InstallMultipleProtocolInterfaces(
      &mUefiPackHandle,
      &gEfiUefiPackProtocolGuid,
      &mUefiPack,
      NULL
      );
  if(EFI_ERROR(Status)) {
    UartPrint("InstallMultipleProtocolInterfaces failed\r\n");
    return EFI_SUCCESS;
  }

  // 
  // 3: Just a testing of UefiPackProtocol
  //
  UartPrint("UefiPackProtocol test start\r\n");

  UartPrint("before enc: ");
  BYTE buf[0x10] = {0};
  UINT32 i;
  for(i=0; i<0x10; i++) {
    buf[i] = i;
    UartPrint("%02X ", buf[i]);
  }
  UartPrint("\r\n");

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, Key, IV);
  AES_CBC_encrypt_buffer(&ctx, (UINT8*)buf, 0x10);

  UartPrint("encryted buf: ");
  for(i=0; i<0x10; i++) {
    UartPrint("%02X ", buf[i]);
  }
  UartPrint("\r\n");

  EFI_UEFI_PACK_PROTOCOL *UefiPackProtocol;
  Status = gBS->LocateProtocol(
      &gEfiUefiPackProtocolGuid,
      NULL,
      (VOID**)&UefiPackProtocol
      );
  if(EFI_ERROR(Status))
    UartPrint("LocateProtocol (UefiPackProtocol) failed %d\r\n", Status);

  Status = UefiPackProtocol->Unpack(buf, 0x10);
  if(EFI_ERROR(Status))
    UartPrint("UefiPackProtocol->Unpack failed %d\r\n", Status);

  UartPrint("decrypted buf: ");
  for(i=0; i<0x10; i++) {
    UartPrint("%02X ", buf[i]);
  }
  UartPrint("\r\n");
  
  UartPrint("<<< UefiPackDxe end\r\n");
  return EFI_SUCCESS;
}
