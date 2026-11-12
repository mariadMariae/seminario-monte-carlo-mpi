#include <stdio.h>
#include <time.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_monte_plain.h>

// Valor exato da integral de 1 / (1 + x²) entre 0 e 1, cujo valor é PI / 4 
// Utilizada para comparar com o resultado correspondente do Monte Carlo
double exact = M_PI / 4.0;

// Consoante a documentação, define-se a função abaixo com os parâmetros esclarecidos pela GSL
// Adaptada para a função f(x) = 1 / (1 + x²), vista em aula
// l: vetor com as coordenadas do ponto sorteado (no caso, somente x, em l[0]
// dim: número de dimensões (no exemplo, apenas 1);
// params: parâmetros extras da função, os quais não foram usados
double g (double *l, size_t dim, void *params) {
	// Indica para o compilador que dim e params não são usados propositalmente
	// Evita  avisos na compilação (visto na documentação)
	(void) (dim);
	(void) (params);
	return 1.0 /  (1.0 + l[0] * l[0]);
}

// Conforme a documentação, a função abaixo exibe os resultados na tela.
// title: nome do método utilizado (aqui, "plain");
// result: valor da integral estimado pelo Monte Carlo;
// error: erro estimado pela própria GSL (chamado de sigma).
void
display_results (char *title, double result, double error)
{
  printf ("%s ==================\n", title);
  printf ("result = % .6f\n", result);   // valor estimado
  printf ("sigma  = % .6f\n", error);    // erro estimado
  printf ("exact  = % .6f\n", exact);    // valor exato (pi/4)
  // Diferença entre o estimado e o exato, e quantas vezes ela cabe no sigma.
  // Se der menos que 2 sigma, o resultado é considerado coerente.
  printf ("error  = % .6f = %.2g sigma\n", result - exact,
          fabs (result - exact) / error);
}

int
main (void)
{
  // Variáveis que receberão o resultado e o erro estimado (conforme a documentação).
  double res, err;

  // Limites da integração definidos de 0 a 1, em uma dimensão.
  // Adaptado da documentação, que usa 3 dimensões, de 0 a pi.
  double xl[1] = { 0 };
  double xu[1] = { 1 };

  // Gerador de números aleatórios, consoante a documentação.
  // T guarda o tipo de gerador e r é o gerador em si.
  const gsl_rng_type *T;
  gsl_rng *r;

  // Estrutura que reúne a função, o número de dimensões e os parâmetros extras.
  gsl_monte_function G = { &g, 1, 0 };

  // Número de amostras sorteadas. A documentação usa 500000; aumentamos
  // para que o tempo seja mensurável. 
  size_t calls = 10000000;

  // Prepara o gerador padrão da GSL, de acordo com a documentação.
  gsl_rng_env_setup ();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  {
    // Aloca a memória de trabalho do método PLAIN (1 dimensão).
    gsl_monte_plain_state *s = gsl_monte_plain_alloc (1);

    // Marca o relógio antes e depois do cálculo.
    clock_t inicio = clock ();
    gsl_monte_plain_integrate (&G, xl, xu, 1, calls, r, s, &res, &err);
    clock_t fim = clock ();

    // Libera a memória de trabalho e exibe os resultados 
    gsl_monte_plain_free (s);
    display_results ("plain", res, err);

    // Exibe o tempo do cálculo em segundos.
    printf ("tempo  = %.3f s\n", (double) (fim - inicio) / CLOCKS_PER_SEC);
  }

  // Libera o gerador e encerra o programa.
  gsl_rng_free (r);

  return 0;
}
