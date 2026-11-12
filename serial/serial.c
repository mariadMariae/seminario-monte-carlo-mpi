#include <stdio.h>
#include <math.h>
#include <time.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_monte_plain.h>

// Valor exato da integral de 1 / (1 + x²) entre 0 e 1 (PI / 4), para comparar com o Monte Carlo
double exact = M_PI / 4.0;

// Consoante a documentação, define-se a função abaixo com os parâmetros esclarecidos pela GSL
// Adaptada para a função f(x) = 1 / (1 + x²), vista em aula
double g (double *l, size_t dim, void *params) {
        // Evita avisos na compilação (visto na documentação)
        (void) (dim);
        (void) (params);
        return 1.0 / (1.0 + l[0] * l[0]);
}

// Conforme a documentação, a função abaixo exibe os resultados na tela
void
display_results (char *title, double result, double error)
{
  printf ("%s ==================\n", title);
  printf ("result = % .6f\n", result);
  printf ("sigma  = % .6f\n", error);
  printf ("exact  = % .6f\n", exact);
  // Se der menos que 2 sigma, o resultado é considerado coerente
  printf ("error  = % .6f = %.2g sigma\n", result - exact,
          fabs (result - exact) / error);
}

int
main (void)
{
  double res, err;

  // Adaptado da documentação, que usa 3 dimensões, de 0 a pi
  double xl[1] = { 0 };
  double xu[1] = { 1 };

  const gsl_rng_type *T;
  gsl_rng *r;

  gsl_monte_function G = { &g, 1, 0 };

  // A documentação usa 500000; aumentamos para que o tempo seja mensurável
  size_t calls = 10000000;

  // Sem GSL_RNG_SEED no ambiente, a semente é sempre a padrão (0), então a
  // execução serial é reprodutível. Na versão MPI, cada rank DEVE receber uma
  // semente diferente (ex.: gsl_rng_set (r, semente_base + rank)); caso
  // contrário, todos geram a mesma sequência e não há ganho estatístico.
  gsl_rng_env_setup ();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  {
    gsl_monte_plain_state *s = gsl_monte_plain_alloc (1);

    // Marca o relógio antes e depois do cálculo (tempo de parede, comparável ao MPI_Wtime)
    struct timespec t0, t1;
    clock_gettime (CLOCK_MONOTONIC, &t0);
    gsl_monte_plain_integrate (&G, xl, xu, 1, calls, r, s, &res, &err);
    clock_gettime (CLOCK_MONOTONIC, &t1);

    gsl_monte_plain_free (s);
    display_results ("plain", res, err);

    printf ("tempo  = %.3f s\n", (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9);
  }

  gsl_rng_free (r);

  return 0;
}
