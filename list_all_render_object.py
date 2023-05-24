import json
import struct
from base64 import b64encode
import os
from tqdm import tqdm

def main():
  src_dir = "assets/dumped_map_data"
  files = os.listdir(src_dir)
  unique_objects = {}
  for f in tqdm(files):
    input_fn = os.path.join(src_dir, f)
    json_obj = json.load(open(input_fn))
    # print(f"process file: {input_fn}")
    if json_obj["type"] not in unique_objects:
      unique_objects[json_obj["type"]] = json_obj

  for k in unique_objects.keys():
    print(json.dumps(k, indent=2))

  print(unique_objects["SignalStopLineRenderObject"])

if __name__ == "__main__":
  main()