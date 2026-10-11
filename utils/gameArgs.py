"""
gameArgs - every command-line argument a game accepts, read from its source,
with the comments that explain each one.

The engine parses no arguments; each game parses its own, in its own style
(`a == "--shots"`, `HasFlag(argc, args, "--bot")`, strcmp...). A hand-kept
list per game would drift, so this reads the code instead: a string literal
that is exactly "--name", in a source file that handles argc/argv, is an
argument. Its description comes from the comments around it, in the styles
the games use:

  usage blocks     //   --shots N    take N screenshots, then quit
                   //                (continuation lines indented deeper)
  prose            // --test3d [section] jumps straight to a 3D-scene ...
  end of line      bool autoText = false;   // --autotext: advance dialogue
  in the branch    else if (a == "--genmodels")
                   {
                       // Regenerate the furniture and character meshes...

Used by buildMonitor.py's per-game "Args" button. Run it directly to print a
game's list:   python gameArgs.py TrainRails
"""

import os
import re
import sys

SOURCE_EXTS = (".cpp", ".h", ".hpp", ".cc", ".cxx")

# Folders that never hold a game's own source (build output, assets, VS state).
# Folders starting with '.' or '_' are skipped too.
SKIP_DIRS = {
    "x64", "win32", "debug", "release", "debug_dll", "release_dll",
    "data", "assets", "cache", "screenshots", "bgm", "se", "fonts", "docs",
    "packages", "node_modules", "bin", "obj", "out", "ipch", "build", "builds",
}

FLAG_TOKEN = re.compile(r"--[A-Za-z0-9][A-Za-z0-9_-]*")
FLAG_LITERAL = re.compile(r"--[A-Za-z0-9][A-Za-z0-9_-]*\Z")
# Where a usage line's head ("--day N --time HH:MM") ends and its text begins:
# a run of 2+ spaces, a colon followed by a space, or " - ".
HEAD_END = re.compile(r"\s{2,}|:\s|:\Z|\s-\s")
BULLET = re.compile(r"([-*•]|\d+[.)])\s")


class Arg:
    """One command-line argument and what the source says about it."""

    def __init__(self, name, path, line, code):
        self.name = name
        self.path = path        # first file/line that reads it
        self.line = line
        self.code = code        # that line of code, stripped
        self.usage = []         # [(head, text)] comment lines that start with this flag
        self.notes = []         # comments right above / inside the code that reads it
        self.mentions = []      # other comments that mention it in passing

    def described(self):
        return bool(self.usage or self.notes or self.mentions)


# ================================== lexing ===================================

def lex(text):
    """One pass over C++ source, telling code from comments and strings.

    Returns (literals, comments):
      literals  [(line, value)] for every string literal outside comments
      comments  {line: (body, own_line)} for every // comment; own_line is
                True when nothing but whitespace comes before it"""
    literals = []
    comments = {}
    i, n = 0, len(text)
    line, line_start = 1, 0

    def advance_lines(start, end):
        nonlocal line, line_start
        count = text.count("\n", start, end)
        if count:
            line += count
            line_start = text.rfind("\n", start, end) + 1

    while i < n:
        c = text[i]
        if c == "\n":
            line += 1
            line_start = i + 1
            i += 1
        elif c == "/" and text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
            body = text[i + 2:end].rstrip("\r").lstrip("/!")
            comments[line] = (body, text[line_start:i].strip() == "")
            i = end
        elif c == "/" and text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            advance_lines(i, end)
            i = end
        elif c == "R" and text.startswith('R"', i) and not (i and (text[i - 1].isalnum() or text[i - 1] == "_")):
            paren = text.find("(", i + 2)
            if paren < 0:
                i += 1
                continue
            close = ")" + text[i + 2:paren] + '"'
            end = text.find(close, paren)
            end = n if end < 0 else end + len(close)
            literals.append((line, text[paren + 1:end - len(close)]))
            advance_lines(i, end)
            i = end
        elif c == '"':
            j = i + 1
            while j < n and text[j] not in '"\n':
                j += 2 if text[j] == "\\" else 1
            literals.append((line, text[i + 1:j]))
            i = j + 1
        elif c == "'" and not (i and text[i - 1].isdigit()):   # 1'000 is a digit separator
            j = i + 1
            while j < n and text[j] not in "'\n":
                j += 2 if text[j] == "\\" else 1
            i = j + 1
        else:
            i += 1
    return literals, comments


