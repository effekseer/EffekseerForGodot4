import sys
import os
import shutil
import subprocess
import multiprocessing

script_path = os.path.abspath(__file__)

def replace_word(file_name, target_str, replace_str):
    text = ""
    with open(file_name, "r") as file:
        text = file.read()

    text = text.replace(target_str, replace_str)

    with open(file_name, "w") as file:
        file.write(text)

def import_generate_bindings():
    binding_generator = __import__("godot-cpp.binding_generator").binding_generator
    cwd = os.getcwd()
    os.chdir(os.path.join(os.path.dirname(script_path), "godot-cpp"))
    binding_generator.generate_bindings("gdextension/extension_api.json", False)
    os.chdir(cwd)

import_generate_bindings()

os.chdir(os.path.dirname(script_path))

if "platform=windows" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/windows", exist_ok = True)

    subprocess.run("scons platform=windows arch=x86_32 target=template_release", shell = True, check=True)
    subprocess.run("scons platform=windows arch=x86_64 target=template_release", shell = True, check=True)

elif "platform=macos" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/macos", exist_ok = True)

    subprocess.run("scons platform=macos arch=x86_64 target=template_release", shell = True, check=True)
    
    subprocess.run("lipo -create ../Godot/addons/effekseer/bin/macos/libeffekseer.x86_64.dylib -output ../Godot/addons/effekseer/bin/macos/libeffekseer.dylib", shell = True, check=True)
    os.remove("../Godot/addons/effekseer/bin/macos/libeffekseer.x86_64.dylib")

elif "platform=android" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/android", exist_ok = True)

    subprocess.run("scons platform=android arch=arm32 target=template_release", shell = True, check=True)
    subprocess.run("scons platform=android arch=arm64 target=template_release", shell = True, check=True)
    subprocess.run("scons platform=android arch=x86_32 target=template_release", shell = True, check=True)
    subprocess.run("scons platform=android arch=x86_64 target=template_release", shell = True, check=True)

elif "platform=ios" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/ios", exist_ok = True)

    subprocess.run("scons platform=ios arch=arm64 target=template_release", shell = True, check=True)

    subprocess.run("lipo -create ../Godot/addons/effekseer/bin/ios/libeffekseer.arm64.dylib -output ../Godot/addons/effekseer/bin/ios/libeffekseer.dylib", shell = True, check=True)
    os.remove("../Godot/addons/effekseer/bin/ios/libeffekseer.arm64.dylib")

elif "platform=linux" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/linux", exist_ok = True)

    subprocess.run("scons platform=linux arch=x86_32 target=template_release", shell = True, check=True)
    subprocess.run("scons platform=linux arch=x86_64 target=template_release", shell = True, check=True)

elif "platform=web" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/web", exist_ok = True)

    subprocess.run("scons platform=web arch=wasm32 target=template_release", shell = True, check=True)
    os.rename("../Godot/addons/effekseer/bin/web/libeffekseer.wasm32.wasm", "../Godot/addons/effekseer/bin/web/libeffekseer.wasm")
