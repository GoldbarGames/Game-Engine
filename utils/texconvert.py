"""texconvert.py - convert images to KTX2 textures for the engine.

A KTX2 file holds a texture's whole mip chain, built here (sRGB-correct), and
usually GPU-compressed: it loads without decoding, takes a quarter of the video
memory (BC7) or less, and samples faster. The engine loads `name.ktx2` in place
of `name.png` whenever it sits beside it and is at least as new, so a project
switches to compressed textures by converting its files - no scene or code
changes (src/ENGINE/render/TextureFiles.h).

Usage:
    python texconvert.py <image or folder> [more ...] [options]
    python texconvert.py --info <file.ktx2>
    python texconvert.py --decode <file.ktx2> <out.png> [--level N]

Each image becomes <name>.ktx2 beside it (or -o <file> for a single image).
A folder converts every .png/.jpg/.jpeg/.tga/.bmp in it (add --recursive for
sub-folders).

Kinds (--kind, default auto: from the file name):
    color   BC7, sRGB. Albedo / base colour / emissive / UI art. (default)
    data    BC7, linear. Packed data maps: ORM (occlusion, roughness,
            metallic), masks. Names ending _orm _arm _mr _rough(ness)
            _metal(lic/ness) _ao _occlusion _mask _height.
    normal  BC5, linear: only x and y are stored (z is rebuilt in the shader);
            mips are renormalised. Names ending _n _nrm _norm _normal(s).
            Tangent-space, OpenGL convention (+Y up: green = up), as glTF uses.

Options:
    --format bc7|bc5|bc4|rgba8   override the kind's format (bc4 = R channel only;
                                 rgba8 = uncompressed, still with mips)
    --no-mips                    store the full-size level only
    --verify                     decode the result and print its PSNR
    --force                      convert even if the .ktx2 is newer than the image
    -o <file>                    output path (one input only)

The BC7 encoder uses mode 6 (one colour line per 4x4 block, 4-bit weights):
good for most art, weaker where a block holds three or more distinct colours.
For the best quality, any encoder that writes plain (not supercompressed)
BC7/BC5/BC4 KTX2 files works too, e.g. AMD Compressonator or NVIDIA Texture
Tools.
"""

import argparse
import os
import struct
import sys

import numpy as np
from PIL import Image

KTX2_ID = bytes([0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A])

VK_R8G8B8A8_UNORM = 37
VK_R8G8B8A8_SRGB = 43
VK_BC4_UNORM = 139
VK_BC5_UNORM = 141
VK_BC7_UNORM = 145
VK_BC7_SRGB = 146

VK_NAMES = {37: 'R8G8B8A8_UNORM', 43: 'R8G8B8A8_SRGB', 131: 'BC1_RGB_UNORM', 132: 'BC1_RGB_SRGB',
            133: 'BC1_RGBA_UNORM', 134: 'BC1_RGBA_SRGB', 137: 'BC3_UNORM', 138: 'BC3_SRGB',
            139: 'BC4_UNORM', 141: 'BC5_UNORM', 145: 'BC7_UNORM', 146: 'BC7_SRGB'}

IMAGE_EXTS = ('.png', '.jpg', '.jpeg', '.tga', '.bmp')

NORMAL_SUFFIXES = ('_n', '_nrm', '_norm', '_normal', '_normals', '-normal')
DATA_SUFFIXES = ('_orm', '_arm', '_mr', '_rough', '_roughness', '_metal', '_metallic', '_metalness',
                 '_ao', '_occlusion', '_mask', '_height')

# BC7 4-bit interpolation weights (out of 64).
W4 = np.array([0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64], np.int32)

CHUNK = 4096   # blocks encoded at once (bounds memory)


# ----------------------------------------------------------------- colour

def srgb_to_linear(c):
    c = c / 255.0
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def linear_to_srgb(c):
    c = np.clip(c, 0.0, 1.0)
    return np.where(c <= 0.0031308, c * 12.92, 1.055 * np.power(c, 1.0 / 2.4) - 0.055) * 255.0


# ----------------------------------------------------------------- mips

