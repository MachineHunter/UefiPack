#include "TestDxe.h"

UINT32 myvarSize      = 30;
CHAR8  myvarValue[30] = {0};
CHAR16 myvarName[30]  = L"MyDxeStatus2";

// 4bd56579-9d59-4e0f-b3ab-7a3acebac187
EFI_GUID myvarGUID = { 0x4bd56579, 0x9d59, 0x4e0f, { 0xb3, 0xab, 0x7a, 0x3a, 0xce, 0xba, 0xc1, 0x87 } };

EFI_HANDLE mDummyHandle = NULL;

EFI_STATUS EFIAPI DummyFunc1() {
  AsciiSPrint(myvarValue, 18, "DummyFunc1 called");
  gRT->SetVariable(
      myvarName,
      &myvarGUID,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      myvarSize,
      myvarValue);
  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI DummyFunc2() {
  AsciiSPrint(myvarValue, 18, "DummyFunc2 called");
  gRT->SetVariable(
      myvarName,
      &myvarGUID,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      myvarSize,
      myvarValue);
  return EFI_SUCCESS;
}

EFI_DUMMY_PROTOCOL mDummy = {
  DummyFunc1,
  DummyFunc2
};

EFI_STATUS EFIAPI DxeEntry(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  gBS->InstallMultipleProtocolInterfaces(&mDummyHandle, &gEfiDummyProtocolGuid, &mDummy, NULL);

  EFI_TIME time;

  gRT->GetTime(&time, NULL);

  AsciiSPrint(myvarValue, 12, "%2d/%2d %2d:%2d", time.Month, time.Day, time.Hour, time.Minute);

  gRT->SetVariable(
      myvarName,
      &myvarGUID,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      myvarSize,
      myvarValue);

  return EFI_SUCCESS;
}
