// MIT License
//
// Copyright(c) 2026 Matthieu Bucchianeri
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace DFR_UI
{
    // Loads DLLs into another process with CreateRemoteThread(LoadLibraryW, <full path>).
    // Supports both 64-bit and 32-bit (WOW64) target processes. This tool must run as a 64-bit process.
    static class Injector
    {
        const uint PROCESS_CREATE_THREAD = 0x0002;
        const uint PROCESS_QUERY_INFORMATION = 0x0400;
        const uint PROCESS_VM_OPERATION = 0x0008;
        const uint PROCESS_VM_WRITE = 0x0020;
        const uint PROCESS_VM_READ = 0x0010;
        const uint MEM_COMMIT = 0x1000;
        const uint MEM_RESERVE = 0x2000;
        const uint MEM_RELEASE = 0x8000;
        const uint PAGE_READWRITE = 0x04;
        const uint WAIT_OBJECT_0 = 0;
        const uint LIST_MODULES_32BIT = 0x01;
        const uint LIST_MODULES_ALL = 0x03;
        const uint LoadTimeoutMs = 10000;

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, uint dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool IsWow64Process(IntPtr hProcess, out bool wow64Process);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, UIntPtr dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress, UIntPtr dwSize, uint dwFreeType);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, UIntPtr nSize, out UIntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, UIntPtr dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        static extern IntPtr GetModuleHandleW(string lpModuleName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", EntryPoint = "K32EnumProcessModulesEx", SetLastError = true)]
        static extern bool EnumProcessModulesEx(IntPtr hProcess, [Out] IntPtr[] lphModule, uint cb, out uint lpcbNeeded, uint dwFilterFlag);

        [DllImport("kernel32.dll", EntryPoint = "K32GetModuleFileNameExW", CharSet = CharSet.Unicode, SetLastError = true)]
        static extern uint GetModuleFileNameEx(IntPtr hProcess, IntPtr hModule, StringBuilder lpFilename, uint nSize);

        struct RemoteModule
        {
            public IntPtr Base;
            public string Path;
        }

        static IntPtr Open(uint pid)
        {
            var process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, pid);
            if (process == IntPtr.Zero)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to open process " + pid);
            }
            return process;
        }

        static bool Is32Bit(IntPtr process)
        {
            if (!IsWow64Process(process, out bool wow64))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to query process architecture");
            }
            return wow64;
        }

        // Unlike System.Diagnostics.Process.Modules, this also lists the 32-bit modules of a WOW64 process.
        static List<RemoteModule> EnumModules(IntPtr process, uint filter)
        {
            var modules = new IntPtr[1024];
            if (!EnumProcessModulesEx(process, modules, (uint)(modules.Length * IntPtr.Size), out uint needed, filter))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to enumerate process modules");
            }
            var result = new List<RemoteModule>();
            var count = Math.Min(modules.Length, (int)(needed / IntPtr.Size));
            for (int i = 0; i < count; i++)
            {
                var name = new StringBuilder(1024);
                if (GetModuleFileNameEx(process, modules[i], name, (uint)name.Capacity) > 0)
                {
                    result.Add(new RemoteModule { Base = modules[i], Path = name.ToString() });
                }
            }
            return result;
        }

        static string FindModule(IntPtr process, string fileName)
        {
            foreach (var module in EnumModules(process, LIST_MODULES_ALL))
            {
                if (string.Equals(Path.GetFileName(module.Path), fileName, StringComparison.OrdinalIgnoreCase))
                {
                    return module.Path;
                }
            }
            return null;
        }

        // Returns the RVA of an exported function by parsing the PE file on disk.
        static uint GetExportRva(string dllPath, string exportName)
        {
            var image = File.ReadAllBytes(dllPath);
            int peHeader = BitConverter.ToInt32(image, 0x3C);
            int fileHeader = peHeader + 4;
            int numberOfSections = BitConverter.ToUInt16(image, fileHeader + 2);
            int sizeOfOptionalHeader = BitConverter.ToUInt16(image, fileHeader + 16);
            int optionalHeader = fileHeader + 20;
            bool pe32Plus = BitConverter.ToUInt16(image, optionalHeader) == 0x20B;
            int dataDirectories = optionalHeader + (pe32Plus ? 112 : 96);
            uint exportDirRva = BitConverter.ToUInt32(image, dataDirectories);
            uint exportDirSize = BitConverter.ToUInt32(image, dataDirectories + 4);
            int sectionTable = optionalHeader + sizeOfOptionalHeader;

            Func<uint, int> rvaToOffset = (rva) =>
            {
                for (int i = 0; i < numberOfSections; i++)
                {
                    int section = sectionTable + i * 40;
                    uint virtualSize = BitConverter.ToUInt32(image, section + 8);
                    uint virtualAddress = BitConverter.ToUInt32(image, section + 12);
                    uint rawSize = BitConverter.ToUInt32(image, section + 16);
                    uint rawPointer = BitConverter.ToUInt32(image, section + 20);
                    if (rva >= virtualAddress && rva < virtualAddress + Math.Max(virtualSize, rawSize))
                    {
                        return (int)(rva - virtualAddress + rawPointer);
                    }
                }
                throw new Exception("Invalid RVA in " + dllPath);
            };

            int exportDir = rvaToOffset(exportDirRva);
            uint numberOfNames = BitConverter.ToUInt32(image, exportDir + 24);
            int addressOfFunctions = rvaToOffset(BitConverter.ToUInt32(image, exportDir + 28));
            int addressOfNames = rvaToOffset(BitConverter.ToUInt32(image, exportDir + 32));
            int addressOfNameOrdinals = rvaToOffset(BitConverter.ToUInt32(image, exportDir + 36));
            for (int i = 0; i < numberOfNames; i++)
            {
                int nameOffset = rvaToOffset(BitConverter.ToUInt32(image, addressOfNames + i * 4));
                int end = Array.IndexOf(image, (byte)0, nameOffset);
                if (Encoding.ASCII.GetString(image, nameOffset, end - nameOffset) == exportName)
                {
                    ushort ordinal = BitConverter.ToUInt16(image, addressOfNameOrdinals + i * 2);
                    uint rva = BitConverter.ToUInt32(image, addressOfFunctions + ordinal * 4);
                    if (rva >= exportDirRva && rva < exportDirRva + exportDirSize)
                    {
                        throw new Exception(exportName + " is a forwarded export in " + dllPath);
                    }
                    return rva;
                }
            }
            throw new Exception("Cannot find " + exportName + " in " + dllPath);
        }

        static IntPtr GetRemoteLoadLibraryW(IntPtr process, bool is32Bit)
        {
            if (!is32Bit)
            {
                // The 64-bit kernel32.dll is mapped at the same address in every process of the session.
                var address = GetProcAddress(GetModuleHandleW("kernel32.dll"), "LoadLibraryW");
                if (address == IntPtr.Zero)
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to locate LoadLibraryW");
                }
                return address;
            }

            // For a WOW64 process, locate its 32-bit kernel32.dll and resolve the export from the file on disk.
            foreach (var module in EnumModules(process, LIST_MODULES_32BIT))
            {
                if (string.Equals(Path.GetFileName(module.Path), "kernel32.dll", StringComparison.OrdinalIgnoreCase))
                {
                    // The reported path is System32 (the WOW64 view), but the 32-bit image on disk is in SysWOW64.
                    var kernel32x86 = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86), "kernel32.dll");
                    return new IntPtr(module.Base.ToInt64() + GetExportRva(kernel32x86, "LoadLibraryW"));
                }
            }
            throw new Exception("The 32-bit kernel32.dll is not loaded in the process yet");
        }

        // Returns true if the process is a 32-bit (WOW64) process.
        public static bool Is32BitProcess(uint pid)
        {
            var process = Open(pid);
            try
            {
                return Is32Bit(process);
            }
            finally
            {
                CloseHandle(process);
            }
        }

        // Loads the DLL at dllPath into the process and waits for its DllMain to complete. Does nothing if a DLL with the
        // same file name is already loaded from that path. The DLL must match the architecture of the process. Throws on
        // failure.
        public static void Inject(uint pid, string dllPath)
        {
            var fileName = Path.GetFileName(dllPath);
            var process = Open(pid);
            try
            {
                var loaded = FindModule(process, fileName);
                if (loaded != null)
                {
                    if (!string.Equals(Path.GetFullPath(loaded), Path.GetFullPath(dllPath), StringComparison.OrdinalIgnoreCase))
                    {
                        throw new Exception("A different " + fileName + " is already loaded in the process: " + loaded);
                    }
                    return;
                }

                var loadLibraryW = GetRemoteLoadLibraryW(process, Is32Bit(process));

                var pathBytes = Encoding.Unicode.GetBytes(dllPath + "\0");
                var size = new UIntPtr((uint)pathBytes.Length);
                var remotePath = VirtualAllocEx(process, IntPtr.Zero, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (remotePath == IntPtr.Zero)
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to allocate memory in process " + pid);
                }

                var freeRemotePath = true;
                try
                {
                    if (!WriteProcessMemory(process, remotePath, pathBytes, size, out _))
                    {
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to write to process " + pid);
                    }

                    var thread = CreateRemoteThread(process, IntPtr.Zero, UIntPtr.Zero, loadLibraryW, remotePath, 0, IntPtr.Zero);
                    if (thread == IntPtr.Zero)
                    {
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "Failed to create remote thread in process " + pid);
                    }

                    try
                    {
                        if (WaitForSingleObject(thread, LoadTimeoutMs) != WAIT_OBJECT_0)
                        {
                            // The thread may still read the path: leak the allocation rather than free it under its feet.
                            freeRemotePath = false;
                            throw new Exception("Timed out loading " + fileName + " in process " + pid);
                        }
                    }
                    finally
                    {
                        CloseHandle(thread);
                    }
                }
                finally
                {
                    if (freeRemotePath)
                    {
                        VirtualFreeEx(process, remotePath, UIntPtr.Zero, MEM_RELEASE);
                    }
                }

                // The remote thread's exit code is a truncated HMODULE, so check the module list instead.
                if (FindModule(process, fileName) == null)
                {
                    throw new Exception("Failed to load " + dllPath + " in process " + pid);
                }
            }
            finally
            {
                CloseHandle(process);
            }
        }
    }
}
