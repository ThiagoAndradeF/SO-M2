using System;
using System.Diagnostics;

class Program
{
    static void RunTest(int size)
    {
        // Alocar memória
        int[] array = new int[size];

        // Preencher o vetor
        for (int i = 0; i < size; i++)
        {
            array[i] = i;
        }

        // Realizar uma operação (soma dos elementos)
        long sum = 0;
        for (int i = 0; i < size; i++)
        {
            sum += array[i];
        }

        Console.WriteLine($"Soma: {sum}");
    }

    static void Main()
    {
        const int size = 10000000; // 10 milhões
        Stopwatch stopwatch = new Stopwatch();
        stopwatch.Start();

        RunTest(size);

        stopwatch.Stop();
        Console.WriteLine($"Tempo de execução em C#: {stopwatch.Elapsed.TotalSeconds} segundos");
    }
}
