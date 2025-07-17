import math


class tracer:
    def __init__(self):
        self.filename = None
        self.trace_file = None
        self._enable = 0
        self.domain_shift = 6  # Default offset, total addr = R:C:RK:BK:CH, RK + BK + CH = 6 bits
        # self.track_shift = self.domain_shift + int(math.log2(4096))  # DOMAIN_NR from C++, log2(4096)= 12 = C = domain(bit in track)
        self.track_shift = self.domain_shift + int(math.log2(128*8)) # a track has 128B domain = 128*8 bits = 1024 bits
        self.trace_count = 0 
        self.DBC_track = 64   # R = 6 => 2^6 = total 64 tracks on one DBC, track has 6 bits for counting

    def initialize_trace(self, filename):
        self._enable = 1
        self.filename = filename
        self.trace_file = open(self.filename, 'a')
        if self.trace_file:
            self.trace_file.write("NVMV 2\n")
            print("[trace enabled]")

    def set_domain(self, domain):
        self.track_shift = self.domain_shift + int(math.log2(domain))

    def close_trace(self):
        if self.trace_file:
            self.trace_file.close()

    def skyrmion_read(self, track, domain, value, cycle):
        if self._enable:
            address = self.calc_address(track, domain)
            if self.trace_file:
                self.trace_file.write(f"{cycle} R 0x{address:X} {self.str_to_hex(value)} {self.str_to_hex(value)} 0\n")

    def skyrmion_write(self, track, domain, oldvalue, newvalue, cycle):
        if self._enable:
            address = self.calc_address(track, domain)
            if self.trace_file:
                self.trace_file.write(f"{cycle} W 0x{address:X} {self.str_to_hex(newvalue)} {self.str_to_hex(oldvalue)} 0\n")

    def skyrmion_insert(self, track, domain, newvalue, cycle):
        if self._enable:
            address = self.calc_address(track, domain)
            if self.trace_file:
                zero="0"*128
                self.trace_file.write(f"{cycle} I 0x{address:X} {self.str_to_hex(newvalue)} {zero} 0\n")

    def skyrmion_delete(self, track, domain, oldvalue, cycle):
        if self._enable:
            address = self.calc_address(track, domain)
            if self.trace_file:
                zero="0"*128
                self.trace_file.write(f"{cycle} D 0x{address:X} {zero} {self.str_to_hex(oldvalue)} 0\n")

    def calc_address(self, track, domain):
        address = (track << self.track_shift) + (domain << self.domain_shift)
        # open("current_domain.txt", mode='a').write(f"{track} {domain}\n")

        return address

    def str_to_hex(self, value):
        if self._enable:
            if isinstance(value, int):
                hex_value = format(value, 'x')
                return hex_value.ljust(128, '0')

            if value == "":
                return "0" * 128

            binary_parts = [bin(ord(char))[2:].zfill(8) for char in value]
            binary_string = ''.join(binary_parts)
            hex_string = hex(int(binary_string, 2))[2:]
            return hex_string.ljust(128, '0')

if __name__ == "__main__":
    skyrmion_mem = tracer()
    skyrmion_mem.initialize_trace("trace_output.nvt")
    skyrmion_mem.skyrmion_read(1, 64, "read_value", 100)  # Simulated read operation
    skyrmion_mem.skyrmion_write(1, 1024, "old_value", "new_value", 101)  # Simulated write operation
    skyrmion_mem.skyrmion_insert(1, 1024, "data_value", 102)  # Simulated insert operation
    skyrmion_mem.skyrmion_delete(1, 1024, "data_value", 103)  # Simulated delete operation
    skyrmion_mem.close_trace()
