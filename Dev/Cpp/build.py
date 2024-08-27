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

def import_generate_bindings(arch: str):
    bits = "32" if "32" in arch else "64"
    binding_generator = __import__("godot-cpp.binding_generator").binding_generator
    cwd = os.getcwd()
    os.chdir(os.path.join(os.path.dirname(script_path), "godot-cpp"))
    binding_generator.generate_bindings("gdextension/extension_api.json", False, bits=bits)
    os.chdir(cwd)

os.chdir(os.path.dirname(script_path))

is_debug = False
options = []

if "debug=yes" in sys.argv:
    options.append("target=template_debug")
    options.append("dev_build=yes")
    is_debug = True

if "platform=windows" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/windows", exist_ok = True)

    for arch in ["x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "platform=windows", f"arch={arch}"] + options, check=True)

    if is_debug:
        shutil.copy2("bin/windows/libeffekseer.x86_32.pdb", "../Godot/addons/effekseer/bin/windows/libeffekseer.x86_32.pdb")
        shutil.copy2("bin/windows/libeffekseer.x86_64.pdb", "../Godot/addons/effekseer/bin/windows/libeffekseer.x86_64.pdb")

elif "platform=macos" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/macos", exist_ok = True)

    import_generate_bindings("64")
    subprocess.run(["scons", "platform=macos", "arch=universal"] + options, check=True)

    os.rename("../Godot/addons/effekseer/bin/macos/libeffekseer.universal.dylib", "../Godot/addons/effekseer/bin/macos/libeffekseer.dylib")

elif "platform=android" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/android", exist_ok = True)

    for arch in ["arm32", "arm64", "x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "platform=android", f"arch={arch}", "use_static_cpp=yes"] + options, check=True)

elif "platform=ios" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/ios", exist_ok = True)

    import_generate_bindings("arm64")
    subprocess.run(["scons", "platform=ios", "arch=arm64"] + options, check=True)

    os.rename("../Godot/addons/effekseer/bin/ios/libeffekseer.arm64.dylib", "../Godot/addons/effekseer/bin/ios/libeffekseer.dylib")

elif "platform=linux" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/linux", exist_ok = True)

    for arch in ["x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "platform=linux", f"arch={arch}"] + options, check=True)

elif "platform=web" in sys.argv:
    os.makedirs("../Godot/addons/effekseer/bin/web", exist_ok = True)

    import_generate_bindings("wasm32")
    subprocess.run(["scons", "platform=web", "arch=wasm32"] + options, check=True)

    os.rename("../Godot/addons/effekseer/bin/web/libeffekseer.wasm32.wasm", "../Godot/addons/effekseer/bin/web/libeffekseer.wasm")
