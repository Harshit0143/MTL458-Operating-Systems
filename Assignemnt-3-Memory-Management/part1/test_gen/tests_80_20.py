import random
from scipy.stats import bernoulli
from tqdm import tqdm
import sys

def show_str(num, filename):
    with open(filename, "a") as f:
        f.write(hex(num)[2:] + '\n')
P_GOOD = 0.8
NUM_GOOD = 230

S = 4096
P = 16
P_shift = 14
N_MAX = 1000000

address_max = (1 << 32) - 1
offset_max = (1 << P_shift) - 1
vpn_max = (1 << (32 - P_shift)) - 1
# print("address_max:", address_max)
# print("offset_max :", offset_max)
# print("vpn_max    :", vpn_max)

K_vals = [1, 43, 87, 191, 401, 813, 1024]
vpn_values = [i for i in range(0, vpn_max + 1)]
frequent_vpn = random.sample(vpn_values, NUM_GOOD)
for i in range(len(K_vals)):
    K = K_vals[i]
    filename = f"tests/input{i + 1}"
    with open(filename, "w") as f:
        f.write(str(1) + '\n')
        f.write(str(S) + '\n')
        f.write(str(P) + '\n')
        f.write(str(K) + '\n')
        f.write(str(N_MAX) + '\n')
    for i in tqdm(range(N_MAX)):
        good = bernoulli.rvs(P_GOOD)
        if (good == 1):
            vpn = random.choice(frequent_vpn)
        else:
            vpn = random.randint(0, vpn_max)
        
        offset = random.randint(0, offset_max)
        address = (vpn << P_shift) + offset
        assert address <= address_max 
        assert (address >> P_shift) == vpn
        show_str(address, filename)