def half(img):
    """2x2 box average of a float (H, W, C) image; odd edges repeat."""
    h, w = img.shape[:2]
    if h > 1 and h % 2:
        img = np.concatenate([img, img[-1:]], 0)
    if w > 1 and w % 2:
        img = np.concatenate([img, img[:, -1:]], 1)
    h, w = img.shape[:2]
    if h > 1:
        img = 0.5 * (img[0::2] + img[1::2])
    if w > 1:
        img = 0.5 * (img[:, 0::2] + img[:, 1::2])
    return img


def build_mips(rgba, kind, mips):
    """Mip chain (list of uint8 (H, W, 4)) of an RGBA uint8 image, filtered in
    the right space for its kind."""
    levels = [rgba]
    if not mips:
        return levels
    if kind == 'normal':
        v = rgba[..., :3].astype(np.float64) / 255.0 * 2.0 - 1.0
        cur = v
        while cur.shape[0] > 1 or cur.shape[1] > 1:
            cur = half(cur)
            n = cur / np.maximum(np.linalg.norm(cur, axis=-1, keepdims=True), 1e-8)
            enc = np.clip(np.round((n * 0.5 + 0.5) * 255.0), 0, 255).astype(np.uint8)
            alpha = np.full(enc.shape[:2] + (1,), 255, np.uint8)
            levels.append(np.concatenate([enc, alpha], -1))
        return levels

    a = rgba[..., 3:4].astype(np.float64) / 255.0
    has_alpha = bool((rgba[..., 3] < 255).any())
    if kind == 'color':
        c = srgb_to_linear(rgba[..., :3].astype(np.float64))
    else:
        c = rgba[..., :3].astype(np.float64) / 255.0
    # Colour weighted by alpha, so transparent texels don't bleed their
    # (often black) colour into the visible edge of a cut-out.
    cur = np.concatenate([c * a, a], -1) if has_alpha else np.concatenate([c, a], -1)
    while cur.shape[0] > 1 or cur.shape[1] > 1:
        cur = half(cur)
        alpha = cur[..., 3:4]
        col = cur[..., :3]
        if has_alpha:
            col = np.where(alpha > 1e-6, col / np.maximum(alpha, 1e-6), 0.0)
        if kind == 'color':
            col = linear_to_srgb(col)
        else:
            col = np.clip(col, 0.0, 1.0) * 255.0
        level = np.concatenate([col, alpha * 255.0], -1)
        levels.append(np.clip(np.round(level), 0, 255).astype(np.uint8))
    return levels


# ----------------------------------------------------------------- blocks

