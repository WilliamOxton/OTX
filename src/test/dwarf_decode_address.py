#-------------------------------------------------------------------------------
# elftools example: dwarf_decode_address.py
#
# Decode an address in an ELF file to find out which function it belongs to
# and from which filename/line it comes in the original source file.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import sys
import os

# If pyelftools is not installed, the example can also run from the root or
# examples/ dir of the source distribution.
sys.path[0:0] = ['.', '..', '../pyelftools/']

from elftools.common.py3compat import maxint, bytes2str
from elftools.dwarf.descriptions import describe_form_class
from elftools.elf.elffile import ELFFile

def get_leakages(file_leakage):
    leakages = set()
    with open(file_leakage, 'rb') as f:
        for line in f.readlines():
            addr = int(line[:-1], 16)
            leakages.add(addr)
    
    return leakages

def get_path_mapping(dwarfinfo):
    path_mapping = dict()
    for CU in dwarfinfo.iter_CUs():
            # DWARFInfo allows to iterate over the compile units contained in
            # the .debug_info section. CU is a CompileUnit object, with some
            # computed attributes (such as its offset in the section) and
            # a header which conforms to the DWARF standard. The access to
            # header elements is, as usual, via item-lookup.
            # print('  Found a compile unit at offset %s, length %s' % (
            #     CU.cu_offset, CU['unit_length']))

            # The first DIE in each compile unit describes it.
            top_DIE = CU.get_top_DIE()
            full_path = top_DIE.get_full_path()
            base_name = os.path.basename(full_path)
            path_mapping[base_name] = full_path
            # print('    Top DIE with tag=%s' % top_DIE.tag)

            # We're interested in the filename...
            # print(base_name, full_path)
    
    return path_mapping

def process(file_bin, file_leakage, output):
    with open(file_bin, 'rb') as f:
        elffile = ELFFile(f)

        if not elffile.has_dwarf_info():
            print('  file has no DWARF info')
            return
        
        leakages = get_leakages(file_leakage)

        # get_dwarf_info returns a DWARFInfo context object, which is the
        # starting point for all DWARF-based processing in pyelftools.
        dwarfinfo = elffile.get_dwarf_info()
        path_mapping = get_path_mapping(dwarfinfo)

        with open(output, 'w') as wf:
            for addr in leakages:
                funcname = decode_funcname(dwarfinfo, addr)
                file, line = decode_file_line(dwarfinfo, addr)
                print('Function:', bytes2str(funcname))
                print('File:', bytes2str(file))
                print('Line:', line)
                content = bytes2str(funcname) + ";" \
                        + bytes2str(file) + ";" \
                        + str(line) + ";" \
                        + path_mapping[bytes2str(file)] + ";\n"
                wf.writelines(content)


def decode_funcname(dwarfinfo, address):
    # Go over all DIEs in the DWARF information, looking for a subprogram
    # entry with an address range that includes the given address. Note that
    # this simplifies things by disregarding subprograms that may have
    # split address ranges.
    for CU in dwarfinfo.iter_CUs():
        for DIE in CU.iter_DIEs():
            try:
                if DIE.tag == 'DW_TAG_subprogram':
                    lowpc = DIE.attributes['DW_AT_low_pc'].value

                    # DWARF v4 in section 2.17 describes how to interpret the
                    # DW_AT_high_pc attribute based on the class of its form.
                    # For class 'address' it's taken as an absolute address
                    # (similarly to DW_AT_low_pc); for class 'constant', it's
                    # an offset from DW_AT_low_pc.
                    highpc_attr = DIE.attributes['DW_AT_high_pc']
                    highpc_attr_class = describe_form_class(highpc_attr.form)
                    if highpc_attr_class == 'address':
                        highpc = highpc_attr.value
                    elif highpc_attr_class == 'constant':
                        highpc = lowpc + highpc_attr.value
                    else:
                        print('Error: invalid DW_AT_high_pc class:',
                              highpc_attr_class)
                        continue

                    if lowpc <= address <= highpc:
                        return DIE.attributes['DW_AT_name'].value
            except KeyError:
                continue
    return None


def decode_file_line(dwarfinfo, address):
    # Go over all the line programs in the DWARF information, looking for
    # one that describes the given address.
    for CU in dwarfinfo.iter_CUs():
        # First, look at line programs to find the file/line for the address
        lineprog = dwarfinfo.line_program_for_CU(CU)
        prevstate = None
        for entry in lineprog.get_entries():
            # We're interested in those entries where a new state is assigned
            if entry.state is None:
                continue
            if entry.state.end_sequence:
                # if the line number sequence ends, clear prevstate.
                prevstate = None
                continue
            # Looking for a range of addresses in two consecutive states that
            # contain the required address.
            if prevstate and prevstate.address <= address < entry.state.address:
                filename = lineprog['file_entry'][prevstate.file - 1].name
                line = prevstate.line
                return filename, line
            prevstate = entry.state
    return None, None


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Expected usage: {0} <executable> <leakage_file> <output>'.format(sys.argv[0]))
        sys.exit(1)
    
    file_bin = sys.argv[1]
    file_leakage = sys.argv[2]
    output = sys.argv[3]
    process(file_bin, file_leakage, output)
