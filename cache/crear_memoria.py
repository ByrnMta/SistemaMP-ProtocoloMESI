DEPTH = 512  # Tamaño total de la memoria principal
WIDTH = 64

N = 8  # Tamaño de los vectores
A = [1, 2, 3, 4, 5, 6, 7, 8]
B = [10, 20, 30, 40, 50, 60, 70, 80]
partial_sums = [0, 0, 0, 0]

# Cálculo de posiciones necesarias: 1 (N) + N (A) + N (B) + len(partial_sums)
total_needed = 2 + len(A) + len(B) + len(partial_sums)

if total_needed > DEPTH:
    raise ValueError(f"¡Error! Se requieren {total_needed} posiciones, pero la memoria solo tiene {DEPTH}.")

with open('mem.mif', 'w') as f:
    f.write(f"DEPTH = {DEPTH};\n")
    f.write(f"WIDTH = {WIDTH};\n")
    f.write("ADDRESS_RADIX = HEX;\n")
    f.write("DATA_RADIX = DEC;\n")
    f.write("CONTENT BEGIN\n")
    addr = 0

    #Guardar resultado en la primera posición
    f.write(f"    {addr:X} : 0;\n")
    addr += 1

    # Guarda N en la segunda posición
    f.write(f"    {addr:X} : {N};\n")
    addr += 1

    # Guarda partial_sums[]
    for val in partial_sums:
        f.write(f"    {addr:X} : {val};\n")
        addr += 1

    # Guarda A[]
    for val in A:
        f.write(f"    {addr:X} : {val};\n")
        addr += 1
        
    # Guarda B[]
    for val in B:
        f.write(f"    {addr:X} : {val};\n")
        addr += 1
    
    # Rellena el resto con ceros
    while addr < DEPTH:
        f.write(f"    {addr:X} : 0;\n")
        addr += 1
    
    f.write("END;\n")