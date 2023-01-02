#include <Protocol/SmmBase2.h>
#include <UefiPackSmm/UefiPackSmm.h>

EFI_STATUS
EFIAPI
SmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  int i;
  EFI_STATUS Status;
  BYTE buf[0x10] = {0};
  for(i=0; i<0x10; i++)
    buf[i] = i;

  UartPrint("\r\n>>> TestSmm Entry\r\n");


  // 
  // 1: Get SMST
  //
  EFI_SMM_BASE2_PROTOCOL *SmmBase2Protocol;
  EFI_SMM_SYSTEM_TABLE2  *Smst;
  Status = SystemTable->BootServices->LocateProtocol(
      &gEfiSmmBase2ProtocolGuid,
      0x0,
      (VOID**)&SmmBase2Protocol
      );
  if(Status!=EFI_SUCCESS)
    UartPrint("LocateProtocol error %d\r\n", Status);

  Status = SmmBase2Protocol->GetSmstLocation(
      SmmBase2Protocol,
      &Smst
      );
  if(Status!=EFI_SUCCESS)
    UartPrint("GetSmstLocation error %d\r\n", Status);


  // 
  // 2: Use UefiPackProtocol
  //
  EFI_UEFI_PACK_PROTOCOL *UefiPackProtocol;
  Status = Smst->SmmLocateProtocol(
      &gEfiUefiPackProtocolGuid,
      NULL,
      (VOID**)&UefiPackProtocol
      );
  if(Status!=EFI_SUCCESS)
    UartPrint("SmmLocateProtocol error %d\r\n", Status);

  Status = UefiPackProtocol->Unpack(
      buf,
      0x10
      );
  UartPrint("buf: ");
  for(i=0; i<0x10; i++)
    UartPrint("%02X",buf[i]);
  UartPrint("\r\n");


  // 
  // 3: Check if is inside SMRAM
  //
  BOOLEAN InSmram = 0;
  Status = SmmBase2Protocol->InSmm(
      SmmBase2Protocol,
      &InSmram
      );
  if(Status!=EFI_SUCCESS)
    UartPrint("SmmBase2InSmram error %d\r\n", Status);
  if(InSmram)
    UartPrint("It is inside SMRAM\r\n");
  else
    UartPrint("It is outside SMRAM\r\n");


  UartPrint("<<< TestSmm Ended\r\n");
  return EFI_SUCCESS;
}
