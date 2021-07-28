import argparse
import pathlib
import zlib

COLUMN_COUNT = 32
INDENT_AMOUNT = 4

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('input', type=pathlib.Path, help='Input file')
    parser.add_argument('output', type=pathlib.Path, help='Output file')
    parser.add_argument('name', type=str, help='Output variable name')
    args = parser.parse_args(argv)

    with open(args.input, 'rb') as fd:
        data = fd.read()
    data = zlib.compress(data)
    data_len = len(data)
    indent_str = ' ' * INDENT_AMOUNT
    c_file = str(args.output) + '.c'
    h_file = str(args.output) + '.h'
    h_file_name = pathlib.Path(h_file).name
    with open(c_file, 'w') as fd:
        fd.write(f'#include "{h_file_name}"\n\n')
        fd.write(f'const uint8_t {args.name}[{data_len}] = {{\n')
        i = 0
        while i < data_len:
            fd.write(indent_str)
            for _ in range(0, min(COLUMN_COUNT, data_len - i)):
                fd.write(f'0x{data[i]:02X}, ')
                i += 1
            fd.write('\n')
        fd.write(indent_str + '};\n')
    with open(h_file, 'w') as fd:
        fd.write('#pragma once\n')
        fd.write('#include <stdint.h>\n\n')
        fd.write('#ifdef __cplusplus\nextern "C" {\n#endif\n\n')
        fd.write(f'#define {args.name}_SIZE ({data_len})\n')
        fd.write(f'extern const uint8_t {args.name}[{data_len}];\n\n')
        fd.write('#ifdef __cplusplus\n}\n#endif\n\n')

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
