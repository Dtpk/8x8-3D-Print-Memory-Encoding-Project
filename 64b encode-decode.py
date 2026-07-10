"""
================================================================================
Keep The Key!!! This uses huffman encoding to compress your text losing the key makes
decoding that much harder to do.
================================================================================
"""

import heapq

# Fixed relative weights based on standard English frequency distributions
# Vowels/spaces are heavy (short codes), numbers/capitals are light (long codes).
ENGLISH_FREQUENCIES = {
    ' ': 1200, 'e': 1100, 'a': 850,  't': 900,  'o': 800,  'i': 750,
    'n': 700,  's': 650,  'r': 600,  'h': 550,  'l': 500,  'd': 450,
    'c': 400,  'u': 350,  'm': 300,  'f': 250,  'p': 200,  'g': 150,
    'w': 130,  'y': 120,  'b': 110,  'v': 100,  'k': 90,   'x': 50,
    'j': 40,   'q': 30,   'z': 20,

    # Capitals
    'A': 40, 'B': 35, 'C': 35, 'D': 35, 'E': 45, 'F': 30, 'G': 30, 'H': 35,
    'I': 40, 'J': 25, 'K': 25, 'L': 30, 'M': 35, 'N': 35, 'O': 35, 'P': 35,
    'Q': 20, 'R': 35, 'S': 40, 'T': 45, 'U': 30, 'V': 25, 'W': 30, 'X': 20,
    'Y': 25, 'Z': 15,

    # Numbers & Symbols
    '0': 25, '1': 30, '2': 28, '3': 25, '4': 25, '5': 25, '6': 25, '7': 25,
    '8': 25, '9': 25, '!': 20, '-': 25, '.': 35, '?': 20
}

class Node:
    def __init__(self, char, freq):
        self.char = char
        self.freq = freq
        self.left = None
        self.right = None
    def __lt__(self, other):
        return self.freq < other.freq

def build_perfect_tree():
    heap = [Node(char, freq) for char, freq in ENGLISH_FREQUENCIES.items()]
    heapq.heapify(heap)
    while len(heap) > 1:
        left = heapq.heappop(heap)
        right = heapq.heappop(heap)
        merged = Node(None, left.freq + right.freq)
        merged.left = left
        merged.right = right
        heapq.heappush(heap, merged)
    return heap[0]

def _generate_codes(node, current_code, codes):
    if not node:
        return
    if node.char is not None:
        codes[node.char] = current_code
        return
    _generate_codes(node.left, current_code + "0", codes)
    _generate_codes(node.right, current_code + "1", codes)

# Automatically generate a mathematically secure, prefix-free codebook
STATIC_CODEBOOK = {}
_generate_codes(build_perfect_tree(), "", STATIC_CODEBOOK)
DECODE_LOOKUP = {v: k for k, v in STATIC_CODEBOOK.items()}


def encode_mode():
    text = input("\nType text to encode (e.g., Square1): ")
    bit_stream = ""

    for char in text:
        if char in STATIC_CODEBOOK:
            bit_stream += STATIC_CODEBOOK[char]
        else:
            print(f"⚠️ Character '{char}' missing from static codebook. Swapped with '.'")
            bit_stream += STATIC_CODEBOOK['.']

    total_bits = len(bit_stream)

    print("\n" + "="*45)
    print(f"STATIC ENCODER OUTPUT FOR: \"{text}\"")
    print(f"Total bits used: {total_bits} / 64 maximum")
    print("="*45)

    if total_bits > 64:
        print(f"❌ OVERFLOW! Truncating text to fit your 8x8 grid...")
        bit_stream = bit_stream[:64]

    print("\n--- 8x8 MARBLE ARRANGEMENT BLUEPRINT ---")
    print("     0 1 2 3 4 5 6 7")
    print("    -----------------")
    for row in range(8):
        row_string = f"R{row}:  "
        for col in range(8):
            bit_index = row * 8 + col
            if bit_index < len(bit_stream):
                row_string += "O " if bit_stream[bit_index] == '1' else ". "
            else:
                row_string += ". "
        print(row_string)
    print("    -----------------")
    print("    ( O = Place Marble  |  . = Leave Empty )")


def decode_mode():
    print("\n=== DIGITAL MARBLE MEMORY DECODER ===")
    print("Type the 8 bits for each row (1 = Marble, 0 = Empty).")
    print("Hit enter on empty rows to autofill with zeros.\n")

    full_bitstream = ""
    for row in range(8):
        while True:
            row_input = input(f"Row {row} bits: ").strip().replace(" ", "")
            if row_input == "":
                row_input = "00000000"
            if len(row_input) == 8 and all(bit in '01' for bit in row_input):
                full_bitstream += row_input
                break
            print("❌ Input must be exactly eight 1s or 0s.")

    decoded_message = ""
    current_bit_buffer = ""
    for bit in full_bitstream:
        current_bit_buffer += bit
        if current_bit_buffer in DECODE_LOOKUP:
            decoded_message += DECODE_LOOKUP[current_bit_buffer]
            current_bit_buffer = ""

    print("\n" + "="*40)
    print(f"DECODED MESSAGE: {decoded_message}")
    print("="*40)


if __name__ == "__main__":
    print("Welcome to the Marble Memory System.")
    print("1: Encode a word (Text -> Marble Map)")
    print("2: Decode a board (Marble Bits -> Text)")
    choice = input("Select mode (1 or 2): ").strip()

    if choice == '1':
        encode_mode()
    elif choice == '2':
        decode_mode()
    else:
        print("Invalid choice. Exiting.")
