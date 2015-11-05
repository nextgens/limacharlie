#include <MemoryModule/MemoryModule.h>

#define RPAL_FILE_ID  40

#ifdef RPAL_PLATFORM_WINDOWS
/*
 * Memory DLL loading code
 * Version 0.0.2
 *
 * Copyright (c) 2004-2005 by Joachim Bauch / mail@joachim-bauch.de
 * http://www.joachim-bauch.de
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is MemoryModule.c
 *
 * The Initial Developer of the Original Code is Joachim Bauch.
 *
 * Portions created by Joachim Bauch are Copyright (C) 2004-2005
 * Joachim Bauch. All Rights Reserved.
 *
 */

// disable warnings about pointer <-> DWORD conversions
#pragma warning( disable : 4311 4312 )

#include <Windows.h>
#include <winnt.h>
#include <stdlib.h>
#ifdef DEBUG_OUTPUT
#include <stdio.h>
#endif

#include <MemoryModule/MemoryModule.h>


#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE 0x0020
#endif

#ifndef IMAGE_SIZEOF_BASE_RELOCATION 
// Vista SDKs no longer define IMAGE_SIZEOF_BASE_RELOCATION!? 
#define IMAGE_SIZEOF_BASE_RELOCATION (sizeof(IMAGE_BASE_RELOCATION)) 
#endif 

typedef BOOL (WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

typedef struct {
	PIMAGE_NT_HEADERS headers;
	unsigned char *codeBase;
	HMODULE *modules;
    HANDLE hFileMapping;
	int numModules;
	int initialized;

    PIMAGE_EXPORT_DIRECTORY ptrExport;
    DllEntryProc entryPoint;
} MEMORYMODULE, *PMEMORYMODULE;


#define GET_HEADER_DICTIONARY(module, idx)	&(module)->headers->OptionalHeader.DataDirectory[idx]

#ifdef DEBUG_OUTPUT
static void
OutputLastError(const char *msg)
{
	LPVOID tmp;
	char *tmpmsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&tmp, 0, NULL);
	tmpmsg = (char *)LocalAlloc(LPTR, strlen(msg) + strlen(tmp) + 3);
	sprintf(tmpmsg, "%s: %s", msg, tmp);
	OutputDebugString(tmpmsg);
	LocalFree(tmpmsg);
	LocalFree(tmp);
}
#endif

void*
    MemoryGetLibraryBase
    (
        HMEMORYMODULE mod
    )
{
    void* base = 0;
    PMEMORYMODULE module = (PMEMORYMODULE)mod;

    if( 0 != module )
    {
        base = module->codeBase;
    }

    return base;
}

static void
CopySections(const unsigned char *data, PIMAGE_NT_HEADERS old_headers, PMEMORYMODULE module)
{
	int i, size;
	unsigned char *codeBase = module->codeBase;
	unsigned char *dest;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
	for (i=0; i<module->headers->FileHeader.NumberOfSections; i++, section++)
	{
		if (section->SizeOfRawData == 0)
		{
			// section doesn't contain data in the dll itself, but may define
			// uninitialized data
			size = old_headers->OptionalHeader.SectionAlignment;
			if (size > 0)
			{
                /*
				dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
					size,
					MEM_COMMIT,
					PAGE_READWRITE);
*/
                dest = codeBase + section->VirtualAddress;

				section->Misc.PhysicalAddress = (DWORD)dest;
				memset(dest, 0, size);
			}

			// section is empty
			continue;
		}

		// commit memory block and copy data from dll
        /*
		dest = (unsigned char *)VirtualAlloc(codeBase + section->VirtualAddress,
							section->SizeOfRawData,
							MEM_COMMIT,
							PAGE_READWRITE);
                            */
        dest = codeBase + section->VirtualAddress;

		memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
		section->Misc.PhysicalAddress = (DWORD)dest;
	}
}

// Protection flags for memory pages (Executable, Readable, Writeable)
static int ProtectionFlags[2][2][2] = {
	{
		// not executable
		{PAGE_NOACCESS, PAGE_WRITECOPY},
		{PAGE_READONLY, PAGE_READWRITE},
	}, {
		// executable
		{PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY},
		{PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE},
	},
};

