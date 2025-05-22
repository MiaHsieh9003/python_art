from .trace_info import tracer


class ARTskrm:

    def __init__(self):
        self.WORD_SIZE = 64

        self._Parallel_Write = 0
        self._Permutation_Write = 0
        self._Sky_Tree = 0
        self._Naive = 0
        self._Bit_Comparasion = 0
        self._degree_limit = 0

        self.trace = tracer()

        self.shift = 0

        self.inject_energy = 0
        self.remove_energy = 0
        self.detect_energy = 0

        self.inject_latency = 0
        self.remove_latency = 0
        self.detect_latency = 0

        self.excessive = {}
        self.cycle = 0

    # Write

    def write(self, original_data: list, new_data: list, node_id, start, end):
        # Caculate operations overhead on each method
        if self._Parallel_Write:
            self.shift += self.WORD_SIZE * 2
            self.detect_l(self.WORD_SIZE)

        data_offset = start
        data_count = 0

        for new, old in zip(new_data, original_data):
            '''
            # node_id: each node has different id
            # 64: 每個betree節點分配的空間大小
            # data_offset: betree節點內部的偏移量
            '''
            current_address = node_id * 64 + data_offset
            if new != old:
                if self._Sky_Tree or self._Permutation_Write:
                    self.shift += self.WORD_SIZE * 2 + 1
                elif self._Naive:
                    self.shift += self.WORD_SIZE * 2

            if self._Permutation_Write:
                self.detect(self.WORD_SIZE)
                skyrmions_on_track = self.skyrmions_counter(old)
                new_data_skyrmions = self.skyrmions_counter(new)
                if new_data_skyrmions > skyrmions_on_track:
                    self.inject(new_data_skyrmions - skyrmions_on_track)
                elif skyrmions_on_track > new_data_skyrmions:
                    self.remove(skyrmions_on_track - new_data_skyrmions)

            elif self._Sky_Tree:
                self.detect(self.WORD_SIZE)
                skyrmions_on_track = self.skyrmions_counter(old)
                new_data_skyrmions = self.skyrmions_counter(new)
                # Need More Skyrmions
                if new_data_skyrmions > skyrmions_on_track:
                    self.recycler_write(new_data_skyrmions, skyrmions_on_track, node_id, data_offset)
                # Store excessive skyrmions
                elif skyrmions_on_track > new_data_skyrmions:
                    excessive_skyrmions = skyrmions_on_track - new_data_skyrmions
                    if (data_count >= end):
                        self.excessive[node_id][data_offset] += excessive_skyrmions
                        self.shift += self.WORD_SIZE
                    else:
                        self.remove(excessive_skyrmions)

            elif self._Naive:
                if new != old:
                    if self._Bit_Comparasion:
                        self.trace.skyrmion_write(0, current_address, old, new, self.cycle)
                        self.cycle += 1
                    if not self._Bit_Comparasion:
                        self.trace.skyrmion_delete(0, current_address, old, self.cycle)
                        self.cycle += 1
                        self.trace.skyrmion_insert(0, current_address, new, self.cycle)
                        self.cycle += 1
                self.remove(self.WORD_SIZE)
                self.inject(self.skyrmions_counter(new))
            data_offset += 1
            if new != old:
                data_count += 1
    # Check the binary representation of the number and count the number of 1s
    def skyrmions_counter(self, number):
        binary_representation = bin(self.str_to_int(number))
        return binary_representation.count('1')