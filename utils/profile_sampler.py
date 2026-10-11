"""A poor man's sampling profiler for a running Windows x64 process.

    python utils/profile_sampler.py <process name> <seconds> [hz] [out.txt] [delay s]

Waits for the process to start (up to 60 s) and then `delay` seconds more (for
loading), takes its busiest thread (the game loop), and every 1/hz s suspends
it, walks its stack with dbghelp and resumes it. Then prints the functions by
SELF samples (where the time is spent) and by INCLUSIVE samples (what is on the
stack), resolved from the PDBs. Needs 64-bit Python; no other tools.

Start the game in the background and the sampler beside it, e.g.:
    (game.exe --windowed ... &) ; python utils/profile_sampler.py TrainRails.exe 15 100 prof.txt 25
"""
import ctypes
import ctypes.wintypes as wt
import sys
import time
from collections import Counter

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
dbg = ctypes.WinDLL("dbghelp", use_last_error=True)

PROCESS_ALL = 0x1F0FFF
THREAD_ALL = 0x1FFFFF
TH32CS_SNAPPROCESS, TH32CS_SNAPTHREAD = 0x2, 0x4


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ProcessID", wt.DWORD),
                ("th32DefaultHeapID", ctypes.c_void_p), ("th32ModuleID", wt.DWORD), ("cntThreads", wt.DWORD),
                ("th32ParentProcessID", wt.DWORD), ("pcPriClassBase", ctypes.c_long), ("dwFlags", wt.DWORD),
                ("szExeFile", ctypes.c_wchar * 260)]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ThreadID", wt.DWORD),
                ("th32OwnerProcessID", wt.DWORD), ("tpBasePri", ctypes.c_long), ("tpDeltaPri", ctypes.c_long),
                ("dwFlags", wt.DWORD)]


k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
k32.OpenProcess.restype = wt.HANDLE
k32.OpenThread.restype = wt.HANDLE
k32.SuspendThread.argtypes = [wt.HANDLE]
k32.ResumeThread.argtypes = [wt.HANDLE]
k32.GetThreadContext.argtypes = [wt.HANDLE, ctypes.c_void_p]
k32.GetThreadTimes.argtypes = [wt.HANDLE] + [ctypes.POINTER(ctypes.c_ulonglong)] * 4


def find_process(name):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    e = PROCESSENTRY32W()
    e.dwSize = ctypes.sizeof(e)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        if e.szExeFile.lower() == name.lower():
            k32.CloseHandle(snap)
            return e.th32ProcessID
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return None


def threads_of(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    e = THREADENTRY32()
    e.dwSize = ctypes.sizeof(e)
    out = []
    ok = k32.Thread32First(snap, ctypes.byref(e))
    while ok:
        if e.th32OwnerProcessID == pid:
            out.append(e.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def cpu_time(h):
    c, x, kt, ut = (ctypes.c_ulonglong() for _ in range(4))
    k32.GetThreadTimes(h, ctypes.byref(c), ctypes.byref(x), ctypes.byref(kt), ctypes.byref(ut))
    return kt.value + ut.value


# --- dbghelp -------------------------------------------------------------------
SYMOPT_UNDNAME, SYMOPT_DEFERRED_LOADS, SYMOPT_LOAD_LINES = 0x2, 0x4, 0x10
dbg.SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS)
dbg.SymInitializeW.argtypes = [wt.HANDLE, ctypes.c_wchar_p, wt.BOOL]
dbg.SymFunctionTableAccess64.restype = ctypes.c_void_p
dbg.SymFunctionTableAccess64.argtypes = [wt.HANDLE, ctypes.c_ulonglong]
dbg.SymGetModuleBase64.restype = ctypes.c_ulonglong
dbg.SymGetModuleBase64.argtypes = [wt.HANDLE, ctypes.c_ulonglong]

FTA = ctypes.WINFUNCTYPE(ctypes.c_void_p, wt.HANDLE, ctypes.c_ulonglong)
GMB = ctypes.WINFUNCTYPE(ctypes.c_ulonglong, wt.HANDLE, ctypes.c_ulonglong)
fta = FTA(lambda h, a: dbg.SymFunctionTableAccess64(h, a))
gmb = GMB(lambda h, a: dbg.SymGetModuleBase64(h, a))
dbg.StackWalk64.argtypes = [wt.DWORD, wt.HANDLE, wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
                            FTA, GMB, ctypes.c_void_p]


class SYMBOL_INFOW(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.ULONG), ("TypeIndex", wt.ULONG), ("Reserved", ctypes.c_ulonglong * 2),
                ("Index", wt.ULONG), ("Size", wt.ULONG), ("ModBase", ctypes.c_ulonglong), ("Flags", wt.ULONG),
                ("Value", ctypes.c_ulonglong), ("Address", ctypes.c_ulonglong), ("Register", wt.ULONG),
                ("Scope", wt.ULONG), ("Tag", wt.ULONG), ("NameLen", wt.ULONG), ("MaxNameLen", wt.ULONG),
                ("Name", ctypes.c_wchar * 1024)]


dbg.SymFromAddrW.argtypes = [wt.HANDLE, ctypes.c_ulonglong, ctypes.POINTER(ctypes.c_ulonglong),
                             ctypes.POINTER(SYMBOL_INFOW)]


