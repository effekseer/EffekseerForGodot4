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

output_dir = "../Godot/addons/effekseer/bin"
target = "template_release"
is_debug = False
options = []

if "debug=yes" in sys.argv:
    options.append("dev_build=yes")
    target = "template_release"
    is_debug = True

options.append(f"target={target}")

if "platform=windows" in sys.argv:
    os.makedirs(f"{output_dir}/windows", exist_ok = True)

    for arch in ["x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "platform=windows", f"arch={arch}"] + options, check=True)

        if is_debug:
            shutil.copy2(f"bin/windows/libeffekseer.{arch}.pdb", f"{output_dir}/windows/libeffekseer.{arch}.pdb")

elif "platform=macos" in sys.argv:
    os.makedirs(f"{output_dir}/macos", exist_ok = True)

    import_generate_bindings("64")
    subprocess.run(["scons", "platform=macos", "arch=universal"] + options, check=True)

    os.rename(f"{output_dir}/macos/libeffekseer.universal.dylib", f"{output_dir}/macos/libeffekseer.dylib")

elif "platform=android" in sys.argv:
    os.makedirs(f"{output_dir}/android", exist_ok = True)

    for arch in ["arm32", "arm64", "x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "platform=android", f"arch={arch}", "use_static_cpp=yes"] + options, check=True)

elif "platform=ios" in sys.argv:
    os.makedirs(f"{output_dir}/ios", exist_ok = True)

    import_generate_bindings("arm64")
    subprocess.run(["scons", "platform=ios", "arch=arm64", "ios_simulator=no"] + options, check=True)
    subprocess.run(["scons", "platform=ios", "arch=universal", "ios_simulator=yes"] + options, check=True)

    os.makedirs(f"{output_dir}/ios/device", exist_ok = True)
    shutil.move(f"{output_dir}/ios/libeffekseer.arm64.a", f"{output_dir}/ios/device/libeffekseer.a")
    shutil.move(f"godot-cpp/bin/libgodot-cpp.ios.{target}.arm64.a", f"{output_dir}/ios/device/libgodot-cpp.a")
    os.makedirs(f"{output_dir}/ios/simulator", exist_ok = True)
    shutil.move(f"{output_dir}/ios/libeffekseer.universal.simulator.a", f"{output_dir}/ios/simulator/libeffekseer.a")
    shutil.move(f"godot-cpp/bin/libgodot-cpp.ios.{target}.universal.simulator.a", f"{output_dir}/ios/simulator/libgodot-cpp.a")

    subprocess.run(["xcodebuild", "-create-xcframework", 
        "-library", f"{output_dir}/ios/device/libeffekseer.a",
        "-library", f"{output_dir}/ios/simulator/libeffekseer.a",
        "-output", f"{output_dir}/ios/libeffekseer.xcframework"
    ])
    subprocess.run(["xcodebuild", "-create-xcframework", 
        "-library", f"{output_dir}/ios/device/libgodot-cpp.a",
        "-library", f"{output_dir}/ios/simulator/libgodot-cpp.a",
        "-output", f"{output_dir}/ios/libgodot-cpp.xcframework"
    ])
    shutil.rmtree(f"{output_dir}/ios/device")
    shutil.rmtree(f"{output_dir}/ios/simulator")

elif "platform=linux" in sys.argv:
    os.makedirs(f"{output_dir}/linux", exist_ok = True)

    for arch in ["x86_32", "x86_64"]:
        import_generate_bindings(arch)
        subprocess.run(["scons", "-c"])
        subprocess.run(["scons", "platform=linux", f"arch={arch}"] + options, check=True)

elif "platform=web" in sys.argv:
    os.makedirs(f"{output_dir}/web", exist_ok = True)

    import_generate_bindings("wasm32")
    subprocess.run(["scons", "platform=web", "arch=wasm32", "threads=yes"] + options, check=True)
    os.rename(f"{output_dir}/web/libeffekseer.wasm32.wasm", f"{output_dir}/web/libeffekseer.threads.wasm")
    subprocess.run(["scons", "platform=web", "arch=wasm32", "threads=no"] + options, check=True)
    os.rename(f"{output_dir}/web/libeffekseer.wasm32.wasm", f"{output_dir}/web/libeffekseer.nothreads.wasm")