# ============================ comment structure ==============================

def comment_blocks(comments):
    """Group // comments into blocks: runs of whole-line comments on
    consecutive lines; an end-of-line comment is a block on its own.
    Each block is a list of (line, body)."""
    blocks = []
    current = []
    for ln in sorted(comments):
        body, own = comments[ln]
        body = body.expandtabs(4).rstrip()
        if own and current and current[-1][0] == ln - 1 and current[-1][2]:
            current.append((ln, body, own))
            continue
        if current:
            blocks.append([(l, b) for l, b, _o in current])
        current = [(ln, body, own)]
    if current:
        blocks.append([(l, b) for l, b, _o in current])
    return blocks


def split_block(block):
    """Split a comment block into units. A line that starts with a flag
    starts a 'usage' unit; lines indented at least as deep continue it; a
    blank comment line or a shallower line ends it. Anything else is prose.
    Returns [(kind, head, lines)], kind 'usage' or 'prose', head the usage
    line's head ('--shots N') and lines the text after it."""
    units = []
    unit = None          # [kind, head, lines, indent]
    for _ln, body in block:
        stripped = body.lstrip()
        indent = len(body) - len(stripped)
        if not stripped:
            unit = None
            continue
        starts_flag = FLAG_TOKEN.match(stripped) is not None
        if unit is not None and unit[0] == "usage":
            if indent > unit[3] or (indent == unit[3] and not starts_flag):
                unit[2].append(stripped)
                continue
            unit = None
        # A sentence wrapped so that a flag starts the next line
        # ("...which is the trap\n// --editcheck fell into") is still prose.
        if (starts_flag and unit is not None and unit[0] == "prose"
                and indent <= unit[3] and not unit[2][-1].endswith((".", ":", ";", "!", "?"))):
            starts_flag = False
        if starts_flag:
            head, text = parse_head(stripped)
            unit = ["usage", head, [text] if text else [], indent]
            units.append(unit)
        elif unit is not None:          # prose continues
            unit[2].append(stripped)
        else:
            unit = ["prose", "", [stripped], indent]
            units.append(unit)
    return [(kind, head, lines) for kind, head, lines, _indent in units]


def parse_head(stripped):
    """'--day N --time HH:MM   start the clock' -> ('--day N --time HH:MM',
    'start the clock'). Without a clear break the head is just the flag."""
    first = FLAG_TOKEN.match(stripped)
    m = HEAD_END.search(stripped, first.end())
    if m:
        return stripped[:m.start()].rstrip(" ,"), stripped[m.end():].strip()
    return first.group(0), stripped[first.end():].strip()


def reflow(lines):
    """Join wrapped comment lines into paragraphs; bullet lines start anew."""
    out = []
    for text in lines:
        if out and not BULLET.match(text):
            prev = out[-1]
            if (len(prev) > 1 and prev.endswith("-") and prev[-2].isalpha()) or prev.endswith("/"):
                out[-1] = prev + text          # a word or path broken over two lines
            else:
                out[-1] = prev + " " + text
        else:
            out.append(text)
    return "\n".join(out)


# ================================= scanning ==================================

def source_files(game_folder):
    """The game's own C++ files, main.cpp first."""
    found = []
    for dirpath, dirnames, filenames in os.walk(game_folder):
        dirnames[:] = sorted(d for d in dirnames
                             if d.lower() not in SKIP_DIRS and not d.startswith((".", "_")))
        for fn in sorted(filenames, key=str.lower):
            if fn.lower().endswith(SOURCE_EXTS):
                found.append(os.path.join(dirpath, fn))
    found.sort(key=lambda p: (os.path.basename(p).lower() != "main.cpp",
                              os.path.dirname(p) != game_folder))
    return found


