/*
 * Trabalho 02 - Problema Produtor-Consumidor
 * Autor: Fernando Dantas
 * Referencia Repositorio Git:
 * https://github.com/fsd-dantas/ppgia-puc-pr-asc/tree/master/atividades/produto-consumidor
 */
package br.pucpr.asc.produtoconsumidor;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ThreadLocalRandom;

public class Main {
    private static final int CAPACIDADE_FILA = 10;
    private static final int QUANTIDADE_PRODUTORES = 3;
    private static final int QUANTIDADE_CONSUMIDORES = 2;

    public static void main(String[] args) throws InterruptedException {
        Configuracao configuracao = Configuracao.from(args);
        BlockingQueue<Integer> fila = new ArrayBlockingQueue<>(CAPACIDADE_FILA, true);
        List<Thread> threads = criarThreads(fila, configuracao);

        imprimirConfiguracao(configuracao);
        threads.forEach(Thread::start);

        if (configuracao.duracao().isPresent()) {
            dormir(configuracao.duracao().get());
            encerrar(threads);
            return;
        }

        Runtime.getRuntime().addShutdownHook(new Thread(() -> encerrar(threads)));
        for (Thread thread : threads) {
            thread.join();
        }
    }

    private static List<Thread> criarThreads(BlockingQueue<Integer> fila, Configuracao configuracao) {
        List<Thread> threads = new ArrayList<>();

        for (int id = 1; id <= QUANTIDADE_PRODUTORES; id++) {
            Thread thread = new Thread(
                    new Produtor(id, fila, configuracao.intervaloProdutor()),
                    "Produtor-%02d".formatted(id));
            threads.add(thread);
        }

        for (int indice = 1; indice <= QUANTIDADE_CONSUMIDORES; indice++) {
            int id = QUANTIDADE_PRODUTORES + indice;
            Thread thread = new Thread(
                    new Consumidor(id, fila, configuracao.intervaloConsumidor()),
                    "Consumidor-%02d".formatted(id));
            threads.add(thread);
        }

        return threads;
    }

    private static void imprimirConfiguracao(Configuracao configuracao) {
        String duracao = configuracao.duracao()
                .map(Main::formatarSegundos)
                .orElse("continua ate interrupcao");

        System.out.printf(
                "Iniciando Produto Consumidor: produtores=%d, consumidores=%d, capacidade=%d, X=%s s, Y=%s s, duracao=%s%n",
                QUANTIDADE_PRODUTORES,
                QUANTIDADE_CONSUMIDORES,
                CAPACIDADE_FILA,
                formatarSegundos(configuracao.intervaloProdutor()),
                formatarSegundos(configuracao.intervaloConsumidor()),
                duracao);
    }

    private static void encerrar(List<Thread> threads) {
        threads.forEach(Thread::interrupt);
    }

    private static void dormir(Duration intervalo) throws InterruptedException {
        long millis = intervalo.toMillis();
        int nanos = intervalo.minusMillis(millis).getNano();
        Thread.sleep(millis, nanos);
    }

    private static String formatarSegundos(Duration duracao) {
        double segundos = duracao.toNanos() / 1_000_000_000.0;
        if (segundos == Math.rint(segundos)) {
            return "%.0f".formatted(segundos);
        }
        return "%.3f".formatted(segundos);
    }

    private record Configuracao(
            Duration intervaloProdutor,
            Duration intervaloConsumidor,
            Optional<Duration> duracao) {

        static Configuracao from(String[] args) {
            if (args.length > 3) {
                throw new IllegalArgumentException(uso());
            }

            Duration intervaloProdutor = args.length >= 1
                    ? parseSegundos(args[0], "X")
                    : Duration.ofSeconds(1);
            Duration intervaloConsumidor = args.length >= 2
                    ? parseSegundos(args[1], "Y")
                    : Duration.ofSeconds(2);
            Optional<Duration> duracao = args.length == 3
                    ? Optional.of(parseSegundos(args[2], "duracao"))
                    : Optional.empty();

            return new Configuracao(intervaloProdutor, intervaloConsumidor, duracao);
        }

        private static Duration parseSegundos(String valor, String nome) {
            try {
                double segundos = Double.parseDouble(valor);
                if (!Double.isFinite(segundos) || segundos <= 0) {
                    throw new IllegalArgumentException();
                }
                return Duration.ofNanos(Math.round(segundos * 1_000_000_000L));
            } catch (IllegalArgumentException erro) {
                throw new IllegalArgumentException(
                        "%s deve ser um numero positivo em segundos.%n%s".formatted(nome, uso()),
                        erro);
            }
        }

        private static String uso() {
            return "Uso: java -cp build br.pucpr.asc.produtoconsumidor.Main [X] [Y] [duracao]";
        }
    }

    private record Produtor(int id, BlockingQueue<Integer> fila, Duration intervalo) implements Runnable {
        @Override
        public void run() {
            try {
                while (!Thread.currentThread().isInterrupted()) {
                    int numero = ThreadLocalRandom.current().nextInt(100);
                    fila.put(numero);
                    System.out.printf("Produtor %02d - Inseriu %d na fila%n", id, numero);
                    dormir(intervalo);
                }
            } catch (InterruptedException erro) {
                Thread.currentThread().interrupt();
            }
        }
    }

    private record Consumidor(int id, BlockingQueue<Integer> fila, Duration intervalo) implements Runnable {
        @Override
        public void run() {
            try {
                while (!Thread.currentThread().isInterrupted()) {
                    int numero = fila.take();
                    System.out.printf("Consumidor %02d - Retirou %d da fila%n", id, numero);
                    dormir(intervalo);
                }
            } catch (InterruptedException erro) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
