"""Editor-only deterministic P0 rejection cue. Preserves an existing asset.

Run with UnrealEditor-Cmd -ExecutePythonScript=.../Create-P0FeedbackAudio.py.
No external audio, runtime generator, or learning-directory dependency.
"""
import json
import math
import os
import struct
import wave
import unreal


def main():
    path = "/Game/LD/Audio/S_P0Rejected"
    root = unreal.SystemLibrary.get_project_directory()
    output = os.path.join(root, "Saved", "P0Runs", "G2-feedback-audio")
    os.makedirs(output, exist_ok=True)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        source = os.path.join(output, "S_P0Rejected.wav")
        rate, duration = 48000, 0.1
        count = int(rate * duration)
        frames = []
        for index in range(count):
            seconds = index / rate
            envelope = math.sin(math.pi * index / (count - 1)) ** 2
            value = 0.16 * envelope * math.sin(2 * math.pi * 220 * seconds)
            frames.append(struct.pack("<h", round(32767 * value)))
        with wave.open(source, "wb") as stream:
            stream.setnchannels(1)
            stream.setsampwidth(2)
            stream.setframerate(rate)
            stream.writeframes(b"".join(frames))
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", "/Game/LD/Audio")
        task.set_editor_property("destination_name", "S_P0Rejected")
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError("P0 rejection sound import/load failed")
    with open(os.path.join(output, "result.json"), "w", encoding="utf-8") as stream:
        json.dump({"result": "Pass", "asset": path, "sampleRate": 48000, "duration": 0.1,
                   "scope": "Generated mono PCM import and SoundWave load; playback not yet verified"}, stream, indent=2)
    unreal.log("P0_FEEDBACK_AUDIO_READY " + path)


try:
    main()
finally:
    unreal.SystemLibrary.quit_editor()
