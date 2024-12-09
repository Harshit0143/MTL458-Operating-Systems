import random
from scipy.stats import bernoulli
from tqdm import tqdm
import sys

def show_str(num, filename):
    with open(filename, "a") as f:
        f.write(hex(num)[2:] + '\n')


S = 4096
P = 16
P_shift = 14
N_MAX = 1000000

address_max = (1 << 32) - 1
offset_max = (1 << P_shift) - 1
vpn_max = (1 << (32 - P_shift)) - 1
JUMP_MAX = 200 * (offset_max + 1)
# print("address_max:", address_max)
# print("offset_max :", offset_max)
# print("vpn_max    :", vpn_max)

K_vals = [1, 43, 87, 191, 401, 813, 1024]


for i in range(len(K_vals)):
    K = K_vals[i]
    filename = f"tests/input{i + 23}"
    with open(filename, "w") as f:
        f.write(str(1) + '\n')
        f.write(str(S) + '\n')
        f.write(str(P) + '\n')
        f.write(str(K) + '\n')
        f.write(str(N_MAX) + '\n')
    address = 0
    for i in tqdm(range(N_MAX)):
        assert address <= address_max 
        show_str(address, filename)
        jump = random.randint(0, JUMP_MAX)
        address = (address + jump) % (address_max + 1)