def scan_file(path, rel, args):
    """Add this file's arguments and comments to args (name -> Arg)."""
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            text = f.read()
    except OSError:
        return
    if '"--' not in text or not ("argc" in text or "argv" in text):
        return
    raw_lines = text.split("\n")
    literals, comments = lex(text)

    # Every literal that is exactly a flag; remember each line that reads it.
    sites = {}
    for ln, value in literals:
        if FLAG_LITERAL.match(value):
            sites.setdefault(value, []).append(ln)
            if value not in args:
                args[value] = Arg(value, rel, ln, raw_lines[ln - 1].strip())
    if not sites:
        return

    blocks = comment_blocks(comments)
    block_at = {}                      # line -> its block
    for block in blocks:
        for ln, _body in block:
            block_at[ln] = block

    # Usage lines and passing mentions, from every comment in the file.
    for block in blocks:
        for kind, head, lines in split_block(block):
            text_ = reflow(lines)
            heads = set(FLAG_TOKEN.findall(head)) if kind == "usage" else set()
            for name in heads:
                if name in args and (head, text_) not in args[name].usage:
                    args[name].usage.append((head, text_))
            mentioned = set(FLAG_TOKEN.findall(" ".join(lines))) - heads
            for name in mentioned:
                if name in args:
                    shown = (head + "   " + text_) if head else text_
                    if shown not in args[name].mentions:
                        args[name].mentions.append(shown)

    # Notes: comments on, right above, or opening the branch of a line that
    # reads the flag - unless they are usage lines (already taken above).
    def add_note(name, block):
        if block is None:
            return
        units = split_block(block)
        if any(kind == "usage" for kind, _h, _l in units):
            return
        text_ = reflow([l for _k, _h, ls in units for l in ls])
        if text_ and text_ not in args[name].notes:
            args[name].notes.append(text_)

    for name, lines in sites.items():
        for ln in lines:
            if ln in comments and not comments[ln][1]:
                add_note(name, block_at.get(ln))                 # same line
            above = ln - 1
            if above in comments and comments[above][1]:
                add_note(name, block_at.get(above))              # right above
            below = ln + 1                                       # opening the branch
            opens = raw_lines[ln - 1].rstrip().endswith("{")
            while below <= len(raw_lines) and raw_lines[below - 1].strip() == "{":
                below += 1
                opens = True
            if opens and below in comments and comments[below][1]:
                add_note(name, block_at.get(below))


def scan_game(game_folder):
    """Every argument the game's source reads, sorted by name."""
    args = {}
    for path in source_files(game_folder):
        scan_file(path, os.path.relpath(path, game_folder).replace("\\", "/"), args)
    return sorted(args.values(), key=lambda a: a.name.lstrip("-").lower())


# ============================== command line =================================

def main():
    if len(sys.argv) < 2:
        print("usage: python gameArgs.py <game folder name or path>")
        return 1
    target = sys.argv[1]
    if not os.path.isdir(target):
        here = os.path.dirname(os.path.abspath(__file__))
        target = os.path.join(os.path.dirname(os.path.dirname(here)), target)
    for a in scan_game(target):
        print("%s   (%s:%d)" % (a.name, a.path, a.line))
        for head, text in a.usage:
            print("    %s" % head)
            if text:
                print("        " + text.replace("\n", "\n        "))
        for note in a.notes:
            print("    note: " + note.replace("\n", "\n          "))
        if not a.usage and not a.notes:
            for m in a.mentions:
                print("    mentioned: " + m.replace("\n", "\n               "))
        if not a.described():
            print("    code: " + a.code)
    return 0


if __name__ == "__main__":
    sys.exit(main())
