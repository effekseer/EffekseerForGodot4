import os
import shutil
import urllib.request
import json
import zipfile

if os.path.isdir("addons"):
    shutil.rmtree("addons")

with urllib.request.urlopen('https://api.github.com/repos/effekseer/EffekseerForGodot4/releases') as response:
    releases_info = json.loads(response.read())

latest_plugin_url = releases_info[0]["assets"][0]["browser_download_url"]
print(f"Latest plugin package URL: {latest_plugin_url}")

package_name, message = urllib.request.urlretrieve(latest_plugin_url)

with zipfile.ZipFile(package_name, "r") as zip:
    all_files = zip.infolist()
    target_files = [file for file in all_files if "addons/" in file.filename]

    for file in target_files:
        dest_path = file.filename[file.filename.index("addons/"):]
        if dest_path.endswith("/"):
            os.makedirs(dest_path)
        else:
            with open(dest_path, "wb") as f:
                f.write(zip.read(file.filename))
