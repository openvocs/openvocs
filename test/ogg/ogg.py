import zlib
import struct

def crc32ogg(seq):

    crc = 0
    for b in seq:
        crc ^= b << 24
        for _ in range(8):
            crc = (crc << 1) ^ 0x104c11db7 if crc & 0x80000000 else crc << 1
    return crc

def crc32ogg2(seq):

    crc = 0
    for b in seq:
        print(f"{b} ")
        crc ^= b << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x104c11db7
            else:
               crc = crc << 1
    return crc

with open("sample3.opus", "rb") as f_:
    file_data = f_.read()


cp, ssv, htf, agp, ssn, psn, pc, ps = struct.unpack_from("<4sBBQIIIB", file_data, 0)

print(f"Catch pattern: {cp}   Stream structure version: {ssv} header type flag: {htf}")
print(f"Absolute granule position: {agp}  Stream serial number: {ssn}")
print(f"Page sequence number:  {psn} Page segments: {ps}")

offset = struct.calcsize("<4sBBQIIIB")
segments = struct.unpack_from(f"<{ps}B", file_data, offset)

packet_size = 0

for num in segments:
    packet_size += num

header_size = offset + len(segments) + packet_size

# Copying the entire packet then changing the crc to 0.
header_copy = bytearray()
header_copy.extend(file_data[0:header_size])
struct.pack_into("<I", header_copy, struct.calcsize("<4sBBQII"), 0)

print(pc)
print(zlib.crc32(header_copy))
print(crc32ogg(header_copy))
print(crc32ogg2(header_copy))