def to_blocks(img):
    """(H, W, C) -> (N, 16, C) 4x4 blocks in row order, edges repeated to fill."""
    h, w = img.shape[:2]
    ph, pw = (-h) % 4, (-w) % 4
    if ph or pw:
        img = np.pad(img, ((0, ph), (0, pw), (0, 0)), mode='edge')
    H, W, C = img.shape
    b = img.reshape(H // 4, 4, W // 4, 4, C).transpose(0, 2, 1, 3, 4)
    return b.reshape(-1, 16, C)


def from_blocks(blocks, w, h):
    bw, bh = (w + 3) // 4, (h + 3) // 4
    C = blocks.shape[-1]
    img = blocks.reshape(bh, bw, 4, 4, C).transpose(0, 2, 1, 3, 4).reshape(bh * 4, bw * 4, C)
    return img[:h, :w]


# ----------------------------------------------------------------- BC7 mode 6

def bc7_palette(v0, v1):
    """(N, 4) 8-bit endpoints -> (N, 16, 4) interpolated colours, exactly as the GPU does."""
    w = W4[None, :, None]
    return ((64 - w) * v0[:, None, :] + w * v1[:, None, :] + 32) >> 6


def bc7_quantize(e):
    """Float endpoint (N, 4) -> 7-bit colour + shared p-bit closest to it."""
    best_c = best_p = best_err = None
    for p in (0, 1):
        c = np.clip(np.round((e - p) / 2.0), 0, 127).astype(np.int32)
        err = ((c * 2 + p - e) ** 2).sum(1)
        if best_err is None:
            best_c, best_p, best_err = c, np.full(len(e), p, np.int32), err
        else:
            better = err < best_err
            best_c = np.where(better[:, None], c, best_c)
            best_p = np.where(better, p, best_p)
            best_err = np.minimum(err, best_err)
    return best_c, best_p


def bc7_fit(px, c0, p0, c1, p1):
    v0 = c0 * 2 + p0[:, None]
    v1 = c1 * 2 + p1[:, None]
    pal = bc7_palette(v0, v1)
    d = ((px[:, :, None, :] - pal[:, None, :, :]) ** 2).sum(-1)
    idx = d.argmin(-1)
    err = np.take_along_axis(d, idx[..., None], -1)[..., 0].sum(-1)
    return idx, err


def bc7_encode_chunk(blocks):
    px = blocks.astype(np.int32)
    pf = blocks.astype(np.float64)
    n = len(blocks)
    mean = pf.mean(1)
    cen = pf - mean[:, None, :]
    cov = np.einsum('nki,nkj->nij', cen, cen)
    # Principal axis by power iteration, started from the widest channel.
    v = np.zeros((n, 4))
    v[np.arange(n), np.argmax(np.einsum('nii->ni', cov), 1)] = 1.0
    for _ in range(8):
        v = np.einsum('nij,nj->ni', cov, v)
        v /= np.maximum(np.linalg.norm(v, axis=1, keepdims=True), 1e-12)
    t = np.einsum('nki,ni->nk', cen, v)
    e0 = np.clip(mean + v * t.min(1)[:, None], 0, 255)
    e1 = np.clip(mean + v * t.max(1)[:, None], 0, 255)

    c0, p0 = bc7_quantize(e0)
    c1, p1 = bc7_quantize(e1)
    idx, err = bc7_fit(px, c0, p0, c1, p1)

    # Least-squares refinement of the endpoints for the chosen weights.
    for _ in range(2):
        w = W4[idx] / 64.0
        a = ((1 - w) ** 2).sum(1)
        b = ((1 - w) * w).sum(1)
        c = (w ** 2).sum(1)
        r0 = np.einsum('nk,nkc->nc', 1 - w, pf)
        r1 = np.einsum('nk,nkc->nc', w, pf)
        det = a * c - b * b
        ok = np.abs(det) > 1e-6
        sd = np.where(ok, det, 1.0)[:, None]
        n0 = np.clip((c[:, None] * r0 - b[:, None] * r1) / sd, 0, 255)
        n1 = np.clip((a[:, None] * r1 - b[:, None] * r0) / sd, 0, 255)
        nc0, np0 = bc7_quantize(n0)
        nc1, np1 = bc7_quantize(n1)
        nidx, nerr = bc7_fit(px, nc0, np0, nc1, np1)
        better = ok & (nerr < err)
        c0 = np.where(better[:, None], nc0, c0)
        c1 = np.where(better[:, None], nc1, c1)
        p0 = np.where(better, np0, p0)
        p1 = np.where(better, np1, p1)
        idx = np.where(better[:, None], nidx, idx)
        err = np.where(better, nerr, err)

    # The first texel's index has an implied 0 top bit: swap the endpoints
    # (and invert every index) where it is 8 or more.
    swap = idx[:, 0] >= 8
    c0, c1 = np.where(swap[:, None], c1, c0), np.where(swap[:, None], c0, c1)
    p0, p1 = np.where(swap, p1, p0), np.where(swap, p0, p1)
    idx = np.where(swap[:, None], 15 - idx, idx)

    u = np.uint64
    lo = np.full(n, 1 << 6, np.uint64)   # mode 6
    shift = 7
    for ch in range(4):
        for cc in (c0, c1):
            lo |= cc[:, ch].astype(np.uint64) << u(shift)
            shift += 7
    lo |= p0.astype(np.uint64) << u(63)
    hi = p1.astype(np.uint64)
    hi |= idx[:, 0].astype(np.uint64) << u(1)
    shift = 4
    for i in range(1, 16):
        hi |= idx[:, i].astype(np.uint64) << u(shift)
        shift += 4
    return np.stack([lo, hi], 1).astype('<u8').tobytes()


def bc7_decode(data, w, h):
    q = np.frombuffer(data, '<u8').reshape(-1, 2)
    lo, hi = q[:, 0], q[:, 1]
    if np.any((lo & np.uint64(0x7F)) != np.uint64(0x40)):
        raise ValueError('only BC7 mode 6 blocks can be decoded here (this file uses other modes)')
    u = np.uint64
    vals = []
    shift = 7
    for _ in range(8):
        vals.append(((lo >> u(shift)) & u(0x7F)).astype(np.int32))
        shift += 7
    p0 = ((lo >> u(63)) & u(1)).astype(np.int32)
    p1 = (hi & u(1)).astype(np.int32)
    v0 = np.stack([vals[0], vals[2], vals[4], vals[6]], 1) * 2 + p0[:, None]
    v1 = np.stack([vals[1], vals[3], vals[5], vals[7]], 1) * 2 + p1[:, None]
    idx = [((hi >> u(1)) & u(7)).astype(np.int32)]
    shift = 4
    for _ in range(1, 16):
        idx.append(((hi >> u(shift)) & u(15)).astype(np.int32))
        shift += 4
    idx = np.stack(idx, 1)
    pal = bc7_palette(v0, v1)
    px = np.take_along_axis(pal, idx[:, :, None].repeat(4, 2), 1)
    return from_blocks(px.astype(np.uint8), w, h)


# ----------------------------------------------------------------- BC4 / BC5

def bc4_palette(e0, e1):
    """(N,) endpoints (e0 > e1: eight-value mode) -> (N, 8) values."""
    cols = [e0, e1] + [((8 - i) * e0 + (i - 1) * e1) / 7.0 for i in range(2, 8)]
    return np.stack(cols, 1)


def bc4_encode_chunk(vals):
    v = vals.astype(np.float64)
    e0 = v.max(1)
    e1 = v.min(1)
    flat = e0 == e1
    pal = bc4_palette(e0, e1)
    idx = np.abs(v[:, :, None] - pal[:, None, :]).argmin(-1)
    err = ((np.take_along_axis(pal, idx, 1) - v) ** 2).sum(1)

    # Least-squares refinement of the endpoints (kept where it helps and the
    # eight-value ordering e0 > e1 still holds).
    wt = np.array([0, 1, 1 / 7, 2 / 7, 3 / 7, 4 / 7, 5 / 7, 6 / 7])[idx]
    a = ((1 - wt) ** 2).sum(1)
    b = ((1 - wt) * wt).sum(1)
    c = (wt ** 2).sum(1)
    r0 = ((1 - wt) * v).sum(1)
    r1 = (wt * v).sum(1)
    det = a * c - b * b
    ok = np.abs(det) > 1e-6
    sd = np.where(ok, det, 1.0)
    n0 = np.clip(np.round((c * r0 - b * r1) / sd), 0, 255)
    n1 = np.clip(np.round((a * r1 - b * r0) / sd), 0, 255)
    ok &= n0 > n1
    npal = bc4_palette(n0, n1)
    nidx = np.abs(v[:, :, None] - npal[:, None, :]).argmin(-1)
    nerr = ((np.take_along_axis(npal, nidx, 1) - v) ** 2).sum(1)
    better = ok & (nerr < err)
    e0 = np.where(better, n0, e0)
    e1 = np.where(better, n1, e1)
    idx = np.where(better[:, None], nidx, idx)
    idx = np.where(flat[:, None], 0, idx)

    bits = np.zeros(len(v), np.uint64)
    for i in range(16):
        bits |= idx[:, i].astype(np.uint64) << np.uint64(3 * i)
    out = np.zeros((len(v), 8), np.uint8)
    out[:, 0] = e0.astype(np.uint8)
    out[:, 1] = e1.astype(np.uint8)
    for k in range(6):
        out[:, 2 + k] = ((bits >> np.uint64(8 * k)) & np.uint64(0xFF)).astype(np.uint8)
    return out


def bc4_decode_blocks(raw):
    """(N, 8) uint8 -> (N, 16) values."""
    e0 = raw[:, 0].astype(np.float64)
    e1 = raw[:, 1].astype(np.float64)
    bits = np.zeros(len(raw), np.uint64)
    for k in range(6):
        bits |= raw[:, 2 + k].astype(np.uint64) << np.uint64(8 * k)
    idx = np.stack([((bits >> np.uint64(3 * i)) & np.uint64(7)).astype(np.int64) for i in range(16)], 1)
    eight = bc4_palette(e0, e1)
    six = np.stack([e0, e1] + [((6 - i) * e0 + (i - 1) * e1) / 5.0 for i in range(2, 6)]
                   + [np.zeros_like(e0), np.full_like(e0, 255.0)], 1)
    pal = np.where((e0 > e1)[:, None], eight, six)
    return np.clip(np.round(np.take_along_axis(pal, idx, 1)), 0, 255).astype(np.uint8)


def encode_level(img, fmt):
    """uint8 (H, W, 4) -> bytes in `fmt`."""
    if fmt == 'rgba8':
        return np.ascontiguousarray(img).tobytes()
    blocks = to_blocks(img)
    out = []
    for s in range(0, len(blocks), CHUNK):
        chunk = blocks[s:s + CHUNK]
        if fmt == 'bc7':
            out.append(bc7_encode_chunk(chunk))
        elif fmt == 'bc4':
            out.append(bc4_encode_chunk(chunk[:, :, 0]).tobytes())
        elif fmt == 'bc5':
            r = bc4_encode_chunk(chunk[:, :, 0])
            g = bc4_encode_chunk(chunk[:, :, 1])
            out.append(np.concatenate([r, g], 1).tobytes())
    return b''.join(out)


def decode_level(data, vk, w, h):
    """Bytes of one level -> uint8 (H, W, 4)."""
    if vk in (VK_R8G8B8A8_UNORM, VK_R8G8B8A8_SRGB):
        return np.frombuffer(data, np.uint8)[:w * h * 4].reshape(h, w, 4)
    if vk in (VK_BC7_UNORM, VK_BC7_SRGB):
        return bc7_decode(data, w, h)
    if vk == VK_BC4_UNORM:
        raw = np.frombuffer(data, np.uint8).reshape(-1, 8)
        r = bc4_decode_blocks(raw)
        px = np.stack([r, r, r, np.full_like(r, 255)], -1)
        return from_blocks(px, w, h)
    if vk == VK_BC5_UNORM:
        raw = np.frombuffer(data, np.uint8).reshape(-1, 16)
        r = bc4_decode_blocks(raw[:, :8])
        g = bc4_decode_blocks(raw[:, 8:])
        x = r / 255.0 * 2 - 1
        y = g / 255.0 * 2 - 1
        z = np.sqrt(np.clip(1 - x * x - y * y, 0, 1))
        b = np.clip(np.round((z * 0.5 + 0.5) * 255), 0, 255).astype(np.uint8)
        px = np.stack([r, g, b, np.full_like(r, 255)], -1)
        return from_blocks(px, w, h)
    raise ValueError('decoding %s is not supported here' % VK_NAMES.get(vk, vk))


# ----------------------------------------------------------------- KTX2

def dfd_for(vk):
    """Khronos data format descriptor (a basic block) for the formats written here."""
    def sample(bit_offset, bit_length, channel, upper, linear=False):
        ctype = channel | (0x10 if linear else 0)
        return struct.pack('<IIII', bit_offset | ((bit_length - 1) << 16) | (ctype << 24), 0, 0, upper)

    srgb = vk in (VK_R8G8B8A8_SRGB, VK_BC7_SRGB)
    transfer = 2 if srgb else 1
    if vk in (VK_R8G8B8A8_UNORM, VK_R8G8B8A8_SRGB):
        model, dims, plane = 1, (0, 0, 0, 0), 4
        samples = [sample(0, 8, 0, 255), sample(8, 8, 1, 255), sample(16, 8, 2, 255),
                   sample(24, 8, 15, 255, linear=srgb)]
    elif vk in (VK_BC7_UNORM, VK_BC7_SRGB):
        model, dims, plane = 134, (3, 3, 0, 0), 16
        samples = [sample(0, 128, 0, 0xFFFFFFFF)]
    elif vk == VK_BC5_UNORM:
        model, dims, plane = 132, (3, 3, 0, 0), 16
        samples = [sample(0, 64, 0, 0xFFFFFFFF), sample(64, 64, 1, 0xFFFFFFFF)]
    elif vk == VK_BC4_UNORM:
        model, dims, plane = 131, (3, 3, 0, 0), 8
        samples = [sample(0, 64, 0, 0xFFFFFFFF)]
    else:
        raise ValueError(vk)
    block_size = 24 + 16 * len(samples)
    body = struct.pack('<IHHBBBB', 0, 2, block_size, model, 1, transfer, 0)
    body += bytes(dims) + bytes([plane, 0, 0, 0, 0, 0, 0, 0])
    body += b''.join(samples)
    return struct.pack('<I', 4 + len(body)) + body


def align(n, a):
    return (n + a - 1) // a * a


def write_ktx2(path, vk, w, h, levels):
    """levels: bytes per mip level, base first."""
    block = {VK_BC4_UNORM: 8}.get(vk, 16 if vk in (VK_BC5_UNORM, VK_BC7_UNORM, VK_BC7_SRGB) else 4)
    level_align = block if block % 4 == 0 else block * 4 // np.gcd(block, 4)
    dfd = dfd_for(vk)
    kv_key = b'KTXwriter\x00Goldbar texconvert.py\x00'
    kvd = struct.pack('<I', len(kv_key)) + kv_key
    kvd += b'\x00' * (align(len(kvd), 4) - len(kvd))

    header_len = 12 + 9 * 4 + 4 * 4 + 2 * 8
    index_len = 24 * len(levels)
    dfd_off = header_len + index_len
    kvd_off = dfd_off + len(dfd)
    pos = kvd_off + len(kvd)

    # Level data is stored smallest first, each start aligned.
    offsets = [0] * len(levels)
    blob = bytearray()
    for i in reversed(range(len(levels))):
        start = align(pos + len(blob), level_align)
        blob += b'\x00' * (start - pos - len(blob))
        offsets[i] = start
        blob += levels[i]

    out = bytearray(KTX2_ID)
    out += struct.pack('<9I', vk, 1, w, h, 0, 0, 1, len(levels), 0)
    out += struct.pack('<4I2Q', dfd_off, len(dfd), kvd_off, len(kvd), 0, 0)
    for i, data in enumerate(levels):
        out += struct.pack('<3Q', offsets[i], len(data), len(data))
    out += dfd + kvd
    out += blob
    with open(path, 'wb') as f:
        f.write(out)


def read_ktx2(path):
    data = open(path, 'rb').read()
    if data[:12] != KTX2_ID:
        raise ValueError('%s is not a KTX2 file' % path)
    vk, type_size, w, h, d, layers, faces, nlevels, scheme = struct.unpack_from('<9I', data, 12)
    dfd_off, dfd_len, kvd_off, kvd_len, sgd_off, sgd_len = struct.unpack_from('<4I2Q', data, 48)
    levels = []
    for i in range(max(nlevels, 1)):
        off, length, ulen = struct.unpack_from('<3Q', data, 80 + 24 * i)
        levels.append(data[off:off + length])
    info = dict(vk=vk, width=w, height=h, depth=d, layers=layers, faces=faces, levels=nlevels,
                supercompression=scheme)
    if dfd_len >= 16:
        word2 = struct.unpack_from('<I', data, dfd_off + 4 + 8)[0]
        info['color_model'] = word2 & 0xFF
        info['transfer'] = {1: 'linear', 2: 'sRGB'}.get((word2 >> 16) & 0xFF, (word2 >> 16) & 0xFF)
    return info, levels


# ----------------------------------------------------------------- driver

def guess_kind(path):
    stem = os.path.splitext(os.path.basename(path))[0].lower()
    if stem.endswith(NORMAL_SUFFIXES):
        return 'normal'
    if stem.endswith(DATA_SUFFIXES):
        return 'data'
    return 'color'


def psnr(a, b):
    mse = np.mean((a.astype(np.float64) - b.astype(np.float64)) ** 2)
    return float('inf') if mse == 0 else 10 * np.log10(255.0 ** 2 / mse)


def convert(src, dst, kind, fmt, mips, verify):
    img = Image.open(src)
    rgba = np.array(img.convert('RGBA'))
    h, w = rgba.shape[:2]
    if kind == 'auto':
        kind = guess_kind(src)
    if fmt is None:
        fmt = {'color': 'bc7', 'data': 'bc7', 'normal': 'bc5'}[kind]
    if fmt == 'bc7':
        vk = VK_BC7_SRGB if kind == 'color' else VK_BC7_UNORM
    elif fmt == 'rgba8':
        vk = VK_R8G8B8A8_SRGB if kind == 'color' else VK_R8G8B8A8_UNORM
    elif fmt == 'bc5':
        vk = VK_BC5_UNORM
    else:
        vk = VK_BC4_UNORM

    chain = build_mips(rgba, kind, mips)
    levels = [encode_level(level, fmt) for level in chain]
    write_ktx2(dst, vk, w, h, levels)

    raw = w * h * 4 * 4 // 3 if mips else w * h * 4
    size = sum(len(x) for x in levels)
    line = '%s -> %s  %dx%d %s %s, %d level%s, %.1f KB (RGBA8 with mips: %.1f KB)' % (
        src, dst, w, h, kind, VK_NAMES[vk], len(levels), '' if len(levels) == 1 else 's', size / 1024.0,
        raw / 1024.0)
    if verify:
        dec = decode_level(levels[0], vk, w, h)
        if fmt == 'bc5':
            line += '  PSNR(xy) %.1f dB' % psnr(dec[..., :2], chain[0][..., :2])
        elif fmt == 'bc4':
            line += '  PSNR(r) %.1f dB' % psnr(dec[..., 0], chain[0][..., 0])
        else:
            line += '  PSNR %.1f dB' % psnr(dec, chain[0])
    print(line)


def gather(inputs, recursive):
    files = []
    for path in inputs:
        if os.path.isdir(path):
            for root, dirs, names in os.walk(path):
                for name in sorted(names):
                    if name.lower().endswith(IMAGE_EXTS):
                        files.append(os.path.join(root, name))
                if not recursive:
                    break
        else:
            files.append(path)
    return files


def main():
    ap = argparse.ArgumentParser(description='Convert images to KTX2 (BC7/BC5/BC4) textures for the engine.')
    ap.add_argument('inputs', nargs='*')
    ap.add_argument('--kind', choices=['auto', 'color', 'data', 'normal'], default='auto')
    ap.add_argument('--format', choices=['bc7', 'bc5', 'bc4', 'rgba8'])
    ap.add_argument('--no-mips', action='store_true')
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--recursive', action='store_true')
    ap.add_argument('-o', '--output')
    ap.add_argument('--info', metavar='KTX2')
    ap.add_argument('--decode', nargs=2, metavar=('KTX2', 'PNG'))
    ap.add_argument('--level', type=int, default=0)
    args = ap.parse_args()

    if args.info:
        info, levels = read_ktx2(args.info)
        print('%s: %s %dx%d, %d level(s), supercompression %d, transfer %s' % (
            args.info, VK_NAMES.get(info['vk'], info['vk']), info['width'], info['height'], info['levels'],
            info['supercompression'], info.get('transfer', '?')))
        for i, data in enumerate(levels):
            print('  level %d: %dx%d, %d bytes' % (i, max(1, info['width'] >> i), max(1, info['height'] >> i),
                                                   len(data)))
        return 0
    if args.decode:
        info, levels = read_ktx2(args.decode[0])
        lw, lh = max(1, info['width'] >> args.level), max(1, info['height'] >> args.level)
        Image.fromarray(decode_level(levels[args.level], info['vk'], lw, lh), 'RGBA').save(args.decode[1])
        print('%s level %d -> %s' % (args.decode[0], args.level, args.decode[1]))
        return 0

    files = gather(args.inputs, args.recursive)
    if not files:
        ap.print_help()
        return 1
    if args.output and len(files) != 1:
        print('-o needs exactly one input image')
        return 1
    for src in files:
        dst = args.output or os.path.splitext(src)[0] + '.ktx2'
        if not args.force and os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
            print('%s is up to date' % dst)
            continue
        convert(src, dst, args.kind, args.format, not args.no_mips, args.verify)
    return 0


if __name__ == '__main__':
    sys.exit(main())
