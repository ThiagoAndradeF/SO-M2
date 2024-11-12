import time
import numpy as np

def run_test(size):
    # Alocar memória
    array = np.arange(size)

    # Realizar uma operação (soma dos elementos)
    total_sum = np.sum(array)

    print(f"Soma: {total_sum}")

if __name__ == "__main__":
    size = 10000000  # 10 milhões
    start_time = time.time()
    
    run_test(size)
    
    end_time = time.time()
    print(f"Tempo de execução em Python: {end_time - start_time} segundos")