import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.text.Normalizer;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Stream;

public class ContadorCaracteres {
    private static final int QUANTIDADE_LETRAS = 26;
    private static final int TAMANHO_BUFFER = 16 * 1024;

    /*
     * Configuracao escolhida apos analise empirica na maquina de
     * desenvolvimento. O valor pode ser sobrescrito pelo segundo argumento da
     * linha de comando.
     */
    private static final int MELHOR_CONFIGURACAO_THREADS = 16;

    private static final int[] THREADS_PARA_ANALISE = {1, 2, 4, 6, 8, 12, 16, 24};

    public static void main(String[] args) throws Exception {
        if (args.length == 0 || contemAjuda(args)) {
            imprimirUso();
            return;
        }

        if ("--analisar".equals(args[0])) {
            executarAnalise(args);
            return;
        }

        Path diretorio = Path.of(args[0]);
        int threads = args.length >= 2 ? parseThreads(args[1]) : MELHOR_CONFIGURACAO_THREADS;

        List<Path> arquivos = listarArquivosTxt(diretorio);
        Resultado resultado = processar(arquivos, threads);
        imprimirResultado(diretorio, arquivos.size(), resultado);
    }

    private static void executarAnalise(String[] args) throws Exception {
        if (args.length < 2) {
            imprimirUso();
            return;
        }

        Path diretorio = Path.of(args[1]);
        int[] candidatos = args.length >= 3 ? parseListaThreads(args[2]) : THREADS_PARA_ANALISE;
        List<Path> arquivos = listarArquivosTxt(diretorio);

        System.out.printf("Diretorio: %s%n", diretorio.toAbsolutePath().normalize());
        System.out.printf("Arquivos .txt encontrados: %d%n", arquivos.size());
        System.out.println("Analise de threads:");

        Resultado melhor = null;
        for (int threads : candidatos) {
            Resultado resultado = processar(arquivos, threads);
            System.out.printf(
                    Locale.ROOT,
                    "%2d threads -> %8.3f s (%d ms)%n",
                    threads,
                    resultado.tempo().toNanos() / 1_000_000_000.0,
                    resultado.tempo().toMillis());

            if (melhor == null || resultado.tempo().compareTo(melhor.tempo()) < 0) {
                melhor = resultado;
            }
        }

        if (melhor != null) {
            System.out.printf("%nMelhor configuracao nesta execucao: %d threads%n", melhor.threads());
            imprimirContagem(melhor.contagem());
        }
    }

    private static List<Path> listarArquivosTxt(Path diretorio) throws IOException {
        if (!Files.isDirectory(diretorio)) {
            throw new IllegalArgumentException("Diretorio invalido: " + diretorio);
        }

        try (Stream<Path> caminhos = Files.walk(diretorio)) {
            return caminhos
                    .filter(Files::isRegularFile)
                    .filter(ContadorCaracteres::ehArquivoTxt)
                    .sorted(Comparator.comparing(path -> path.toAbsolutePath().normalize().toString()))
                    .toList();
        }
    }

    private static boolean ehArquivoTxt(Path arquivo) {
        String nome = arquivo.getFileName().toString().toLowerCase(Locale.ROOT);
        return nome.endsWith(".txt");
    }

    private static Resultado processar(List<Path> arquivos, int threads) throws Exception {
        Instant inicio = Instant.now();
        long[] contagem = contarLetras(arquivos, threads);
        Duration tempo = Duration.between(inicio, Instant.now());
        return new Resultado(contagem, tempo, threads);
    }

    private static long[] contarLetras(List<Path> arquivos, int threads) throws Exception {
        int quantidadeWorkers = Math.max(1, threads);
        ExecutorService executor = Executors.newFixedThreadPool(quantidadeWorkers);
        AtomicInteger proximoArquivo = new AtomicInteger(0);
        List<Future<long[]>> futuros = new ArrayList<>();

        try {
            for (int i = 0; i < quantidadeWorkers; i++) {
                futuros.add(executor.submit(criarWorker(arquivos, proximoArquivo)));
            }

            long[] total = new long[QUANTIDADE_LETRAS];
            for (Future<long[]> futuro : futuros) {
                somar(total, obterResultado(futuro));
            }
            return total;
        } finally {
            executor.shutdownNow();
        }
    }

    private static Callable<long[]> criarWorker(List<Path> arquivos, AtomicInteger proximoArquivo) {
        return () -> {
            long[] contagemLocal = new long[QUANTIDADE_LETRAS];

            while (true) {
                int indice = proximoArquivo.getAndIncrement();
                if (indice >= arquivos.size()) {
                    return contagemLocal;
                }
                contarArquivo(arquivos.get(indice), contagemLocal);
            }
        };
    }

