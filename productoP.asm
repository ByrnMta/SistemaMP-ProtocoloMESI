LOADI REG0, 0x6            @ REG0 = dirección de A[0]
LOADI REG1, 0xF              @ REG1 = dirección de B[0]
LOADI REG2, 0x2              @ REG2 = dirección de suma parcial
LOADI REG3, 0              @ REG3 = contador de iteraciones
LOAD REG4, [REG2]          @ REG4 = suma parcial inicial (debería ser 0)
LOOP:
LOAD REG5, [REG0]       @ REG5 = A[i]
LOAD REG6, [REG1]       @ REG6 = B[i]
FMUL REG7, REG5, REG6   @ REG7 = REG5 * REG6
FADD REG4, REG4, REG7   @ REG4 += REG7
INC REG0                @ REG0++
INC REG1                @ REG1++
DEC REG3                @ REG3--
JNZ REG3, LOOP                @ Si REG3 != 0, salta a LOOP
STORE REG4, [REG2]      @ Guarda suma parcial en memoria

MOV REG0, REG2             @ REG0 = dirección base de sumas parciales
LOADI REG1, 4              @ REG1 = contador (4 sumas)
WAIT_SUMAS:
LOAD REG3, [REG0]      @ REG3 = suma parcial
JZ REG3, WAIT_SUMAS          @ Si es cero, espera
INC REG0               @ Siguiente suma parcial
DEC REG1
JNZ REG1, WAIT_SUMAS         @ Repite hasta que todas estén listas
    
resultado_final:
LOADI REG1, 4
LOADI REG4, 0
LOOP2:
LOAD REG3, [REG2]
FADD REG4, REG4, REG3
INC REG2
DEC REG1
JNZ REG1, LOOP2
LOADI REG1, 0x0           @ REG1 = dirección de resultado final
STORE REG4, [REG1]        @ REG2 tendrá el resultado final
LOAD REG5, [REG1]        @ Carga el resultado final para verificar
LOAD REG6, [REG2]
INC REG2
LOAD REG7, [REG2]