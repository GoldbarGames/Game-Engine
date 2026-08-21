"""
slnRegistry - register a game project into GameEngine.sln so it can be built
with MSBuild -t:<Name> (used by rebuild_game.bat, the Build Monitor, and the
debug workflow). A folder with a .vcxproj that is NOT in the .sln cannot be
built through the solution at all - the gap that broke Eggwhite's first build.

Shared by buildMonitor.py (per-row "Register" button) and newProjectEditor.py
(auto-register freshly generated projects). Import and call:

    ok, message = register_project(sln_path, project_folder)

What it writes (mirrors the hand-made registrations of Eggwhite/SnowballSummer):
  - a Project(...) block using the C++ project-type GUID and the .vcxproj's
    own <ProjectGuid>
  - ActiveCfg + Build.0 mappings for every solution configuration, derived
    mechanically: project config = solution config minus "_DLL"
    (games have no DLL configs), platform x86 -> Win32.

Safety: preserves the file byte-for-byte outside the insertions (UTF-8 BOM,
CRLF line endings), writes a GameEngine.sln.bak backup first, and refuses
duplicate names/GUIDs or a vcxproj whose name is already registered from a
different folder (a duplicate checkout).
"""

import os
import re

CPP_PROJECT_TYPE_GUID = "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}"


def find_vcxproj(folder):
    """Path of the first .vcxproj in a project folder, or None."""
    try:
        for f in sorted(os.listdir(folder)):
            if f.lower().endswith(".vcxproj"):
                return os.path.join(folder, f)
    except OSError:
        pass
    return None


def read_vcxproj_guid(vcxproj_path):
    """The <ProjectGuid> ({...} form, uppercase), or None."""
    try:
        with open(vcxproj_path, "r", encoding="utf-8", errors="replace") as f:
            m = re.search(r"<ProjectGuid>\s*(\{[0-9A-Fa-f-]+\})\s*</ProjectGuid>", f.read())
            return m.group(1).upper() if m else None
    except OSError:
        return None


def registered_names(sln_text):
    """Project names already present in the solution text."""
    return set(re.findall(r'Project\("\{[^}]+\}"\)\s*=\s*"([^"]+)"', sln_text))


def register_project(sln_path, project_folder):
    """Register <project_folder>'s .vcxproj into the solution.
    Returns (ok, message). No-ops (ok=True) if already registered."""
    folder_name = os.path.basename(os.path.normpath(project_folder))

    vcxproj = find_vcxproj(project_folder)
    if vcxproj is None:
        return False, "%s: no .vcxproj found in the folder" % folder_name

    name = os.path.splitext(os.path.basename(vcxproj))[0]
    guid = read_vcxproj_guid(vcxproj)
    if guid is None:
        return False, "%s: no <ProjectGuid> in %s" % (name, os.path.basename(vcxproj))

    # Read as bytes -> str so the BOM and CRLF endings survive the round-trip.
    try:
        with open(sln_path, "rb") as f:
            text = f.read().decode("utf-8")
    except OSError as e:
        return False, "cannot read %s: %s" % (sln_path, e)

    names = registered_names(text)
    if name in names:
        if name != folder_name:
            return False, ("%s: its project '%s' is already registered from "
                           "another folder (duplicate checkout?) - not adding twice"
                           % (folder_name, name))
        return True, "%s: already registered" % name
    if guid.upper() in text.upper():
        return False, ("%s: GUID %s is already used by another project in the "
                       "solution - give this .vcxproj a fresh <ProjectGuid> first"
                       % (name, guid))

    # Relative path from the solution folder (always with backslashes). Refuse
    # projects outside the solution tree - this repo's layout never does that,
    # and a "..\..\" path in the .sln is a sign something is being misused.
    sln_dir = os.path.dirname(os.path.abspath(sln_path))
    rel = os.path.relpath(os.path.abspath(vcxproj), sln_dir).replace("/", "\\")
    if rel.startswith(".."):
        return False, ("%s: project folder is not under the solution folder "
                       "(%s) - not registering" % (name, sln_dir))

    # 1) Project block after the last EndProject.
    idx = text.rfind("EndProject")
    if idx < 0:
        return False, "no EndProject line found - not a valid .sln?"
    idx += len("EndProject")
    block = ('\r\nProject("%s") = "%s", "%s", "%s"\r\nEndProject'
             % (CPP_PROJECT_TYPE_GUID, name, rel, guid))
    text = text[:idx] + block + text[idx:]

    # 2) Config mappings, derived from the solution's own configuration list.
    m = re.search(r"GlobalSection\(SolutionConfigurationPlatforms\)[^\r\n]*\r?\n(.*?)EndGlobalSection",
                  text, re.DOTALL)
    if m is None:
        return False, "no SolutionConfigurationPlatforms section found"
    sln_configs = re.findall(r"^\s*([^=\s|]+)\|(\S+)\s*=", m.group(1), re.MULTILINE)
    if not sln_configs:
        return False, "no solution configurations found"

    lines = []
    for cfg, plat in sln_configs:
        proj_cfg = cfg.replace("_DLL", "")                    # games have no DLL configs
        proj_plat = "Win32" if plat.lower() == "x86" else plat
        for kind in ("ActiveCfg", "Build.0"):
            lines.append("\t\t%s.%s|%s.%s = %s|%s" % (guid, cfg, plat, kind, proj_cfg, proj_plat))
    mappings = "\r\n".join(lines) + "\r\n"

    m = re.search(r"GlobalSection\(ProjectConfigurationPlatforms\)[^\r\n]*\r?\n.*?(\t?EndGlobalSection)",
                  text, re.DOTALL)
    if m is None:
        return False, "no ProjectConfigurationPlatforms section found"
    insert_at = m.start(1)
    text = text[:insert_at] + mappings + text[insert_at:]

    # Backup, then write back byte-preserving.
    try:
        with open(sln_path, "rb") as f:
            original = f.read()
        with open(sln_path + ".bak", "wb") as f:
            f.write(original)
        with open(sln_path, "wb") as f:
            f.write(text.encode("utf-8"))
    except OSError as e:
        return False, "cannot write %s: %s" % (sln_path, e)

    return True, ("%s: registered (%d config mappings; backup at %s)"
                  % (name, len(lines), os.path.basename(sln_path) + ".bak"))