class IMAGEHLP_MODULEW64(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.DWORD), ("BaseOfImage", ctypes.c_ulonglong), ("ImageSize", wt.DWORD),
                ("TimeDateStamp", wt.DWORD), ("CheckSum", wt.DWORD), ("NumSyms", wt.DWORD), ("SymType", wt.DWORD),
                ("ModuleName", ctypes.c_wchar * 32), ("ImageName", ctypes.c_wchar * 256),
                ("LoadedImageName", ctypes.c_wchar * 256), ("pad", ctypes.c_byte * 4096)]


dbg.SymGetModuleInfoW64.argtypes = [wt.HANDLE, ctypes.c_ulonglong, ctypes.POINTER(IMAGEHLP_MODULEW64)]

CONTEXT_SIZE = 1232
CONTEXT_FULL = 0x10000B


def main():
    name, seconds = sys.argv[1], float(sys.argv[2])
    hz = float(sys.argv[3]) if len(sys.argv) > 3 else 100.0
    out = sys.argv[4] if len(sys.argv) > 4 else None
    delay = float(sys.argv[5]) if len(sys.argv) > 5 else 0.0
    pid = None
    wait_end = time.perf_counter() + 60.0
    while pid is None and time.perf_counter() < wait_end:     # it may still be starting
        pid = find_process(name)
        if pid is None:
            time.sleep(0.5)
    if pid is None:
        sys.exit("no process " + name)
    time.sleep(delay)                                         # ...and loading
    hp = k32.OpenProcess(PROCESS_ALL, False, pid)
    if not dbg.SymInitializeW(hp, None, True):
        sys.exit("SymInitialize failed %d" % ctypes.get_last_error())

    # The busiest thread over half a second: the game loop.
    hs = {t: k32.OpenThread(THREAD_ALL, False, t) for t in threads_of(pid)}
    t0 = {t: cpu_time(h) for t, h in hs.items()}
    time.sleep(0.5)
    tid = max(hs, key=lambda t: cpu_time(hs[t]) - t0[t])
    ht = hs[tid]

    raw = ctypes.create_string_buffer(CONTEXT_SIZE + 16)
    base = (ctypes.addressof(raw) + 15) & ~15
    frame = ctypes.create_string_buffer(1024)
    stacks = []
    end = time.perf_counter() + seconds
    period = 1.0 / hz
    while time.perf_counter() < end:
        k32.SuspendThread(ht)
        ctypes.memset(base, 0, CONTEXT_SIZE)
        ctypes.c_uint32.from_address(base + 0x30).value = CONTEXT_FULL
        pcs = []
        if k32.GetThreadContext(ht, base):
            rip = ctypes.c_uint64.from_address(base + 0xF8).value
            rsp = ctypes.c_uint64.from_address(base + 0x98).value
            rbp = ctypes.c_uint64.from_address(base + 0xA0).value
            ctypes.memset(frame, 0, 1024)
            fb = ctypes.addressof(frame)
            for off, val in ((0, rip), (32, rbp), (48, rsp)):     # AddrPC, AddrFrame, AddrStack
                ctypes.c_uint64.from_address(fb + off).value = val
                ctypes.c_uint32.from_address(fb + off + 12).value = 3   # AddrModeFlat
            for _ in range(48):
                if not dbg.StackWalk64(0x8664, hp, ht, fb, base, None, fta, gmb, None):
                    break
                pc = ctypes.c_uint64.from_address(fb).value
                if pc == 0:
                    break
                pcs.append(pc)
        k32.ResumeThread(ht)
        if pcs:
            stacks.append(pcs)
        time.sleep(period)

    cache = {}
    sym = SYMBOL_INFOW()

    def resolve(a):
        if a in cache:
            return cache[a]
        sym.SizeOfStruct = 88                                  # sizeof(SYMBOL_INFOW) in C, one WCHAR of name
        sym.MaxNameLen = 1023
        disp = ctypes.c_ulonglong()
        mod = IMAGEHLP_MODULEW64()
        mod.SizeOfStruct = ctypes.sizeof(IMAGEHLP_MODULEW64) - 4096
        mname = "?"
        if dbg.SymGetModuleInfoW64(hp, a, ctypes.byref(mod)):
            mname = mod.ModuleName
        if dbg.SymFromAddrW(hp, a, ctypes.byref(disp), ctypes.byref(sym)):
            s = "%s!%s" % (mname, sym.Name)
        else:
            s = "%s!0x%x" % (mname, a)
        cache[a] = s
        return s

    self_c, incl_c, mod_c = Counter(), Counter(), Counter()
    for pcs in stacks:
        names = [resolve(a) for a in pcs]
        self_c[names[0]] += 1
        mod_c[names[0].split("!")[0]] += 1
        for n in set(names):
            incl_c[n] += 1
    n = max(1, len(stacks))
    lines = ["%d samples of thread %d over %.0f s" % (len(stacks), tid, seconds), "", "MODULES (self):"]
    lines += ["  %5.1f%%  %s" % (100.0 * c / n, m) for m, c in mod_c.most_common(12)]
    lines += ["", "SELF:"]
    lines += ["  %5.1f%%  %s" % (100.0 * c / n, f[:150]) for f, c in self_c.most_common(45)]
    lines += ["", "INCLUSIVE:"]
    lines += ["  %5.1f%%  %s" % (100.0 * c / n, f[:150]) for f, c in incl_c.most_common(90)]
    text = "\n".join(lines)
    if out:
        open(out, "w", encoding="utf-8").write(text)
    print(text)


if __name__ == "__main__":
    main()
