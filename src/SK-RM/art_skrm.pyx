# artskrm.pyx
# 這些函式只能接受「C 原生型別」，不能用 str、list 等 Python 型別。
# cpdef 的意思是：「這是一個可以同時讓 Python 和 Cython 調用 的函式」, C 語言 無法直接呼叫！因為它不會出現在 .h 檔案裡

from libc.time cimport timespec
from libc.string cimport strlen
from libc.string cimport memcpy
from libc.stdint cimport uint8_t, uint16_t, uint32_t
from cpython.bytes cimport PyBytes_FromStringAndSize
from libc.stdio cimport printf
from cpython.object cimport PyObject

# from .art_trace_info import tracer  # art_trace_info.py 中的 class
cdef int MAX_TRACK = (1 << 30)

cdef class ARTskrm:
    cdef public int WORD_SIZE
    cdef public int _Parallel_Write
    cdef public int _Permutation_Write
    cdef public int _Sky_Tree
    cdef public int _Naive
    cdef public int _Bit_Comparasion
    cdef public int _degree_limit

    cdef object trace  # Python object, not typed
    cdef public int shift
    cdef public int inject_energy
    cdef public int remove_energy
    cdef public int detect_energy
    cdef public int inject_latency
    cdef public int remove_latency
    cdef public int detect_latency
    cdef public dict excessive
    cdef public int cycle

    def __init__(self):
        self.WORD_SIZE = 32

        self._Parallel_Write = 0
        self._Permutation_Write = 0
        self._Sky_Tree = 0
        self._Naive = 0
        self._Bit_Comparasion = 0
        self._degree_limit = 0

        # self.trace = tracer()
        self.shift = 0

        self.inject_energy = 0
        self.remove_energy = 0
        self.detect_energy = 0

        self.inject_latency = 0
        self.remove_latency = 0
        self.detect_latency = 0

        self.excessive = {}
        self.cycle = 0

    # inject latency and energy
    cdef public inject(self, count):
        self.inject_latency += count
        self.inject_energy += count

    cpdef public inject_e(self, count):
        self.inject_energy += count

    cpdef public inject_l(self, count):
        self.inject_latency += count

    # get total energy
    cpdef public list get_energy(self):
        cdef double detect = 2.0 * self.detect_energy
        cdef double shift = 20.0 * self.shift
        cdef double remove = 20.0 * self.remove_energy
        cdef double insert = 200.0 * self.inject_energy
        cdef double total = detect + shift + remove + insert
        return [detect, shift, remove, insert, total]
        
    # get total latency
    cpdef public get_latency(self):
        # Detect, Shift, Remove, Inject
        detect = 0.1 * self.detect_latency
        shift = 0.5 * self.shift
        remove = 0.8 * self.remove_latency
        insert = 1 * self.inject_latency
        total = detect + shift + remove + insert
        return [detect, shift, remove, insert, total]

    cpdef public int skyrmions_counter(self, number):  # count how many '1' in number
        cdef unsigned int n = self.str_to_int(number)
        cdef int count = 0
        while n:
            n &= n - 1  # 每次消去最低的 1-bit
            '''
            n      =  1 0 1 1 0 0
            n - 1  =  1 0 1 0 1 1
            ---------------------
            AND    =  1 0 1 0 0 0
            '''
            count += 1
        return count

    # 將string當中的每一個char都轉成int再串接起來
    cdef int str_to_int_fast(self, str value):  # eg: input = "AB"
        cdef bytes s = value.encode('utf-8')    # "AB" = [65, 66] = [0b01000001, 0b01000010]
        cdef int result = 0
        cdef int i
        for i in range(len(s)):
            result = (result << 8) | s[i]   # i=0 => (0 << 8) | 65 = 65 = 0b01000001 = result
        return result                       # i=1 => (0b01000001 << 8) | 66 = 0b01000001_01000010 = 65_66

    cpdef public clear(self):
        self.shift = 0
        self.inject_energy = 0
        self.remove_energy = 0
        self.detect_energy = 0
        self.inject_latency = 0
        self.remove_latency = 0
        self.detect_latency = 0
        self.cycle = 0

    # insert leaf
    cdef public void insert_leaf(self, char* key, char* value, uint32_t track_domain_id):
        cdef uint32_t track
        cdef uint16_t domain # physical addr
        track, domain = self.trans_track_domain(track_domain_id)

        cdef int key_len = strlen(key)
        cdef int val_len = strlen(value)

        # 將 C 字串轉為 Python 字串
        cdef str key_str = key[:strlen(key)].decode('utf-8')
        cdef str value_str = value[:strlen(value)].decode('utf-8')
        # count skyrmion
        cdef int skyrmions_count = 0

        # insert key
        self.trace.skyrmion_insert(track, domain, key_str, self.cycle)
        # update addr after adding key
        if((track + int((domain + key_len*8)//(128*8))) < MAX_TRACK):
            track += int((domain + key_len*8)//(128*8))
            domain = (domain + key_len*8) % (128*8)
            print("After adding key domain:", domain, " track:", track)
        # else
            # TODO
            # when memory full
        
        cdef int i = 0
        # count key skyrmion
        while key[i] != b'\0':
            skyrmions_count += self.skyrmions_counter(key[i])
            i += 1

        # insert value
        self.trace.skyrmion_insert(track, domain, value_str, self.cycle)
        # update addr after adding value
        if((track + int((domain + val_len*8)//(128*8))) < MAX_TRACK):
            track += int((domain + val_len*8)//(128*8))
            domain = (domain + val_len*8)%(128*8)
            print("domain:", domain, " track:", track)
        #else
            # TODO
            # when memory full
        
        i = 0
        # count key skyrmion
        while value[i] != b'\0':
            skyrmions_count += self.skyrmions_counter(value[i])
            i += 1

        self.inject(skyrmions_count)
        self.shift += self.WORD_SIZE*2
        self.cycle += 1

    # initial insert node 4
    cdef public void insert_node4(self,
                                bint prefix_too_long,
                                uint8_t prefix_len,
                                unsigned char * key_char,
                                uint8_t num_of_child,
                                unsigned char * child_track_domain_id,
                                uint8_t type):
        cdef int skyrmions_count
        cdef int prefix_info
        cdef int i
        print("in insert_node4")

        prefix_info = self.trans_prefix_info(prefix_too_long, type, prefix_len)
        skyrmions_count = self.skyrmions_counter(prefix_info) + self.skyrmions_counter(num_of_child)

        for i in range(num_of_child):
            print("in for")
            skyrmions_count += self.skyrmions_counter(key_char[i])
            skyrmions_count += self.skyrmions_counter(child_track_domain_id[i])

        self.shift += (self.WORD_SIZE*2)
        self.inject(skyrmions_count)
        self.cycle += 1

    # 給art.c用, domain用fundamental unit計算
    cdef public uint32_t track_domain_trans(self, uint32_t track, uint16_t domain):
        print("in track_domain_trans")
        cdef uint32_t track_domain_id = track << 2 + domain
        return track_domain_id

    # 內部轉換使用, domain trans into real addr
    cdef trans_track_domain(self, uint32_t track_domain_id):
        cdef uint32_t track = track_domain_id >> 2
        cdef uint16_t domain = (track_domain_id % 4) * 32 * 8
        return track, domain

    cdef trans_prefix_info(self, bint prefix_too_long, uint8_t type, uint8_t prefix_len):
        cdef prefix_info
        prefix_info = (int(prefix_too_long) << 7) + (type << 5) + prefix_len
        return prefix_info

cdef ARTskrm _global_artskrm = None

# if _global_artskrm is not None:
#    raise RuntimeError("ARTskrm not initialized")

cdef void _init_artskrm_with_gil() noexcept:
    global _global_artskrm
    if <void*>_global_artskrm == NULL:
        printf(b"[Cython] _global_artskrm is NULL, initializing...\n")
        tmp = ARTskrm()
        if not <PyObject*>tmp:
            printf(b"[Cython] ARTskrm() returned NULL!\n")
        else:
            _global_artskrm = tmp
            printf(b"[Cython] ARTskrm() created at %p\n", <void*>_global_artskrm)
    else:
        printf(b"[Cython] _global_artskrm already exists at %p\n", <void*>_global_artskrm)
        
    #if <void*>_global_artskrm == NULL:
    #    print("in init null")
    #    _global_artskrm = ARTskrm()
    # if not <PyObject*>_global_artskrm:
    #    print("in init")  # 應該印得出來
    #    _global_artskrm = ARTskrm()
    # if _global_artskrm is None: 這在 Cython 的 cdef 類別變數上是錯誤的，因為 None 是 Python object，_global_artskrm 是低階 C pointer
                

cdef public void init_artskrm() noexcept nogil:
    with gil:
        _init_artskrm_with_gil()


cdef public void art_clear():
    _global_artskrm.clear()

cdef public uint32_t art_track_domain_trans(uint32_t track, uint16_t domain):
    assert _global_artskrm is  None, "global ARTskrm is NULL"
    print("in art_track_domain_trans")
    return _global_artskrm.track_domain_trans(track, domain)
    # return (track << 2) + (domain // (32 * 8))

cdef public void art_insert_leaf(char* key, char* value, uint32_t track_domain_id):
    _global_artskrm.insert_leaf(key, value, track_domain_id)

cdef public void art_insert_node4(bint prefix_too_long, uint8_t prefix_len, unsigned char * key_char, uint8_t num_of_child, unsigned char * child_track_domain_id, uint8_t type):
    _global_artskrm.insert_node4(prefix_too_long, prefix_len, key_char, num_of_child, child_track_domain_id, type)