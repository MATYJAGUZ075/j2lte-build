import json, sys, hashlib, re

def main():
    if len(sys.argv) < 3:
        print("usage: gen_manifest.py <macho_dump.txt> <kernel_path> [out.json]", file=sys.stderr)
        sys.exit(1)
    dump_path = sys.argv[1]
    kernel_path = sys.argv[2]
    out_path = sys.argv[3] if len(sys.argv) > 3 else "manifest.json"
    text = open(dump_path).read()
    segs = []
    for line in text.splitlines():
        m = re.match(r"SEG\s+\d+\s+vmaddr=(0x[0-9a-fA-F]+)\s+vmsize=(0x[0-9a-fA-F]+)\s+fileoff=(0x[0-9a-fA-F]+)\s+filesize=(0x[0-9a-fA-F]+)", line)
        if m:
            segs.append({"vmaddr": m.group(1), "vmsize": m.group(2), "fileoff": m.group(3), "filesize": m.group(4)})
    fs = re.search(r"filesize=(\d+)", text)
    filesize = int(fs.group(1)) if fs else None
    h = hashlib.sha256()
    with open(kernel_path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    manifest = {
        "kernel_sha256": h.hexdigest(),
        "kernel_filesize": filesize,
        "segments": segs,
        "note": "VA only; PA mapping not demonstrated"
    }
    with open(out_path, "w") as f:
        json.dump(manifest, f, indent=2)
    # also emit a C header for memplan consumption
    hdr_path = os.path.join(os.path.dirname(out_path), "manifest_values.h")
    with open(hdr_path, "w") as f:
        f.write("#ifndef MANIFEST_VALUES_H\n#define MANIFEST_VALUES_H\n")
        f.write("#define MANIFEST_DT_SIZE %d\n" % (segs and 0 or 0))
        # dt size comes from apple dt serializer, not macho; keep 0 here
        f.write("#define MANIFEST_KERNEL_FILESIZE %d\n" % (filesize or 0))
        f.write("#define MANIFEST_KERNEL_SHA256 \"%s\"\n" % h.hexdigest())
        f.write("#endif\n")
    print(json.dumps(manifest, indent=2))

if __name__ == "__main__":
    main()