    private static long[] obterResultado(Future<long[]> futuro) throws Exception {
        try {
            return futuro.get();
        } catch (ExecutionException erro) {
            Throwable causa = erro.getCause();
            if (causa instanceof Exception exception) {
                throw exception;
            }
            throw new RuntimeException(causa);
        }
    }

    private static void contarArquivo(Path arquivo, long[] contagem) throws IOException {
        try (BufferedReader leitor = new BufferedReader(abrirLeitor(arquivo), TAMANHO_BUFFER)) {
            char[] buffer = new char[TAMANHO_BUFFER];
            int lidos;

            while ((lidos = leitor.read(buffer)) != -1) {
                contarTexto(buffer, lidos, contagem);
            }
        }
    }

    private static Reader abrirLeitor(Path arquivo) throws IOException {
        return new InputStreamReader(
                new BufferedInputStream(Files.newInputStream(arquivo), TAMANHO_BUFFER),
                StandardCharsets.UTF_8
                        .newDecoder()
                        .onMalformedInput(CodingErrorAction.REPLACE)
                        .onUnmappableCharacter(CodingErrorAction.REPLACE));
    }

    private static void contarTexto(char[] buffer, int tamanho, long[] contagem) {
        String normalizado = Normalizer.normalize(new String(buffer, 0, tamanho), Normalizer.Form.NFD);

        for (int i = 0; i < normalizado.length(); i++) {
            char caractere = normalizado.charAt(i);

            if (caractere >= 'A' && caractere <= 'Z') {
                contagem[caractere - 'A']++;
            } else if (caractere >= 'a' && caractere <= 'z') {
                contagem[caractere - 'a']++;
            }
        }
    }

    private static void somar(long[] destino, long[] origem) {
        for (int i = 0; i < destino.length; i++) {
            destino[i] += origem[i];
        }
    }

    private static void imprimirResultado(Path diretorio, int quantidadeArquivos, Resultado resultado) {
        System.out.printf("Diretorio: %s%n", diretorio.toAbsolutePath().normalize());
        System.out.printf("Arquivos .txt processados: %d%n", quantidadeArquivos);
        System.out.printf("Threads utilizadas: %d%n%n", resultado.threads());
        imprimirContagem(resultado.contagem());
        System.out.printf(
                Locale.ROOT,
                "%nTempo de processamento: %.3f s (%d ms)%n",
                resultado.tempo().toNanos() / 1_000_000_000.0,
                resultado.tempo().toMillis());
    }

    private static void imprimirContagem(long[] contagem) {
        for (int i = 0; i < QUANTIDADE_LETRAS; i++) {
            System.out.printf("%c: %d%n", (char) ('A' + i), contagem[i]);
        }
    }

    private static int parseThreads(String valor) {
        try {
            int threads = Integer.parseInt(valor);
            if (threads <= 0) {
                throw new NumberFormatException();
            }
            return threads;
        } catch (NumberFormatException erro) {
            throw new IllegalArgumentException("Quantidade de threads deve ser um inteiro positivo: " + valor, erro);
        }
    }

    private static int[] parseListaThreads(String valor) {
        int[] threads = Arrays.stream(valor.split(","))
                .map(String::trim)
                .filter(item -> !item.isEmpty())
                .mapToInt(ContadorCaracteres::parseThreads)
                .distinct()
                .sorted()
                .toArray();

        if (threads.length == 0) {
            throw new IllegalArgumentException("Lista de threads vazia.");
        }

        return threads;
    }

    private static boolean contemAjuda(String[] args) {
        return Arrays.asList(args).contains("--help") || Arrays.asList(args).contains("-h");
    }

    private static void imprimirUso() {
        System.out.println("Uso:");
        System.out.println("  javac ContadorCaracteres.java");
        System.out.println("  java ContadorCaracteres <diretorio> [threads]");
        System.out.println("  java ContadorCaracteres --analisar <diretorio> [threadsSeparadasPorVirgula]");
        System.out.println();
        System.out.println("Exemplos:");
        System.out.println("  java ContadorCaracteres dados/amostra");
        System.out.println("  java ContadorCaracteres dados/todosArquivos 12");
        System.out.println("  java ContadorCaracteres --analisar dados/todosArquivos 1,2,4,8,12,16,24");
    }

    private record Resultado(long[] contagem, Duration tempo, int threads) {
    }
}
