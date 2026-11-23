import urllib.request
import os
import ssl

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

vendor_dir = r"D:\NIT Trichy Campus Route Planner\gui\vendor"
os.makedirs(vendor_dir, exist_ok=True)

files = [
    ("https://unpkg.com/leaflet@1.9.4/dist/leaflet.js", "leaflet.js"),
    ("https://unpkg.com/leaflet@1.9.4/dist/leaflet.css", "leaflet.css")
]

for url, filename in files:
    dest = os.path.join(vendor_dir, filename)
    print(f"Downloading {url} to {dest}...")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=15, context=ctx) as resp:
            content = resp.read()
            with open(dest, "wb") as f:
                f.write(content)
            print(f"Successfully downloaded {filename} ({len(content)} bytes)")
    except Exception as e:
        print(f"Failed to download {url}: {e}")
