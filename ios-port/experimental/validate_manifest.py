import json, os, sys

def main():
    bd = os.environ.get("BUILD_DIR", "")
    m = json.load(open(os.path.join(bd, "manifest.json")))
    assert "kernel_sha256" in m and len(m["kernel_sha256"]) == 64
    assert "segments" in m and len(m["segments"]) > 0
    prev = None
    for s in m["segments"]:
        va = int(s["vmaddr"], 16); vs = int(s["vmsize"], 16)
        assert vs > 0, "zero vmsize"
        if prev: assert va >= prev[0] + prev[1], "overlapping segments"
        prev = (va, vs)
    print("manifest OK: %d segments, sha=%s" % (len(m["segments"]), m["kernel_sha256"][:12]))

if __name__ == "__main__":
    main()
