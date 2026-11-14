#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_monte_plain.h>

double exact = M_PI / 4.0;

double g (double *l, size_t dim, void *params)
{
  (void) (dim);
  (void) (params);
  return 1.0 / (1.0 + l[0] * l[0]);
}

void integra_mpi (size_t calls_total, gsl_rng *r, int rank, int size,
                  double *resultado, double *erro)
{
  // Divide as amostras igualmente entre os processos.
  size_t calls_local = calls_total / size;

  double xl[1] = { 0 };
  double xu[1] = { 1 };
  gsl_monte_function G = { &g, 1, 0 };
  gsl_monte_plain_state *s = gsl_monte_plain_alloc (1);

  double res_local = 0.0, err_local = 0.0;
  // Calcula a parte local da integral.
  gsl_monte_plain_integrate (&G, xl, xu, 1, calls_local, r, s,
                             &res_local, &err_local);

  gsl_monte_plain_free (s);

  // Envia o resultado e o erro ao quadrado num unico Reduce; o rank 0 soma tudo.
  double local[2] = { res_local, err_local * err_local };
  double soma[2] = { 0.0, 0.0 };
  MPI_Reduce (local, soma, 2, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  // Media dos resultados; erro = raiz da soma dos erros ao quadrado / P.
  if (rank == 0)
    {
      *resultado = soma[0] / size;
      *erro = sqrt (soma[1]) / size;
    }
}

int main (int argc, char **argv)
{
  int rank = 0, size = 1;

  // Inicia o MPI e identifica este processo.
  MPI_Init (&argc, &argv);
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &size);

  size_t calls = (argc > 1) ? strtoull (argv[1], NULL, 10) : 10000000;

  // Usa uma semente reproduzível e diferente em cada processo.
  gsl_rng_env_setup ();
  gsl_rng *r = gsl_rng_alloc (gsl_rng_default);
  unsigned long base = gsl_rng_default_seed ? gsl_rng_default_seed : 42;
  gsl_rng_set (r, base + rank);

  double res = 0.0, err = 0.0;

  // Espera todos os processos para comecarem juntos; a barreira fica fora do tempo.
  MPI_Barrier (MPI_COMM_WORLD);
  double t0 = MPI_Wtime ();

  integra_mpi (calls, r, rank, size, &res, &err);

  double t1 = MPI_Wtime ();

  if (rank == 0)
    {
      printf ("P = %d, calls = %zu\n", size, calls);
      printf ("result = % .6f\n", res);
      printf ("sigma  = % .6f\n", err);
      printf ("exact  = % .6f\n", exact);
      printf ("error  = % .6f = %.2g sigma\n", res - exact,
              err > 0 ? fabs (res - exact) / err : 0.0);
      printf ("tempo  = %.6f s\n", t1 - t0);
    }

  gsl_rng_free (r);

  // Encerra o ambiente MPI.
  MPI_Finalize ();

  return 0;
}
