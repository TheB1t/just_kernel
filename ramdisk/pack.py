import os
import struct

TYPE_START          = 1
TYPE_END            = 2
TYPE_FILE           = 3
TYPE_DIR_START      = 4
TYPE_DIR_END        = 5
TYPE_DEV            = 6

types = {
    'block':    0x01,
    'char':     0x02,
}

majors = {
    'unnamed':  0x00,
    'mem':      0x01,
    'hd':       0x02,
    'tty':      0x03,
    'ttyaux':   0x04,
}

dev_files = {
    # <name>:   (type,              major,         minor)

    'mem':      (types['block'],    majors['mem'],      0),
    'null':     (types['char'],     majors['mem'],      1),
    'zero':     (types['char'],     majors['mem'],      2),

    'tty':      (types['char'],     majors['ttyaux'],   0),
    'tty0':     (types['char'],     majors['tty'],      0),
}

def write_binary(file, data):
    file.write(data)

def write_signature(file):
    write_binary(file, struct.pack('I', 0x495f5244))

def write_entry(file, type, name = '', data = b''):
    write_binary(file, struct.pack('BI', type, len(data)))

    name_bytes = name.encode('utf-8')
    write_binary(file, struct.pack('32s', name_bytes))
    write_binary(file, data)

def write_dev(file, name, data):
    type, major, minor = data

    rdev = major << 8 | minor

    print(f'+ [DEV] {name} {rdev:x}')
    write_entry(file, TYPE_DEV, name, struct.pack('BH', type, rdev))

def write_dir(file, root, dir):
    if root:
        dir_path = os.path.join(root, dir)
    else:
        dir_path = dir

    print(f'+ [START] {dir}')
    write_entry(file, TYPE_DIR_START, dir)

    for entry in os.listdir(f"{dir_path}"):
        path = os.path.join(dir_path, entry)

        if os.path.isdir(path):
            write_dir(file, dir_path, entry)
        else:
            print(f'- [FILE] {entry}')
            with open(path, 'rb') as f:
                file_data = f.read()

            write_entry(file, TYPE_FILE, entry, file_data)

    print(f'+ [START] dev')
    write_entry(file, TYPE_DIR_START, "dev")
    for name, data in dev_files.items():
        write_dev(file, name, data)
    write_entry(file, TYPE_DIR_END, "dev")
    print(f'+ [END] dev')

    print(f'+ [END] {dir}')
    write_entry(file, TYPE_DIR_END, dir)

def create_ramdisk(output_filename, input_dir):

    with open(output_filename, 'wb') as file:
        write_signature(file)
        write_entry(file, TYPE_START, "volume")
        write_dir(file, None, input_dir)
        write_entry(file, TYPE_END, "volume")

    print(f"ramdisk created: {output_filename}")

input_directory = "root"
output_file = "ramdisk.bin"

create_ramdisk(output_file, input_directory)