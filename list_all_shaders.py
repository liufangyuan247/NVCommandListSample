import json
import struct
from base64 import b64encode
import os
from tqdm import tqdm

def collect_shaders(json_obj, shaders):
  if type(json_obj) not in [dict, list]:
    return

  if type(json_obj) == dict:
    for k in json_obj:
      if k == "shader":
        shaders.add(json_obj[k])
        continue
      else:
        collect_shaders(json_obj[k], shaders)
  if type(json_obj) == list:
      for i in range(len(json_obj)):
        collect_shaders(json_obj[i], shaders)

def main():
  src_dir = "assets/dumped_map_data"
  files = os.listdir(src_dir)
  shaders = set()
  for f in tqdm(files):
    input_fn = os.path.join(src_dir, f)
    json_obj = json.load(open(input_fn))
    # print(f"process file: {input_fn}")
    collect_shaders(json_obj, shaders)

  for s in shaders:
    print(s)

if __name__ == "__main__":
  main()