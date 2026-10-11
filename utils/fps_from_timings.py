"""Frame rate from a game run with KINJO_GPU_TIMINGS=<n>: reads the game's
output on stdin, stamps each line with the time it arrived, and from the first
to the last "Pass timings" report works out frames per second. It also averages
the reports' CPU-record and GPU totals.

    KINJO_GPU_TIMINGS=60 game.exe ... | python utils/fps_from_timings.py 60 [label]
"""
import re
import sys
import time

per = int(sys.argv[1])
label = sys.argv[2] if len(sys.argv) > 2 else ""
stamps = []
cpu_totals, gpu_frames, draw_totals = [], [], []
cpu = 0.0
draws = 0.0
in_report = False
for line in sys.stdin:
    now = time.time()
    if "Pass timings" in line:
        stamps.append(now)
        in_report = True
        cpu = 0.0
        draws = 0.0
        continue
    if in_report:
        m = re.match(r"\s+(\S+)\s+([\d.]+)\s*/\s*([\d.]+)(?:\s+(\d+))?", line)
        if m and not line.startswith("    "):     # top-level passes only (nested ones are inside them)
            cpu += float(m.group(3))
            if m.group(4):
                draws += float(m.group(4))
            continue
        g = re.search(r"GPU frame \(first pass to last\): ([\d.]+)", line)
        if g:
            gpu_frames.append(float(g.group(1)))
            cpu_totals.append(cpu)
            draw_totals.append(draws)
            in_report = False
# Skip the first two reports: they include loading.
use = stamps[2:]
if len(use) >= 2:
    fps = (len(use) - 1) * per / (use[-1] - use[0])
    c = cpu_totals[2:]
    g = gpu_frames[2:]
    d = draw_totals[2:]
    print("%s: %.1f fps over %d reports; CPU recording %.1f ms, GPU frame %.1f ms, %.0f draw calls (averages)"
          % (label, fps, len(use), sum(c) / len(c), sum(g) / len(g), sum(d) / len(d)))
else:
    print("%s: not enough reports (%d)" % (label, len(stamps)))