static void
FinalizeSections(PMEMORYMODULE module)
{
	int i;
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(module->headers);
	
	// loop through all sections and change access flags
	for (i=0; i<module->headers->FileHeader.NumberOfSections; i++, section++)
	{
		DWORD protect, oldProtect, size;
		int executable = (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
		int readable =   (section->Characteristics & IMAGE_SCN_MEM_READ) != 0;
		int writeable =  (section->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;

		if (section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
		{
			// section is not needed any more and can safely be freed
            VirtualFree((LPVOID)( module->codeBase + section->VirtualAddress ), section->SizeOfRawData, MEM_DECOMMIT);
			continue;
		}

		// determine protection flags based on characteristics
		protect = ProtectionFlags[executable][readable][writeable];
		if (section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
			protect |= PAGE_NOCACHE;

		// determine size of region
		size = section->SizeOfRawData;
		if (size == 0)
		{
			if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
				size = module->headers->OptionalHeader.SizeOfInitializedData;
			else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
				size = module->headers->OptionalHeader.SizeOfUninitializedData;
		}

		if (size > 0)
		{
			// change memory access flags
            if (VirtualProtect((LPVOID)( module->codeBase + section->VirtualAddress ), section->SizeOfRawData, protect, &oldProtect) == 0)
#ifdef DEBUG_OUTPUT
				OutputLastError("Error protecting memory page")
#endif
			;
		}
	}
}

static void
PerformBaseRelocation(PMEMORYMODULE module, PBYTE delta)
{
	DWORD i;
	unsigned char *codeBase = module->codeBase;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
	if (directory->Size > 0)
	{
		PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)(codeBase + directory->VirtualAddress);
		for (; relocation->VirtualAddress > 0; )
		{
			unsigned char *dest = (unsigned char *)(codeBase + relocation->VirtualAddress);
			unsigned short *relInfo = (unsigned short *)((unsigned char *)relocation + IMAGE_SIZEOF_BASE_RELOCATION);
			for (i=0; i<((relocation->SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION) / 2); i++, relInfo++)
			{
				DWORD *patchAddrHL;
                DWORD64 *patchAddrHL64;
				int type, offset;

				// the upper 4 bits define the type of relocation
				type = *relInfo >> 12;
				// the lower 12 bits define the offset
				offset = *relInfo & 0xfff;
				
				switch (type)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					// skip relocation
					break;

                case IMAGE_REL_BASED_DIR64:
                    //change complete 64 bit address
                    patchAddrHL64 = (DWORD64 *)(dest + offset);
                    *patchAddrHL64 += (DWORD64)delta;
                    break;

				case IMAGE_REL_BASED_HIGHLOW:
					// change complete 32 bit address
					patchAddrHL = (DWORD *)(dest + offset);
					*patchAddrHL += (DWORD)delta;
					break;

				default:
					//printf("Unknown relocation: %d\n", type);
					break;
				}
			}

			// advance to next relocation block
			relocation = (PIMAGE_BASE_RELOCATION)(((unsigned char*)relocation) + relocation->SizeOfBlock);
		}
	}
}

static int
BuildImportTable(PMEMORYMODULE module)
{
	int result=1;
	unsigned char *codeBase = module->codeBase;

	PIMAGE_DATA_DIRECTORY directory = GET_HEADER_DICTIONARY(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
	if (directory->Size > 0)
	{
		PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(codeBase + directory->VirtualAddress);
		for (; !IsBadReadPtr(importDesc, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && importDesc->Name; importDesc++)
		{
			PVOID *thunkRef;
            FARPROC *funcRef;
			HMODULE handle = LoadLibraryA((LPCSTR)(codeBase + importDesc->Name));
			if (handle == INVALID_HANDLE_VALUE)
			{
#if DEBUG_OUTPUT
				OutputLastError("Can't load library");
#endif
				result = 0;
				break;
			}

			module->modules = (HMODULE *)realloc(module->modules, (module->numModules+1)*(sizeof(HMODULE)));
			if (module->modules == NULL)
			{
				result = 0;
				break;
			}

			module->modules[module->numModules++] = handle;
			if (importDesc->OriginalFirstThunk)
			{
				thunkRef = (PVOID*)(codeBase + importDesc->OriginalFirstThunk);
				funcRef = (FARPROC*)(codeBase + importDesc->FirstThunk);
			} else {
				// no hint table
				thunkRef = (PVOID*)(codeBase + importDesc->FirstThunk);
				funcRef = (FARPROC*)(codeBase + importDesc->FirstThunk);
			}
			for (; *thunkRef; thunkRef++, funcRef++)
			{
				if IMAGE_SNAP_BY_ORDINAL((DWORDLONG)*thunkRef)
					*funcRef = GetProcAddress(handle, (LPCSTR)IMAGE_ORDINAL((DWORDLONG)*thunkRef));
				else {
					PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(codeBase + (DWORDLONG)*thunkRef);
					*funcRef = GetProcAddress(handle, (LPCSTR)&thunkData->Name);
				}
				if (*funcRef == 0)
				{
					result = 0;
					break;
				}
			}

			if (!result)
				break;
		}
	}

	return result;
}

HMEMORYMODULE MemoryLoadLibrary(const void *data, unsigned int size)
{
	PMEMORYMODULE result = NULL;
    HANDLE hFileMapping = NULL;
	PIMAGE_DOS_HEADER dos_header = NULL;
	PIMAGE_NT_HEADERS old_header = NULL;
	unsigned char *code, *headers;
	PBYTE locationDelta = NULL;
	DllEntryProc DllEntry = NULL;
	BOOL successfull = FALSE;
    DWORD sectionIndex = 0;
    PIMAGE_SECTION_HEADER sectionHeader = NULL;
    UNREFERENCED_PARAMETER( size );

	dos_header = (PIMAGE_DOS_HEADER)data;
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
	{
#if DEBUG_OUTPUT
		OutputDebugString("Not a valid executable file.\n");
#endif
		return NULL;
	}

	old_header = (PIMAGE_NT_HEADERS)&((const unsigned char *)(data))[dos_header->e_lfanew];
	if (old_header->Signature != IMAGE_NT_SIGNATURE)
	{
#if DEBUG_OUTPUT
		OutputDebugString("No PE header found.\n");
#endif
		return NULL;
	}

    if( ( sizeof(PVOID) == sizeof(DWORD) &&
        IMAGE_NT_OPTIONAL_HDR32_MAGIC != old_header->OptionalHeader.Magic ) 
        ||
        ( sizeof(PVOID) != sizeof(DWORD) &&
        IMAGE_NT_OPTIONAL_HDR64_MAGIC != old_header->OptionalHeader.Magic ) )
    {
#if DEBUG_OUTPUT
        OutputDebugString( "PE type mismatch current process space.\n" );
#endif
        return NULL;
    }

	// reserve memory for image of library
    /*
	code = (unsigned char *)VirtualAlloc((LPVOID)(old_header->OptionalHeader.ImageBase),
		old_header->OptionalHeader.SizeOfImage,
		MEM_RESERVE,
		PAGE_READWRITE);

    if (code == NULL)
        // try to allocate memory at arbitrary position
        code = (unsigned char *)VirtualAlloc(NULL,
            old_header->OptionalHeader.SizeOfImage,
            MEM_RESERVE,
            PAGE_READWRITE);
    */

    hFileMapping = CreateFileMapping( INVALID_HANDLE_VALUE, 
                                      NULL, 
                                      PAGE_EXECUTE_READWRITE, 
                                      0, 
                                      old_header->OptionalHeader.SizeOfImage, 
                                      NULL );

    if( NULL == hFileMapping &&
        ERROR_INVALID_PARAMETER == GetLastError() )
    {
        hFileMapping = CreateFileMapping( INVALID_HANDLE_VALUE, 
                                      NULL, 
                                      PAGE_READWRITE, 
                                      0, 
                                      old_header->OptionalHeader.SizeOfImage, 
                                      NULL );
    }

    if( NULL == hFileMapping )
    {
        goto error;
    }

    code = MapViewOfFile( hFileMapping, FILE_MAP_WRITE | FILE_MAP_EXECUTE, 0, 0, 0 );

	if (code == NULL)
	{
#if DEBUG_OUTPUT
		OutputLastError("Can't reserve memory");
#endif
		return NULL;
	}

	result = (PMEMORYMODULE)HeapAlloc(GetProcessHeap(), 0, sizeof(MEMORYMODULE));
	result->codeBase = code;
	result->numModules = 0;
	result->modules = NULL;
	result->initialized = 0;
    result->hFileMapping = hFileMapping;

	// XXX: is it correct to commit the complete memory region at once?
    //      calling DllEntry raises an exception if we don't...
    /*
	VirtualAlloc(code,
		old_header->OptionalHeader.SizeOfImage,
		MEM_COMMIT,
		PAGE_READWRITE);

	// commit memory for headers
	headers = (unsigned char *)VirtualAlloc(code,
		old_header->OptionalHeader.SizeOfHeaders,
		MEM_COMMIT,
		PAGE_READWRITE);
        */

    headers = result->codeBase;
	
	// copy PE header to code
	memcpy(headers, dos_header, dos_header->e_lfanew + old_header->OptionalHeader.SizeOfHeaders);
	result->headers = (PIMAGE_NT_HEADERS)&((const unsigned char *)(headers))[dos_header->e_lfanew];

	// update position
	result->headers->OptionalHeader.ImageBase = (ULONGLONG)code;

	// copy sections from DLL file block to new memory location
	CopySections(data, old_header, result);

	// adjust base address of imported data
	locationDelta = (code - old_header->OptionalHeader.ImageBase);
	if (locationDelta != 0)
		PerformBaseRelocation(result, locationDelta);

	// load required dlls and adjust function table of imports
	if (!BuildImportTable(result))
		goto error;

	// mark memory pages depending on section headers and release
	// sections that are marked as "discardable"
	FinalizeSections(result);

	// get entry point of loaded library
	if (result->headers->OptionalHeader.AddressOfEntryPoint != 0)
	{
		DllEntry = (DllEntryProc)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
		if (DllEntry == 0)
		{
#if DEBUG_OUTPUT
			OutputDebugString("Library has no entry point.\n");
#endif
			goto error;
		}

		// notify library about attaching to process
		successfull = (*DllEntry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0);
		if (!successfull)
		{
#if DEBUG_OUTPUT
			OutputDebugString("Can't attach library.\n");
#endif
			goto error;
		}
		result->initialized = 1;
	}

    // Backups for header removal
    if( 0 != (GET_HEADER_DICTIONARY(result, IMAGE_DIRECTORY_ENTRY_EXPORT))->Size )
    {
        result->ptrExport = (PIMAGE_EXPORT_DIRECTORY)(result->codeBase + (GET_HEADER_DICTIONARY(result, IMAGE_DIRECTORY_ENTRY_EXPORT))->VirtualAddress );
    }
    else
    {
        result->ptrExport = NULL;
    }
    result->entryPoint = (DllEntryProc)(result->codeBase + result->headers->OptionalHeader.AddressOfEntryPoint);

    // Erase the headers
    sectionHeader = IMAGE_FIRST_SECTION( result->headers );
    for( sectionIndex = 0; sectionIndex < result->headers->FileHeader.NumberOfSections; sectionIndex++ )
    {
        memset( &sectionHeader->Name, 0, sizeof( sectionHeader->Name ) );
        sectionHeader++;
    }

    memset( result->codeBase + ((PIMAGE_DOS_HEADER)result->codeBase)->e_lfanew, 0, sizeof( IMAGE_NT_HEADERS ) );
    memset( result->codeBase, 0, sizeof( IMAGE_DOS_HEADER ) );

	return (HMEMORYMODULE)result;

error:
	// cleanup
    if( NULL != result )
    {
	    MemoryFreeLibrary(result);
    }
	return NULL;
}

RPVOID MemoryGetProcAddress(HMEMORYMODULE module, RPCHAR name)
{
	unsigned char *codeBase = ((PMEMORYMODULE)module)->codeBase;
	int idx=-1;
	DWORD i, *nameRef;
	WORD *ordinal;
	PIMAGE_EXPORT_DIRECTORY exports = ((PMEMORYMODULE)module)->ptrExport;

	if (NULL == exports)
		// no export table found
		return NULL;

	if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0)
		// DLL doesn't export anything
		return NULL;

	// search function name in list of exported names
	nameRef = (DWORD *)(codeBase + exports->AddressOfNames);
	ordinal = (WORD *)(codeBase + exports->AddressOfNameOrdinals);
	for (i=0; i<exports->NumberOfNames; i++, nameRef++, ordinal++)
		if (_stricmp(name, (const char *)(codeBase + *nameRef)) == 0)
		{
			idx = *ordinal;
			break;
		}

	if (idx == -1)
		// exported symbol not found
		return NULL;

	if ((DWORD)idx > exports->NumberOfFunctions)
		// name <-> ordinal number don't match
		return NULL;

	// AddressOfFunctions contains the RVAs to the "real" functions
	return (RPVOID)(codeBase + *(DWORD *)(codeBase + exports->AddressOfFunctions + (idx*4)));
}

void MemoryFreeLibrary(HMEMORYMODULE mod)
{
	int i;
	PMEMORYMODULE module = (PMEMORYMODULE)mod;

	if (module != NULL)
	{
		if (module->initialized != 0)
		{
			// notify library about detaching from process
            DllEntryProc DllEntry = module->entryPoint;
			//DllEntryProc DllEntry = (DllEntryProc)(module->codeBase + module->headers->OptionalHeader.AddressOfEntryPoint);
			(*DllEntry)((HINSTANCE)module->codeBase, DLL_PROCESS_DETACH, 0);
			module->initialized = 0;
		}

		if (module->modules != NULL)
		{
			// free previously opened libraries
			for (i=0; i<module->numModules; i++)
				if (module->modules[i] != INVALID_HANDLE_VALUE)
					FreeLibrary(module->modules[i]);

			free(module->modules);
		}

		if (module->codeBase != NULL)
        {
			// release memory of library
            UnmapViewOfFile( module->codeBase );
            CloseHandle( module->hFileMapping );
			//VirtualFree(module->codeBase, 0, MEM_RELEASE);
        }

		HeapFree(GetProcessHeap(), 0, module);
	}
}







#elif defined( RPAL_PLATFORM_LINUX ) || defined( RPAL_PLATFORM_MACOSX )

#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <rpal/rpal.h>


typedef struct {
	RPVOID hMod;
} MEMORYMODULE, *PMEMORYMODULE;


HMEMORYMODULE
	MemoryLoadLibrary
	(
		RPVOID buffer,
		unsigned int bufferSize
	)
{
	PMEMORYMODULE h = NULL;
	char modName[] = "/tmp/rpXXXXXX";
	int fd = 0;
	RBOOL isWritten = FALSE;
	RBOOL isLoaded = FALSE;
	
	if( NULL != ( h = rpal_memory_alloc( sizeof( MEMORYMODULE ) ) ) )
	{
		h->hMod = NULL;
		
		if( -1 != ( fd = mkstemp( (RPCHAR)&modName ) ) )
		{
			if( bufferSize == write( fd, buffer, bufferSize ) )
			{
				isWritten = TRUE;
			}
			
			close( fd );
			
			
			if( isWritten )
			{
				if( NULL != ( h->hMod = dlopen( (RPCHAR)&modName, RTLD_NOW | RTLD_LOCAL ) ) )
				{
					isLoaded = TRUE;
				}
			}
			
			unlink( modName );
		}
	}
	
	if( !isLoaded )
	{
		if( NULL != h )
		{
			rpal_memory_free( h );
			h = NULL;
		}
	}
	
	return h;
}

RPVOID
	MemoryGetLibraryBase
	(
		HMEMORYMODULE hModule
	)
{
	if( rpal_memory_isValid( hModule ) )
	{
		return ((PMEMORYMODULE)hModule)->hMod;
	}
	else
	{
		return NULL;
	}
}

RPVOID
	MemoryGetProcAddress
	(
		HMEMORYMODULE hModule,
		RPCHAR procName
	)
{
	RPVOID sym = NULL;
	PMEMORYMODULE pMod = (PMEMORYMODULE)hModule;
	
	if( rpal_memory_isValid( pMod ) &&
	    NULL != procName )
	{
		sym = dlsym( pMod->hMod, procName );
	}
	
	return sym;
}

RVOID
	MemoryFreeLibrary
	(
		HMEMORYMODULE hModule
	)
{
	PMEMORYMODULE pMod = (PMEMORYMODULE)hModule;
	
	if( rpal_memory_isValid( pMod ) )
	{
		dlclose( pMod->hMod );
		
		rpal_memory_free( hModule );
	}
}

#endif
