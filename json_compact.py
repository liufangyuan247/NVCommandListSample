import json
import struct
from base64 import b64encode
import os

def convert_array(key, json_val):
  data = []
  for e in json_val:
    data += e
  if key == "color":
    return b64encode(struct.pack(f'{len(data)}B', *data)).decode("utf-8")
  else:
    return b64encode(struct.pack(f'{len(data)}f', *data)).decode("utf-8")

def convert_mesh(mesh):
  for k in mesh:
    mesh[k] = convert_array(k, mesh[k])
  return mesh

def convert_json(json_obj):
  if type(json_obj) not in [dict, list]:
    return json_obj

  if type(json_obj) == dict:
    for k in json_obj:
      if k == "mesh":
        json_obj[k] = convert_mesh(json_obj[k])
      else:
        json_obj[k] = convert_json(json_obj[k])
  if type(json_obj) == list:
      for i in range(len(json_obj)):
        json_obj[i] = convert_json(json_obj[i])
  return json_obj

def main():
  src_dir = "assets/dumped_map_data"
  dst_dir = "assets/dumped_map_data_compact"
  if not os.path.exists(dst_dir):
    os.makedirs(dst_dir)
  files = os.listdir(src_dir)
  for f in files:
    input_fn = os.path.join(src_dir, f)
    output_fn = os.path.join(dst_dir, f)
    json_obj = json.load(open(input_fn))
    # print(json_obj)
    new_json_obj = convert_json(json_obj)
    # print(new_json_obj)
    json.dump(new_json_obj, open(output_fn, "w"))
    # break

if __name__ == "__main__":
  main()