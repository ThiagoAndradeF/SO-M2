using System;
using System.Diagnostics;



class Program
{
    private const int IterationCount = 100;

    static void Main(string[] args)
    {
        Console.WriteLine("Digite os tamanhos de buffer em MB (digite '0' para sair):");
        
        while (true)
        {
            string input = Console.ReadLine();
            if (int.TryParse(input, out int size) && size >= 0)
            {
                if (size == 0)
                {
                    break;
                }
                MeasureMemoryAllocation(size * 1024 * 1024); // Convertendo MB para bytes
            }
            else
            {
                Console.WriteLine("Por favor, insira um número válido.");
            }
        }
    }

    private static void MeasureMemoryAllocation(int bufSize)
    {
        Console.WriteLine($"Testando com buffer de {bufSize / (1024 * 1024)} MB");
        Stopwatch stopwatch = new Stopwatch();
        stopwatch.Start();

        for (int i = 0; i < IterationCount; i++)
        {
            int[] array = new int[bufSize / sizeof(int)]; // Alocando memória (4 bytes por int)
            // Simulação de trabalho com a memória alocada
            for (int j = 0; j < array.Length; j++)
            {
                array[j] = j;
            }
        }

        stopwatch.Stop();
        Console.WriteLine($"Tempo gasto para alocar e inicializar {bufSize / (1024 * 1024)} MB {IterationCount} vezes: {stopwatch.Elapsed.TotalSeconds:F4} segundos");
    }
}